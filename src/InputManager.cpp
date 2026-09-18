/**
 * @file InputManager.cpp
 * @brief 輸入管理器實作：調高甩動門檻至真正用力甩動，徹底防止日常持握誤觸
 */

#include "InputManager.h"

InputManager::InputManager()
    : joyX(0), joyY(0),
      joyBtnPressed(false), btnAPressed(false), btnBPressed(false), btnBLongPressed(false),
      joyPulledDown(false), joyPushedUp(false), joyPushedLeft(false), joyPushedRight(false),
      isShaken(false),
      _prevJoyBtn(false), _prevPulledDown(false), _prevPushedUp(false),
      _prevPushedLeft(false), _prevPushedRight(false),
      _btnBPressedTime(0), _btnBHandled(false),
      _lastAx(0), _lastAy(0), _lastAz(0), _lastShakeTime(0) {}

bool InputManager::begin() {
    bool ret = _joyc.begin(&Wire, MINI_JOYC_ADDR, HAT_I2C_SDA, HAT_I2C_SCL, 400000L);
    _prevJoyBtn = _joyc.getButtonStatus();
    M5.Imu.Init();
    return ret;
}

void InputManager::update() {
    joyBtnPressed = false;
    btnAPressed = false;
    btnBPressed = false;
    btnBLongPressed = false;
    joyPulledDown = false;
    joyPushedUp = false;
    joyPushedLeft = false;
    joyPushedRight = false;
    isShaken = false;

    // 讀取搖桿數值並修正 Y 軸方向
    joyX = (int8_t)_joyc.getPOSValue(POS_X, _8bit);
    joyY = -((int8_t)_joyc.getPOSValue(POS_Y, _8bit));

    if (abs(joyX) < JOY_DEADZONE) joyX = 0;
    if (abs(joyY) < JOY_DEADZONE) joyY = 0;

    bool currentJoyBtn = _joyc.getButtonStatus();
    if (currentJoyBtn && !_prevJoyBtn) {
        joyBtnPressed = true;
    }
    _prevJoyBtn = currentJoyBtn;

    bool currPulledDown = (joyY > JOY_TRIGGER_PULL);
    if (currPulledDown && !_prevPulledDown) joyPulledDown = true;
    _prevPulledDown = currPulledDown;

    bool currPushedUp = (joyY < -JOY_TRIGGER_PULL);
    if (currPushedUp && !_prevPushedUp) joyPushedUp = true;
    _prevPushedUp = currPushedUp;

    bool currPushedLeft = (joyX < -JOY_TRIGGER_PULL);
    if (currPushedLeft && !_prevPushedLeft) joyPushedLeft = true;
    _prevPushedLeft = currPushedLeft;

    bool currPushedRight = (joyX > JOY_TRIGGER_PULL);
    if (currPushedRight && !_prevPushedRight) joyPushedRight = true;
    _prevPushedRight = currPushedRight;

    if (M5.BtnA.wasPressed()) btnAPressed = true;

    if (M5.BtnB.wasPressed()) {
        _btnBPressedTime = millis();
        _btnBHandled = false;
    }
    if (M5.BtnB.isPressed()) {
        if (!_btnBHandled && (millis() - _btnBPressedTime >= 500)) {
            btnBLongPressed = true;
            _btnBHandled = true;
        }
    }
    if (M5.BtnB.wasReleased()) {
        if (!_btnBHandled) btnBPressed = true;
    }

    // 調高體感門檻：必須刻意用力甩動機身才觸發
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getGyroData(&gx, &gy, &gz);
    M5.Imu.getAccelData(&ax, &ay, &az);

    float gyroMag = abs(gx) + abs(gy) + abs(gz);
    float deltaA = abs(ax - _lastAx) + abs(ay - _lastAy) + abs(az - _lastAz);
    _lastAx = ax;
    _lastAy = ay;
    _lastAz = az;

    // 角速度 > 450 deg/s 或瞬時加速度差 > 3.2G，冷卻時間拉長至 900ms
    if ((gyroMag > 450.0f || deltaA > 3.2f) && (millis() - _lastShakeTime > 900)) {
        isShaken = true;
        _lastShakeTime = millis();
    }
}
