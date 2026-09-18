/**
 * @file SceneEightBall.cpp
 * @brief 直式粒子神秘八號球實作：修復字形指針讀取、Font 4 橫向英文與單色純光
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

    if (!_isRevealing && (input.isShaken || input.btnAPressed || input.joyBtnPressed)) {
        startDivination(audio, led);
    }

    if (_isRevealing) {
        uint32_t now = millis();
        if (now - _revealStartTime > 45) {
            _revealStartTime = now;
            _revealStep++;
            audio.playTick();

            if (_revealStep >= 16) {
                _isRevealing = false;
                _isRevealed = true;

                uint8_t cat = FORTUNE_LIST[_fortuneIdx].category;
                if (cat == 0) {
                    audio.playCrit();
                    led.setColor(0, 255, 0);       // 純綠 (無混光色差)
                } else if (cat == 1) {
                    audio.playClick();
                    led.setColor(200, 0, 255);     // 霓虹紫
                } else {
                    audio.playFumble();
                    led.setColor(255, 0, 0);       // 純紅
                }
            }
            _needsRedraw = true;
        }
    }
}

void SceneEightBall::drawChineseChar(int x, int y, const char* utf8Char, uint8_t maxRow, uint16_t color) {
    const uint16_t* rows = getGlyphBitmap(utf8Char);
    for (int r = 0; r < 16; r++) {
        if (r > maxRow) break;
        uint16_t rowData = rows[r]; // 直接存取 Flash .rodata
        for (int c = 0; c < 16; c++) {
            if (rowData & (0x8000 >> c)) {
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

    // 清空動態占卜區
    M5.Lcd.fillRect(0, 26, SCREEN_WIDTH, 186, TFT_BLACK);

    if (!_isRevealing && !_isRevealed) {
        int cx = SCREEN_WIDTH / 2;
        int cy = 105;
        int radius = 44;

        M5.Lcd.fillCircle(cx, cy, radius, 0x18C3);
        M5.Lcd.drawCircle(cx, cy, radius, COLOR_PURPLE);
        M5.Lcd.fillCircle(cx, cy, 18, TFT_WHITE);
        M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Lcd.drawCentreString("8", cx, cy - 14, 4);

        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("SHAKE TO ASK", cx, 165, 2);
        return;
    }

    const FortunePhrase& fp = FORTUNE_LIST[_fortuneIdx];
    uint16_t themeColor = (fp.category == 0) ? TFT_GREEN :
                          (fp.category == 1) ? TFT_MAGENTA : TFT_RED;

    // 繁體中文 32x32 直向居中 (X = 51)
    int startY = 32;
    int charSpacing = 36;
    int centerX = (SCREEN_WIDTH - 32) / 2; // 51

    for (int i = 0; i < 4; i++) {
        uint8_t step = _isRevealing ? constrain(_revealStep - i * 3, 0, 15) : 15;
        if (step > 0 || _isRevealed) {
            drawChineseChar(centerX, startY + i * charSpacing, fp.cn[i], step, themeColor);
        }
    }

    // 英文副標橫向居中於中文字下方，使用字型 2 或 4 (支援英文字母)，絕不跑版
    if (_isRevealed) {
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawCentreString(fp.en, SCREEN_WIDTH / 2, 180, 2);
    }

    // 底部指引
    M5.Lcd.drawFastHLine(8, 212, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("[SHAKE TO RE-ASK]", SCREEN_WIDTH / 2, 220, 1);
}
