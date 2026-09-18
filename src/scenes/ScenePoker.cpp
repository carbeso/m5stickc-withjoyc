/**
 * @file ScenePoker.cpp
 * @brief 極簡大字幸運撲克實作：真實花色圖案繪製、修復 JQK 字母顯示與無閃爍體驗
 */

#include "scenes/ScenePoker.h"

const char* VALUE_NAMES[] = {
    "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

ScenePoker::ScenePoker()
    : _includeJokers(false), _deckSize(52), _deckIndex(0),
      _isCardRevealed(false), _needsRedraw(true),
      _btnAPressTime(0), _btnAClickCount(0) {
    _currentCard = {0, 1}; // 預設黑桃 A
}

void ScenePoker::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    shuffleDeck();
    M5.Lcd.fillScreen(TFT_BLACK);
}

void ScenePoker::shuffleDeck() {
    _deckSize = _includeJokers ? 54 : 52;
    _deckIndex = 0;
    _isCardRevealed = false;

    uint8_t idx = 0;
    for (uint8_t s = 0; s < 4; s++) {
        for (uint8_t v = 1; v <= 13; v++) {
            _deck[idx++] = {s, v};
        }
    }
    if (_includeJokers) {
        _deck[idx++] = {4, 1}; // 小鬼
        _deck[idx++] = {4, 2}; // 大鬼
    }

    for (int i = _deckSize - 1; i > 0; i--) {
        int j = random(0, i + 1);
        Card temp = _deck[i];
        _deck[i] = _deck[j];
        _deck[j] = temp;
    }

    _needsRedraw = true;
}

void ScenePoker::drawCard(AudioManager& audio, LedManager& led) {
    if (_deckIndex >= _deckSize) {
        shuffleDeck();
        audio.playDiceRoll();
    }

    _currentCard = _deck[_deckIndex++];
    _isCardRevealed = true;
    audio.playCardDraw();

    if (_currentCard.suit == 1 || _currentCard.suit == 2) {
        led.setColor(255, 0, 0);
    } else if (_currentCard.suit == 0 || _currentCard.suit == 3) {
        led.setColor(220, 220, 255);
    } else {
        led.flash(255, 0, 255, 3, 70);
    }

    _needsRedraw = true;
}

void ScenePoker::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (input.joyPushedLeft || input.joyPushedRight) {
        _includeJokers = !_includeJokers;
        audio.playClick();
        shuffleDeck();
        _needsRedraw = true;
    }

    // 向上推搖桿、按鍵 A、中心鍵或晃動抽牌
    if (input.joyPushedUp || input.joyBtnPressed || input.btnAPressed || input.isShaken) {
        drawCard(audio, led);
    }
}

/**
 * @brief 繪製真實撲克四花色圖案
 */
