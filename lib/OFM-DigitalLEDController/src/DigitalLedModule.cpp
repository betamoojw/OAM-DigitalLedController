#include "DigitalLedModule.h"
#undef memcpy_P
#include <ArduinoJson.h>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>

DigitalLedModule digitalLedModule;

namespace
{
    // WLED JSON API field reference: https://kno.wled.ge/interfaces/json-api/
    template <typename Document>
    void sendJson(Document& document)
    {
        if (ParamAPP_SuppressSyncNotifications && !document.containsKey("v") && !document.containsKey("rb"))
        {
            DynamicJsonDocument outgoing(document.memoryUsage() + 128U);
            outgoing.set(document);
            outgoing["udpn"]["nn"] = true;
            serializeJson(outgoing, DIGITAL_LED_SERIAL);
        }
        else
            serializeJson(document, DIGITAL_LED_SERIAL);
        DIGITAL_LED_SERIAL.print("\r\n");
    }

    template <typename Document>
    void addTransition(Document& document, uint16_t milliseconds)
    {
        document["tt"] = std::min<uint32_t>((static_cast<uint32_t>(milliseconds) + 50U) / 100U, 655U);
    }

    template <typename Document>
    JsonObject createSegmentCommand(Document& document, uint8_t segmentId, uint16_t transitionMs)
    {
        addTransition(document, transitionMs);
        JsonObject segment = document["seg"].createNestedObject();
        segment["id"] = segmentId;
        return segment;
    }

    bool validInteger(JsonVariantConst value, int minimum, int maximum)
    {
        return value.is<int>() && value.as<int>() >= minimum && value.as<int>() <= maximum;
    }

    bool validOptionalInteger(JsonObjectConst object, const char* key, int minimum, int maximum)
    {
        return !object.containsKey(key) || validInteger(object[key], minimum, maximum);
    }

    bool validOptionalUnsigned(JsonObjectConst object, const char* key)
    {
        return !object.containsKey(key) || object[key].is<uint32_t>();
    }

    bool validOptionalBool(JsonObjectConst object, const char* key)
    {
        return !object.containsKey(key) || object[key].is<bool>();
    }

    int16_t relativeDelta(uint8_t command, uint8_t step, uint16_t fullScale)
    {
        if (step == 0 || step > 7)
            return 0;
        const uint16_t divisor = 1U << (step - 1);
        const int16_t delta = static_cast<int16_t>((fullScale + divisor / 2U) / divisor);
        return command != 0 ? delta : -delta;
    }

    uint8_t addClamped(uint8_t value, int16_t delta)
    {
        return static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(value) + delta)));
    }

    bool parseIpAddress(const char* text, uint32_t& address)
    {
        unsigned int octets[4];
        char trailing;
        if (text == nullptr || std::sscanf(text, "%u.%u.%u.%u%c", &octets[0], &octets[1], &octets[2], &octets[3], &trailing) != 4)
            return false;
        for (unsigned int octet : octets)
            if (octet > 255)
                return false;
        address = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
        return true;
    }

    void compactBssid(const char* source, char (&destination)[15])
    {
        size_t output = 0;
        if (source != nullptr)
            for (size_t input = 0; source[input] != '\0' && output < sizeof(destination) - 1; ++input)
                if (std::isxdigit(static_cast<unsigned char>(source[input])))
                    destination[output++] = static_cast<char>(std::toupper(static_cast<unsigned char>(source[input])));
        destination[output] = '\0';
    }

    bool parseLocalTime(const char* text, struct tm& result)
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        char marker[3]{};
        const int fields = text == nullptr ? 0 : std::sscanf(
            text, "%d-%d-%d, %d:%d:%d %2s", &year, &month, &day, &hour, &minute, &second, marker);
        if (fields < 6 || year < 1990 || year > 2089 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
            return false;
        if (fields == 7)
        {
            if (std::strcmp(marker, "PM") == 0 && hour < 12)
                hour += 12;
            else if (std::strcmp(marker, "AM") == 0 && hour == 12)
                hour = 0;
        }
        result = {};
        result.tm_year = year;
        result.tm_mon = month;
        result.tm_mday = day;
        result.tm_hour = hour;
        result.tm_min = minute;
        result.tm_sec = second;
        return true;
    }

    bool validManagedSegment(JsonObjectConst segment)
    {
        if (!validOptionalBool(segment, "on") || !validOptionalBool(segment, "sel") ||
            !validOptionalBool(segment, "rev") || !validOptionalBool(segment, "mi") ||
            !validOptionalBool(segment, "frz") || !validOptionalInteger(segment, "bri", 0, 255) ||
            (segment.containsKey("n") && !segment["n"].is<const char*>()))
            return false;
        if (segment.containsKey("cct") && !validInteger(segment["cct"], 0, 255) &&
            !validInteger(segment["cct"], 1900, 10091))
            return false;
        if (!segment.containsKey("col"))
            return true;
        JsonArrayConst colors = segment["col"];
        if (colors.isNull() || colors.size() < 1 || colors.size() > 3)
            return false;
        for (JsonArrayConst color : colors)
        {
            if (color.size() != 3 && color.size() != 4)
                return false;
            for (JsonVariantConst component : color)
                if (!validInteger(component, 0, 255))
                    return false;
        }
        return true;
    }
}

const std::string DigitalLedModule::name()
{
    return "OFM Digital LED Controller";
}

const std::string DigitalLedModule::version()
{
    return "0.6.0";
}

void DigitalLedModule::setup(bool configured)
{
    (void)configured;
    firstRequest = true;
    DIGITAL_LED_SERIAL.setTX(DIGITAL_LED_UART_TX_PIN);
    DIGITAL_LED_SERIAL.setRX(DIGITAL_LED_UART_RX_PIN);
    DIGITAL_LED_SERIAL.setFIFOSize(512);
    DIGITAL_LED_SERIAL.begin(DIGITAL_LED_UART_BAUD_RATE);
    state.brightness = ParamAPP_DefaultBrightness;
    state.palette = ParamAPP_DefaultPalette;
    state.effect = ParamAPP_DefaultEffect;
    state.effectSpeed = ParamAPP_DefaultEffectSpeed;
    state.effectIntensity = ParamAPP_DefaultEffectIntensity;
    state.transitionMs = ParamAPP_DefaultTransitionMs;
    state.nightlightDuration = ParamAPP_NightlightDuration;
    state.nightlightMode = ParamAPP_NightlightMode;
    state.nightlightTarget = ParamAPP_NightlightTargetBrightness;
    state.syncSendGroups = ParamAPP_DefaultSyncSendGroups;
    state.syncReceiveGroups = ParamAPP_DefaultSyncReceiveGroups;
    applyConfiguredDefaults();
    lastRequest = millis();
}

void DigitalLedModule::loop()
{
    receiveSerial();
    updateCommunicationHealth();
    if (state.presetActionArmed && millis() - presetActionArmedAt >= 30000UL)
        setPresetActionArm(false);
    if (state.segmentActionArmed && millis() - segmentActionArmedAt >= 30000UL)
        setSegmentActionArm(false);
    if (depth != 0)
        return;
    if (firstRequest)
    {
        requestState();
        firstRequest = false;
        lastRequest = millis();
        return;
    }
    const uint32_t intervalSeconds = ParamAPP_RequestXInterval;
    if (intervalSeconds == 0)
        return;
    if (millis() - lastRequest >= intervalSeconds * 1000UL)
    {
        requestState();
        lastRequest = millis();
    }
}

