#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include <Arduino.h>
#include <ctime>
#include <stdint.h>

class DigitalLedModule : public OpenKNX::Module
{
  public:
    void setup(bool configured) override;
    void loop() override;
    void processInputKo(GroupObject& ko) override;
    const std::string name() override;
    const std::string version() override;
    bool processCommand(const std::string command, bool diagnoseKo) override;
    void showHelp() override;
    void showInformations() override;

  private:
    struct ManagedSegment
    {
        bool present = false;
        bool on = false;
        uint8_t brightness = 0;
        uint8_t cct = 127;
        uint8_t color[4]{};
        bool selected = false;
        bool reverse = false;
        bool mirror = false;
        bool freeze = false;
        char name[15]{};
    };

    struct State
    {
        bool on = false;
        uint8_t brightness = 0;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        uint8_t white = 0;
        uint8_t cct = 127;
        bool nightlight = false;
        uint8_t nightlightDuration = 0;
        uint8_t nightlightMode = 0;
        uint8_t nightlightTarget = 0;
        int16_t nightlightRemaining = -1;
        bool segmentOn = true;
        bool segmentPresent = false;
        bool segmentReverse = false;
        bool segmentMirror = false;
        bool segmentFreeze = false;
        bool segmentSelected = false;
        bool syncSend = false;
        bool syncReceive = false;
        uint8_t palette = 0;
        uint8_t effect = 0;
        uint8_t effectSpeed = 0;
        uint8_t effectIntensity = 0;
        uint8_t effectCustom1 = 0;
        uint8_t effectCustom2 = 0;
        uint8_t effectCustom3 = 0;
        bool effectOption1 = false;
        bool effectOption2 = false;
        bool effectOption3 = false;
        uint16_t segmentStart = 0;
        uint16_t segmentStop = 0;
        uint8_t segmentGrouping = 1;
        uint8_t segmentSpacing = 0;
        int16_t segmentOffset = 0;
        char segmentName[15]{};
        uint16_t matrixStartY = 0;
        uint16_t matrixStopY = 1;
        bool matrixReverseY = false;
        bool matrixMirrorY = false;
        bool matrixTranspose = false;
        uint8_t segmentSet = 0;
        uint8_t soundSimulation = 0;
        uint8_t map1D2D = 0;
        ManagedSegment managedSegments[3]{};
        uint8_t secondaryColor[4]{};
        uint8_t tertiaryColor[4]{};
        uint8_t segmentBrightness = 255;
        uint8_t syncSendGroups = 1;
        uint8_t syncReceiveGroups = 1;
        uint8_t liveOverride = 0;
        uint8_t mainSegment = 0;
        uint16_t transitionMs = 0;
        int preset = -1;
        int playlist = -1;
        bool presetActionArmed = false;
        bool segmentActionArmed = false;
        bool live = false;
        uint8_t liveSegment = 0;
        char liveMode[15]{};
        uint8_t ledmap = 0;
        int8_t wifiRssi = -127;
        uint8_t wifiSignal = 0;
        uint32_t uptime = 0;
        uint32_t freeHeap = 0;
        uint8_t error = 0;
        uint32_t buildId = 0;
        uint16_t effectCount = 0;
        uint16_t paletteCount = 0;
        uint8_t maxSegments = 0;
        int16_t nodeCount = -1;
        uint16_t ledCurrentMa = 0;
        uint16_t ledMaxCurrentMa = 0;
        uint32_t ipAddress = 0;
        char wifiBssid[15]{};
        uint8_t wifiChannel = 0;
        uint16_t ledCount = 0;
        uint16_t ledFps = 0;
        uint16_t matrixWidth = 0;
        uint16_t matrixHeight = 0;
        uint32_t fsUsedKb = 0;
        uint32_t fsTotalKb = 0;
        uint32_t presetsModified = 0;
        uint16_t ledMapCount = 0;
        uint32_t realtimeSourceIp = 0;
        int16_t websocketClients = -1;
        bool online = false;
        uint32_t jsonErrorCount = 0;
        uint32_t oversizedFrameCount = 0;
        uint32_t uartOverflowCount = 0;
        uint8_t currentPercent = 0;
        bool currentWarning = false;
        bool currentExceeded = false;
        bool hasRgbw = false;
        bool hasWhite = false;
        bool hasCct = false;
        bool isMatrix = false;
        struct tm localTime{};
        bool localTimeValid = false;
        char version[15]{};
    } state;

    char frame[8192]{};
    size_t frameLength = 0;
    uint8_t depth = 0;
    bool inString = false;
    bool escaped = false;
    bool discardingFrame = false;
    bool resyncAfterOverrun = false;
    bool firstRequest = true;
    uint32_t frameStarted = 0;
    uint32_t lastRequest = 0;
    uint32_t presetActionArmedAt = 0;
    uint32_t segmentActionArmedAt = 0;
    uint32_t lastValidResponseAt = 0;
    bool validResponseReceived = false;

    void requestState();
    void applyConfiguredDefaults();
    void sendSwitch(bool on);
    void sendBrightness(uint8_t brightness);
    void sendColor(uint8_t red, uint8_t green, uint8_t blue);
    void sendRgbw(uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
    void sendColorTemperature(uint16_t kelvin);
    void sendWarmWhite(uint8_t value);
    void sendColdWhite(uint8_t value);
    void sendHsv(uint16_t hue, uint8_t saturation, uint8_t value);
    void sendNightlight(bool enabled);
    void sendNightlightValue(const char* key, uint8_t value);
    void sendPreset(uint8_t preset);
    void sendSegmentValue(const char* key, uint8_t value);
    void sendSegmentBool(const char* key, bool value);
    void sendSegmentWord(const char* key, uint16_t value);
    void sendSegmentOffset(int16_t value);
    void sendSegmentName(const char* name);
    void processManagedSegmentInput(GroupObject& ko);
    void publishManagedSegments();
    uint8_t managedSegmentId(uint8_t index) const;
    void applySegmentConfiguration();
    void setSegmentActionArm(bool armed);
    void deleteSegment();
    void sendSegmentColor(uint8_t index, uint32_t color, uint8_t mask);
    void sendTransition(uint16_t milliseconds);
    void sendSync(bool send, bool enabled);
    void sendSyncGroups(bool send, uint8_t groups);
    void sendLiveOverride(uint8_t mode);
    void sendMainSegment(uint8_t segment);
    void sendTrigger(const char* key);
    void sendPlaylist();
    void setPresetActionArm(bool armed);
    void sendPresetAction(const char* key, uint8_t preset);
    void sendPresetSelection(const char* selection);
    void stopPlaylist();
    void sendLive(bool enabled);
    void sendLedmap(uint8_t map);
    void sendTimebase(uint32_t timebase);
    void sendDeviceTime(uint32_t epoch);
    void updateCommunicationHealth();
    void publishCommunicationHealth();
    void receiveSerial();
    void processFrame();
    void resetFrame();
    void publishColor();
    void publishColorTemperature();
    void publishPreset();
    void publishEffects();
    void publishAdvancedState();
    void publishExtendedSegmentState();
    void publishDeviceInfo();
    void publishRgbw(GroupObject& object, const uint8_t color[4]);
    uint16_t cctToKelvin(uint8_t cct) const;
    uint8_t kelvinToCct(uint16_t kelvin) const;
    static void rgbToHsv(uint8_t red, uint8_t green, uint8_t blue, float& hue, float& saturation, float& value);
    static void hsvToRgb(float hue, float saturation, float value, uint8_t& red, uint8_t& green, uint8_t& blue);
};

extern DigitalLedModule digitalLedModule;
