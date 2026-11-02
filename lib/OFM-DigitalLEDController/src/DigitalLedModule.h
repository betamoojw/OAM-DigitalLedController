#pragma once

#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include <Arduino.h>
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
        uint8_t palette = 0;
        uint8_t effect = 0;
        uint8_t effectSpeed = 0;
        uint8_t effectIntensity = 0;
        int preset = -1;
    } state;

    char frame[1024]{};
    size_t frameLength = 0;
    uint8_t depth = 0;
    bool inString = false;
    bool escaped = false;
    bool discardingFrame = false;
    bool resyncAfterOverrun = false;
    bool firstRequest = true;
    uint32_t frameStarted = 0;
    uint32_t lastRequest = 0;

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
    void sendPreset(uint8_t preset);
    void sendSegmentValue(const char* key, uint8_t value);
    void receiveSerial();
    void processFrame();
    void resetFrame();
    void publishColor();
    void publishColorTemperature();
    void publishPreset();
    void publishEffects();
    uint16_t cctToKelvin(uint8_t cct) const;
    uint8_t kelvinToCct(uint16_t kelvin) const;
    static void rgbToHsv(uint8_t red, uint8_t green, uint8_t blue, float& hue, float& saturation, float& value);
    static void hsvToRgb(float hue, float saturation, float value, uint8_t& red, uint8_t& green, uint8_t& blue);
};

extern DigitalLedModule digitalLedModule;