void DigitalLedModule::processInputKo(GroupObject& ko)
{
    if (ko.asap() >= APP_KoChManagedSegment1On && ko.asap() <= APP_KoChManagedSegment3Name)
    {
        processManagedSegmentInput(ko);
        return;
    }
    switch (ko.asap())
    {
    case APP_KoChDimSw:
        sendSwitch(ko.value(DPT_Switch));
        break;
    case APP_KoChDimAbs:
        sendBrightness(ko.value(DPT_Percent_U8));
        break;
    case APP_KoChDimRel:
    {
        const uint8_t command = ko.value(Dpt(3, 7, 0));
        const uint8_t step = ko.value(Dpt(3, 7, 1));
        const int16_t delta = relativeDelta(command, step, 255);
        if (delta != 0)
            sendBrightness(addClamped(state.brightness, delta));
        break;
    }
    case APP_KoChRed:
        sendColor(ko.value(DPT_Percent_U8), state.green, state.blue);
        break;
    case APP_KoChGreen:
        sendColor(state.red, ko.value(DPT_Percent_U8), state.blue);
        break;
    case APP_KoChBlue:
        sendColor(state.red, state.green, ko.value(DPT_Percent_U8));
        break;
    case APP_KoChRGB:
    {
        const uint32_t color = ko.value(DPT_Colour_RGB);
        sendColor(static_cast<uint8_t>(color >> 16), static_cast<uint8_t>(color >> 8), static_cast<uint8_t>(color));
        break;
    }
    case APP_KoChWhite:
        sendRgbw(state.red, state.green, state.blue, ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChCct:
        sendColorTemperature(ko.value(Dpt(7, 600)));
        break;
    case APP_KoChWarmWhite:
        sendWarmWhite(ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChColdWhite:
        sendColdWhite(ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChHue:
    {
        float hue;
        float saturation;
        float value;
        rgbToHsv(state.red, state.green, state.blue, hue, saturation, value);
        const uint8_t rawHue = ko.value(DPT_Percent_U8);
        const uint16_t hueDegrees = static_cast<uint16_t>((static_cast<uint32_t>(rawHue) * 360U + 127U) / 255U);
        sendHsv(hueDegrees, static_cast<uint8_t>(std::round(saturation * 255.0F)), static_cast<uint8_t>(std::round(value * 255.0F)));
        break;
    }
    case APP_KoChSaturation:
    {
        float hue;
        float saturation;
        float value;
        rgbToHsv(state.red, state.green, state.blue, hue, saturation, value);
        sendHsv(static_cast<uint16_t>(std::round(hue)), ko.value(DPT_Percent_U8), static_cast<uint8_t>(std::round(value * 255.0F)));
        break;
    }
    case APP_KoChValue:
    {
        float hue;
        float saturation;
        float value;
        rgbToHsv(state.red, state.green, state.blue, hue, saturation, value);
        sendHsv(static_cast<uint16_t>(std::round(hue)), static_cast<uint8_t>(std::round(saturation * 255.0F)), ko.value(DPT_Percent_U8));
        break;
    }
    case APP_KoChRGBW:
    {
        const uint32_t color = ko.value(DPT_Colour_RGBW);
        const uint8_t mask = ko.value(Dpt(251, 600, 1));
        if ((mask & 0x0F) == 0)
            break;
        sendRgbw(
            (mask & 0x08) != 0 ? static_cast<uint8_t>(color >> 24) : state.red,
            (mask & 0x04) != 0 ? static_cast<uint8_t>(color >> 16) : state.green,
            (mask & 0x02) != 0 ? static_cast<uint8_t>(color >> 8) : state.blue,
            (mask & 0x01) != 0 ? static_cast<uint8_t>(color) : state.white);
        break;
    }
    case APP_KoChRedRel:
    case APP_KoChGreenRel:
    case APP_KoChBlueRel:
    case APP_KoChWhiteRel:
    {
        const uint8_t command = ko.value(Dpt(3, 7, 0));
        const uint8_t step = ko.value(Dpt(3, 7, 1));
        const int16_t delta = relativeDelta(command, step, 255);
        if (delta == 0)
            break;
        uint8_t red = state.red;
        uint8_t green = state.green;
        uint8_t blue = state.blue;
        uint8_t white = state.white;
        if (ko.asap() == APP_KoChRedRel)
            red = addClamped(red, delta);
        else if (ko.asap() == APP_KoChGreenRel)
            green = addClamped(green, delta);
        else if (ko.asap() == APP_KoChBlueRel)
            blue = addClamped(blue, delta);
        else
            white = addClamped(white, delta);
        sendRgbw(red, green, blue, white);
        break;
    }
    case APP_KoChWarmWhiteRel:
    case APP_KoChColdWhiteRel:
    {
        const int16_t delta = relativeDelta(ko.value(Dpt(3, 7, 0)), ko.value(Dpt(3, 7, 1)), 255);
        if (delta == 0)
            break;
        const uint8_t warm = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * (255U - state.cct) + 127U) / 255U);
        const uint8_t cold = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * state.cct + 127U) / 255U);
        if (ko.asap() == APP_KoChWarmWhiteRel)
            sendWarmWhite(addClamped(warm, delta));
        else
            sendColdWhite(addClamped(cold, delta));
        break;
    }
    case APP_KoChHueRel:
    case APP_KoChSaturationRel:
    case APP_KoChValueRel:
    {
        const uint8_t command = ko.value(Dpt(3, 7, 0));
        const uint8_t step = ko.value(Dpt(3, 7, 1));
        const int16_t delta = relativeDelta(command, step, ko.asap() == APP_KoChHueRel ? 30 : 255);
        if (delta == 0)
            break;
        float hue;
        float saturation;
        float value;
        rgbToHsv(state.red, state.green, state.blue, hue, saturation, value);
        if (ko.asap() == APP_KoChHueRel)
        {
            hue += delta;
            while (hue < 0.0F)
                hue += 360.0F;
            while (hue >= 360.0F)
                hue -= 360.0F;
        }
        else if (ko.asap() == APP_KoChSaturationRel)
            saturation = std::max(0.0F, std::min(1.0F, saturation + delta / 255.0F));
        else
            value = std::max(0.0F, std::min(1.0F, value + delta / 255.0F));
        sendHsv(static_cast<uint16_t>(std::round(hue)), static_cast<uint8_t>(std::round(saturation * 255.0F)), static_cast<uint8_t>(std::round(value * 255.0F)));
        break;
    }
    case APP_KoChEffectRel:
    {
        const int16_t delta = relativeDelta(ko.value(Dpt(3, 7, 0)), ko.value(Dpt(3, 7, 1)), 10);
        if (delta != 0)
            sendSegmentValue("fx", addClamped(state.effect, delta));
        break;
    }
    case APP_KoChNightlightSw:
        sendNightlight(ko.value(DPT_Switch));
        break;
    case APP_KoChNightlightDuration:
        sendNightlightValue("dur", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChNightlightMode:
    {
        const uint8_t mode = ko.value(DPT_Value_1_Ucount);
        if (mode <= 3)
            sendNightlightValue("mode", mode);
        break;
    }
    case APP_KoChNightlightTarget:
        sendNightlightValue("tbri", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChScene:
    {
        const uint8_t scene = ko.value(DPT_SceneNumber);
        if (scene <= 63)
            sendPreset(static_cast<uint8_t>(scene + 1));
        break;
    }
    case APP_KoChPreset:
    {
        const uint8_t preset = ko.value(DPT_Percent_U8);
        if (preset >= 1 && preset <= 250)
            sendPreset(preset);
        break;
    }
    case APP_KoChPalette:
        sendSegmentValue("pal", ko.value(DPT_Percent_U8));
        break;
    case APP_KoChEffectId:
        sendSegmentValue("fx", ko.value(DPT_Percent_U8));
        break;
    case APP_KoChEffectSpeed:
        sendSegmentValue("sx", ko.value(DPT_Percent_U8));
        break;
    case APP_KoChEffectIntensity:
        sendSegmentValue("ix", ko.value(DPT_Percent_U8));
        break;
    case APP_KoChSegmentSw:
        sendSegmentBool("on", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentBrightness:
        sendSegmentValue("bri", ko.value(DPT_Percent_U8));
        break;
    case APP_KoChSegmentReverse:
        sendSegmentBool("rev", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentMirror:
        sendSegmentBool("mi", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentFreeze:
        sendSegmentBool("frz", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentSelected:
        sendSegmentBool("sel", ko.value(DPT_Switch));
        break;
    case APP_KoChTransition:
        sendTransition(ko.value(Dpt(7, 2)));
        break;
    case APP_KoChSyncSend:
        sendSync(true, ko.value(DPT_Switch));
        break;
    case APP_KoChSyncReceive:
        sendSync(false, ko.value(DPT_Switch));
        break;
    case APP_KoChSyncSendGroups:
        sendSyncGroups(true, ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChSyncReceiveGroups:
        sendSyncGroups(false, ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChLiveOverride:
    {
        const uint8_t mode = ko.value(DPT_Value_1_Ucount);
        if (mode <= 2)
            sendLiveOverride(mode);
        break;
    }
    case APP_KoChMainSegment:
        sendMainSegment(ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChNextPlaylist:
        if (ko.value(DPT_Switch))
            sendTrigger("np");
        break;
    case APP_KoChStateRequest:
        if (ko.value(DPT_Switch))
            requestState();
        break;
    case APP_KoChReboot:
        if (ko.value(DPT_Switch))
            sendTrigger("rb");
        break;
    case APP_KoChEffectCustom1:
        sendSegmentValue("c1", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChEffectCustom2:
        sendSegmentValue("c2", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChEffectCustom3:
    {
        const uint8_t value = ko.value(DPT_Value_1_Ucount);
        if (value <= 31)
            sendSegmentValue("c3", value);
        break;
    }
    case APP_KoChEffectOption1:
        sendSegmentBool("o1", ko.value(DPT_Switch));
        break;
    case APP_KoChEffectOption2:
        sendSegmentBool("o2", ko.value(DPT_Switch));
        break;
    case APP_KoChEffectOption3:
        sendSegmentBool("o3", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentStart:
        sendSegmentWord("start", ko.value(DPT_Value_2_Ucount));
        break;
    case APP_KoChSegmentStop:
        sendSegmentWord("stop", ko.value(DPT_Value_2_Ucount));
        break;
    case APP_KoChSegmentGrouping:
        sendSegmentValue("grp", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChSegmentSpacing:
        sendSegmentValue("spc", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChSegmentOffset:
        sendSegmentOffset(ko.value(DPT_Value_2_Count));
        break;
    case APP_KoChSegmentName:
        sendSegmentName(ko.value(DPT_String_ASCII));
        break;
    case APP_KoChSegmentActionArm:
        setSegmentActionArm(ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentDelete:
        if (ko.value(DPT_Switch))
            deleteSegment();
        break;
    case APP_KoChSegmentApply:
        if (ko.value(DPT_Switch))
            applySegmentConfiguration();
        break;
    case APP_KoChSecondaryRGBW:
        sendSegmentColor(1, ko.value(DPT_Colour_RGBW), ko.value(Dpt(251, 600, 1)));
        break;
    case APP_KoChTertiaryRGBW:
        sendSegmentColor(2, ko.value(DPT_Colour_RGBW), ko.value(Dpt(251, 600, 1)));
        break;
    case APP_KoChPlaylistSave:
        if (ko.value(DPT_Switch))
            sendPlaylist();
        break;
    case APP_KoChPlaylistStart:
        if (ko.value(DPT_Switch))
            sendPreset(ParamAPP_PlaylistPresetSlot);
        break;
    case APP_KoChPresetActionArm:
        setPresetActionArm(ko.value(DPT_Switch));
        break;
    case APP_KoChPresetSave:
        sendPresetAction("psave", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChPresetDelete:
        sendPresetAction("pdel", ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChPresetNext:
        if (ko.value(DPT_Switch))
            sendPresetSelection("~");
        break;
    case APP_KoChPresetPrevious:
        if (ko.value(DPT_Switch))
            sendPresetSelection("~-");
        break;
    case APP_KoChPresetRandom:
        if (ko.value(DPT_Switch))
        {
            char selection[12];
            const uint8_t minimum = std::min(ParamAPP_PresetRandomMin, ParamAPP_PresetRandomMax);
            const uint8_t maximum = std::max(ParamAPP_PresetRandomMin, ParamAPP_PresetRandomMax);
            snprintf(selection, sizeof(selection), "%u~%ur", minimum, maximum);
            sendPresetSelection(selection);
        }
        break;
    case APP_KoChPlaylistStop:
        if (ko.value(DPT_Switch))
            stopPlaylist();
        break;
    case APP_KoChLive:
        sendLive(ko.value(DPT_Switch));
        break;
    case APP_KoChLedmap:
        sendLedmap(ko.value(DPT_Value_1_Ucount));
        break;
    case APP_KoChTimebase:
        sendTimebase(ko.value(DPT_Value_4_Ucount));
        break;
    case APP_KoChDeviceTime:
        sendDeviceTime(ko.value(DPT_Value_4_Ucount));
        break;
    case APP_KoChMatrixStartY:
        sendSegmentWord("startY", ko.value(DPT_Value_2_Ucount));
        break;
    case APP_KoChMatrixStopY:
        sendSegmentWord("stopY", ko.value(DPT_Value_2_Ucount));
        break;
    case APP_KoChMatrixReverseY:
        sendSegmentBool("rY", ko.value(DPT_Switch));
        break;
    case APP_KoChMatrixMirrorY:
        sendSegmentBool("mY", ko.value(DPT_Switch));
        break;
    case APP_KoChMatrixTranspose:
        sendSegmentBool("tp", ko.value(DPT_Switch));
        break;
    case APP_KoChSegmentSet:
    {
        const uint8_t value = ko.value(DPT_Value_1_Ucount);
        if (value <= 3)
            sendSegmentValue("set", value);
        break;
    }
    case APP_KoChSoundSimulation:
    {
        const uint8_t value = ko.value(DPT_Value_1_Ucount);
        if (value <= 3)
            sendSegmentValue("si", value);
        break;
    }
    case APP_KoChMap1D2D:
    {
        const uint8_t value = ko.value(DPT_Value_1_Ucount);
        if (value <= 7)
            sendSegmentValue("m12", value);
        break;
    }
    default:
        break;
    }
}

bool DigitalLedModule::processCommand(const std::string command, bool diagnoseKo)
{
    (void)diagnoseKo;
    if (command.rfind("wled ", 0) != 0)
        return false;

    const std::string argument = command.substr(5);
    if (argument == "on" || argument == "off")
    {
        sendSwitch(argument == "on");
        return true;
    }
    if (argument == "nl on" || argument == "nl off")
    {
        sendNightlight(argument == "nl on");
        return true;
    }
    if (argument.rfind("bri ", 0) == 0 || argument.rfind("ps ", 0) == 0)
    {
        const bool preset = argument[0] == 'p';
        const char* first = argument.data() + 4;
        const char* last = argument.data() + argument.size();
        int value = 0;
        const auto result = std::from_chars(first, last, value);
        const int minimum = preset ? 1 : 0;
        const int maximum = preset ? 250 : 255;
        if (result.ec != std::errc{} || result.ptr != last || value < minimum || value > maximum)
            return true;
        if (preset)
            sendPreset(static_cast<uint8_t>(value));
        else
            sendBrightness(static_cast<uint8_t>(value));
        return true;
    }
    return false;
}

void DigitalLedModule::showHelp()
{
    openknx.console.printHelpLine("wled on|off", "Switch the LED controller");
    openknx.console.printHelpLine("wled bri <0..255>", "Set brightness");
    openknx.console.printHelpLine("wled ps <1..250>", "Select WLED preset");
    openknx.console.printHelpLine("wled nl on|off", "Set the three-minute nightlight");
}

void DigitalLedModule::showInformations()
{
    logInfoP("=== OFM Digital LED Controller ===");
    logInfoP("UART: %i baud, poll interval: %i", DIGITAL_LED_UART_BAUD_RATE, ParamAPP_RequestXInterval);
}

void DigitalLedModule::requestState()
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["v"] = true;
    sendJson(document);
}

void DigitalLedModule::applyConfiguredDefaults()
{
    StaticJsonDocument<256> document;
    document["on"] = state.brightness != 0;
    document["bri"] = state.brightness;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["pal"] = state.palette;
    segment["fx"] = state.effect;
    segment["sx"] = state.effectSpeed;
    segment["ix"] = state.effectIntensity;
    sendJson(document);
    state.on = state.brightness != 0;
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
    KoAPP_ChDimValue.valueCompare(state.brightness, DPT_Percent_U8);
    publishEffects();
}

void DigitalLedModule::sendSwitch(bool on)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2)> document;
    document["on"] = on;
    addTransition(document, state.transitionMs);
    sendJson(document);
    state.on = on;
    KoAPP_ChDimState.valueCompare(on, DPT_Switch);
}

void DigitalLedModule::sendBrightness(uint8_t brightness)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2)> document;
    document["bri"] = brightness;
    addTransition(document, state.transitionMs);
    sendJson(document);
    state.brightness = brightness;
    state.on = brightness != 0;
    KoAPP_ChDimValue.valueCompare(brightness, DPT_Percent_U8);
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
}

void DigitalLedModule::sendColor(uint8_t red, uint8_t green, uint8_t blue)
{
    sendRgbw(red, green, blue, state.white);
}

void DigitalLedModule::sendRgbw(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
    StaticJsonDocument<256> document;
    document["on"] = red != 0 || green != 0 || blue != 0 || white != 0;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    JsonArray color = segment["col"].createNestedArray();
    color.add(red);
    color.add(green);
    color.add(blue);
    color.add(white);
    sendJson(document);
    state.red = red;
    state.green = green;
    state.blue = blue;
    state.white = white;
    state.on = document["on"];
    publishColor();
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
}

void DigitalLedModule::sendColorTemperature(uint16_t kelvin)
{
    StaticJsonDocument<128> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["cct"] = kelvinToCct(kelvin);
    sendJson(document);
    state.cct = segment["cct"];
    publishColor();
    publishColorTemperature();
}

void DigitalLedModule::sendWarmWhite(uint8_t value)
{
    const uint8_t cold = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * state.cct + 127U) / 255U);
    const uint16_t sum = static_cast<uint16_t>(value) + cold;
    const uint8_t white = static_cast<uint8_t>(std::min(sum, static_cast<uint16_t>(255)));
    const uint8_t cct = sum == 0 ? state.cct : static_cast<uint8_t>((static_cast<uint16_t>(cold) * 255U + sum / 2U) / sum);

    StaticJsonDocument<256> document;
    document["on"] = state.red != 0 || state.green != 0 || state.blue != 0 || white != 0;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    JsonArray color = segment["col"].createNestedArray();
    color.add(state.red);
    color.add(state.green);
    color.add(state.blue);
    color.add(white);
    segment["cct"] = cct;
    sendJson(document);
    state.white = white;
    state.cct = cct;
    state.on = document["on"];
    publishColor();
    publishColorTemperature();
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
}

void DigitalLedModule::sendColdWhite(uint8_t value)
{
    const uint8_t warm = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * (255U - state.cct) + 127U) / 255U);
    const uint16_t sum = static_cast<uint16_t>(warm) + value;
    const uint8_t white = static_cast<uint8_t>(std::min(sum, static_cast<uint16_t>(255)));
    const uint8_t cct = sum == 0 ? state.cct : static_cast<uint8_t>((static_cast<uint16_t>(value) * 255U + sum / 2U) / sum);

    StaticJsonDocument<256> document;
    document["on"] = state.red != 0 || state.green != 0 || state.blue != 0 || white != 0;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    JsonArray color = segment["col"].createNestedArray();
    color.add(state.red);
    color.add(state.green);
    color.add(state.blue);
    color.add(white);
    segment["cct"] = cct;
    sendJson(document);
    state.white = white;
    state.cct = cct;
    state.on = document["on"];
    publishColor();
    publishColorTemperature();
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
}

void DigitalLedModule::sendHsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    hsvToRgb(hue, saturation / 255.0F, value / 255.0F, red, green, blue);
    sendColor(red, green, blue);
}

void DigitalLedModule::sendNightlight(bool enabled)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4)> document;
    JsonObject nightlight = document["nl"].to<JsonObject>();
    nightlight["on"] = enabled;
    nightlight["dur"] = ParamAPP_NightlightDuration;
    nightlight["mode"] = ParamAPP_NightlightMode;
    nightlight["tbri"] = ParamAPP_NightlightTargetBrightness;
    addTransition(document, state.transitionMs);
    sendJson(document);
    state.nightlight = enabled;
    state.nightlightDuration = ParamAPP_NightlightDuration;
    state.nightlightMode = ParamAPP_NightlightMode;
    state.nightlightTarget = ParamAPP_NightlightTargetBrightness;
    KoAPP_ChNightlightState.valueCompare(enabled, DPT_Switch);
    publishAdvancedState();
}

void DigitalLedModule::sendNightlightValue(const char* key, uint8_t value)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(1)> document;
    document["nl"][key] = value;
    sendJson(document);
    if (std::strcmp(key, "dur") == 0)
        state.nightlightDuration = value;
    else if (std::strcmp(key, "mode") == 0)
        state.nightlightMode = value;
    else if (std::strcmp(key, "tbri") == 0)
        state.nightlightTarget = value;
    publishAdvancedState();
}

void DigitalLedModule::sendPreset(uint8_t preset)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2)> document;
    document["ps"] = preset;
    addTransition(document, state.transitionMs);
    sendJson(document);
    state.preset = preset;
    publishPreset();
    if (preset <= 64)
        KoAPP_ChSceneState.valueCompare(static_cast<uint8_t>(preset - 1), DPT_SceneNumber);
}

void DigitalLedModule::sendSegmentValue(const char* key, uint8_t value)
{
    StaticJsonDocument<128> document;
    document["on"] = true;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment[key] = value;
    sendJson(document);
    if (std::strcmp(key, "pal") == 0)
        state.palette = value;
    else if (std::strcmp(key, "fx") == 0)
        state.effect = value;
    else if (std::strcmp(key, "sx") == 0)
        state.effectSpeed = value;
    else if (std::strcmp(key, "ix") == 0)
        state.effectIntensity = value;
    else if (std::strcmp(key, "bri") == 0)
    {
        state.segmentBrightness = value;
        KoAPP_ChSegmentBrightnessValue.valueCompare(value, DPT_Percent_U8);
    }
    else if (std::strcmp(key, "c1") == 0)
        state.effectCustom1 = value;
    else if (std::strcmp(key, "c2") == 0)
        state.effectCustom2 = value;
    else if (std::strcmp(key, "c3") == 0)
        state.effectCustom3 = value;
    else if (std::strcmp(key, "grp") == 0)
        state.segmentGrouping = value;
    else if (std::strcmp(key, "spc") == 0)
        state.segmentSpacing = value;
    else if (std::strcmp(key, "set") == 0)
        state.segmentSet = value;
    else if (std::strcmp(key, "si") == 0)
        state.soundSimulation = value;
    else if (std::strcmp(key, "m12") == 0)
        state.map1D2D = value;
    state.on = true;
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
    publishEffects();
}

void DigitalLedModule::sendSegmentBool(const char* key, bool value)
{
    StaticJsonDocument<128> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment[key] = value;
    sendJson(document);

    if (std::strcmp(key, "on") == 0)
    {
        state.segmentOn = value;
        KoAPP_ChSegmentState.valueCompare(value, DPT_Switch);
    }
    else if (std::strcmp(key, "rev") == 0)
    {
        state.segmentReverse = value;
        KoAPP_ChSegmentReverseState.valueCompare(value, DPT_Switch);
    }
    else if (std::strcmp(key, "mi") == 0)
    {
        state.segmentMirror = value;
        KoAPP_ChSegmentMirrorState.valueCompare(value, DPT_Switch);
    }
    else if (std::strcmp(key, "frz") == 0)
    {
        state.segmentFreeze = value;
        KoAPP_ChSegmentFreezeState.valueCompare(value, DPT_Switch);
    }
    else if (std::strcmp(key, "sel") == 0)
    {
        state.segmentSelected = value;
        KoAPP_ChSegmentSelectedState.valueCompare(value, DPT_Switch);
    }
    else if (std::strcmp(key, "o1") == 0)
        state.effectOption1 = value;
    else if (std::strcmp(key, "o2") == 0)
        state.effectOption2 = value;
    else if (std::strcmp(key, "o3") == 0)
        state.effectOption3 = value;
    else if (std::strcmp(key, "rY") == 0)
        state.matrixReverseY = value;
    else if (std::strcmp(key, "mY") == 0)
        state.matrixMirrorY = value;
    else if (std::strcmp(key, "tp") == 0)
        state.matrixTranspose = value;
    publishExtendedSegmentState();
}

void DigitalLedModule::sendSegmentWord(const char* key, uint16_t value)
{
    StaticJsonDocument<160> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment[key] = value;
    if (state.segmentName[0] != '\0')
        segment["n"] = state.segmentName;
    sendJson(document);
    if (std::strcmp(key, "start") == 0)
        state.segmentStart = value;
    else if (std::strcmp(key, "stop") == 0)
        state.segmentStop = value;
    else if (std::strcmp(key, "startY") == 0)
        state.matrixStartY = value;
    else if (std::strcmp(key, "stopY") == 0)
        state.matrixStopY = value;
    publishExtendedSegmentState();
}

void DigitalLedModule::sendSegmentOffset(int16_t value)
{
    StaticJsonDocument<160> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["of"] = value;
    sendJson(document);
    state.segmentOffset = value;
    publishExtendedSegmentState();
}

void DigitalLedModule::sendSegmentName(const char* name)
{
    if (name == nullptr)
        return;
    std::strncpy(state.segmentName, name, sizeof(state.segmentName) - 1);
    state.segmentName[sizeof(state.segmentName) - 1] = '\0';
    StaticJsonDocument<192> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["n"] = state.segmentName;
    sendJson(document);
    publishExtendedSegmentState();
}

uint8_t DigitalLedModule::managedSegmentId(uint8_t index) const
{
    switch (index)
    {
    case 0:
        return ParamAPP_ManagedSegment1Id;
    case 1:
        return ParamAPP_ManagedSegment2Id;
    default:
        return ParamAPP_ManagedSegment3Id;
    }
}

void DigitalLedModule::processManagedSegmentInput(GroupObject& ko)
{
    const uint16_t offset = ko.asap() - APP_KoChManagedSegment1On;
    const uint8_t index = static_cast<uint8_t>(offset / 6U);
    const uint8_t field = static_cast<uint8_t>(offset % 6U);
    if (index >= 3)
        return;

    ManagedSegment& managed = state.managedSegments[index];
    StaticJsonDocument<320> document;
    JsonObject segment = createSegmentCommand(document, managedSegmentId(index), state.transitionMs);
    switch (field)
    {
    case 0:
        managed.on = ko.value(DPT_Switch);
        segment["on"] = managed.on;
        break;
    case 1:
        managed.brightness = ko.value(DPT_Percent_U8);
        segment["bri"] = managed.brightness;
        break;
    case 2:
        managed.cct = kelvinToCct(ko.value(Dpt(7, 600)));
        segment["cct"] = managed.cct;
        break;
    case 3:
    {
        const uint32_t color = ko.value(DPT_Colour_RGBW);
        const uint8_t mask = ko.value(Dpt(251, 600, 1));
        if ((mask & 0x0F) == 0)
            return;
        if ((mask & 0x08) != 0)
            managed.color[0] = static_cast<uint8_t>(color >> 24);
        if ((mask & 0x04) != 0)
            managed.color[1] = static_cast<uint8_t>(color >> 16);
        if ((mask & 0x02) != 0)
            managed.color[2] = static_cast<uint8_t>(color >> 8);
        if ((mask & 0x01) != 0)
            managed.color[3] = static_cast<uint8_t>(color);
        JsonArray colors = segment.createNestedArray("col");
        JsonArray primary = colors.createNestedArray();
        for (uint8_t component : managed.color)
            primary.add(component);
        break;
    }
    case 4:
    {
        const uint8_t options = ko.value(DPT_Value_1_Ucount);
        managed.selected = (options & 0x01) != 0;
        managed.reverse = (options & 0x02) != 0;
        managed.mirror = (options & 0x04) != 0;
        managed.freeze = (options & 0x08) != 0;
        segment["sel"] = managed.selected;
        segment["rev"] = managed.reverse;
        segment["mi"] = managed.mirror;
        segment["frz"] = managed.freeze;
        break;
    }
    case 5:
    {
        const char* name = ko.value(DPT_String_ASCII);
        if (name == nullptr)
            return;
        std::strncpy(managed.name, name, sizeof(managed.name) - 1);
        managed.name[sizeof(managed.name) - 1] = '\0';
        segment["n"] = managed.name;
        break;
    }
    default:
        return;
    }
    sendJson(document);
    publishManagedSegments();
}

void DigitalLedModule::publishManagedSegments()
{
    for (uint8_t index = 0; index < 3; ++index)
    {
        ManagedSegment& managed = state.managedSegments[index];
        const uint16_t base = APP_KoChManagedSegment1On + index * 6U;
        knx.getGroupObject(base).valueCompare(managed.on, DPT_Switch);
        knx.getGroupObject(base + 1U).valueCompare(managed.brightness, DPT_Percent_U8);
        knx.getGroupObject(base + 2U).valueCompare(cctToKelvin(managed.cct), Dpt(7, 600));
        publishRgbw(knx.getGroupObject(base + 3U), managed.color);
        const uint8_t options = (managed.present ? 0x80 : 0) | (managed.selected ? 0x01 : 0) |
                                (managed.reverse ? 0x02 : 0) | (managed.mirror ? 0x04 : 0) |
                                (managed.freeze ? 0x08 : 0);
        knx.getGroupObject(base + 4U).valueCompare(options, DPT_Value_1_Ucount);
        knx.getGroupObject(base + 5U).valueCompare(managed.name, DPT_String_ASCII);
    }
}

void DigitalLedModule::applySegmentConfiguration()
{
    if (state.segmentStop <= state.segmentStart)
    {
        logWarningP("Segment apply rejected: stop %u must be greater than start %u", state.segmentStop, state.segmentStart);
        return;
    }
    StaticJsonDocument<256> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["start"] = state.segmentStart;
    segment["stop"] = state.segmentStop;
    segment["grp"] = std::max<uint8_t>(state.segmentGrouping, 1);
    segment["spc"] = state.segmentSpacing;
    segment["of"] = state.segmentOffset;
    if (state.segmentName[0] != '\0')
        segment["n"] = state.segmentName;
    sendJson(document);
}

void DigitalLedModule::setSegmentActionArm(bool armed)
{
    state.segmentActionArmed = armed && ParamAPP_EnableDestructiveSegmentActions;
    segmentActionArmedAt = state.segmentActionArmed ? millis() : 0;
    KoAPP_ChSegmentActionArmState.valueCompare(state.segmentActionArmed, DPT_Switch);
}

void DigitalLedModule::deleteSegment()
{
    if (!ParamAPP_EnableDestructiveSegmentActions || !state.segmentActionArmed)
    {
        logWarningP("Segment delete rejected: destructive actions are not armed");
        return;
    }
    StaticJsonDocument<160> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    segment["stop"] = 0;
    sendJson(document);
    setSegmentActionArm(false);
}

void DigitalLedModule::sendSegmentColor(uint8_t index, uint32_t color, uint8_t mask)
{
    if (index < 1 || index > 2 || (mask & 0x0F) == 0)
        return;

    uint8_t* target = index == 1 ? state.secondaryColor : state.tertiaryColor;
    if ((mask & 0x08) != 0)
        target[0] = static_cast<uint8_t>(color >> 24);
    if ((mask & 0x04) != 0)
        target[1] = static_cast<uint8_t>(color >> 16);
    if ((mask & 0x02) != 0)
        target[2] = static_cast<uint8_t>(color >> 8);
    if ((mask & 0x01) != 0)
        target[3] = static_cast<uint8_t>(color);

    StaticJsonDocument<384> document;
    JsonObject segment = createSegmentCommand(document, ParamAPP_SegmentId, state.transitionMs);
    JsonArray colors = segment.createNestedArray("col");
    const uint8_t primary[4] = {state.red, state.green, state.blue, state.white};
    const uint8_t* entries[3] = {primary, state.secondaryColor, state.tertiaryColor};
    for (uint8_t colorIndex = 0; colorIndex <= index; ++colorIndex)
    {
        JsonArray entry = colors.createNestedArray();
        for (uint8_t component : {entries[colorIndex][0], entries[colorIndex][1], entries[colorIndex][2], entries[colorIndex][3]})
            entry.add(component);
    }
    sendJson(document);
    publishExtendedSegmentState();
}

void DigitalLedModule::sendTransition(uint16_t milliseconds)
{
    const uint16_t deciseconds = static_cast<uint16_t>(
        std::min<uint32_t>((static_cast<uint32_t>(milliseconds) + 50U) / 100U, 655U));
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["transition"] = deciseconds;
    sendJson(document);
    state.transitionMs = static_cast<uint16_t>(std::min<uint32_t>(static_cast<uint32_t>(deciseconds) * 100U, 65535U));
    KoAPP_ChTransitionValue.valueCompare(state.transitionMs, Dpt(7, 2));
}

void DigitalLedModule::sendSync(bool send, bool enabled)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2)> document;
    JsonObject sync = document["udpn"].to<JsonObject>();
    if (send)
    {
        sync["send"] = enabled;
        sync["sgrp"] = state.syncSendGroups;
    }
    else
        sync["rgrp"] = enabled ? state.syncReceiveGroups : 0;
    sendJson(document);
    if (send)
    {
        state.syncSend = enabled;
        KoAPP_ChSyncSendState.valueCompare(enabled, DPT_Switch);
    }
    else
    {
        state.syncReceive = enabled;
        KoAPP_ChSyncReceiveState.valueCompare(enabled, DPT_Switch);
    }
}

