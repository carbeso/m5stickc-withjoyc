/**
 * @file SceneEightBall.cpp
 * @brief 直式粒子神秘八號球實作：2x2 放大繁體中文字模、居中橫向英文與防閃爍設計
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
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 靈敏甩動或按鍵觸發占卜
    if (!_isRevealing && (input.isShaken || input.btnAPressed || input.joyBtnPressed)) {
        startDivination(audio, led);
    }

    if (_isRevealing) {
        uint32_t now = millis();
        if (now - _revealStartTime > 50) {
            _revealStartTime = now;
            _revealStep++;
            audio.playTick();

            if (_revealStep >= 16) {
                _isRevealing = false;
                _isRevealed = true;

                uint8_t cat = FORTUNE_LIST[_fortuneIdx].category;
                if (cat == 0) {
                    audio.playCrit();
                    led.setColor(0, 255, 100);
                } else if (cat == 1) {
                    audio.playClick();
                    led.setColor(180, 0, 255);
                } else {
                    audio.playFumble();
                    led.setColor(255, 30, 0);
                }
            }
            _needsRedraw = true;
        }
    }
}

/**
 * @brief 繪製 16x16 漢字點陣，以 2x2 像素放大至 32x32，清晰大字看得懂
 */
void SceneEightBall::drawChineseChar(int x, int y, const char* utf8Char, uint8_t maxRow, uint16_t color) {
    const uint16_t* rows = getGlyphBitmap(utf8Char);
    for (int r = 0; r < 16; r++) {
        if (r > maxRow) break;
        uint16_t rowData = pgm_read_word(&(rows[r]));
        for (int c = 0; c < 16; c++) {
            if (rowData & (0x8000 >> c)) {
                // 放大為 2x2 像素方塊
                M5.Lcd.fillRect(x + c * 2, y + r * 2, 2, 2, color);
            }
        }
    }
}

void SceneEightBall::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 頂部標題
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_PURPLE, 0x18C3);
    M5.Lcd.drawString("MAGIC 8-BALL", 8, 5, 2);

    // 清除中央動態占卜區 (Y: 28 ~ 212)
    M5.Lcd.fillRect(0, 28, SCREEN_WIDTH, 184, TFT_BLACK);

    // 待機黑球狀態
    if (!_isRevealing && !_isRevealed) {
        int cx = SCREEN_WIDTH / 2;
        int cy = 110;
        int radius = 46;

        M5.Lcd.fillCircle(cx, cy, radius, 0x18C3);
        M5.Lcd.drawCircle(cx, cy, radius, COLOR_PURPLE);
        M5.Lcd.fillCircle(cx, cy, 20, TFT_WHITE);
        M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Lcd.drawCentreString("8", cx, cy - 14, 4);

        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("ASK & SHAKE", cx, 175, 2);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Shake to Reveal", cx, 195, 1);
        return;
    }

    // 籤文呈現
    const FortunePhrase& fp = FORTUNE_LIST[_fortuneIdx];
    uint16_t themeColor = (fp.category == 0) ? TFT_GREEN :
                          (fp.category == 1) ? TFT_MAGENTA : TFT_RED;

    // 繁體中文 32x32 居中直向排列 (X = 51, 四字居中)
    int startY = 32;
    int charSpacing = 37;
    int centerX = (SCREEN_WIDTH - 32) / 2; // 51

    for (int i = 0; i < 4; i++) {
        uint8_t step = _isRevealing ? constrain(_revealStep - i * 3, 0, 15) : 15;
        if (step > 0 || _isRevealed) {
            drawChineseChar(centerX, startY + i * charSpacing, fp.cn[i], step, themeColor);
        }
    }

    // 英文副標橫向居中顯示於中文字下方 (Y: 188)
    if (_isRevealed) {
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawCentreString(fp.en, SCREEN_WIDTH / 2, 186, 2);
    } else {
        // 凝聚星塵動態微光
        for (int p = 0; p < 6; p++) {
            M5.Lcd.drawPixel(20 + random(0, 95), 40 + random(0, 140), COLOR_PURPLE);
        }
    }

    // 底部指引
    M5.Lcd.drawFastHLine(8, 212, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("[SHAKE TO RE-ASK]", SCREEN_WIDTH / 2, 220, 1);
}
