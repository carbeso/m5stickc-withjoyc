/**
 * @file LedManager.h
 * @brief SK6812 全彩 RGB LED 燈效管理器標頭檔
 */

#pragma once

#include <Arduino.h>
#include "M5HatMiniJoyC.h"

class LedManager {
public:
    LedManager(M5HatMiniJoyC& joyc);
    void update();

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setHexColor(uint32_t rgb);
    void flash(uint8_t r, uint8_t g, uint8_t b, uint8_t count = 3, uint16_t interval = 80);
    void setRainbowMode(bool enabled) { _rainbowMode = enabled; }

    // 匯流排隔離控制 (用於 GPIO 0 / I2S 麥克風音訊模式)
    void setBusSuspended(bool suspended);
    bool isBusSuspended() const { return _busSuspended; }

private:
    M5HatMiniJoyC& _joyc;
    bool _busSuspended;
    bool _rainbowMode;
    uint8_t _hue;
    uint32_t _lastUpdate;

    // 爆閃狀態
    bool _flashing;
    uint8_t _flashR, _flashG, _flashB;
    uint8_t _flashCount;
    uint16_t _flashInterval;
    uint32_t _nextFlashTime;
    bool _flashState;

    uint32_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v);
};
