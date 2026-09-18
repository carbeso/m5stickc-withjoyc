/**
 * @file SceneRoulette.cpp
 * @brief 直向垂直幸運輪盤實作：邊界安全裁剪防止壓標題、局部無閃爍捲動
 */

#include "scenes/SceneRoulette.h"

const RoulettePocket WHEEL_POCKETS[37] = {
    {0, 0}, {32, 1}, {15, 2}, {19, 1}, {4, 2}, {21, 1}, {2, 2}, {25, 1},
    {17, 2}, {34, 1}, {6, 2}, {27, 1}, {13, 2}, {36, 1}, {11, 2}, {30, 1},
    {8, 2}, {23, 1}, {10, 2}, {5, 1}, {24, 2}, {16, 1}, {33, 2}, {1, 1},
    {20, 2}, {14, 1}, {31, 2}, {9, 1}, {22, 2}, {18, 1}, {29, 2}, {7, 1},
    {28, 2}, {12, 1}, {35, 2}, {3, 1}, {26, 2}
};

const int ITEM_HEIGHT = 38;

SceneRoulette::SceneRoulette()
    : _stripPos(0.0f), _stripSpeed(0.0f), _isSpinning(false),
      _targetIndex(0), _needsRedraw(true), _lastTickTime(0) {}

void SceneRoulette::init() {
    _needsRedraw = true;
    _isSpinning = false;
    _stripSpeed = 0.0f;
    _stripPos = 0.0f;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneRoulette::spinRoulette(AudioManager& audio, LedManager& led) {
    _isSpinning = true;
    _stripSpeed = 22.0f + ((float)random(0, 100) / 10.0f);
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void SceneRoulette::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isSpinning && (input.joyPulledDown || input.joyBtnPressed || input.btnAPressed || input.isShaken)) {
        spinRoulette(audio, led);
    }

    if (_isSpinning) {
        _stripPos += _stripSpeed;
        _stripSpeed *= 0.982f;

        float maxPos = 37 * ITEM_HEIGHT;
        while (_stripPos >= maxPos) _stripPos -= maxPos;

        if (millis() - _lastTickTime > (uint32_t)constrain(400.0f / (_stripSpeed + 1.0f), 20.0f, 250.0f)) {
            _lastTickTime = millis();
            audio.playTick();
        }

        if (_stripSpeed < 0.35f) {
            _isSpinning = false;
            _stripSpeed = 0.0f;

            int centerIdx = ((int)(_stripPos + ITEM_HEIGHT / 2) / ITEM_HEIGHT) % 37;
            _stripPos = centerIdx * ITEM_HEIGHT;
            _targetIndex = centerIdx;

            const RoulettePocket& winPocket = WHEEL_POCKETS[_targetIndex];
            if (winPocket.colorType == 0) {
                audio.playCrit();
                led.flash(0, 255, 100, 4, 60);
            } else if (winPocket.colorType == 1) {
                audio.playClick();
                led.setColor(255, 0, 0);
            } else {
                audio.playClick();
                led.setColor(220, 220, 255);
            }
        }
        _needsRedraw = true;
    }
}

void SceneRoulette::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 清除中央捲軸區 (Y: 28 ~ 194)，避免整面黑屏閃爍
    M5.Lcd.fillRect(0, 28, SCREEN_WIDTH, 166, TFT_BLACK);

    // 中央指針基準線 Y = 110
    int centerY = 110;
    int baseIdx = ((int)_stripPos / ITEM_HEIGHT);
    float offset = _stripPos - (baseIdx * ITEM_HEIGHT);

    // 繪製捲軸格子
    for (int i = -3; i <= 3; i++) {
        int pocketIdx = (baseIdx + i + 370) % 37;
        const RoulettePocket& p = WHEEL_POCKETS[pocketIdx];
        int itemY = centerY + (i * ITEM_HEIGHT) - (int)offset - (ITEM_HEIGHT / 2);

        // 嚴格邊界保護：超出動態顯示區則跳過，絕不壓住頂部 Title 與底部
        if (itemY < 28 || itemY + ITEM_HEIGHT > 194) continue;

        uint16_t bgColor = (p.colorType == 0) ? COLOR_ROU_GRN :
                           (p.colorType == 1) ? COLOR_ROU_RED : COLOR_ROU_BLK;
        M5.Lcd.fillRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, bgColor);
        M5.Lcd.drawRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, 0x52AA);

        char numStr[8];
        snprintf(numStr, sizeof(numStr), "%d", p.number);
        M5.Lcd.setTextColor(TFT_WHITE, bgColor);
        M5.Lcd.drawCentreString(numStr, SCREEN_WIDTH / 2, itemY + 8, 4);
    }

    // 左右指針標記獲勝中央線 (Y: 110)
    M5.Lcd.fillTriangle(4, centerY - 8, 4, centerY + 8, 15, centerY, COLOR_GOLD);
    M5.Lcd.fillTriangle(SCREEN_WIDTH - 4, centerY - 8, SCREEN_WIDTH - 4, centerY + 8, SCREEN_WIDTH - 15, centerY, COLOR_GOLD);

    // 頂部狀態列 (始終覆蓋在最上層，保證 Title 絕對不被蓋住)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 28, 0x18C3);
    M5.Lcd.setTextColor(COLOR_CYAN, 0x18C3);
    M5.Lcd.drawString("ROULETTE", 8, 6, 2);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawRightString("EUR 0-36", SCREEN_WIDTH - 8, 6, 2);

    // 底部開獎與操作指引 (Y: 194 ~ 238)
    M5.Lcd.fillRect(0, 194, SCREEN_WIDTH, 46, TFT_BLACK);
    M5.Lcd.drawFastHLine(8, 194, SCREEN_WIDTH - 16, 0x39E7);

    if (!_isSpinning) {
        const RoulettePocket& win = WHEEL_POCKETS[_targetIndex];
        const char* colName = (win.colorType == 0) ? "GREEN" :
                              (win.colorType == 1) ? "RED" : "BLACK";
        const char* oddEven = (win.number == 0) ? "" :
                              (win.number % 2 == 1) ? " (ODD)" : " (EVEN)";

        char resStr[24];
        snprintf(resStr, sizeof(resStr), "%d %s%s", win.number, colName, oddEven);
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString(resStr, SCREEN_WIDTH / 2, 202, 2);
    } else {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 202, 2);
    }

    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy DOWN / A: Spin", SCREEN_WIDTH / 2, 224, 1);
}