void DigitalLedModule::sendSyncGroups(bool send, uint8_t groups)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(1)> document;
    document["udpn"][send ? "sgrp" : "rgrp"] = groups;
    sendJson(document);
    if (send)
    {
        state.syncSendGroups = groups;
        KoAPP_ChSyncSendGroupsValue.valueCompare(groups, DPT_Value_1_Ucount);
    }
    else
    {
        state.syncReceiveGroups = groups;
        KoAPP_ChSyncReceiveGroupsValue.valueCompare(groups, DPT_Value_1_Ucount);
    }
}

void DigitalLedModule::sendLiveOverride(uint8_t mode)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["lor"] = mode;
    sendJson(document);
    state.liveOverride = mode;
    KoAPP_ChLiveOverrideValue.valueCompare(mode, DPT_Value_1_Ucount);
}

void DigitalLedModule::sendMainSegment(uint8_t segment)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["mainseg"] = segment;
    sendJson(document);
    state.mainSegment = segment;
    KoAPP_ChMainSegmentValue.valueCompare(segment, DPT_Value_1_Ucount);
}

void DigitalLedModule::sendTrigger(const char* key)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document[key] = true;
    sendJson(document);
}

void DigitalLedModule::sendPlaylist()
{
    if (!ParamAPP_EnableDestructivePresetActions || !state.presetActionArmed)
    {
        logWarningP("Playlist save rejected: destructive actions are not armed");
        return;
    }
    if (ParamAPP_PlaylistPresetSlot < 1 || ParamAPP_PlaylistPresetSlot > 250)
    {
        logWarningP("Playlist save rejected: Preset slot %u is out of range", ParamAPP_PlaylistPresetSlot);
        setPresetActionArm(false);
        return;
    }
    StaticJsonDocument<768> document;
    document["psave"] = ParamAPP_PlaylistPresetSlot;
    document["n"] = "KNX Playlist";
    document["o"] = true;
    document["on"] = true;
    JsonObject playlist = document.createNestedObject("playlist");
    JsonArray presets = playlist.createNestedArray("ps");
    JsonArray durations = playlist.createNestedArray("dur");
    JsonArray transitions = playlist.createNestedArray("transition");
    const uint8_t presetValues[5] = {
        ParamAPP_PlaylistPreset1, ParamAPP_PlaylistPreset2, ParamAPP_PlaylistPreset3,
        ParamAPP_PlaylistPreset4, ParamAPP_PlaylistPreset5};
    const uint16_t durationValues[5] = {
        ParamAPP_PlaylistDuration1, ParamAPP_PlaylistDuration2, ParamAPP_PlaylistDuration3,
        ParamAPP_PlaylistDuration4, ParamAPP_PlaylistDuration5};
    const uint16_t transitionValues[5] = {
        ParamAPP_PlaylistTransition1, ParamAPP_PlaylistTransition2, ParamAPP_PlaylistTransition3,
        ParamAPP_PlaylistTransition4, ParamAPP_PlaylistTransition5};
    for (uint8_t index = 0; index < 5; ++index)
    {
        if (presetValues[index] < 1 || presetValues[index] > 250)
            continue;
        presets.add(presetValues[index]);
        durations.add(durationValues[index]);
        transitions.add(transitionValues[index]);
    }
    if (presets.size() == 0)
    {
        logWarningP("Playlist not started: no configured preset entries");
        return;
    }
    playlist["repeat"] = ParamAPP_PlaylistRepeat;
    playlist["end"] = ParamAPP_PlaylistEnd;
    playlist["r"] = ParamAPP_PlaylistShuffle;
    sendJson(document);
    setPresetActionArm(false);
}

