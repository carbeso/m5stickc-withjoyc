/**
 * @file InputManager.cpp
 * @brief 輸入管理器實作：修正搖桿方向、調降體感搖晃靈敏度至合理用力甩動門檻
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
    // 1. 初始化頂部 HAT 槽 I2C (SDA = 0, SCL = 26)
    bool ret = _joyc.begin(&Wire, MINI_JOYC_ADDR, HAT_I2C_SDA, HAT_I2C_SCL, 400000L);
    _prevJoyBtn = _joyc.getButtonStatus();

    // 2. 初始化 MPU6886 姿態感測器
    M5.Imu.Init();

    return ret;
}

void InputManager::update() {
    // 1. 重置單次邊緣事件
    joyBtnPressed = false;
    btnAPressed = false;
    btnBPressed = false;
    btnBLongPressed = false;
    joyPulledDown = false;
    joyPushedUp = false;
    joyPushedLeft = false;
    joyPushedRight = false;
    isShaken = false;

    // 2. 讀取 MiniJoyC 搖桿數值 (-128 ~ 127)
    joyX = (int8_t)_joyc.getPOSValue(POS_X, _8bit);
    // 修正上下方向取反
    joyY = -((int8_t)_joyc.getPOSValue(POS_Y, _8bit));

    if (abs(joyX) < JOY_DEADZONE) joyX = 0;
    if (abs(joyY) < JOY_DEADZONE) joyY = 0;

    // 3. 搖桿按鍵邊緣偵測
    bool currentJoyBtn = _joyc.getButtonStatus();
    if (currentJoyBtn && !_prevJoyBtn) {
        joyBtnPressed = true;
    }
    _prevJoyBtn = currentJoyBtn;

    // 4. 搖桿方向邊緣偵測
    bool currPulledDown = (joyY > JOY_TRIGGER_PULL);
    if (currPulledDown && !_prevPulledDown) {
        joyPulledDown = true;
    }
    _prevPulledDown = currPulledDown;

    bool currPushedUp = (joyY < -JOY_TRIGGER_PULL);
    if (currPushedUp && !_prevPushedUp) {
        joyPushedUp = true;
    }
    _prevPushedUp = currPushedUp;

    bool currPushedLeft = (joyX < -JOY_TRIGGER_PULL);
    if (currPushedLeft && !_prevPushedLeft) {
        joyPushedLeft = true;
    }
    _prevPushedLeft = currPushedLeft;

    bool currPushedRight = (joyX > JOY_TRIGGER_PULL);
    if (currPushedRight && !_prevPushedRight) {
        joyPushedRight = true;
    }
    _prevPushedRight = currPushedRight;

    // 5. 正面 Button A 偵測
    if (M5.BtnA.wasPressed()) {
        btnAPressed = true;
    }

    // 6. 側面 Button B 長短按偵測
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
        if (!_btnBHandled) {
            btnBPressed = true;
        }
    }

    // 7. MPU6886 體感甩動偵測 (調高至「用力甩動」門檻，避免普通手持誤觸)
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getGyroData(&gx, &gy, &gz);
    M5.Imu.getAccelData(&ax, &ay, &az);

    float gyroMag = abs(gx) + abs(gy) + abs(gz);
    float deltaA = abs(ax - _lastAx) + abs(ay - _lastAy) + abs(az - _lastAz);
    _lastAx = ax;
    _lastAy = ay;
    _lastAz = az;

    // 門檻提高：角速度 > 320 deg/s 或瞬時加速度差 > 2.6G，冷卻 800ms
    if ((gyroMag > 320.0f || deltaA > 2.6f) && (millis() - _lastShakeTime > 800)) {
        isShaken = true;
        _lastShakeTime = millis();
    }
}
