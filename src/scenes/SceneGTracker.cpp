/**
 * @file SceneGTracker.cpp
 * @brief G-Force 歷史峰值衝擊追蹤器 (G-Tracker) 場景實作檔
 * @details 即時追蹤三軸加速度向量長度，長按 Button B 返回主選單，按下 JoyBtn 重置峰值
 */

#include "scenes/SceneGTracker.h"
#include <cmath>

SceneGTracker::SceneGTracker()
    : _gHistIdx(0), _currentG(1.0f), _peakG(1.0f), _peakAx(0), _peakAy(0),
      _lastSampleTime(0), _needsRedraw(true) {
    for (uint8_t i = 0; i < G_HIST_SIZE; i++) _gHistory[i] = 1.0f;
}

void SceneGTracker::init() {
    _gHistIdx = 0;
    _currentG = 1.0f;
    _peakG = 1.0f;
    _peakAx = 0;
    _peakAy = 0;
    _lastSampleTime = millis();
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    for (uint8_t i = 0; i < G_HIST_SIZE; i++) _gHistory[i] = 1.0f;
}

void SceneGTracker::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 按下搖桿中心鍵：重置峰值
    if (input.joyBtnPressed) {
        audio.playTick();
        _peakG = 1.0f;
        for (uint8_t i = 0; i < G_HIST_SIZE; i++) _gHistory[i] = 1.0f;
        _needsRedraw = true;
    }

    // 3. 定時採樣 (10Hz, 100ms 一次)
    uint32_t now = millis();
    if (now - _lastSampleTime >= 100) {
        _lastSampleTime = now;
        float ax = 0, ay = 0, az = 0;
        M5.Imu.getAccelData(&ax, &ay, &az);

        _currentG = sqrtf(ax * ax + ay * ay + az * az);
        _gHistory[_gHistIdx] = _currentG;
        _gHistIdx = (_gHistIdx + 1) % G_HIST_SIZE;

        float maxVal = 1.0f;
        for (uint8_t i = 0; i < G_HIST_SIZE; i++) {
            if (_gHistory[i] > maxVal) maxVal = _gHistory[i];
        }
        _peakG = maxVal;
        _peakAx = ax;
        _peakAy = ay;

        // 若受到劇烈衝擊 (G > 2.5) 給予音效與燈光反饋
        if (_currentG > 2.5f) {
            led.setColor(255, 50, 0);
        } else if (_currentG > 1.8f) {
            led.setColor(255, 200, 0);
        } else {
            led.setColor(0, 0, 0);
        }

        _needsRedraw = true;
    }
}

void SceneGTracker::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    int centerX = SCREEN_WIDTH / 2;

    // 頂部標題
    g_canvas.setTextColor(TFT_ORANGE, TFT_BLACK);
    g_canvas.drawCentreString("G-FORCE TRACKER", centerX, 10, 2);
    g_canvas.drawFastHLine(10, 28, SCREEN_WIDTH - 20, 0x18C3);

    // 當前 G 數值 (超大字型)
    char gStr[16];
    snprintf(gStr, sizeof(gStr), "%.2f G", _currentG);
    uint16_t gColor = (_currentG > 2.5f) ? TFT_RED : (_currentG > 1.5f) ? COLOR_GOLD : TFT_GREEN;
    g_canvas.setTextColor(gColor, TFT_BLACK);
    g_canvas.drawCentreString(gStr, centerX, 36, 4);

    // 5 秒峰值紀錄
    char peakStr[32];
    snprintf(peakStr, sizeof(peakStr), "5s Peak: %.2f G", _peakG);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(peakStr, centerX, 68, 2);

    // 歷史衝擊柱狀圖
    int chartX = 15;
    int chartY = 95;
    int chartW = SCREEN_WIDTH - 30;
    int chartH = 65;

    g_canvas.drawRect(chartX, chartY, chartW, chartH, 0x2965);
    g_canvas.drawFastHLine(chartX, chartY + chartH - 16, chartW, 0x18C3);

    for (uint8_t i = 0; i < G_HIST_SIZE && i * 2 < chartW; i++) {
        uint8_t readIdx = (_gHistIdx + i) % G_HIST_SIZE;
        float val = _gHistory[readIdx];
        int barH = (int)((val / 4.0f) * chartH);
        if (barH > chartH) barH = chartH;
        if (barH < 1) barH = 1;

        int bx = chartX + i * 2;
        int by = chartY + chartH - barH;
        uint16_t bColor = (val > 2.5f) ? TFT_RED : (val > 1.5f) ? COLOR_GOLD : COLOR_CYAN;
        g_canvas.drawFastVLine(bx, by, barH, bColor);
    }

    // 衝擊向量
    char dirStr[32];
    snprintf(dirStr, sizeof(dirStr), "Vector Ax:%+.1f Ay:%+.1f", _peakAx, _peakAy);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString(dirStr, centerX, 172, 1);

    // 操作說明
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString("Joy Click: Reset Peak", centerX, 190, 1);

    // 底部提示
    g_canvas.drawFastHLine(10, 218, SCREEN_WIDTH - 20, 0x18C3);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", centerX, 224, 1);

    g_canvas.pushSprite(0, 0);
}

void SceneGTracker::exit() {
    // 退出時清空 LED
}
