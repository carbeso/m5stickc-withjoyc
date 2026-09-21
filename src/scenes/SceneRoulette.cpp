/**
 * @file SceneRoulette.cpp
 * @brief 直向垂直幸運輪盤實作：支援搖桿拉持漸入漸出、體感立刻旋轉與靜止定格動力學
 */

#include "scenes/SceneRoulette.h"
#include "EntropyManager.h"

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
      _targetIndex(0), _needsRedraw(true), _lastTickTime(0), _spinStartTime(0),
      _cruiseDuration(2500) {}

void SceneRoulette::init() {
    _needsRedraw = true;
    _isSpinning = false;
    _stripSpeed = 0.0f;
    _stripPos = 0.0f;
    _spinStartTime = 0;
    _nextScene = SCENE_COUNT;
    _targetIndex = 0;
}

void SceneRoulette::spinRoulette(AudioManager& audio, LedManager& led) {
    _isSpinning = true;
    _spinStartTime = millis();
    _cruiseDuration = EntropyManager::random(1700, 5200); // 隨機巡航時長：加上煞停滑行，總時長約 2.5 ~ 6.0 秒隨機！
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void SceneRoulette::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    bool isPullingJoy = (input.joyY > 35 || input.isBtnAHeld || input.isActivelyShaking);

    if (!_isSpinning) {
        // 啟動旋轉：必須是明確按鍵、下拉或甩動脈衝觸發，絕不因常態推持誤觸
        if (input.joyPulledDown || input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            _isSpinning = true;
            _spinStartTime = millis();
            _stripSpeed = input.isShaken ? 24.0f : 12.0f;
            _cruiseDuration = EntropyManager::random(1700, 5200); // 隨機 2.5 ~ 6.0 秒總時長
            audio.playDiceRoll();
            led.setRainbowMode(true);
        }
    } else {
        // 旋轉進行中
        _stripPos += _stripSpeed;
        uint32_t now = millis();
        uint32_t elapsed = now - _spinStartTime;

        float maxPos = 37 * ITEM_HEIGHT;
        while (_stripPos >= maxPos) _stripPos -= maxPos;

        // 齒輪卡榫音效
        if (millis() - _lastTickTime > (uint32_t)constrain(300.0f / (_stripSpeed + 1.0f), 20.0f, 180.0f)) {
            _lastTickTime = millis();
            audio.playTick();
        }

        // 動力學狀態處理：
        if (isPullingJoy && elapsed < 30000) {
            // 玩家持續拉著搖桿 (joyY > 35) 或甩動：加速至極速 25.0f 並持續全速飛轉！
            if (_stripSpeed < 25.0f) _stripSpeed += 1.5f;
            else _stripSpeed = 25.0f;
        } else if (elapsed < _cruiseDuration) {
            // 隨機巡航期間 (1.7s ~ 5.2s)：維持全速飛轉，加上後續滑行達到 2.5s ~ 6.0s 隨機未知感！
            if (_stripSpeed < 24.0f) _stripSpeed += 1.2f;
        } else {
            // 巡航結束或放開搖桿：自然滑行減速 (Ease-Out Deceleration)
            _stripSpeed *= 0.945f;

            // 停定判定
            if (_stripSpeed < 1.2f) {
                _isSpinning = false;
                _stripSpeed = 0.0f;

                int centerIdx = ((int)(_stripPos + ITEM_HEIGHT / 2) / ITEM_HEIGHT) % 37;
                _stripPos = centerIdx * ITEM_HEIGHT;
                _targetIndex = centerIdx;

                const RoulettePocket& winPocket = WHEEL_POCKETS[_targetIndex];
                if (winPocket.colorType == 0) {
                    audio.playCrit();
                    led.flash(0, 255, 0, 4, 60);
                } else if (winPocket.colorType == 1) {
                    audio.playClick();
                    led.setColor(255, 0, 0);
                } else {
                    audio.playClick();
                    led.setColor(0, 200, 255);
                }
            }
        }
        _needsRedraw = true;
    }
}

void SceneRoulette::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布離線繪圖，消除垂直輪盤捲動與指針閃爍
    g_canvas.fillSprite(TFT_BLACK);

    int centerY = 110;
    int baseIdx = ((int)_stripPos / ITEM_HEIGHT);
    float offset = _stripPos - (baseIdx * ITEM_HEIGHT);

    for (int i = -3; i <= 3; i++) {
        int pocketIdx = (baseIdx + i + 370) % 37;
        const RoulettePocket& p = WHEEL_POCKETS[pocketIdx];
        int itemY = centerY + (i * ITEM_HEIGHT) - (int)offset - (ITEM_HEIGHT / 2);

        if (itemY < 26 || itemY + ITEM_HEIGHT > 194) continue;

        uint16_t bgColor = (p.colorType == 0) ? COLOR_ROU_GRN :
                           (p.colorType == 1) ? COLOR_ROU_RED : COLOR_ROU_BLK;
        g_canvas.fillRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, bgColor);
        g_canvas.drawRoundRect(18, itemY, SCREEN_WIDTH - 36, ITEM_HEIGHT - 4, 4, 0x52AA);

        char numStr[8];
        snprintf(numStr, sizeof(numStr), "%d", p.number);
        g_canvas.setTextColor(TFT_WHITE, bgColor);
        g_canvas.drawCentreString(numStr, SCREEN_WIDTH / 2, itemY + 8, 4);
    }

    // 指針
    g_canvas.fillTriangle(4, centerY - 8, 4, centerY + 8, 15, centerY, COLOR_GOLD);
    g_canvas.fillTriangle(SCREEN_WIDTH - 4, centerY - 8, SCREEN_WIDTH - 4, centerY + 8, SCREEN_WIDTH - 15, centerY, COLOR_GOLD);

    // 頂部狀態列
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
    g_canvas.drawString("ROULETTE", 8, 5, 2);
    g_canvas.setTextColor(TFT_WHITE, 0x18C3);
    g_canvas.drawRightString("0-36", SCREEN_WIDTH - 8, 7, 1);

    // 底部狀態
    g_canvas.fillRect(0, 194, SCREEN_WIDTH, 46, TFT_BLACK);
    g_canvas.drawFastHLine(8, 194, SCREEN_WIDTH - 16, 0x39E7);

    if (!_isSpinning) {
        const RoulettePocket& win = WHEEL_POCKETS[_targetIndex];
        const char* colName = (win.colorType == 0) ? "GREEN" :
                              (win.colorType == 1) ? "RED" : "BLACK";
        const char* oddEven = (win.number == 0) ? "" :
                              (win.number % 2 == 1) ? " (ODD)" : " (EVEN)";

        char resStr[24];
        snprintf(resStr, sizeof(resStr), "%d %s%s", win.number, colName, oddEven);
        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawCentreString(resStr, SCREEN_WIDTH / 2, 202, 2);
    } else {
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 202, 2);
    }

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold DOWN to Spin", SCREEN_WIDTH / 2, 224, 1);

    // 一次性推送整幀至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
