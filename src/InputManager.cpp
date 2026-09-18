/**
 * @file InputManager.cpp
 * @brief 輸入管理器實作：支援搖桿持續按壓/推拉、放開邊緣偵測與持續體感/靜止動力學判斷
 */

#include "InputManager.h"

InputManager::InputManager()
    : joyX(0), joyY(0),
      isJoyPulledDown(false), isJoyPushedUp(false), isJoyBtnHeld(false), isBtnAHeld(false),
      joyBtnPressed(false), btnAPressed(false), btnBPressed(false), btnBLongPressed(false),
      joyPulledDown(false), joyPushedUp(false), joyPushedLeft(false), joyPushedRight(false),
      joyReleased(false),
      isActivelyShaking(false), isNearlyStill(true), isShaken(false),
      _prevJoyBtn(false), _prevPulledDown(false), _prevPushedUp(false),
      _prevPushedLeft(false), _prevPushedRight(false), _prevJoyEngaged(false),
      _btnBPressedTime(0), _btnBHandled(false),
      _lastAx(0), _lastAy(0), _lastAz(0), _lastActiveShakeTime(0) {}

bool InputManager::begin() {
    bool ret = _joyc.begin(&Wire, MINI_JOYC_ADDR, HAT_I2C_SDA, HAT_I2C_SCL, 400000L);
    _prevJoyBtn = _joyc.getButtonStatus();
    M5.Imu.Init();
    return ret;
}

void InputManager::update() {
    // 重置單幀邊緣脈衝
    joyBtnPressed = false;
    btnAPressed = false;
    btnBPressed = false;
    btnBLongPressed = false;
    joyPulledDown = false;
    joyPushedUp = false;
    joyPushedLeft = false;
    joyPushedRight = false;
    joyReleased = false;
    isShaken = false;

    // 1. 讀取搖桿並修正 Y 軸方向
    joyX = (int8_t)_joyc.getPOSValue(POS_X, _8bit);
    joyY = -((int8_t)_joyc.getPOSValue(POS_Y, _8bit));

    if (abs(joyX) < JOY_DEADZONE) joyX = 0;
    if (abs(joyY) < JOY_DEADZONE) joyY = 0;

    // 2. 搖桿中心按鍵狀態
    bool currJoyBtn = _joyc.getButtonStatus();
    isJoyBtnHeld = currJoyBtn;
    if (currJoyBtn && !_prevJoyBtn) joyBtnPressed = true;
    _prevJoyBtn = currJoyBtn;

    // 3. 搖桿持續方向狀態
    isJoyPulledDown = (joyY > JOY_TRIGGER_PULL);
    isJoyPushedUp = (joyY < -JOY_TRIGGER_PULL);
    bool isLeft = (joyX < -JOY_TRIGGER_PULL);
    bool isRight = (joyX > JOY_TRIGGER_PULL);

    // 4. 方向邊緣觸發
    if (isJoyPulledDown && !_prevPulledDown) joyPulledDown = true;
    _prevPulledDown = isJoyPulledDown;

    if (isJoyPushedUp && !_prevPushedUp) joyPushedUp = true;
    _prevPushedUp = isJoyPushedUp;

    if (isLeft && !_prevPushedLeft) joyPushedLeft = true;
    _prevPushedLeft = isLeft;

    if (isRight && !_prevPushedRight) joyPushedRight = true;
    _prevPushedRight = isRight;

    // 5. 搖桿從推持狀態回彈放開偵測 (Joy Released)
    bool joyEngaged = (isJoyPulledDown || isJoyPushedUp || isLeft || isRight || isJoyBtnHeld);
    if (!joyEngaged && _prevJoyEngaged) {
        joyReleased = true; // 放開搖桿
    }
    _prevJoyEngaged = joyEngaged;

    // 6. 實體按鍵
    isBtnAHeld = M5.BtnA.isPressed();
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

    // 7. 持續體感甩動與靜止狀態動力學 (Continuous Shake & Stillness Detection)
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getGyroData(&gx, &gy, &gz);
    M5.Imu.getAccelData(&ax, &ay, &az);

    float gyroMag = abs(gx) + abs(gy) + abs(gz);
    float deltaA = abs(ax - _lastAx) + abs(ay - _lastAy) + abs(az - _lastAz);
    _lastAx = ax;
    _lastAy = ay;
    _lastAz = az;

    uint32_t now = millis();

    // 判定持續激烈甩動
    if (gyroMag > 350.0f || deltaA > 2.5f) {
        _lastActiveShakeTime = now;
        isActivelyShaking = true;
        isNearlyStill = false;
    } else {
        // 若已超過 280ms 無大動作，脫離激烈甩動狀態
        if (now - _lastActiveShakeTime > 280) {
            isActivelyShaking = false;
        }
        // 若角速度極低，判定為幾乎靜止
        isNearlyStill = (gyroMag < 90.0f && deltaA < 0.6f);
    }

    // 單次邊緣觸發脈衝 (配合較嚴格門檻)
    if (isActivelyShaking && (now - _lastActiveShakeTime < 40)) {
        isShaken = true;
    }
}