void DigitalLedModule::setPresetActionArm(bool armed)
{
    state.presetActionArmed = armed && ParamAPP_EnableDestructivePresetActions;
    presetActionArmedAt = state.presetActionArmed ? millis() : 0;
    KoAPP_ChPresetActionArmState.valueCompare(state.presetActionArmed, DPT_Switch);
}

void DigitalLedModule::sendPresetAction(const char* key, uint8_t preset)
{
    if (!ParamAPP_EnableDestructivePresetActions || !state.presetActionArmed)
    {
        logWarningP("Preset %s rejected: destructive actions are not armed", key);
        return;
    }
    if (preset < 1 || preset > 250)
    {
        logWarningP("Preset %s rejected: ID %u is out of range", key, preset);
        setPresetActionArm(false);
        return;
    }
    StaticJsonDocument<JSON_OBJECT_SIZE(4)> document;
    document[key] = preset;
    if (std::strcmp(key, "psave") == 0)
    {
        document["sb"] = ParamAPP_PresetSaveSegmentBounds;
        document["ib"] = ParamAPP_PresetSaveBrightness;
        document["sc"] = ParamAPP_PresetSaveSelectedSegments;
    }
    sendJson(document);
    setPresetActionArm(false);
}

void DigitalLedModule::sendPresetSelection(const char* selection)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["ps"] = selection;
    sendJson(document);
}

