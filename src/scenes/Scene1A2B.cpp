/**
 * @file Scene1A2B.cpp
 * @brief 1A2B 益智猜數字遊戲實作：直向排版、十字防呆、輪次記錄與彩虹勝利反饋
 */

#include "scenes/Scene1A2B.h"

const int BOX_W = 25;
const int BOX_H = 46;
const int BOX_GAP = 5;
const int BOX_START_X = (SCREEN_WIDTH - (4 * BOX_W + 3 * BOX_GAP)) / 2; // 10px
const int BOX_Y = 65;

Scene1A2B::Scene1A2B()
    : _state(STATE_1A2B_IDLE), _cursor(0), _attempts(0),
      _lastA(0), _lastB(0), _hasEvaluated(false), _lastNavTime(0), _needsRedraw(true) {
    for (int i = 0; i < 4; i++) {
        _target[i] = 0;
        _guess[i] = 0;
    }
}

void Scene1A2B::init() {
    _state = STATE_1A2B_IDLE;
    _cursor = 0;
    _attempts = 0;
    _lastA = 0;
    _lastB = 0;
    _hasEvaluated = false;
    _nextScene = SCENE_COUNT;
    _needsRedraw = true;

    for (int i = 0; i < 4; i++) {
        _guess[i] = 0;
    }
}

void Scene1A2B::generateTarget() {
    // 建立 0~9 陣列並利用物理熵源進行 Fisher-Yates 洗牌
    uint8_t digits[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int i = 9; i > 0; i--) {
        int j = EntropyManager::random(0, i + 1);
        uint8_t tmp = digits[i];
        digits[i] = digits[j];
        digits[j] = tmp;
    }
    for (int i = 0; i < 4; i++) {
        _target[i] = digits[i];
    }
}

void Scene1A2B::evaluateGuess(AudioManager& audio, LedManager& led) {
    _attempts++;
    _lastA = 0;
    _lastB = 0;

    bool targetCounted[4] = {false, false, false, false};
    bool guessCounted[4] = {false, false, false, false};

    // 1. 先判定 A (位置與數字完全一致)
    for (int i = 0; i < 4; i++) {
        if (_guess[i] == _target[i]) {
            _lastA++;
            targetCounted[i] = true;
            guessCounted[i] = true;
        }
    }

    // 2. 再判定 B (數字存在但位置不同)
    for (int i = 0; i < 4; i++) {
        if (guessCounted[i]) continue;
        for (int j = 0; j < 4; j++) {
            if (!targetCounted[j] && _guess[i] == _target[j]) {
                _lastB++;
                targetCounted[j] = true;
                break;
            }
        }
    }

    _hasEvaluated = true;

    if (_lastA == 4) {
        _state = STATE_1A2B_WON;
        audio.playWin();
        led.setRainbowMode(true);
    } else {
        audio.playClick();
        if (_lastA > 0) {
            led.setColor(50, 255, 50); // 有 A 亮綠光
        } else if (_lastB > 0) {
            led.setColor(255, 180, 0); // 僅 B 亮橙光
        } else {
            led.setColor(180, 0, 0);   // 0A0B 亮紅光
        }
    }
    _needsRedraw = true;
}

void Scene1A2B::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回全域主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    uint32_t now = millis();

    // 2. 待機狀態 (IDLE)
    if (_state == STATE_1A2B_IDLE) {
        led.setColor(0, 150, 255);
        if (input.joyBtnPressed || input.btnAPressed) {
            generateTarget();
            _state = STATE_1A2B_PLAYING;
            _cursor = 0;
            _attempts = 0;
            _hasEvaluated = false;
            for (int i = 0; i < 4; i++) _guess[i] = 0;
            audio.playClick();
            _needsRedraw = true;
        }
        return;
    }

    // 3. 獲勝狀態 (WON)
    if (_state == STATE_1A2B_WON) {
        if (input.joyBtnPressed || input.btnAPressed) {
            generateTarget();
            _state = STATE_1A2B_PLAYING;
            _cursor = 0;
            _attempts = 0;
            _hasEvaluated = false;
            for (int i = 0; i < 4; i++) _guess[i] = 0;
            audio.playClick();
            led.setRainbowMode(false);
            led.setColor(0, 200, 255);
            _needsRedraw = true;
        }
        return;
    }

    // 4. 作答狀態 (PLAYING)：搖桿精密防抖與軸向互斥
    if (now - _lastNavTime > 200) {
        int16_t absX = abs(input.joyX);
        int16_t absY = abs(input.joyY);

        if (absX > 45 && absX >= absY) {
            // 左右切換位數游標
            if (input.joyX < 0) {
                if (_cursor > 0) _cursor--;
                else _cursor = 3;
            } else {
                if (_cursor < 3) _cursor++;
                else _cursor = 0;
            }
            audio.playTick();
            _lastNavTime = now;
            _needsRedraw = true;
        } else if (absY > 45 && absY > absX) {
            // 上下滾動數字 (0~9 循環：joyY < 0 向上推數字遞增，joyY > 0 向下推數字遞減)
            if (input.joyY < 0) {
                _guess[_cursor] = (_guess[_cursor] + 1) % 10;
            } else {
                _guess[_cursor] = (_guess[_cursor] + 9) % 10;
            }
            audio.playTick();
            _lastNavTime = now;
            _needsRedraw = true;
        }
    }

    // 5. 按下搖桿中鍵或 Button A：送出比對
    if (input.joyBtnPressed || input.btnAPressed) {
        evaluateGuess(audio, led);
    }
}

