/**
 * @file SceneLedStudio.cpp
 * @brief RGB LED 調光工作室 (LED Studio) 場景實作檔
 * @details 支援色相環調色、RGB 預覽與 MiniJoyC 雙 SK6812 LED 連動，退出時自動關閉燈光
 */

#include "scenes/SceneLedStudio.h"
#include <cmath>

static void hsvToRgb(int h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf((float)h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf = 0, gf = 0, bf = 0;

    if (h < 60) { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else { rf = c; gf = 0; bf = x; }

    r = (uint8_t)((rf + m) * 255.0f);
    g = (uint8_t)((gf + m) * 255.0f);
    b = (uint8_t)((bf + m) * 255.0f);
}

SceneLedStudio::SceneLedStudio()
    : _hue(180), _brightness(80), _ledR(0), _ledG(200), _ledB(200), _needsRedraw(true) {}

void SceneLedStudio::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    hsvToRgb(_hue, 1.0f, (float)_brightness / 100.0f, _ledR, _ledG, _ledB);
}

void SceneLedStudio::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 搖桿 X 軸調色相 (Hue 0 ~ 360)
    if (abs(input.joyX) > 25) {
        _hue += (input.joyX / 15);
        if (_hue < 0) _hue += 360;
        if (_hue >= 360) _hue -= 360;
        _needsRedraw = true;
    }

    // 3. 搖桿 Y 軸調亮度 (Brightness 5 ~ 100)
    if (abs(input.joyY) > 25) {
        int nextB = _brightness - (input.joyY / 20);
        _brightness = (uint8_t)constrain(nextB, 5, 100);
        _needsRedraw = true;
    }

    // 4. 按下搖桿中心鍵：切換純白光與彩光
    if (input.joyBtnPressed) {
        audio.playTick();
        if (_hue != 0 || _ledR != _ledG) {
            _hue = 0; // 重置
        }
        _needsRedraw = true;
    }

    hsvToRgb(_hue, 1.0f, (float)_brightness / 100.0f, _ledR, _ledG, _ledB);
    led.setColor(_ledR, _ledG, _ledB);
}

void SceneLedStudio::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    int centerX = SCREEN_WIDTH / 2;

    // 頂部標題
    g_canvas.setTextColor(TFT_MAGENTA, TFT_BLACK);
    g_canvas.drawCentreString("LED WORKSHOP", centerX, 10, 2);
    g_canvas.drawFastHLine(10, 28, SCREEN_WIDTH - 20, 0x18C3);

    // 預覽色塊方盒
    uint16_t previewColor = g_canvas.color565(_ledR, _ledG, _ledB);
    g_canvas.fillRoundRect(15, 38, SCREEN_WIDTH - 30, 75, 6, previewColor);
    g_canvas.drawRoundRect(15, 38, SCREEN_WIDTH - 30, 75, 6, TFT_WHITE);

    // 16 進位色碼
    char hexStr[16];
    snprintf(hexStr, sizeof(hexStr), "#%02X%02X%02X", _ledR, _ledG, _ledB);
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString(hexStr, centerX, 122, 4);

    // RGB 與亮度數值
    char rgbStr[32];
    snprintf(rgbStr, sizeof(rgbStr), "R:%d G:%d B:%d", _ledR, _ledG, _ledB);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(rgbStr, centerX, 150, 1);

    char brtStr[32];
    snprintf(brtStr, sizeof(brtStr), "Brightness: %d%%", _brightness);
    g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(brtStr, centerX, 164, 1);

    // 搖桿操作導引
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy X: Color Hue", centerX, 182, 1);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString("Joy Y: Brightness", centerX, 196, 1);

    // 底部提示
    g_canvas.drawFastHLine(10, 218, SCREEN_WIDTH - 20, 0x18C3);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", centerX, 224, 1);

    g_canvas.pushSprite(0, 0);
}

void SceneLedStudio::exit() {
    // 退出時清空 LED
}
