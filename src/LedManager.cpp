/**
 * @file LedManager.cpp
 * @brief SK6812 全彩 RGB LED 燈效管理器實作
 */

#include "LedManager.h"

LedManager::LedManager(M5HatMiniJoyC& joyc)
    : _joyc(joyc), _busSuspended(false), _rainbowMode(false), _hue(0), _lastUpdate(0),
      _flashing(false), _flashR(0), _flashG(0), _flashB(0),
      _flashCount(0), _flashInterval(80), _nextFlashTime(0), _flashState(false) {}

void LedManager::setBusSuspended(bool suspended) {
    _busSuspended = suspended;
    if (suspended) {
        _flashing = false;
        _rainbowMode = false;
    }
}

uint32_t LedManager::hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r = 0, g = 0, b = 0;
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void LedManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (_busSuspended) return;
    _rainbowMode = false;
    _flashing = false;
    _joyc.setLEDColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void LedManager::setHexColor(uint32_t rgb) {
    if (_busSuspended) return;
    _rainbowMode = false;
    _flashing = false;
    _joyc.setLEDColor(rgb);
}

void LedManager::flash(uint8_t r, uint8_t g, uint8_t b, uint8_t count, uint16_t interval) {
    if (_busSuspended) return;
    _flashing = true;
    _rainbowMode = false;
    _flashR = r;
    _flashG = g;
    _flashB = b;
    _flashCount = count * 2; // 包含亮與暗
    _flashInterval = interval;
    _nextFlashTime = millis();
    _flashState = false;
}

void LedManager::update() {
    if (_busSuspended) return;
    uint32_t now = millis();

    // 處理爆閃
    if (_flashing) {
        if (now >= _nextFlashTime) {
            if (_flashCount > 0) {
                _flashState = !_flashState;
                if (_flashState) {
                    _joyc.setLEDColor(((uint32_t)_flashR << 16) | ((uint32_t)_flashG << 8) | _flashB);
                } else {
                    _joyc.setLEDColor(0x000000);
                }
                _nextFlashTime = now + _flashInterval;
                _flashCount--;
            } else {
                _flashing = false;
                _joyc.setLEDColor(0x000000);
            }
        }
        return;
    }

    // 處理彩虹流光
    if (_rainbowMode) {
        if (now - _lastUpdate > 30) {
            _lastUpdate = now;
            _hue += 3;
            _joyc.setLEDColor(hsvToRgb(_hue, 255, 100));
        }
    }
}
