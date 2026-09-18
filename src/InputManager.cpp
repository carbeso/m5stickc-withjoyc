/**
 * @file InputManager.cpp
 * @brief 輸入管理器實作：修正搖桿 Y 軸方向、整合陀螺儀靈敏甩動偵測與邊緣事件
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

    // 2. 明確初始化 MPU6886 IMU 姿態感測器 (避免未初始化導致讀值為 0)
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
    // 修正上下方向：硬體上推與下拉取反，符合直立頂部握持直覺
    joyY = -((int8_t)_joyc.getPOSValue(POS_Y, _8bit));

    // 濾除中心死區
    if (abs(joyX) < JOY_DEADZONE) joyX = 0;
    if (abs(joyY) < JOY_DEADZONE) joyY = 0;

    // 3. 搖桿按鍵邊緣偵測
    bool currentJoyBtn = _joyc.getButtonStatus();
    if (currentJoyBtn && !_prevJoyBtn) {
        joyBtnPressed = true;
    }
    _prevJoyBtn = currentJoyBtn;

    // 4. 搖桿方向邊緣偵測 (下拉拉桿、推上、推左、推右)
    // joyY > 0 為向下拉桿，joyY < 0 為向上推
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

    // 6. 側面 Button B 長短按偵測 (長按 500ms 返回主選單，短按由場景自行處置如靜音)
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
            btnBPressed = true; // 短按放開
        }
    }

    // 7. MPU6886 體感劇烈甩動偵測 (同時採樣角速度 Gyro 與加速度 Accel)
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getGyroData(&gx, &gy, &gz);
    M5.Imu.getAccelData(&ax, &ay, &az);

    float gyroMag = abs(gx) + abs(gy) + abs(gz);
    float deltaA = abs(ax - _lastAx) + abs(ay - _lastAy) + abs(az - _lastAz);
    _lastAx = ax;
    _lastAy = ay;
    _lastAz = az;

    // 只要角速度 > 120 deg/s 或加速度差 > 1.2G 即判定為甩動 (大幅提升靈敏度)
    if ((gyroMag > 120.0f || deltaA > 1.2f) && (millis() - _lastShakeTime > 500)) {
        isShaken = true;
        _lastShakeTime = millis();
    }
}