void DigitalLedModule::stopPlaylist()
{
    if (ParamAPP_PlaylistStopPreset < 1 || ParamAPP_PlaylistStopPreset > 250)
    {
        logWarningP("Playlist stop rejected: Preset %u is out of range", ParamAPP_PlaylistStopPreset);
        return;
    }
    sendPreset(ParamAPP_PlaylistStopPreset);
}

void DigitalLedModule::sendLive(bool enabled)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["live"] = enabled;
    sendJson(document);
    state.live = enabled;
    KoAPP_ChLiveState.valueCompare(enabled, DPT_Switch);
}

void DigitalLedModule::sendLedmap(uint8_t map)
{
    if (map > 15)
    {
        logWarningP("LED map %u rejected: expected 0..15", map);
        return;
    }
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["ledmap"] = map;
    sendJson(document);
    state.ledmap = map;
    KoAPP_ChLedmapValue.valueCompare(map, DPT_Value_1_Ucount);
}

void DigitalLedModule::sendTimebase(uint32_t timebase)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["tb"] = timebase;
    sendJson(document);
}

void DigitalLedModule::sendDeviceTime(uint32_t epoch)
{
    if (epoch == 0 || epoch == UINT32_MAX)
    {
        logWarningP("WLED time rejected: epoch %lu is not accepted by WLED", static_cast<unsigned long>(epoch));
        return;
    }
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["time"] = epoch;
    sendJson(document);
}

void DigitalLedModule::updateCommunicationHealth()
{
    uint32_t ageSeconds = 0;
    bool online = false;
    if (validResponseReceived)
    {
        ageSeconds = (millis() - lastValidResponseAt) / 1000UL;
        online = ParamAPP_CommunicationTimeoutSeconds == 0 ||
                 ageSeconds < ParamAPP_CommunicationTimeoutSeconds;
    }
    state.online = online;
    KoAPP_ChWledOnlineState.valueCompare(state.online, DPT_Switch);
    KoAPP_ChLastResponseAgeValue.valueCompare(ageSeconds, DPT_LongDeltaTimeSec);
    publishCommunicationHealth();
}

void DigitalLedModule::publishCommunicationHealth()
{
    KoAPP_ChJsonErrorCountValue.valueCompare(state.jsonErrorCount, DPT_Value_4_Ucount);
    KoAPP_ChOversizedFrameCountValue.valueCompare(state.oversizedFrameCount, DPT_Value_4_Ucount);
    KoAPP_ChUartOverflowCountValue.valueCompare(state.uartOverflowCount, DPT_Value_4_Ucount);
}

void DigitalLedModule::receiveSerial()
{
    if (DIGITAL_LED_SERIAL.overflow())
    {
        ++state.uartOverflowCount;
        publishCommunicationHealth();
        resetFrame();
        resyncAfterOverrun = true;
    }
    const uint32_t started = micros();
    for (size_t count = 0; count < 256 && micros() - started < 2000 && DIGITAL_LED_SERIAL.available() > 0; ++count)
    {
        const int input = DIGITAL_LED_SERIAL.read();
        if (input < 0)
            break;
        const char character = static_cast<char>(input);
        if (character == '\r' || character == '\n')
        {
            if (depth != 0 || discardingFrame || resyncAfterOverrun)
                resetFrame();
            continue;
        }
        if (discardingFrame || resyncAfterOverrun)
            continue;
        if (depth == 0)
        {
            if (character != '{')
                continue;
            depth = 0;
            frameLength = 0;
            frameStarted = millis();
            inString = false;
            escaped = false;
        }
        if (frameLength + 1 >= sizeof(frame))
        {
            ++state.oversizedFrameCount;
            publishCommunicationHealth();
            discardingFrame = true;
            continue;
        }
        frame[frameLength++] = character;
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (character == '\\')
                escaped = true;
            else if (character == '"')
                inString = false;
        }
        else if (character == '"')
            inString = true;
        else if (character == '{')
            ++depth;
        else if (character == '}' && --depth == 0)
        {
            processFrame();
            resetFrame();
            return;
        }
    }
    if (depth != 0 && millis() - frameStarted >= 1000)
        resetFrame();
}

