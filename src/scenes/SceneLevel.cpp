/**
 * @file SceneLevel.cpp
 * @brief 三軸電子水平儀 (Bubble Level) 場景實作檔
 * @details 讀取 MPU6886 姿態資訊，以雙緩衝 g_canvas 零閃爍繪製電子水平儀
 */

#include "scenes/SceneLevel.h"
#include <cmath>

SceneLevel::SceneLevel()
    : _bubbleX(0), _bubbleY(0), _targetBubbleX(0), _targetBubbleY(0),
      _isCentered(false), _lastCenterTick(0), _needsRedraw(true) {}

void SceneLevel::init() {
    _bubbleX = 0;
    _bubbleY = 0;
    _targetBubbleX = 0;
    _targetBubbleY = 0;
    _isCentered = false;
    _lastCenterTick = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
}

void SceneLevel::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 讀取 MPU6886 加速度感測數據
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getAccelData(&ax, &ay, &az);

    _targetBubbleX = -ax * 42.0f;
    _targetBubbleY = ay * 42.0f;

    // 低通平滑濾波 (Alpha = 0.25)
    _bubbleX += (_targetBubbleX - _bubbleX) * 0.25f;
    _bubbleY += (_targetBubbleY - _bubbleY) * 0.25f;

    float distSq = _bubbleX * _bubbleX + _bubbleY * _bubbleY;
    bool centeredNow = (distSq < 36.0f); // 半徑 6px 內判定完美水平

    uint32_t now = millis();
    if (centeredNow && !_isCentered && (now - _lastCenterTick > 800)) {
        audio.playTick();
        led.setColor(0, 255, 0); // 完美水平時點亮綠燈
        _lastCenterTick = now;
    } else if (!centeredNow) {
        led.setColor(0, 0, 0);
    }
    _isCentered = centeredNow;
    _needsRedraw = true;
}

void SceneLevel::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    int centerX = SCREEN_WIDTH / 2;
    int centerY = 110;

    // 頂部標題列
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("BUBBLE LEVEL", centerX, 12, 2);
    g_canvas.drawFastHLine(10, 32, SCREEN_WIDTH - 20, 0x18C3);

    // 外環與刻度十字線
    uint16_t ringColor = _isCentered ? TFT_GREEN : 0x39E7;
    g_canvas.drawCircle(centerX, centerY, 52, ringColor);
    g_canvas.drawCircle(centerX, centerY, 26, 0x2124);
    g_canvas.drawCircle(centerX, centerY, 8, _isCentered ? TFT_GREEN : 0x2965);

    g_canvas.drawFastHLine(centerX - 52, centerY, 104, 0x2124);
    g_canvas.drawFastVLine(centerX, centerY - 52, 104, 0x2124);

    // 水平氣泡繪製
    int bx = centerX + (int)_bubbleX;
    int by = centerY + (int)_bubbleY;
    uint16_t bubbleColor = _isCentered ? TFT_GREEN : COLOR_CYAN;
    g_canvas.fillCircle(bx, by, 8, bubbleColor);
    g_canvas.drawCircle(bx, by, 8, TFT_WHITE);

    // 數值顯示
    char infoStr[32];
    snprintf(infoStr, sizeof(infoStr), "X:%+.1f  Y:%+.1f", _bubbleX / 4.2f, _bubbleY / 4.2f);
    g_canvas.setTextColor(_isCentered ? TFT_GREEN : COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(infoStr, centerX, 175, 2);

    if (_isCentered) {
        g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
        g_canvas.drawCentreString("[ PERFECT LEVEL ]", centerX, 198, 1);
    } else {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("Tilt to center bubble", centerX, 198, 1);
    }

    // 底部提示
    g_canvas.drawFastHLine(10, 218, SCREEN_WIDTH - 20, 0x18C3);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", centerX, 224, 1);

    g_canvas.pushSprite(0, 0);
}

void SceneLevel::exit() {
    // 離開時安全關閉 LED
}
