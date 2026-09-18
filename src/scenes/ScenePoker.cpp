/**
 * @file ScenePoker.cpp
 * @brief 極簡大字幸運撲克實作：單抽/銷牌雙模式、牌堆抽空手動重置、搖桿推持持續洗牌與甩動立刻動效
 */

#include "scenes/ScenePoker.h"

const char* VALUE_NAMES[] = {
    "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

ScenePoker::ScenePoker()
    : _singleMode(false), _includeJokers(false), _deckSize(52), _deckIndex(0),
      _isCardRevealed(false), _deckEmpty(false), _needsRedraw(true),
      _isShuffling(false), _triggeredByJoy(false),
      _animStartTime(0), _lastTickTime(0) {
    _currentCard = {0, 1};
    _tempAnimCard = {0, 1};
}

void ScenePoker::init() {
    _needsRedraw = true;
    _isShuffling = false;
    _deckEmpty = false;
    _nextScene = SCENE_COUNT;
    resetAndShuffle();
    M5.Lcd.fillScreen(TFT_BLACK);
}

void ScenePoker::resetAndShuffle() {
    _deckSize = _includeJokers ? 54 : 52;
    _deckIndex = 0;
    _isCardRevealed = false;
    _deckEmpty = false;

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

void ScenePoker::startShuffle(bool byJoy, AudioManager& audio, LedManager& led) {
    if (_isShuffling) return;

    // 若牌堆已空，不可再抽，切換為 EMPTY 畫面需先手動重置
    if (!_singleMode && _deckIndex >= _deckSize) {
        _deckEmpty = true;
        audio.playFumble();
        _needsRedraw = true;
        return;
    }

    _isShuffling = true;
    _triggeredByJoy = byJoy;
    _animStartTime = millis();
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void ScenePoker::finalizeDraw(AudioManager& audio, LedManager& led) {
    _isShuffling = false;

    if (_singleMode) {
        // 單抽模式：全牌堆純隨機單抽
        uint8_t s = random(0, _includeJokers ? 5 : 4);
        uint8_t v = (s == 4) ? random(1, 3) : random(1, 14);
        _currentCard = {s, v};
    } else {
        // 銷牌模式：按牌堆依序開出 (抽到最後一張亦完整秀出，下次再抽時才提示 EMPTY)
        _currentCard = _deck[_deckIndex++];
    }

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

void ScenePoker::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    // 牌堆已空時手動按 Button A 或中心鍵重置
    if (_deckEmpty && (input.btnAPressed || input.joyBtnPressed)) {
        resetAndShuffle();
        audio.playDiceRoll();
        _needsRedraw = true;
        return;
    }

    if (!_isShuffling) {
        // 搖桿向左推：切換抽牌模式 (銷牌 DECK ↔ 單抽 SINGLE)
        if (input.joyPushedLeft) {
            _singleMode = !_singleMode;
            audio.playTick();
            resetAndShuffle();
            _needsRedraw = true;
        }

        // 搖桿向右推：切換鬼牌開關 (JOKER ON ↔ OFF)
        if (input.joyPushedRight) {
            _includeJokers = !_includeJokers;
            audio.playClick();
            resetAndShuffle();
            _needsRedraw = true;
        }

        // 啟動洗牌：推/拉搖桿、按鍵或甩動脈衝觸發
        if (input.joyPushedUp || input.joyPulledDown || input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            startShuffle(!input.isShaken, audio, led);
        }
    } else {
        // 洗牌持續進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _animStartTime;

        if (now - _lastTickTime > 45) {
            _lastTickTime = now;
            _tempAnimCard.suit = random(0, _includeJokers ? 5 : 4);
            _tempAnimCard.value = (_tempAnimCard.suit == 4) ? random(1, 3) : random(1, 14);
            audio.playTick();
            _needsRedraw = true;
        }

        // 持續洗牌判斷：搖桿向上推或向下拉著，或持續按著 Button A
        bool isHoldingJoy = (abs(input.joyY) > 35 || input.isBtnAHeld);

        // 停止判定：
        // 1. 若為搖桿長推/長拉操作：只要還拉著/推著就持續洗牌；放開後 350ms 停牌開牌！
        // 2. 若為短按或體感甩動：至少持續洗牌 2500ms (2.5 秒) 再定格開牌，不再太快停止！
        bool readyToStop = false;
        if (_triggeredByJoy) {
            if (!isHoldingJoy && (elapsed > 400)) {
                readyToStop = true;
            }
        } else {
            // 體感甩動：手部脫離激烈甩動且已洗牌滿 2500ms
            if (!input.isActivelyShaking && (elapsed >= 2500)) {
                readyToStop = true;
            }
        }
        if (elapsed > 30000) readyToStop = true; // 30 秒安全超時

        if (readyToStop) {
            finalizeDraw(audio, led);
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

    // 頂部狀態列：左側 POKER，右側顯示模式與張數
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("POKER", 8, 5, 2);

    char infoStr[14];
    if (_singleMode) {
        snprintf(infoStr, sizeof(infoStr), "%s", _includeJokers ? "SGL [JK]" : "SINGLE");
    } else {
        snprintf(infoStr, sizeof(infoStr), "%d/%d", _deckSize - _deckIndex, _deckSize);
    }
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString(infoStr, SCREEN_WIDTH - 8, 6, 2);

    // 清除中央牌面區
    M5.Lcd.fillRect(0, 26, SCREEN_WIDTH, 168, TFT_BLACK);

    Card showCard = _isShuffling ? _tempAnimCard : _currentCard;

    if (_deckEmpty && !_isShuffling) {
        // 牌堆已抽空：明確顯示 EMPTY，提示手動重置！
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.drawCentreString("[ EMPTY ]", SCREEN_WIDTH / 2, 75, 4);
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("DECK FINISHED", SCREEN_WIDTH / 2, 110, 2);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawCentreString("PRESS A TO RESET", SCREEN_WIDTH / 2, 135, 2);
    } else if (!_isCardRevealed && !_isShuffling) {
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[ READY ]", SCREEN_WIDTH / 2, 85, 4);
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Push UP & Hold", SCREEN_WIDTH / 2, 120, 2);
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
    M5.Lcd.drawFastHLine(8, 194, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("UP/A: DRAW (Hold)", SCREEN_WIDTH / 2, 200, 2);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Joy L:Mode  R:Joker", SCREEN_WIDTH / 2, 222, 1);
}
