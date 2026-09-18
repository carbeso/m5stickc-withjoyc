/**
 * @file SceneEightBall.cpp
 * @brief 純英文經典神秘八號球實作：100% 移除中文能力，採用清晰居中英文與二十面體浮牌
 */

#include "scenes/SceneEightBall.h"

// 經典英文八號球籤詩清單 (純 ASCII，絕無字模異常)
const ClassicFortune CLASSIC_FORTUNES[] = {
    // 肯定類 (吉)
    {"IT IS", "CERTAIN", 0},
    {"DEFINITELY", "YES", 0},
    {"WITHOUT", "A DOUBT", 0},
    {"OUTLOOK", "GOOD", 0},
    {"SIGNS POINT", "TO YES", 0},
    {"MOST", "LIKELY", 0},
    {"YES,", "ABSOLUTELY", 0},

    // 猶豫類 (惑)
    {"REPLY HAZY", "TRY AGAIN", 1},
    {"ASK AGAIN", "LATER", 1},
    {"BETTER NOT", "TELL NOW", 1},
    {"CANNOT", "PREDICT", 1},
    {"CONCENTRATE", "& ASK", 1},

    // 否定類 (凶)
    {"DON'T", "COUNT ON IT", 2},
    {"MY REPLY", "IS NO", 2},
    {"MY SOURCES", "SAY NO", 2},
    {"OUTLOOK", "NOT GOOD", 2},
    {"VERY", "DOUBTFUL", 2}
};

const uint8_t CLASSIC_COUNT = sizeof(CLASSIC_FORTUNES) / sizeof(CLASSIC_FORTUNES[0]);

SceneEightBall::SceneEightBall()
    : _fortuneIdx(0), _isRevealing(false), _isRevealed(false), _triggeredByBtn(false),
      _revealStartTime(0), _lastBubbleTime(0), _needsRedraw(true) {}

void SceneEightBall::init() {
    _needsRedraw = true;
    _isRevealing = false;
    _isRevealed = false;
    _triggeredByBtn = false;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneEightBall::startDivination(bool byBtn, AudioManager& audio, LedManager& led) {
    _fortuneIdx = random(0, CLASSIC_COUNT);
    _isRevealing = true;
    _isRevealed = false;
    _triggeredByBtn = byBtn;
    _revealStartTime = millis();
    _lastBubbleTime = millis();

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

    if (!_isRevealing) {
        // 啟動占卜：必須是明確按鍵或甩動脈衝觸發，絕不因常態推持誤觸
        if (input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            startDivination(true, audio, led);
        }
    } else {
        // 翻騰冒泡進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _revealStartTime;

        // 冒泡期間每 250ms 定時發出擬真水聲
        if (now - _lastBubbleTime > 250) {
            _lastBubbleTime = now;
            audio.playBubble();
        }

        // 持續 3000ms (3 秒) 翻騰冒泡後再開籤解答！
        if (elapsed >= 3000) {
            _isRevealing = false;
            _isRevealed = true;

            uint8_t cat = CLASSIC_FORTUNES[_fortuneIdx].category;
            if (cat == 0) {
                audio.playCrit();
                led.setColor(0, 255, 0);       // 吉：純綠
            } else if (cat == 1) {
                audio.playClick();
                led.setColor(200, 0, 255);     // 惑：霓虹紫
            } else {
                audio.playFumble();
                led.setColor(255, 0, 0);       // 凶：純紅
            }
            _needsRedraw = true;
        }
    }
}

void SceneEightBall::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 頂部狀態列：左側 8-BALL，簡潔不壓右側
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_PURPLE, 0x18C3);
    M5.Lcd.drawString("8-BALL", 8, 5, 2);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString("ORACLE", SCREEN_WIDTH - 8, 7, 1);

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

    if (_isRevealing) {
        // 浮現翻滾動效
        int cx = SCREEN_WIDTH / 2;
        int cy = 105;
        M5.Lcd.fillTriangle(cx, cy - 40, cx - 45, cy + 40, cx + 45, cy + 40, 0x0113);
        M5.Lcd.setTextColor(COLOR_CYAN, 0x0113);
        M5.Lcd.drawCentreString("WAIT...", cx, cy - 5, 2);
    } else {
        // 占卜結果：經典深藍色三角形浮動視窗
        const ClassicFortune& cf = CLASSIC_FORTUNES[_fortuneIdx];
        uint16_t txtColor = (cf.category == 0) ? TFT_GREEN :
                            (cf.category == 1) ? COLOR_CYAN : TFT_RED;

        int cx = SCREEN_WIDTH / 2;
        int cy = 105;

        // 倒三角二十面體浮牌
        M5.Lcd.fillTriangle(cx, cy - 55, cx - 58, cy + 48, cx + 58, cy + 48, 0x09CD);
        M5.Lcd.drawTriangle(cx, cy - 55, cx - 58, cy + 48, cx + 58, cy + 48, COLOR_CYAN);

        // 橫向居中顯示純英文文字 (使用抗鋸齒向量字型，字字清楚大氣，絕不跑版)
        M5.Lcd.setTextColor(txtColor, 0x09CD);
        M5.Lcd.drawCentreString(cf.line1, cx, cy - 20, 2);
        M5.Lcd.setTextColor(TFT_WHITE, 0x09CD);
        M5.Lcd.drawCentreString(cf.line2, cx, cy + 2, 2);
    }

    // 底部指引
    M5.Lcd.drawFastHLine(8, 212, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("[SHAKE TO RE-ASK]", SCREEN_WIDTH / 2, 220, 1);
}