void Scene1A2B::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布離線繪製，杜絕位數與游標切換閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部標題列 (Y: 0 ~ 26)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18E3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18E3);
    g_canvas.drawString("1A2B PUZZLE", 6, 5, 2);

    char roundStr[10];
    snprintf(roundStr, sizeof(roundStr), "R:%d", _attempts);
    g_canvas.setTextColor(COLOR_CYAN, 0x18E3);
    g_canvas.drawRightString(roundStr, SCREEN_WIDTH - 6, 5, 2);

    // 2. 狀態與提示條 (Y: 34 ~ 52)
    if (_state == STATE_1A2B_IDLE) {
        g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
        g_canvas.drawCentreString("PRESS JOY TO START", SCREEN_WIDTH / 2, 36, 2);
    } else if (_state == STATE_1A2B_WON) {
        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawCentreString("YOU WIN! 4A 0B", SCREEN_WIDTH / 2, 36, 2);
    } else {
        char promptStr[24];
        snprintf(promptStr, sizeof(promptStr), "ROUND #%d GUESS", _attempts + 1);
        g_canvas.setTextColor(COLOR_LIGHT_BLUE, TFT_BLACK);
        g_canvas.drawCentreString(promptStr, SCREEN_WIDTH / 2, 36, 2);
    }

    // 3. 中央 4 位數位槽 (Y: 65 ~ 111)
    for (int i = 0; i < 4; i++) {
        int x = BOX_START_X + i * (BOX_W + BOX_GAP);
        bool isCur = (_state == STATE_1A2B_PLAYING && i == _cursor);

        if (isCur) {
            g_canvas.fillRoundRect(x, BOX_Y, BOX_W, BOX_H, 4, COLOR_CYAN);
            g_canvas.setTextColor(TFT_BLACK, COLOR_CYAN);
        } else {
            g_canvas.drawRoundRect(x, BOX_Y, BOX_W, BOX_H, 4, 0x39E7);
            g_canvas.setTextColor(TFT_WHITE, TFT_BLACK);
        }

        if (_state == STATE_1A2B_IDLE) {
            g_canvas.drawCentreString("-", x + BOX_W / 2, BOX_Y + 10, 4);
        } else {
            char dStr[2];
            dStr[0] = '0' + _guess[i];
            dStr[1] = '\0';
            g_canvas.drawCentreString(dStr, x + BOX_W / 2, BOX_Y + 10, 4);
        }
    }

    // 4. 最新判定結果卡片 (Y: 124 ~ 184)
    g_canvas.drawRoundRect(10, 124, SCREEN_WIDTH - 20, 60, 4, 0x2965);

    if (!_hasEvaluated && _state != STATE_1A2B_WON) {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("Set digits & Click", SCREEN_WIDTH / 2, 142, 2);
        g_canvas.drawCentreString("to submit answer", SCREEN_WIDTH / 2, 160, 1);
    } else {
        char resStr[20];
        snprintf(resStr, sizeof(resStr), "%dA %dB", _lastA, _lastB);

        uint16_t resColor = (_lastA == 4) ? TFT_GREEN :
                            (_lastA > 0) ? COLOR_GOLD :
                            (_lastB > 0) ? COLOR_LIGHT_BLUE : TFT_RED;

        g_canvas.setTextColor(resColor, TFT_BLACK);
        g_canvas.drawCentreString(resStr, SCREEN_WIDTH / 2, 132, 4);

        char subInfo[32];
        if (_lastA == 4) {
            snprintf(subInfo, sizeof(subInfo), "SOLVED IN %d ROUNDS!", _attempts);
        } else {
            snprintf(subInfo, sizeof(subInfo), "Attempts: %d", _attempts);
        }
        g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        g_canvas.drawCentreString(subInfo, SCREEN_WIDTH / 2, 166, 1);
    }

    // 5. 底部操作指示 (Y: 194 ~ 238)
    g_canvas.drawFastHLine(8, 194, SCREEN_WIDTH - 16, 0x39E7);

    if (_state == STATE_1A2B_PLAYING) {
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("Joy L/R: Digits", SCREEN_WIDTH / 2, 200, 1);
        g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
        g_canvas.drawCentreString("Joy U/D: 0-9  Click: Send", SCREEN_WIDTH / 2, 212, 1);
    } else if (_state == STATE_1A2B_WON) {
        g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
        g_canvas.drawCentreString("Press Joy to Restart", SCREEN_WIDTH / 2, 206, 1);
    } else {
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("Press Joy to Start", SCREEN_WIDTH / 2, 206, 1);
    }

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold BtnB: Menu", SCREEN_WIDTH / 2, 225, 1);

    // 一次性推送畫面至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