void DigitalLedModule::processFrame()
{
    DynamicJsonDocument document(sizeof(frame));
    if (deserializeJson(document, frame, frameLength))
    {
        ++state.jsonErrorCount;
        publishCommunicationHealth();
        return;
    }
    JsonObjectConst stateObject = document["state"];
    if (stateObject.isNull())
        return;
    if (!validOptionalBool(stateObject, "on") || !validOptionalInteger(stateObject, "bri", 0, 255) ||
        !validOptionalInteger(stateObject, "transition", 0, 655) || !validOptionalInteger(stateObject, "ps", -1, 250) ||
        !validOptionalInteger(stateObject, "pl", -1, 250) || !validOptionalInteger(stateObject, "lor", 0, 2) ||
        !validOptionalInteger(stateObject, "mainseg", 0, 255) || !validOptionalInteger(stateObject, "error", 0, 255) ||
        !validOptionalInteger(stateObject, "ledmap", 0, 15))
        return;

    JsonObjectConst nightlight = stateObject["nl"];
    if (stateObject.containsKey("nl") && nightlight.isNull())
        return;
    if (!validOptionalBool(nightlight, "on") || !validOptionalInteger(nightlight, "dur", 0, 255) ||
        !validOptionalInteger(nightlight, "mode", 0, 3) || !validOptionalInteger(nightlight, "tbri", 0, 255) ||
        !validOptionalInteger(nightlight, "rem", -1, 15300))
        return;

    JsonObjectConst sync = stateObject["udpn"];
    if (stateObject.containsKey("udpn") && sync.isNull())
        return;
    if (!validOptionalBool(sync, "send") || !validOptionalBool(sync, "recv") ||
        !validOptionalInteger(sync, "sgrp", 0, 255) || !validOptionalInteger(sync, "rgrp", 0, 255))
        return;

    JsonArrayConst segments = stateObject["seg"];
    if (stateObject.containsKey("seg") && segments.isNull())
        return;
    JsonObjectConst segment;
    JsonObjectConst managedSegmentObjects[3];
    state.segmentPresent = false;
    for (ManagedSegment& managed : state.managedSegments)
        managed.present = false;
    if (!segments.isNull())
    {
        size_t index = 0;
        for (JsonObjectConst candidate : segments)
        {
            const int id = candidate["id"].is<int>() ? candidate["id"].as<int>() : static_cast<int>(index);
            if (id == ParamAPP_SegmentId)
            {
                segment = candidate;
                state.segmentPresent = true;
            }
            for (uint8_t managedIndex = 0; managedIndex < 3; ++managedIndex)
                if (id == managedSegmentId(managedIndex))
                {
                    managedSegmentObjects[managedIndex] = candidate;
                    state.managedSegments[managedIndex].present = true;
                }
            ++index;
        }
    }
    if (!segment.isNull() && (!validOptionalInteger(segment, "fx", 0, 255) || !validOptionalInteger(segment, "sx", 0, 255) ||
                              !validOptionalInteger(segment, "ix", 0, 255) || !validOptionalInteger(segment, "pal", 0, 255) ||
                              !validOptionalInteger(segment, "bri", 0, 255) || !validOptionalInteger(segment, "c1", 0, 255) ||
                              !validOptionalInteger(segment, "c2", 0, 255) || !validOptionalInteger(segment, "c3", 0, 31) ||
                              !validOptionalInteger(segment, "start", 0, 65535) || !validOptionalInteger(segment, "stop", 0, 65535) ||
                              !validOptionalInteger(segment, "grp", 0, 255) || !validOptionalInteger(segment, "spc", 0, 255) ||
                              !validOptionalInteger(segment, "of", -32768, 32767) ||
                              !validOptionalInteger(segment, "startY", 0, 65535) ||
                              !validOptionalInteger(segment, "stopY", 0, 65535) ||
                              !validOptionalInteger(segment, "set", 0, 3) ||
                              !validOptionalInteger(segment, "si", 0, 3) ||
                              !validOptionalInteger(segment, "m12", 0, 7) ||
                              !validOptionalBool(segment, "on") || !validOptionalBool(segment, "rev") ||
                              !validOptionalBool(segment, "mi") || !validOptionalBool(segment, "frz") ||
                              !validOptionalBool(segment, "sel") || !validOptionalBool(segment, "o1") ||
                              !validOptionalBool(segment, "o2") || !validOptionalBool(segment, "o3") ||
                              !validOptionalBool(segment, "rY") || !validOptionalBool(segment, "mY") ||
                              !validOptionalBool(segment, "tp") ||
                              (segment.containsKey("n") && !segment["n"].is<const char*>())))
        return;
    if (!segment.isNull() && segment.containsKey("cct") &&
        !validInteger(segment["cct"], 0, 255) && !validInteger(segment["cct"], 1900, 10091))
        return;
    for (JsonObjectConst managed : managedSegmentObjects)
        if (!managed.isNull() && !validManagedSegment(managed))
            return;

    JsonArrayConst colors;
    JsonArrayConst color;
    JsonArrayConst secondaryColor;
    JsonArrayConst tertiaryColor;
    if (!segment.isNull() && segment.containsKey("col"))
    {
        colors = segment["col"];
        if (colors.isNull() || colors.size() < 1 || colors.size() > 3)
            return;
        for (JsonArrayConst entry : colors)
        {
            if (entry.size() != 3 && entry.size() != 4)
                return;
            for (JsonVariantConst component : entry)
                if (!validInteger(component, 0, 255))
                    return;
        }
        color = colors[0];
        if (colors.size() > 1)
            secondaryColor = colors[1];
        if (colors.size() > 2)
            tertiaryColor = colors[2];
    }

    if (stateObject["on"].is<bool>())
    {
        state.on = stateObject["on"];
        KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
    }
    if (stateObject["bri"].is<uint8_t>())
    {
        state.brightness = stateObject["bri"];
        KoAPP_ChDimValue.valueCompare(state.brightness, DPT_Percent_U8);
    }
    if (stateObject["ps"].is<int>())
    {
        state.preset = stateObject["ps"];
        publishPreset();
    }
    if (stateObject["transition"].is<int>())
    {
        state.transitionMs = static_cast<uint16_t>(
            std::min<uint32_t>(static_cast<uint32_t>(stateObject["transition"].as<int>()) * 100U, 65535U));
    }
    if (stateObject["pl"].is<int>())
        state.playlist = stateObject["pl"];
    if (stateObject["lor"].is<int>())
        state.liveOverride = stateObject["lor"];
    if (stateObject["mainseg"].is<int>())
        state.mainSegment = stateObject["mainseg"];
    state.error = stateObject["error"].is<int>() ? stateObject["error"].as<uint8_t>() : 0;
    if (stateObject["ledmap"].is<int>())
        state.ledmap = stateObject["ledmap"];
    if (!nightlight.isNull() && nightlight["on"].is<bool>())
    {
        state.nightlight = nightlight["on"];
        KoAPP_ChNightlightState.valueCompare(state.nightlight, DPT_Switch);
    }
    if (!nightlight.isNull())
    {
        if (nightlight["dur"].is<int>())
            state.nightlightDuration = nightlight["dur"];
        if (nightlight["mode"].is<int>())
            state.nightlightMode = nightlight["mode"];
        if (nightlight["tbri"].is<int>())
            state.nightlightTarget = nightlight["tbri"];
        if (nightlight["rem"].is<int>())
            state.nightlightRemaining = nightlight["rem"];
    }
    if (!sync.isNull())
    {
        if (sync["send"].is<bool>())
            state.syncSend = sync["send"];
        if (sync["recv"].is<bool>())
            state.syncReceive = sync["recv"];
        if (sync["sgrp"].is<int>())
            state.syncSendGroups = sync["sgrp"];
        if (sync["rgrp"].is<int>())
            state.syncReceiveGroups = sync["rgrp"];
    }
    if (!segment.isNull())
    {
        if (segment["on"].is<bool>())
            state.segmentOn = segment["on"];
        if (segment["bri"].is<int>())
            state.segmentBrightness = segment["bri"];
        if (segment["rev"].is<bool>())
            state.segmentReverse = segment["rev"];
        if (segment["mi"].is<bool>())
            state.segmentMirror = segment["mi"];
        if (segment["frz"].is<bool>())
            state.segmentFreeze = segment["frz"];
        if (segment["sel"].is<bool>())
            state.segmentSelected = segment["sel"];
        if (segment["c1"].is<int>())
            state.effectCustom1 = segment["c1"];
        if (segment["c2"].is<int>())
            state.effectCustom2 = segment["c2"];
        if (segment["c3"].is<int>())
            state.effectCustom3 = segment["c3"];
        if (segment["o1"].is<bool>())
            state.effectOption1 = segment["o1"];
        if (segment["o2"].is<bool>())
            state.effectOption2 = segment["o2"];
        if (segment["o3"].is<bool>())
            state.effectOption3 = segment["o3"];
        if (segment["start"].is<int>())
            state.segmentStart = segment["start"];
        if (segment["stop"].is<int>())
            state.segmentStop = segment["stop"];
        if (segment["grp"].is<int>())
            state.segmentGrouping = segment["grp"];
        if (segment["spc"].is<int>())
            state.segmentSpacing = segment["spc"];
        if (segment["of"].is<int>())
            state.segmentOffset = segment["of"];
        if (segment["startY"].is<int>())
            state.matrixStartY = segment["startY"];
        if (segment["stopY"].is<int>())
            state.matrixStopY = segment["stopY"];
        if (segment["rY"].is<bool>())
            state.matrixReverseY = segment["rY"];
        if (segment["mY"].is<bool>())
            state.matrixMirrorY = segment["mY"];
        if (segment["tp"].is<bool>())
            state.matrixTranspose = segment["tp"];
        if (segment["set"].is<int>())
            state.segmentSet = segment["set"];
        if (segment["si"].is<int>())
            state.soundSimulation = segment["si"];
        if (segment["m12"].is<int>())
            state.map1D2D = segment["m12"];
        if (segment["n"].is<const char*>())
        {
            std::strncpy(state.segmentName, segment["n"].as<const char*>(), sizeof(state.segmentName) - 1);
            state.segmentName[sizeof(state.segmentName) - 1] = '\0';
        }
        if (segment["fx"].is<int>())
            state.effect = segment["fx"];
        if (segment["sx"].is<int>())
            state.effectSpeed = segment["sx"];
        if (segment["ix"].is<int>())
            state.effectIntensity = segment["ix"];
        if (segment["pal"].is<int>())
            state.palette = segment["pal"];
        publishEffects();
        if (segment["cct"].is<int>())
        {
            const uint16_t cct = segment["cct"];
            state.cct = cct <= 255 ? static_cast<uint8_t>(cct) : kelvinToCct(cct);
            publishColor();
            publishColorTemperature();
        }
        if (!color.isNull())
        {
            state.red = color[0];
            state.green = color[1];
            state.blue = color[2];
            if (color.size() == 4)
                state.white = color[3];
            publishColor();
        }
        const JsonArrayConst extraColors[2] = {secondaryColor, tertiaryColor};
        uint8_t* targets[2] = {state.secondaryColor, state.tertiaryColor};
        for (uint8_t index = 0; index < 2; ++index)
        {
            if (extraColors[index].isNull())
                continue;
            targets[index][0] = extraColors[index][0];
            targets[index][1] = extraColors[index][1];
            targets[index][2] = extraColors[index][2];
            targets[index][3] = extraColors[index].size() == 4 ? extraColors[index][3].as<uint8_t>() : 0;
        }
    }
    for (uint8_t index = 0; index < 3; ++index)
    {
        JsonObjectConst managedObject = managedSegmentObjects[index];
        if (managedObject.isNull())
            continue;
        ManagedSegment& managed = state.managedSegments[index];
        if (managedObject["on"].is<bool>())
            managed.on = managedObject["on"];
        if (managedObject["bri"].is<int>())
            managed.brightness = managedObject["bri"];
        if (managedObject["cct"].is<int>())
        {
            const uint16_t cct = managedObject["cct"];
            managed.cct = cct <= 255 ? static_cast<uint8_t>(cct) : kelvinToCct(cct);
        }
        if (managedObject["sel"].is<bool>())
            managed.selected = managedObject["sel"];
        if (managedObject["rev"].is<bool>())
            managed.reverse = managedObject["rev"];
        if (managedObject["mi"].is<bool>())
            managed.mirror = managedObject["mi"];
        if (managedObject["frz"].is<bool>())
            managed.freeze = managedObject["frz"];
        if (managedObject["n"].is<const char*>())
        {
            std::strncpy(managed.name, managedObject["n"].as<const char*>(), sizeof(managed.name) - 1);
            managed.name[sizeof(managed.name) - 1] = '\0';
        }
        JsonArrayConst managedColors = managedObject["col"];
        if (!managedColors.isNull() && managedColors.size() > 0)
        {
            JsonArrayConst managedColor = managedColors[0];
            managed.color[0] = managedColor[0];
            managed.color[1] = managedColor[1];
            managed.color[2] = managedColor[2];
            managed.color[3] = managedColor.size() == 4 ? managedColor[3].as<uint8_t>() : 0;
        }
    }
    publishAdvancedState();
    publishExtendedSegmentState();
    publishManagedSegments();

    JsonObjectConst info = document["info"];
    if (!info.isNull())
    {
        JsonObjectConst wifi = info["wifi"];
        JsonObjectConst leds = info["leds"];
        JsonObjectConst matrix = leds["matrix"];
        JsonObjectConst filesystem = info["fs"];
        JsonArrayConst maps = info["maps"];
        if ((!wifi.isNull() && (!validOptionalInteger(wifi, "rssi", -127, 0) || !validOptionalInteger(wifi, "signal", 0, 100) ||
                                !validOptionalInteger(wifi, "channel", 0, 255) ||
                                (wifi.containsKey("bssid") && !wifi["bssid"].is<const char*>()))) ||
            (!leds.isNull() && (!validOptionalInteger(leds, "maxseg", 0, 255) ||
                                !validOptionalInteger(leds, "count", 0, 65535) ||
                                !validOptionalInteger(leds, "fps", 0, 65535) ||
            !validOptionalInteger(leds, "pwr", 0, 65535) ||
            !validOptionalBool(leds, "rgbw") ||
            !validOptionalInteger(leds, "wv", 0, 2) ||
            !validOptionalInteger(leds, "cct", 0, 4) ||
            (leds.containsKey("wv") && leds["wv"].as<int>() != 0 && leds["wv"].as<int>() != 2) ||
            (leds.containsKey("cct") && leds["cct"].as<int>() != 0 && leds["cct"].as<int>() != 4) ||
            !validOptionalInteger(leds, "maxpwr", 0, 65535))) ||
            (!leds.isNull() && leds.containsKey("matrix") && matrix.isNull()) ||
            (!matrix.isNull() && (!validOptionalInteger(matrix, "w", 0, 65535) ||
                                  !validOptionalInteger(matrix, "h", 0, 65535))) ||
            (info.containsKey("fs") && filesystem.isNull()) ||
            (!filesystem.isNull() && (!validOptionalUnsigned(filesystem, "u") ||
                                      !validOptionalUnsigned(filesystem, "t") ||
                                      !validOptionalUnsigned(filesystem, "pmt"))) ||
            (info.containsKey("maps") && maps.isNull()) ||
            !validOptionalUnsigned(info, "vid") || !validOptionalUnsigned(info, "uptime") ||
            !validOptionalUnsigned(info, "freeheap") || !validOptionalInteger(info, "fxcount", 0, 65535) ||
            !validOptionalInteger(info, "palcount", 0, 65535) || !validOptionalBool(info, "live") ||
            !validOptionalInteger(info, "liveseg", -1, 255) || !validOptionalInteger(info, "ndc", -1, 150) ||
            !validOptionalInteger(info, "ws", -1, 32767) ||
            (info.containsKey("lm") && !info["lm"].is<const char*>()) ||
            (info.containsKey("ver") && !info["ver"].is<const char*>()) ||
            (info.containsKey("ip") && !info["ip"].is<const char*>()) ||
            (info.containsKey("lip") && !info["lip"].is<const char*>()) ||
            (info.containsKey("time") && !info["time"].is<const char*>()))
            return;
        if (!wifi.isNull())
        {
            if (wifi["rssi"].is<int>())
                state.wifiRssi = wifi["rssi"];
            if (wifi["signal"].is<int>())
                state.wifiSignal = wifi["signal"];
            if (wifi["channel"].is<int>())
                state.wifiChannel = wifi["channel"];
            if (wifi["bssid"].is<const char*>())
                compactBssid(wifi["bssid"], state.wifiBssid);
        }
        if (!leds.isNull())
        {
            if (leds["maxseg"].is<int>())
                state.maxSegments = leds["maxseg"];
            if (leds["count"].is<int>())
                state.ledCount = leds["count"];
            if (leds["fps"].is<int>())
                state.ledFps = leds["fps"];
            if (leds["pwr"].is<int>())
                state.ledCurrentMa = leds["pwr"];
            if (leds["maxpwr"].is<int>())
                state.ledMaxCurrentMa = leds["maxpwr"];
            state.matrixWidth = 0;
            state.matrixHeight = 0;
            state.isMatrix = !matrix.isNull();
            if (!matrix.isNull())
            {
                if (matrix["w"].is<int>())
                    state.matrixWidth = matrix["w"];
                if (matrix["h"].is<int>())
                    state.matrixHeight = matrix["h"];
            }
            if (leds["rgbw"].is<bool>())
                state.hasRgbw = leds["rgbw"];
            if (leds["wv"].is<int>())
                state.hasWhite = leds["wv"].as<int>() != 0;
            if (leds["cct"].is<int>())
                state.hasCct = leds["cct"].as<int>() != 0;
            if (state.ledMaxCurrentMa == 0)
            {
                state.currentPercent = 0;
                state.currentWarning = false;
                state.currentExceeded = false;
            }
            else
            {
                state.currentPercent = static_cast<uint8_t>(std::min<uint32_t>(
                    static_cast<uint32_t>(state.ledCurrentMa) * 255U / state.ledMaxCurrentMa, 255U));
                const uint32_t percent = static_cast<uint32_t>(state.ledCurrentMa) * 100U / state.ledMaxCurrentMa;
                state.currentWarning = percent >= ParamAPP_PowerWarningPercent;
                state.currentExceeded = state.ledCurrentMa > state.ledMaxCurrentMa;
            }
        }
        state.fsUsedKb = 0;
        state.fsTotalKb = 0;
        state.presetsModified = 0;
        if (!filesystem.isNull())
        {
            if (filesystem["u"].is<uint32_t>())
                state.fsUsedKb = filesystem["u"];
            if (filesystem["t"].is<uint32_t>())
                state.fsTotalKb = filesystem["t"];
            if (filesystem["pmt"].is<uint32_t>())
                state.presetsModified = filesystem["pmt"];
        }
        state.ledMapCount = maps.isNull() ? 0 : static_cast<uint16_t>(maps.size());
        if (info["vid"].is<uint32_t>())
            state.buildId = info["vid"];
        if (info["uptime"].is<uint32_t>())
            state.uptime = info["uptime"];
        if (info["freeheap"].is<uint32_t>())
            state.freeHeap = info["freeheap"];
        if (info["fxcount"].is<int>())
            state.effectCount = info["fxcount"];
        if (info["palcount"].is<int>())
            state.paletteCount = info["palcount"];
        if (info["ndc"].is<int>())
            state.nodeCount = info["ndc"];
        if (info["ip"].is<const char*>())
        {
            state.ipAddress = 0;
            parseIpAddress(info["ip"], state.ipAddress);
        }
        if (info["time"].is<const char*>())
            state.localTimeValid = parseLocalTime(info["time"], state.localTime);
        if (info["lip"].is<const char*>())
        {
            state.realtimeSourceIp = 0;
            parseIpAddress(info["lip"], state.realtimeSourceIp);
        }
        if (info["ws"].is<int>())
            state.websocketClients = info["ws"];
        if (info["live"].is<bool>())
            state.live = info["live"];
        if (info["liveseg"].is<int>())
            state.liveSegment = info["liveseg"].as<int>() < 0 ? 255 : info["liveseg"].as<uint8_t>();
        if (info["lm"].is<const char*>())
        {
            std::strncpy(state.liveMode, info["lm"].as<const char*>(), sizeof(state.liveMode) - 1);
            state.liveMode[sizeof(state.liveMode) - 1] = '\0';
        }
        if (info["ver"].is<const char*>())
        {
            std::strncpy(state.version, info["ver"].as<const char*>(), sizeof(state.version) - 1);
            state.version[sizeof(state.version) - 1] = '\0';
        }
        publishDeviceInfo();
    }
    validResponseReceived = true;
    lastValidResponseAt = millis();
    state.online = true;
    updateCommunicationHealth();
}

