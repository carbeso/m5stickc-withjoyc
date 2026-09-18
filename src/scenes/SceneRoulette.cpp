/**
 * @file SceneRoulette.cpp
 * @brief 直向垂直幸運輪盤實作：標準歐式 37 格單零輪盤、物理阻尼垂直捲軸
 */

#include "scenes/SceneRoulette.h"

// 歐式輪盤真實盤面順序 (37 格)
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
    _stripSpeed = 22.0f + ((float)random(0, 100) / 10.0f); // 初始高速
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void SceneRoulette::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 啟動旋轉：搖桿向後拉、中心鍵、按鍵 A 或晃動
    if (!_isSpinning && (input.joyPulledDown || input.joyBtnPressed || input.btnAPressed || input.isShaken)) {
        spinRoulette(audio, led);
    }

    // 物理阻尼運算
    if (_isSpinning) {
        _stripPos += _stripSpeed;
        _stripSpeed *= 0.982f; // 指數衰減模擬轉盤阻尼

        // 輪盤循環回捲
        float maxPos = 37 * ITEM_HEIGHT;
        while (_stripPos >= maxPos) _stripPos -= maxPos;

        // 齒輪滴答音效
        if (millis() - _lastTickTime > (uint32_t)constrain(400.0f / (_stripSpeed + 1.0f), 20.0f, 250.0f)) {
            _lastTickTime = millis();
            audio.playTick();
        }

        // 煞車定格判定
        if (_stripSpeed < 0.35f) {
            _isSpinning = false;
            _stripSpeed = 0.0f;

            // 計算停定位置對應之格子
            int centerIdx = ((int)(_stripPos + ITEM_HEIGHT / 2) / ITEM_HEIGHT) % 37;
            _stripPos = centerIdx * ITEM_HEIGHT; // 吸附對齊
            _targetIndex = centerIdx;

            // 停定音效與 LED 色彩映射
            const RoulettePocket& winPocket = WHEEL_POCKETS[_targetIndex];
            if (winPocket.colorType == 0) {
                audio.playCrit();
                led.flash(0, 255, 100, 4, 60); // 綠色 0 莊家大吉爆閃
            } else if (winPocket.colorType == 1) {
                audio.playClick();
                led.setColor(255, 0, 0);       // 紅色
            } else {
                audio.playClick();
                led.setColor(220, 220, 255);   // 黑色
            }
        }
        _needsRedraw = true;
    }
}

void SceneRoulette::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部狀態 (Y: 0 ~ 28)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_CYAN, 0x18C3);
    M5.Lcd.drawString("ROULETTE", 8, 5, 2);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawRightString("EUR 0-36", SCREEN_WIDTH - 8, 6, 2);

    // 2. 中央垂直直向捲動帶 (Y: 34 ~ 186)
    int centerY = 110;
    int baseIdx = ((int)_stripPos / ITEM_HEIGHT);
    float offset = _stripPos - (baseIdx * ITEM_HEIGHT);

    // 繪製前後 5 格
    for (int i = -2; i <= 2; i++) {
        int pocketIdx = (baseIdx + i + 370) % 37;
        const RoulettePocket& p = WHEEL_POCKETS[pocketIdx];
        int itemY = centerY + (i * ITEM_HEIGHT) - (int)offset - (ITEM_HEIGHT / 2);

        if (itemY + ITEM_HEIGHT < 32 || itemY > 190) continue;

        uint16_t bgColor = (p.colorType == 0) ? COLOR_ROU_GRN :
                           (p.colorType == 1) ? COLOR_ROU_RED : COLOR_ROU_BLK;
        M5.Lcd.fillRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, bgColor);
        M5.Lcd.drawRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, 0x52AA);

        char numStr[8];
        snprintf(numStr, sizeof(numStr), "%d", p.number);
        M5.Lcd.setTextColor(TFT_WHITE, bgColor);
        M5.Lcd.drawCentreString(numStr, SCREEN_WIDTH / 2, itemY + 8, 4);
    }

    // 左右指針標記中央獲勝線 (Y: 110)
    M5.Lcd.fillTriangle(4, centerY - 8, 4, centerY + 8, 14, centerY, COLOR_GOLD);
    M5.Lcd.fillTriangle(SCREEN_WIDTH - 4, centerY - 8, SCREEN_WIDTH - 4, centerY + 8, SCREEN_WIDTH - 14, centerY, COLOR_GOLD);

    // 3. 定格揭曉或操作提示 (Y: 194 ~ 238)
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
