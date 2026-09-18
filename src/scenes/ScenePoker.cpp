/**
 * @file ScenePoker.cpp
 * @brief 極簡大字幸運撲克實作：大花色與文字呈現，無笨重卡牌框
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

    // 建立牌組
    uint8_t idx = 0;
    for (uint8_t s = 0; s < 4; s++) {
        for (uint8_t v = 1; v <= 13; v++) {
            _deck[idx++] = {s, v};
        }
    }
    if (_includeJokers) {
        _deck[idx++] = {4, 1}; // 小鬼牌 (Black Joker)
        _deck[idx++] = {4, 2}; // 大鬼牌 (Red Joker)
    }

    // 費雪-葉慈洗牌法 (Fisher-Yates Shuffle)
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

    // 根據花色設定 LED
    if (_currentCard.suit == 1 || _currentCard.suit == 2) {
        led.setColor(255, 0, 0);       // 紅心/方塊 -> 紅光
    } else if (_currentCard.suit == 0 || _currentCard.suit == 3) {
        led.setColor(220, 220, 255);   // 黑桃/梅花 -> 冷白光
    } else {
        led.flash(255, 0, 255, 3, 70); // 鬼牌 -> 紫彩爆閃
    }

    _needsRedraw = true;
}

void ScenePoker::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 搖桿左右推：切換鬼牌開關
    if (input.joyPushedLeft || input.joyPushedRight) {
        _includeJokers = !_includeJokers;
        audio.playClick();
        shuffleDeck();
        _needsRedraw = true;
    }

    // 搖桿向上推、搖桿中心鍵或按鍵 A：抽牌 / 翻開
    if (input.joyPushedUp || input.joyBtnPressed || input.btnAPressed || input.isShaken) {
        drawCard(audio, led);
    }
}

void ScenePoker::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 30)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 28, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("POKER DRAW", 8, 6, 2);

    // 鬼牌開關與剩餘張數標記
    char infoStr[16];
    snprintf(infoStr, sizeof(infoStr), "%d/%d %s",
             _deckSize - _deckIndex, _deckSize,
             _includeJokers ? "[JK:ON]" : "[JK:OFF]");
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString(infoStr, SCREEN_WIDTH - 6, 8, 1);

    // 2. 中央大字花色與文字呈現區 (Y: 34 ~ 190)
    if (!_isCardRevealed) {
        // 尚未抽牌狀態
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[ READY ]", SCREEN_WIDTH / 2, 90, 4);
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Push UP / Press", SCREEN_WIDTH / 2, 125, 2);
    } else {
        // 已翻開：極簡大字顯示
        bool isRed = (_currentCard.suit == 1 || _currentCard.suit == 2 || (_currentCard.suit == 4 && _currentCard.value == 2));
        uint16_t textColor = isRed ? TFT_RED : TFT_WHITE;

        if (_currentCard.suit == 4) {
            // 鬼牌 JOKER
            M5.Lcd.setTextColor(isRed ? TFT_MAGENTA : TFT_CYAN, TFT_BLACK);
            M5.Lcd.drawCentreString("* JOKER *", SCREEN_WIDTH / 2, 70, 4);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Lcd.drawCentreString(isRed ? "(BIG RED)" : "(BLACK)", SCREEN_WIDTH / 2, 115, 2);
        } else {
            // 四花色與大點數
            const char* suitSymbols[] = {"SPADE", "HEART", "DIAMOND", "CLUB"};
            const char* suitShort[] = {"[S]", "[H]", "[D]", "[C]"};

            // 花色文字標籤
            M5.Lcd.setTextColor(textColor, TFT_BLACK);
            M5.Lcd.drawCentreString(suitSymbols[_currentCard.suit], SCREEN_WIDTH / 2, 48, 4);

            // 巨大點數字體 (A, 2~10, J, Q, K)
            M5.Lcd.setTextColor(textColor, TFT_BLACK);
            M5.Lcd.drawCentreString(VALUE_NAMES[_currentCard.value], SCREEN_WIDTH / 2, 85, 7);
        }
    }

    // 3. 底部操作指引 (Y: 196 ~ 238)
    M5.Lcd.drawFastHLine(8, 196, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy UP / A: DRAW", SCREEN_WIDTH / 2, 204, 2);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy L/R: Toggle Joker", SCREEN_WIDTH / 2, 224, 1);
}