void DigitalLedModule::resetFrame()
{
    frameLength = 0;
    depth = 0;
    inString = false;
    escaped = false;
    discardingFrame = false;
    resyncAfterOverrun = false;
    frameStarted = 0;
}

void DigitalLedModule::publishColor()
{
    KoAPP_ChRedValue.valueCompare(state.red, DPT_Percent_U8);
    KoAPP_ChGreenValue.valueCompare(state.green, DPT_Percent_U8);
    KoAPP_ChBlueValue.valueCompare(state.blue, DPT_Percent_U8);
    KoAPP_ChRGBValue.valueCompare((static_cast<uint32_t>(state.red) << 16) | (static_cast<uint32_t>(state.green) << 8) | state.blue, DPT_Colour_RGB);
    KoAPP_ChWhiteValue.valueCompare(state.white, DPT_Value_1_Ucount);
    const uint8_t warm = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * (255U - state.cct) + 127U) / 255U);
    const uint8_t cold = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * state.cct + 127U) / 255U);
    KoAPP_ChWarmWhiteValue.valueCompare(warm, DPT_Value_1_Ucount);
    KoAPP_ChColdWhiteValue.valueCompare(cold, DPT_Value_1_Ucount);

    float hue;
    float saturation;
    float value;
    rgbToHsv(state.red, state.green, state.blue, hue, saturation, value);
    KoAPP_ChHueValue.valueCompare(static_cast<uint16_t>(std::round(hue)), DPT_Angle);
    KoAPP_ChSaturationValue.valueCompare(static_cast<uint8_t>(std::round(saturation * 255.0F)), DPT_Percent_U8);
    KoAPP_ChValueValue.valueCompare(static_cast<uint8_t>(std::round(value * 255.0F)), DPT_Percent_U8);

    GroupObject& rgbwObject = KoAPP_ChRGBWValue;
    const uint32_t rgbw = (static_cast<uint32_t>(state.red) << 24) | (static_cast<uint32_t>(state.green) << 16) |
                          (static_cast<uint32_t>(state.blue) << 8) | state.white;
    const bool changed = !rgbwObject.initialized() || static_cast<uint32_t>(rgbwObject.value(DPT_Colour_RGBW)) != rgbw ||
                         static_cast<uint8_t>(rgbwObject.value(Dpt(251, 600, 1))) != 0x0F;
    const bool valueEncoded = rgbwObject.valueNoSend(rgbw, DPT_Colour_RGBW);
    const bool maskEncoded = rgbwObject.valueNoSend(static_cast<uint8_t>(0x0F), Dpt(251, 600, 1));
    if (changed && valueEncoded && maskEncoded)
        rgbwObject.objectWritten();
}

void DigitalLedModule::publishColorTemperature()
{
    KoAPP_ChCctValue.valueCompare(cctToKelvin(state.cct), Dpt(7, 600));
}

void DigitalLedModule::publishPreset()
{
    KoAPP_ChPresetValue.valueCompare(static_cast<uint8_t>(state.preset > 0 ? state.preset : 0), DPT_Percent_U8);
    if (state.preset >= 1 && state.preset <= 64)
        KoAPP_ChSceneState.valueCompare(static_cast<uint8_t>(state.preset - 1), DPT_SceneNumber);
}

void DigitalLedModule::publishEffects()
{
    KoAPP_ChPaletteValue.valueCompare(state.palette, DPT_Percent_U8);
    KoAPP_ChEffectIdValue.valueCompare(state.effect, DPT_Percent_U8);
    KoAPP_ChEffectSpeedValue.valueCompare(state.effectSpeed, DPT_Percent_U8);
    KoAPP_ChEffectIntensityValue.valueCompare(state.effectIntensity, DPT_Percent_U8);
}

void DigitalLedModule::publishAdvancedState()
{
    KoAPP_ChSegmentState.valueCompare(state.segmentOn, DPT_Switch);
    KoAPP_ChSegmentBrightnessValue.valueCompare(state.segmentBrightness, DPT_Percent_U8);
    KoAPP_ChSegmentReverseState.valueCompare(state.segmentReverse, DPT_Switch);
    KoAPP_ChSegmentMirrorState.valueCompare(state.segmentMirror, DPT_Switch);
    KoAPP_ChSegmentFreezeState.valueCompare(state.segmentFreeze, DPT_Switch);
    KoAPP_ChSegmentSelectedState.valueCompare(state.segmentSelected, DPT_Switch);
    KoAPP_ChTransitionValue.valueCompare(state.transitionMs, Dpt(7, 2));
    KoAPP_ChSyncSendState.valueCompare(state.syncSend, DPT_Switch);
    KoAPP_ChSyncReceiveState.valueCompare(state.syncReceive, DPT_Switch);
    KoAPP_ChSyncSendGroupsValue.valueCompare(state.syncSendGroups, DPT_Value_1_Ucount);
    KoAPP_ChSyncReceiveGroupsValue.valueCompare(state.syncReceiveGroups, DPT_Value_1_Ucount);
    KoAPP_ChLiveOverrideValue.valueCompare(state.liveOverride, DPT_Value_1_Ucount);
    KoAPP_ChMainSegmentValue.valueCompare(state.mainSegment, DPT_Value_1_Ucount);
    KoAPP_ChPlaylistValue.valueCompare(static_cast<uint8_t>(state.playlist > 0 ? state.playlist : 0), DPT_Value_1_Ucount);
    KoAPP_ChNightlightDurationValue.valueCompare(state.nightlightDuration, DPT_Value_1_Ucount);
    KoAPP_ChNightlightModeValue.valueCompare(state.nightlightMode, DPT_Value_1_Ucount);
    KoAPP_ChNightlightTargetValue.valueCompare(state.nightlightTarget, DPT_Value_1_Ucount);
    KoAPP_ChNightlightRemainingValue.valueCompare(state.nightlightRemaining, DPT_Value_2_Count);
    KoAPP_ChPlaylistActiveState.valueCompare(state.playlist >= 0, DPT_Switch);
    KoAPP_ChPresetActiveState.valueCompare(state.preset >= 1, DPT_Switch);
}

