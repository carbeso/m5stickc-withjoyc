/**
 * @file InputManager.h
 * @brief 輸入管理器標頭檔：整合 MiniJoyC 搖桿、微動按鍵、StickC 按鈕與 MPU6886 體感
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

    // 搖桿即時狀態
    int16_t joyX;
    int16_t joyY;

    // 邊緣觸發事件 (當幀被觸發即為 true)
    bool joyBtnPressed;
    bool btnAPressed;
    bool btnBPressed;
    bool btnBLongPressed;   // Button B 長按 > 500ms (返回選單)
    
    // 搖桿手勢邊緣事件
    bool joyPulledDown;     // 搖桿向下拉桿 (Joy Y > JOY_TRIGGER_PULL)
    bool joyPushedUp;       // 搖桿向上推 (Joy Y < -JOY_TRIGGER_PULL)
    bool joyPushedLeft;     // 搖桿向左推 (Joy X < -JOY_TRIGGER_PULL)
    bool joyPushedRight;    // 搖桿向右推 (Joy X > JOY_TRIGGER_PULL)

    // 體感甩動事件
    bool isShaken;          // 機身被劇烈晃動

    // 取得底層 MiniJoyC 物件引用 (供 LED 控制等使用)
    M5HatMiniJoyC& getJoyC() { return _joyc; }

private:
    M5HatMiniJoyC _joyc;

    // 歷史狀態追蹤 (邊緣偵測)
    bool _prevJoyBtn;
    bool _prevPulledDown;
    bool _prevPushedUp;
    bool _prevPushedLeft;
    bool _prevPushedRight;

    uint32_t _btnBPressedTime;
    bool _btnBHandled;

    // 體感加速度歷史
    float _lastAx, _lastAy, _lastAz;
    uint32_t _lastShakeTime;
};