static void drawPokerSuit(int cx, int cy, uint8_t suit, uint16_t color) {
    switch (suit) {
        case 0: // ♠ 黑桃 (Spade)
            // 上尖三角
            M5.Lcd.fillTriangle(cx, cy - 18, cx - 14, cy + 2, cx + 14, cy + 2, color);
            // 左右兩個小圓弧
            M5.Lcd.fillCircle(cx - 7, cy + 1, 8, color);
            M5.Lcd.fillCircle(cx + 7, cy + 1, 8, color);
            // 底部立足
            M5.Lcd.fillTriangle(cx, cy - 2, cx - 6, cy + 16, cx + 6, cy + 16, color);
            break;

        case 1: // ♥ 紅心 (Heart)
            // 左右兩瓣圓弧
            M5.Lcd.fillCircle(cx - 8, cy - 6, 9, color);
            M5.Lcd.fillCircle(cx + 8, cy - 6, 9, color);
            // 下方尖三角
            M5.Lcd.fillTriangle(cx - 16, cy - 4, cx + 16, cy - 4, cx, cy + 16, color);
            break;

        case 2: // ♦ 方塊 (Diamond)
            // 菱形
            M5.Lcd.fillTriangle(cx, cy - 18, cx - 14, cy, cx + 14, cy, color);
            M5.Lcd.fillTriangle(cx, cy + 18, cx - 14, cy, cx + 14, cy, color);
            break;

        case 3: // ♣ 梅花 (Club)
            // 上、左、右三個小圓
            M5.Lcd.fillCircle(cx, cy - 9, 8, color);
            M5.Lcd.fillCircle(cx - 9, cy + 2, 8, color);
            M5.Lcd.fillCircle(cx + 9, cy + 2, 8, color);
            // 底部立足
            M5.Lcd.fillTriangle(cx, cy - 2, cx - 6, cy + 16, cx + 6, cy + 16, color);
            break;

        case 4: // JOKER 星芒圖案
            M5.Lcd.fillCircle(cx, cy, 14, color);
            M5.Lcd.fillTriangle(cx, cy - 18, cx - 6, cy, cx + 6, cy, TFT_WHITE);
            M5.Lcd.fillTriangle(cx, cy + 18, cx - 6, cy, cx + 6, cy, TFT_WHITE);
            M5.Lcd.fillTriangle(cx - 18, cy, cx, cy - 6, cx, cy + 6, TFT_WHITE);
            M5.Lcd.fillTriangle(cx + 18, cy, cx, cy - 6, cx, cy + 6, TFT_WHITE);
            break;
    }
}

void ScenePoker::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 局部重繪：頂部靜態列
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 28, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("POKER", 8, 6, 2);

    char infoStr[16];
    snprintf(infoStr, sizeof(infoStr), "%d/%d %s",
             _deckSize - _deckIndex, _deckSize,
             _includeJokers ? "[JK:ON]" : "[JK:OFF]");
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString(infoStr, SCREEN_WIDTH - 6, 8, 1);

    // 清除中央動態牌面區 (Y: 30 ~ 194)
    M5.Lcd.fillRect(0, 30, SCREEN_WIDTH, 164, TFT_BLACK);

    if (!_isCardRevealed) {
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[ READY ]", SCREEN_WIDTH / 2, 90, 4);
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Push UP / Press", SCREEN_WIDTH / 2, 125, 2);
    } else {
        bool isRed = (_currentCard.suit == 1 || _currentCard.suit == 2 || (_currentCard.suit == 4 && _currentCard.value == 2));
        uint16_t themeColor = isRed ? TFT_RED : TFT_WHITE;

        if (_currentCard.suit == 4) {
            // 鬼牌：繪製專屬星芒與大字 JOKER
            drawPokerSuit(SCREEN_WIDTH / 2, 70, 4, isRed ? TFT_MAGENTA : TFT_CYAN);
            M5.Lcd.setTextColor(isRed ? TFT_MAGENTA : TFT_CYAN, TFT_BLACK);
            M5.Lcd.drawCentreString("JOKER", SCREEN_WIDTH / 2, 105, 4);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Lcd.drawCentreString(isRed ? "(COLOR)" : "(BLACK)", SCREEN_WIDTH / 2, 145, 2);
        } else {
            // 繪製撲克花色圖案 (♠ ♥ ♦ ♣)
            drawPokerSuit(SCREEN_WIDTH / 2, 65, _currentCard.suit, themeColor);

            // 繪製巨幅點數 (A, 2~10, J, Q, K) - 使用 Font 6 (48點陣 ASCII，100% 支援字母)
            M5.Lcd.setTextColor(themeColor, TFT_BLACK);
            M5.Lcd.drawCentreString(VALUE_NAMES[_currentCard.value], SCREEN_WIDTH / 2, 105, 6);
        }
    }

    // 底部指引
    M5.Lcd.drawFastHLine(8, 196, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy UP / A: DRAW", SCREEN_WIDTH / 2, 204, 2);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy L/R: Toggle Joker", SCREEN_WIDTH / 2, 224, 1);
}
