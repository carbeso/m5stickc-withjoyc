/**
 * @file SceneEightBall.cpp
 * @brief 直式粒子神秘八號球實作：直書繁體中文、隨機粒子凝聚與英文橫式標籤
 */

#include "scenes/SceneEightBall.h"

SceneEightBall::SceneEightBall()
    : _fortuneIdx(0), _isRevealing(false), _isRevealed(false),
      _revealStartTime(0), _revealStep(0), _needsRedraw(true) {}

void SceneEightBall::init() {
    _needsRedraw = true;
    _isRevealing = false;
    _isRevealed = false;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneEightBall::startDivination(AudioManager& audio, LedManager& led) {
    _fortuneIdx = random(0, FORTUNE_COUNT);
    _isRevealing = true;
    _isRevealed = false;
    _revealStep = 0;
    _revealStartTime = millis();

    audio.playBubble();
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneEightBall::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 觸發占卜：晃動機身、按鍵 A 或中心鍵
    if (!_isRevealing && (input.isShaken || input.btnAPressed || input.joyBtnPressed)) {
        startDivination(audio, led);
    }

    // 粒子凝聚動畫推進
    if (_isRevealing) {
        uint32_t now = millis();
        if (now - _revealStartTime > 60) {
            _revealStartTime = now;
            _revealStep++;
            audio.playTick();

            if (_revealStep >= 16) {
                _isRevealing = false;
                _isRevealed = true;

                // 根據吉凶設定最終 LED 與音效
                uint8_t cat = FORTUNE_LIST[_fortuneIdx].category;
                if (cat == 0) {
                    audio.playCrit();
                    led.setColor(0, 255, 100);       // 吉：綠光
                } else if (cat == 1) {
                    audio.playClick();
                    led.setColor(180, 0, 255);     // 惑：紫光
                } else {
                    audio.playFumble();
                    led.setColor(255, 30, 0);       // 凶：暗紅光
                }
            }
            _needsRedraw = true;
        }
    }
}

/**
 * @brief 繪製 16x16 點陣漢字 (支援粒子逐步點亮)
 */
void SceneEightBall::drawChineseChar(int x, int y, const char* utf8Char, uint8_t maxRow, uint16_t color) {
    const uint16_t* rows = getGlyphBitmap(utf8Char);
    for (int r = 0; r < 16; r++) {
        if (r > maxRow) break; // 粒子逐步展開限制
        uint16_t rowData = pgm_read_word(&(rows[r]));
        for (int c = 0; c < 16; c++) {
            if (rowData & (0x8000 >> c)) {
                // 每個點繪製成 1x1 或 2x2 像素
                M5.Lcd.fillRect(x + c, y + r, 1, 1, color);
            }
        }
    }
}

void SceneEightBall::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部標題列 (Y: 0 ~ 28)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_PURPLE, 0x18C3);
    M5.Lcd.drawString("MAGIC 8-BALL", 8, 5, 2);

    // 2. 待機狀態：繪製經典黑球 8 號
    if (!_isRevealing && !_isRevealed) {
        int cx = SCREEN_WIDTH / 2;
        int cy = 110;
        int radius = 46;

        M5.Lcd.fillCircle(cx, cy, radius, 0x18C3);
        M5.Lcd.drawCircle(cx, cy, radius, COLOR_PURPLE);
        M5.Lcd.fillCircle(cx, cy, 20, TFT_WHITE);
        M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Lcd.drawCentreString("8", cx, cy - 14, 4);

        // 提示文字
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("ASK & SHAKE", cx, 175, 2);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Shake / Press A", cx, 195, 1);
        return;
    }

    // 3. 籤詩結果浮現區 (直書中文 + 橫向英文)
    const FortunePhrase& fp = FORTUNE_LIST[_fortuneIdx];
    uint16_t themeColor = (fp.category == 0) ? TFT_GREEN :
                          (fp.category == 1) ? TFT_MAGENTA : TFT_RED;

    // 繪製背景深藍色占卜視窗邊框
    M5.Lcd.drawRoundRect(6, 32, SCREEN_WIDTH - 12, 160, 6, 0x216A);

    // (A) 直向繁體中文 (4 個字，由上而下，置於左側 X = 20)
    int startY = 40;
    int charSpacing = 28;
    for (int i = 0; i < 4; i++) {
        uint8_t step = _isRevealing ? constrain(_revealStep - i * 3, 0, 15) : 15;
        if (step > 0 || _isRevealed) {
            drawChineseChar(22, startY + i * charSpacing, fp.cn[i], step, themeColor);
        }
    }

    // (B) 橫向英文副標 (置於右側或中下方)
    if (_isRevealed) {
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawString(fp.en, 50, 95, 2);

        M5.Lcd.setTextColor(themeColor, TFT_BLACK);
        const char* catLabel = (fp.category == 0) ? "[POSITIVE]" :
                               (fp.category == 1) ? "[UNCERTAIN]" : "[NEGATIVE]";
        M5.Lcd.drawString(catLabel, 50, 115, 1);
    } else {
        // 凝聚中星塵粒子散布
        for (int p = 0; p < 8; p++) {
            M5.Lcd.drawPixel(50 + random(0, 60), 60 + random(0, 80), COLOR_PURPLE);
        }
    }

    // 4. 底部提示 (Y: 200 ~ 238)
    M5.Lcd.drawFastHLine(8, 202, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("[SHAKE TO RE-ASK]", SCREEN_WIDTH / 2, 212, 1);
}
