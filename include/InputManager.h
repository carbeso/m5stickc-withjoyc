/**
 * @file InputManager.h
 * @brief 輸入管理器標頭檔：支援持續按壓/推持狀態、持續甩動與靜止偵測
 */

#pragma once

#include <Arduino.h>
#include <M5StickCPlus.h>
#include "M5HatMiniJoyC.h"
#include "Config.h"

class InputManager {
public:
    InputManager();
    bool begin();
    void update();

    // 搖桿即時座標 (-128 ~ 127)
    int16_t joyX;
    int16_t joyY;

    // 持續狀態 (Hold States)
    bool isJoyPulledDown;   // 搖桿持續向下拉著
    bool isJoyPushedUp;     // 搖桿持續向上推著
    bool isJoyBtnHeld;      // 搖桿中心鍵持續按著
    bool isBtnAHeld;        // Button A 持續按著

    // 邊緣事件 (Edge Pulses)
    bool joyBtnPressed;
    bool btnAPressed;
    bool btnBPressed;
    bool btnBLongPressed;

    bool joyPulledDown;     // 剛向下拉
    bool joyPushedUp;       // 剛向上推
    bool joyPushedLeft;     // 剛向左推
    bool joyPushedRight;    // 剛向右推

    bool joyReleased;       // 剛放開搖桿回彈至中心

    // 體感持續狀態
    bool isActivelyShaking; // 當前正處於持續激烈甩動中
    bool isNearlyStill;      // 機身當前幾乎處於靜止狀態
    bool isShaken;          // 單次甩動觸發邊緣脈衝

    // 活躍時間戳 (用於螢幕保護與待機休眠)
    uint32_t lastActivityTime;
    void resetActivityTimer() { lastActivityTime = millis(); }

    M5HatMiniJoyC& getJoyC() { return _joyc; }
    void clearEvents(); // 場景切換時清除殘留按鍵邊緣與狀態

    // 匯流排隔離控制 (用於 GPIO 0 / I2S 麥克風音訊模式)
    void setBusSuspended(bool suspended);
    bool isBusSuspended() const { return _busSuspended; }

private:
    M5HatMiniJoyC _joyc;
    bool _busSuspended;

    bool _prevJoyBtn;
    bool _prevPulledDown;
    bool _prevPushedUp;
    bool _prevPushedLeft;
    bool _prevPushedRight;
    bool _prevJoyEngaged;

    uint32_t _btnBPressedTime;
    bool _btnBHandled;

    float _lastAx, _lastAy, _lastAz;
    uint32_t _lastActiveShakeTime;
    uint32_t _lastShakePulseTime;
};