void DigitalLedModule::publishExtendedSegmentState()
{
    KoAPP_ChEffectCustom1Value.valueCompare(state.effectCustom1, DPT_Value_1_Ucount);
    KoAPP_ChEffectCustom2Value.valueCompare(state.effectCustom2, DPT_Value_1_Ucount);
    KoAPP_ChEffectCustom3Value.valueCompare(state.effectCustom3, DPT_Value_1_Ucount);
    KoAPP_ChEffectOption1State.valueCompare(state.effectOption1, DPT_Switch);
    KoAPP_ChEffectOption2State.valueCompare(state.effectOption2, DPT_Switch);
    KoAPP_ChEffectOption3State.valueCompare(state.effectOption3, DPT_Switch);
    KoAPP_ChSegmentStartValue.valueCompare(state.segmentStart, DPT_Value_2_Ucount);
    KoAPP_ChSegmentStopValue.valueCompare(state.segmentStop, DPT_Value_2_Ucount);
    KoAPP_ChSegmentGroupingValue.valueCompare(state.segmentGrouping, DPT_Value_1_Ucount);
    KoAPP_ChSegmentSpacingValue.valueCompare(state.segmentSpacing, DPT_Value_1_Ucount);
    KoAPP_ChSegmentOffsetValue.valueCompare(state.segmentOffset, DPT_Value_2_Count);
    KoAPP_ChSegmentNameValue.valueCompare(state.segmentName, DPT_String_ASCII);
    KoAPP_ChSegmentPresentState.valueCompare(state.segmentPresent, DPT_Switch);
    KoAPP_ChMatrixStartY.valueCompare(state.matrixStartY, DPT_Value_2_Ucount);
    KoAPP_ChMatrixStopY.valueCompare(state.matrixStopY, DPT_Value_2_Ucount);
    KoAPP_ChMatrixReverseY.valueCompare(state.matrixReverseY, DPT_Switch);
    KoAPP_ChMatrixMirrorY.valueCompare(state.matrixMirrorY, DPT_Switch);
    KoAPP_ChMatrixTranspose.valueCompare(state.matrixTranspose, DPT_Switch);
    KoAPP_ChSegmentSet.valueCompare(state.segmentSet, DPT_Value_1_Ucount);
    KoAPP_ChSoundSimulation.valueCompare(state.soundSimulation, DPT_Value_1_Ucount);
    KoAPP_ChMap1D2D.valueCompare(state.map1D2D, DPT_Value_1_Ucount);
    publishRgbw(KoAPP_ChSecondaryRGBWValue, state.secondaryColor);
    publishRgbw(KoAPP_ChTertiaryRGBWValue, state.tertiaryColor);
}

void DigitalLedModule::publishDeviceInfo()
{
    KoAPP_ChLiveState.valueCompare(state.live, DPT_Switch);
    KoAPP_ChLiveSegmentValue.valueCompare(state.liveSegment, DPT_Value_1_Ucount);
    KoAPP_ChLiveModeValue.valueCompare(state.liveMode, DPT_String_ASCII);
    KoAPP_ChWifiRssiValue.valueCompare(state.wifiRssi, DPT_Value_1_Count);
    KoAPP_ChWifiSignalValue.valueCompare(state.wifiSignal, DPT_Percent_U8);
    KoAPP_ChUptimeValue.valueCompare(state.uptime, DPT_LongDeltaTimeSec);
    KoAPP_ChFreeHeapValue.valueCompare(state.freeHeap, DPT_Value_4_Ucount);
    KoAPP_ChErrorValue.valueCompare(state.error, DPT_Value_1_Ucount);
    KoAPP_ChBuildIdValue.valueCompare(state.buildId, DPT_Value_4_Ucount);
    KoAPP_ChEffectCountValue.valueCompare(state.effectCount, DPT_Value_2_Ucount);
    KoAPP_ChPaletteCountValue.valueCompare(state.paletteCount, DPT_Value_2_Ucount);
    KoAPP_ChMaxSegmentsValue.valueCompare(state.maxSegments, DPT_Value_1_Ucount);
    KoAPP_ChVersionValue.valueCompare(state.version, DPT_String_ASCII);
    KoAPP_ChLedmapValue.valueCompare(state.ledmap, DPT_Value_1_Ucount);
    KoAPP_ChNodeCountValue.valueCompare(state.nodeCount, DPT_Value_2_Count);
    KoAPP_ChLedCurrentValue.valueCompare(state.ledCurrentMa, DPT_Value_2_Ucount);
    KoAPP_ChLedMaxCurrentValue.valueCompare(state.ledMaxCurrentMa, DPT_Value_2_Ucount);
    KoAPP_ChIpAddressValue.valueCompare(state.ipAddress, DPT_Value_4_Ucount);
    KoAPP_ChWifiBssidValue.valueCompare(state.wifiBssid, DPT_String_ASCII);
    KoAPP_ChWifiChannelValue.valueCompare(state.wifiChannel, DPT_Value_1_Ucount);
    KoAPP_ChLedCountValue.valueCompare(state.ledCount, DPT_Value_2_Ucount);
    KoAPP_ChLedFpsValue.valueCompare(state.ledFps, DPT_Value_2_Ucount);
    KoAPP_ChMatrixWidthValue.valueCompare(state.matrixWidth, DPT_Value_2_Ucount);
    KoAPP_ChMatrixHeightValue.valueCompare(state.matrixHeight, DPT_Value_2_Ucount);
    KoAPP_ChFsUsedValue.valueCompare(state.fsUsedKb, DPT_Value_4_Ucount);
    KoAPP_ChFsTotalValue.valueCompare(state.fsTotalKb, DPT_Value_4_Ucount);
    KoAPP_ChPresetsModifiedValue.valueCompare(state.presetsModified, DPT_Value_4_Ucount);
    KoAPP_ChLedMapCountValue.valueCompare(state.ledMapCount, DPT_Value_2_Ucount);
    KoAPP_ChRealtimeSourceIpValue.valueCompare(state.realtimeSourceIp, DPT_Value_4_Ucount);
    KoAPP_ChWebsocketClientsValue.valueCompare(state.websocketClients, DPT_Value_2_Count);
    KoAPP_ChCurrentPercentValue.valueCompare(state.currentPercent, DPT_Percent_U8);
    KoAPP_ChCurrentWarningState.valueCompare(state.currentWarning, DPT_Switch);
    KoAPP_ChCurrentExceededState.valueCompare(state.currentExceeded, DPT_Switch);
    KoAPP_ChHasRgbwState.valueCompare(state.hasRgbw, DPT_Switch);
    KoAPP_ChHasWhiteState.valueCompare(state.hasWhite, DPT_Switch);
    KoAPP_ChHasCctState.valueCompare(state.hasCct, DPT_Switch);
    KoAPP_ChIsMatrixState.valueCompare(state.isMatrix, DPT_Switch);
    if (state.localTimeValid)
    {
        KoAPP_ChLocalDateValue.valueCompare(state.localTime, DPT_Date);
        KoAPP_ChLocalTimeValue.valueCompare(state.localTime, DPT_TimeOfDay);
    }
}

void DigitalLedModule::publishRgbw(GroupObject& object, const uint8_t color[4])
{
    const uint32_t value = (static_cast<uint32_t>(color[0]) << 24) | (static_cast<uint32_t>(color[1]) << 16) |
                           (static_cast<uint32_t>(color[2]) << 8) | color[3];
    const bool changed = !object.initialized() || static_cast<uint32_t>(object.value(DPT_Colour_RGBW)) != value ||
                         static_cast<uint8_t>(object.value(Dpt(251, 600, 1))) != 0x0F;
    const bool valueEncoded = object.valueNoSend(value, DPT_Colour_RGBW);
    const bool maskEncoded = object.valueNoSend(static_cast<uint8_t>(0x0F), Dpt(251, 600, 1));
    if (changed && valueEncoded && maskEncoded)
        object.objectWritten();
}

uint16_t DigitalLedModule::cctToKelvin(uint8_t cct) const
{
    uint16_t minimum = ParamAPP_CctKelvinMin;
    uint16_t maximum = ParamAPP_CctKelvinMax;
    if (minimum > maximum)
        std::swap(minimum, maximum);
    if (minimum == maximum)
        return minimum;
    return static_cast<uint16_t>(minimum + (static_cast<uint32_t>(cct) * (maximum - minimum) + 127U) / 255U);
}

uint8_t DigitalLedModule::kelvinToCct(uint16_t kelvin) const
{
    uint16_t minimum = ParamAPP_CctKelvinMin;
    uint16_t maximum = ParamAPP_CctKelvinMax;
    if (minimum > maximum)
        std::swap(minimum, maximum);
    if (minimum == maximum)
        return 127;
    kelvin = std::max(minimum, std::min(maximum, kelvin));
    return static_cast<uint8_t>((static_cast<uint32_t>(kelvin - minimum) * 255U + (maximum - minimum) / 2U) / (maximum - minimum));
}

void DigitalLedModule::rgbToHsv(uint8_t red, uint8_t green, uint8_t blue, float& hue, float& saturation, float& value)
{
    const float r = red / 255.0F;
    const float g = green / 255.0F;
    const float b = blue / 255.0F;
    const float maximum = std::max(r, std::max(g, b));
    const float minimum = std::min(r, std::min(g, b));
    const float delta = maximum - minimum;

    hue = 0.0F;
    if (delta > 0.0F)
    {
        if (maximum == r)
            hue = 60.0F * std::fmod((g - b) / delta, 6.0F);
        else if (maximum == g)
            hue = 60.0F * (((b - r) / delta) + 2.0F);
        else
            hue = 60.0F * (((r - g) / delta) + 4.0F);
        if (hue < 0.0F)
            hue += 360.0F;
    }
    saturation = maximum == 0.0F ? 0.0F : delta / maximum;
    value = maximum;
}

void DigitalLedModule::hsvToRgb(float hue, float saturation, float value, uint8_t& red, uint8_t& green, uint8_t& blue)
{
    while (hue < 0.0F)
        hue += 360.0F;
    while (hue >= 360.0F)
        hue -= 360.0F;
    const float chroma = value * saturation;
    const float x = chroma * (1.0F - std::fabs(std::fmod(hue / 60.0F, 2.0F) - 1.0F));
    const float match = value - chroma;
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    if (hue < 60.0F)
        r = chroma, g = x;
    else if (hue < 120.0F)
        r = x, g = chroma;
    else if (hue < 180.0F)
        g = chroma, b = x;
    else if (hue < 240.0F)
        g = x, b = chroma;
    else if (hue < 300.0F)
        r = x, b = chroma;
    else
        r = chroma, b = x;
    red = static_cast<uint8_t>(std::round((r + match) * 255.0F));
    green = static_cast<uint8_t>(std::round((g + match) * 255.0F));
    blue = static_cast<uint8_t>(std::round((b + match) * 255.0F));
}
