#include "DigitalLedModule.h"
#undef memcpy_P
#include <ArduinoJson.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>

DigitalLedModule digitalLedModule;

namespace
{
    // WLED JSON API field reference: https://kno.wled.ge/interfaces/json-api/
    template <typename Document>
    void sendJson(Document& document)
    {
        serializeJson(document, DIGITAL_LED_SERIAL);
        DIGITAL_LED_SERIAL.print("\r\n");
    }

    bool validInteger(JsonVariantConst value, int minimum, int maximum)
    {
        return value.is<int>() && value.as<int>() >= minimum && value.as<int>() <= maximum;
    }

    bool validOptionalInteger(JsonObjectConst object, const char* key, int minimum, int maximum)
    {
        return !object.containsKey(key) || validInteger(object[key], minimum, maximum);
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
}

const std::string DigitalLedModule::name()
{
    return "OFM Digital LED Controller";
}

const std::string DigitalLedModule::version()
{
    return "0.2.0";
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
    applyConfiguredDefaults();
    lastRequest = millis();
}

void DigitalLedModule::loop()
{
    receiveSerial();
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
    StaticJsonDocument<JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(4)> document;
    document["on"] = state.brightness != 0;
    document["bri"] = state.brightness;
    document["seg"][0]["pal"] = state.palette;
    document["seg"][0]["fx"] = state.effect;
    document["seg"][0]["sx"] = state.effectSpeed;
    document["seg"][0]["ix"] = state.effectIntensity;
    sendJson(document);
    state.on = state.brightness != 0;
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
    KoAPP_ChDimValue.valueCompare(state.brightness, DPT_Percent_U8);
    publishEffects();
}

void DigitalLedModule::sendSwitch(bool on)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["on"] = on;
    sendJson(document);
    state.on = on;
    KoAPP_ChDimState.valueCompare(on, DPT_Switch);
}

void DigitalLedModule::sendBrightness(uint8_t brightness)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["bri"] = brightness;
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
    StaticJsonDocument<JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4)> document;
    document["on"] = red != 0 || green != 0 || blue != 0 || white != 0;
    JsonArray color = document["seg"][0]["col"].createNestedArray();
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
    StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1)> document;
    document["seg"][0]["cct"] = kelvinToCct(kelvin);
    sendJson(document);
    state.cct = document["seg"][0]["cct"];
    publishColor();
    publishColorTemperature();
}

void DigitalLedModule::sendWarmWhite(uint8_t value)
{
    const uint8_t cold = static_cast<uint8_t>((static_cast<uint16_t>(state.white) * state.cct + 127U) / 255U);
    const uint16_t sum = static_cast<uint16_t>(value) + cold;
    const uint8_t white = static_cast<uint8_t>(std::min(sum, static_cast<uint16_t>(255)));
    const uint8_t cct = sum == 0 ? state.cct : static_cast<uint8_t>((static_cast<uint16_t>(cold) * 255U + sum / 2U) / sum);

    StaticJsonDocument<JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4)> document;
    document["on"] = state.red != 0 || state.green != 0 || state.blue != 0 || white != 0;
    JsonObject segment = document["seg"][0];
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

    StaticJsonDocument<JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4)> document;
    document["on"] = state.red != 0 || state.green != 0 || state.blue != 0 || white != 0;
    JsonObject segment = document["seg"][0];
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
    StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4)> document;
    JsonObject nightlight = document["nl"].to<JsonObject>();
    nightlight["on"] = enabled;
    nightlight["dur"] = ParamAPP_NightlightDuration;
    nightlight["mode"] = ParamAPP_NightlightMode;
    nightlight["tbri"] = ParamAPP_NightlightTargetBrightness;
    sendJson(document);
    state.nightlight = enabled;
    KoAPP_ChNightlightState.valueCompare(enabled, DPT_Switch);
}

void DigitalLedModule::sendPreset(uint8_t preset)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> document;
    document["ps"] = preset;
    sendJson(document);
    state.preset = preset;
    publishPreset();
    if (preset <= 64)
        KoAPP_ChSceneState.valueCompare(static_cast<uint8_t>(preset - 1), DPT_SceneNumber);
}

void DigitalLedModule::sendSegmentValue(const char* key, uint8_t value)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1)> document;
    document["on"] = true;
    document["seg"][0][key] = value;
    sendJson(document);
    if (std::strcmp(key, "pal") == 0)
        state.palette = value;
    else if (std::strcmp(key, "fx") == 0)
        state.effect = value;
    else if (std::strcmp(key, "sx") == 0)
        state.effectSpeed = value;
    else if (std::strcmp(key, "ix") == 0)
        state.effectIntensity = value;
    state.on = true;
    KoAPP_ChDimState.valueCompare(state.on, DPT_Switch);
    publishEffects();
}

void DigitalLedModule::receiveSerial()
{
    if (DIGITAL_LED_SERIAL.overflow())
    {
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
    StaticJsonDocument<1024> document;
    if (deserializeJson(document, frame, frameLength))
        return;
    JsonObjectConst stateObject = document["state"];
    if (stateObject.isNull())
        return;
    if (!validOptionalBool(stateObject, "on") || !validOptionalInteger(stateObject, "bri", 0, 255) ||
        !validOptionalInteger(stateObject, "ps", -1, 250))
        return;

    JsonObjectConst nightlight = stateObject["nl"];
    if (stateObject.containsKey("nl") && nightlight.isNull())
        return;
    if (!validOptionalBool(nightlight, "on") || !validOptionalInteger(nightlight, "dur", 1, 255) ||
        !validOptionalInteger(nightlight, "mode", 0, 3) || !validOptionalInteger(nightlight, "tbri", 0, 255))
        return;

    JsonArrayConst segments = stateObject["seg"];
    if (stateObject.containsKey("seg") && segments.isNull())
        return;
    JsonObjectConst segment;
    if (!segments.isNull() && segments.size() > 0)
        segment = segments[0];
    if (!segment.isNull() && (!validOptionalInteger(segment, "fx", 0, 255) || !validOptionalInteger(segment, "sx", 0, 255) ||
                              !validOptionalInteger(segment, "ix", 0, 255) || !validOptionalInteger(segment, "pal", 0, 255) ||
                              !validOptionalInteger(segment, "cct", 0, 255)))
        return;

    JsonArrayConst color;
    if (!segment.isNull() && segment.containsKey("col"))
    {
        color = segment["col"][0];
        if (color.isNull() || (color.size() != 3 && color.size() != 4))
            return;
        for (JsonVariantConst component : color)
            if (!validInteger(component, 0, 255))
                return;
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
    if (!nightlight.isNull() && nightlight["on"].is<bool>())
    {
        state.nightlight = nightlight["on"];
        KoAPP_ChNightlightState.valueCompare(state.nightlight, DPT_Switch);
    }
    if (!segment.isNull())
    {
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
            state.cct = segment["cct"];
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
    }
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
