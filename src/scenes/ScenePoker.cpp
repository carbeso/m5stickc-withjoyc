/**
 * @file ScenePoker.cpp
 * @brief 極簡大字幸運撲克實作：加入抽牌高速洗牌跳動動畫、標題防重疊
 */

#include "scenes/ScenePoker.h"

const char* VALUE_NAMES[] = {
    "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

ScenePoker::ScenePoker()
    : _includeJokers(false), _deckSize(52), _deckIndex(0),
      _isCardRevealed(false), _needsRedraw(true),
      _isDrawingAnim(false), _animStartTime(0), _lastTickTime(0) {
    _currentCard = {0, 1};
    _tempAnimCard = {0, 1};
}

void ScenePoker::init() {
    _needsRedraw = true;
    _isDrawingAnim = false;
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
        _deck[idx++] = {4, 1};
        _deck[idx++] = {4, 2};
    }

    for (int i = _deckSize - 1; i > 0; i--) {
        int j = random(0, i + 1);
        Card temp = _deck[i];
        _deck[i] = _deck[j];
        _deck[j] = temp;
    }

    _needsRedraw = true;
}

void ScenePoker::startDrawCard(AudioManager& audio, LedManager& led) {
    if (_deckIndex >= _deckSize) {
        shuffleDeck();
    }

    _isDrawingAnim = true;
    _animStartTime = millis();
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void ScenePoker::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isDrawingAnim) {
        if (input.joyPushedLeft || input.joyPushedRight) {
            _includeJokers = !_includeJokers;
            audio.playClick();
            shuffleDeck();
            _needsRedraw = true;
        }

        if (input.joyPushedUp || input.joyBtnPressed || input.btnAPressed || input.isShaken) {
            startDrawCard(audio, led);
        }
    } else {
        uint32_t now = millis();
        // 抽牌跳動中：每 50ms 隨機變換一張牌面，製造強烈期待感
        if (now - _lastTickTime > 50) {
            _lastTickTime = now;
            _tempAnimCard.suit = random(0, _includeJokers ? 5 : 4);
            _tempAnimCard.value = random(1, 14);
            audio.playTick();
            _needsRedraw = true;
        }

        // 400ms 後定格揭牌
        if (now - _animStartTime > 400) {
            _isDrawingAnim = false;
            _currentCard = _deck[_deckIndex++];
            _isCardRevealed = true;
            audio.playCardDraw();

            if (_currentCard.suit == 1 || _currentCard.suit == 2) {
                led.setColor(255, 0, 0);
            } else if (_currentCard.suit == 0 || _currentCard.suit == 3) {
                led.setColor(0, 200, 255);
            } else {
                led.flash(220, 0, 255, 3, 70);
            }
            _needsRedraw = true;
        }
    }
}

static void drawPokerSuit(int cx, int cy, uint8_t suit, uint16_t color) {
    switch (suit) {
        case 0: // ♠
            M5.Lcd.fillTriangle(cx, cy - 20, cx - 15, cy + 2, cx + 15, cy + 2, color);
            M5.Lcd.fillCircle(cx - 8, cy + 2, 8, color);
            M5.Lcd.fillCircle(cx + 8, cy + 2, 8, color);
            M5.Lcd.fillTriangle(cx, cy, cx - 5, cy + 18, cx + 5, cy + 18, color);
            break;
        case 1: // ♥
            M5.Lcd.fillCircle(cx - 8, cy - 6, 9, color);
            M5.Lcd.fillCircle(cx + 8, cy - 6, 9, color);
            M5.Lcd.fillTriangle(cx - 16, cy - 4, cx + 16, cy - 4, cx, cy + 18, color);
            break;
        case 2: // ♦
            M5.Lcd.fillTriangle(cx, cy - 19, cx - 15, cy, cx + 15, cy, color);
            M5.Lcd.fillTriangle(cx, cy + 19, cx - 15, cy, cx + 15, cy, color);
            break;
        case 3: // ♣
            M5.Lcd.fillCircle(cx, cy - 9, 8, color);
            M5.Lcd.fillCircle(cx - 9, cy + 2, 8, color);
            M5.Lcd.fillCircle(cx + 9, cy + 2, 8, color);
            M5.Lcd.fillTriangle(cx, cy, cx - 5, cy + 18, cx + 5, cy + 18, color);
            break;
        case 4: // JOKER
            M5.Lcd.fillCircle(cx, cy, 14, color);
            M5.Lcd.fillTriangle(cx, cy - 18, cx - 5, cy, cx + 5, cy, TFT_WHITE);
            M5.Lcd.fillTriangle(cx, cy + 18, cx - 5, cy, cx + 5, cy, TFT_WHITE);
            M5.Lcd.fillTriangle(cx - 18, cy, cx, cy - 5, cx, cy + 5, TFT_WHITE);
            M5.Lcd.fillTriangle(cx + 18, cy, cx, cy - 5, cx, cy + 5, TFT_WHITE);
            break;
    }
}

void ScenePoker::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 頂部狀態列：左 POKER，右精簡 52/54 [JK]，兩者相隔 > 40px，絕不重疊！
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("POKER", 8, 5, 2);

    char infoStr[12];
    snprintf(infoStr, sizeof(infoStr), "%d %s",
             _deckSize - _deckIndex,
             _includeJokers ? "[JK]" : "");
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString(infoStr, SCREEN_WIDTH - 6, 7, 1);

    // 清除中央牌面區
    M5.Lcd.fillRect(0, 26, SCREEN_WIDTH, 170, TFT_BLACK);

    Card showCard = _isDrawingAnim ? _tempAnimCard : _currentCard;

    if (!_isCardRevealed && !_isDrawingAnim) {
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[ READY ]", SCREEN_WIDTH / 2, 85, 4);
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Push UP / Press", SCREEN_WIDTH / 2, 120, 2);
    } else {
        bool isRed = (showCard.suit == 1 || showCard.suit == 2 || (showCard.suit == 4 && showCard.value == 2));
        uint16_t themeColor = isRed ? TFT_RED : TFT_WHITE;

        if (showCard.suit == 4) {
            drawPokerSuit(SCREEN_WIDTH / 2, 65, 4, isRed ? TFT_MAGENTA : TFT_CYAN);
            M5.Lcd.setTextColor(isRed ? TFT_MAGENTA : TFT_CYAN, TFT_BLACK);
            M5.Lcd.drawCentreString("JOKER", SCREEN_WIDTH / 2, 105, 4);
        } else {
            drawPokerSuit(SCREEN_WIDTH / 2, 65, showCard.suit, themeColor);

            M5.Lcd.setTextColor(themeColor, TFT_BLACK);
            M5.Lcd.setTextSize(2);
            M5.Lcd.drawCentreString(VALUE_NAMES[showCard.value], SCREEN_WIDTH / 2, 105, 4);
            M5.Lcd.setTextSize(1);
        }
    }

    // 底部指引
    M5.Lcd.drawFastHLine(8, 196, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy UP / A: DRAW", SCREEN_WIDTH / 2, 204, 2);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy L/R: Toggle Joker", SCREEN_WIDTH / 2, 224, 1);
}
