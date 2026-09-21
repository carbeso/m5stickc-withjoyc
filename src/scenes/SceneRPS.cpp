/**
 * @file SceneRPS.cpp
 * @brief 剪刀石頭布 (Rock-Paper-Scissors) 場景實作：支援 1~2 隻手、長拉飛轉、放開煞停與勝負結算
 */

#include "scenes/SceneRPS.h"
#include "EntropyManager.h"

const char* GESTURE_NAMES[] = {"ROCK", "SCISSORS", "PAPER"};

SceneRPS::SceneRPS()
    : _handCount(1), _isSpinning(false), _triggeredByJoy(false),
      _spinStartTime(0), _lastTickTime(0), _animCycle(0), _needsRedraw(true) {
    _handResults[0] = GESTURE_ROCK;
    _handResults[1] = GESTURE_SCISSORS;
}

void SceneRPS::init() {
    _needsRedraw = true;
    _isSpinning = false;
    _triggeredByJoy = false;
    _animCycle = 0;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneRPS::startDuel(bool byJoy, AudioManager& audio, LedManager& led) {
    _isSpinning = true;
    _triggeredByJoy = byJoy;
    _spinStartTime = millis();
    _lastTickTime = millis();
    _animCycle = 0;

    audio.playClick();
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneRPS::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isSpinning) {
        // 2. 靜止狀態下：搖桿左右單次邊緣觸發切換 1 或 2 隻手
        if (input.joyPushedLeft) {
            if (_handCount > 1) {
                _handCount = 1;
                audio.playTick();
                _needsRedraw = true;
            }
        } else if (input.joyPushedRight) {
            if (_handCount < 2) {
                _handCount = 2;
                audio.playTick();
                _needsRedraw = true;
            }
        }

        // 3. 觸發旋轉：搖桿下拉 (joyY > 35)、Button A、搖桿中心鍵或機身甩動
        if (input.joyY > 35 || input.btnAPressed || input.joyBtnPressed) {
            startDuel(true, audio, led);
        } else if (input.isShaken) {
            startDuel(false, audio, led);
        }
    } else {
        // 旋轉進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _spinStartTime;

        // 手勢飛速輪替動畫與卡榫音效 (每 70ms 切換一次手勢)
        if (now - _lastTickTime > 70) {
            _lastTickTime = now;
            _animCycle++;
            _handResults[0] = EntropyManager::random(0, 3);
            if (_handCount == 2) {
                _handResults[1] = EntropyManager::random(0, 3);
            }
            audio.playClick();
            _needsRedraw = true;
        }

        // 長拉維持旋轉判斷：純粹依據實體搖桿拉桿 joyY > 35 或長按 Btn A
        bool isHolding = _triggeredByJoy && (input.joyY > 35 || input.isBtnAHeld);

        if (isHolding) {
            // 長拉中持續重設時間，放開後優雅煞停 1.2 秒定格
            if (elapsed > 1500) {
                _spinStartTime = now - 1500;
            }
        } else {
            uint32_t minDuration = _triggeredByJoy ? 2400 : 2000;
            bool stopCondition = false;

            if (_triggeredByJoy) {
                stopCondition = (elapsed >= minDuration);
            } else {
                stopCondition = (elapsed >= minDuration) && input.isNearlyStill;
            }

            if (stopCondition) {
                _isSpinning = false;
                _handResults[0] = EntropyManager::random(0, 3);
                if (_handCount == 2) {
                    _handResults[1] = EntropyManager::random(0, 3);
                }

                if (_handCount == 2) {
                    // 判定雙手勝負音效
                    uint8_t h1 = _handResults[0];
                    uint8_t h2 = _handResults[1];
                    if (h1 == h2) {
                        audio.playClick();
                        led.setColor(0, 200, 255); // 平手青光
                    } else if ((h1 == 0 && h2 == 1) || (h1 == 1 && h2 == 2) || (h1 == 2 && h2 == 0)) {
                        audio.playCrit();
                        led.flash(255, 200, 0, 3, 60); // 上手贏金光
                    } else {
                        audio.playCrit();
                        led.flash(0, 255, 100, 3, 60); // 下手贏綠光
                    }
                } else {
                    audio.playCrit();
                    led.flash(255, 200, 0, 2, 80);
                }
                _needsRedraw = true;
            }
        }
    }
}

void SceneRPS::drawGesture(int cx, int cy, uint8_t gesture, int radius, const char* label) {
    // 繪製外圈立體圓盤
    uint16_t themeColor = (gesture == GESTURE_ROCK) ? COLOR_GOLD :
                          (gesture == GESTURE_SCISSORS) ? COLOR_CYAN : TFT_GREEN;

    M5.Lcd.fillCircle(cx, cy, radius, 0x18C3);
    M5.Lcd.drawCircle(cx, cy, radius, themeColor);
    M5.Lcd.drawCircle(cx, cy, radius - 1, 0x39E7);

    if (gesture == GESTURE_ROCK) {
        // --- 拳頭 / 石頭 (ROCK ✊) ---
        int rw = radius * 5 / 4;
        int rh = radius;
        M5.Lcd.fillRoundRect(cx - rw / 2, cy - rh / 2, rw, rh, 6, COLOR_GOLD);
        M5.Lcd.drawRoundRect(cx - rw / 2, cy - rh / 2, rw, rh, 6, TFT_WHITE);
        M5.Lcd.drawFastVLine(cx - rw / 6, cy - rh / 2 + 3, rh - 6, 0x8A40);
        M5.Lcd.drawFastVLine(cx + rw / 6, cy - rh / 2 + 3, rh - 6, 0x8A40);
        M5.Lcd.fillRoundRect(cx - rw / 2 - 2, cy + 2, rw / 2 + 4, rh / 3, 2, 0xD4A0);
    } else if (gesture == GESTURE_SCISSORS) {
        // --- 剪刀 (SCISSORS ✌) ---
        int palmR = radius / 2;
        M5.Lcd.fillCircle(cx, cy + palmR / 2, palmR, COLOR_CYAN);
        M5.Lcd.drawCircle(cx, cy + palmR / 2, palmR, TFT_WHITE);
        int fingerW = (radius >= 30) ? 8 : 5;
        int fingerH = radius * 4 / 5;
        M5.Lcd.fillRoundRect(cx - fingerW - 2, cy - fingerH, fingerW, fingerH, 3, COLOR_CYAN);
        M5.Lcd.drawRoundRect(cx - fingerW - 2, cy - fingerH, fingerW, fingerH, 3, TFT_WHITE);
        M5.Lcd.fillRoundRect(cx + 2, cy - fingerH, fingerW, fingerH, 3, COLOR_CYAN);
        M5.Lcd.drawRoundRect(cx + 2, cy - fingerH, fingerW, fingerH, 3, TFT_WHITE);
    } else {
        // --- 布 / 手掌 (PAPER ✋) ---
        int palmW = radius * 6 / 5;
        int palmH = radius * 4 / 5;
        M5.Lcd.fillRoundRect(cx - palmW / 2, cy - 2, palmW, palmH, 5, TFT_GREEN);
        M5.Lcd.drawRoundRect(cx - palmW / 2, cy - 2, palmW, palmH, 5, TFT_WHITE);
        int fw = (radius >= 30) ? 6 : 4;
        int fh = radius * 3 / 5;
        for (int i = 0; i < 4; i++) {
            int fx = cx - palmW / 2 + 2 + i * (fw + 2);
            M5.Lcd.fillRoundRect(fx, cy - fh, fw, fh + 4, 2, TFT_GREEN);
            M5.Lcd.drawRoundRect(fx, cy - fh, fw, fh + 4, 2, TFT_WHITE);
        }
    }
}

void SceneRPS::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 1. 頂部狀態列 (Y: 0 ~ 24，標題簡潔大器，絕不重疊)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 24, 0x18C3);
    M5.Lcd.setTextColor(0xFBE0, 0x18C3);
    M5.Lcd.drawString("RPS DUEL", 8, 4, 2);

    M5.Lcd.setTextColor(COLOR_CYAN, 0x18C3);
    M5.Lcd.drawRightString((_handCount == 1) ? "1-HAND" : "2-HANDS", SCREEN_WIDTH - 8, 6, 1);

    // 2. 清空中央手勢動態區 (Y: 25 ~ 170)
    M5.Lcd.fillRect(0, 25, SCREEN_WIDTH, 146, TFT_BLACK);

    if (_handCount == 1) {
        // 單手模式：居中超大手勢 (R = 36)
        drawGesture(SCREEN_WIDTH / 2, 90, _handResults[0], 36, nullptr);

        uint16_t themeColor = (_handResults[0] == GESTURE_ROCK) ? COLOR_GOLD :
                              (_handResults[0] == GESTURE_SCISSORS) ? COLOR_CYAN : TFT_GREEN;
        M5.Lcd.setTextColor(themeColor, TFT_BLACK);
        M5.Lcd.drawCentreString(GESTURE_NAMES[_handResults[0]], SCREEN_WIDTH / 2, 140, 4);
    } else {
        // 雙手模式：上下分割對決佈局 (R = 22)
        // 上手 HAND 1
        uint16_t color1 = (_handResults[0] == GESTURE_ROCK) ? COLOR_GOLD :
                          (_handResults[0] == GESTURE_SCISSORS) ? COLOR_CYAN : TFT_GREEN;
        char p1Str[20];
        snprintf(p1Str, sizeof(p1Str), "P1: %s", GESTURE_NAMES[_handResults[0]]);
        M5.Lcd.setTextColor(color1, TFT_BLACK);
        M5.Lcd.drawCentreString(p1Str, SCREEN_WIDTH / 2, 28, 1);
        drawGesture(SCREEN_WIDTH / 2, 58, _handResults[0], 21, nullptr);

        // 中間 VS 分割標誌
        M5.Lcd.setTextColor(0x7BEF, TFT_BLACK);
        M5.Lcd.drawCentreString("- VS -", SCREEN_WIDTH / 2, 88, 1);

        // 下手 HAND 2
        uint16_t color2 = (_handResults[1] == GESTURE_ROCK) ? COLOR_GOLD :
                          (_handResults[1] == GESTURE_SCISSORS) ? COLOR_CYAN : TFT_GREEN;
        char p2Str[20];
        snprintf(p2Str, sizeof(p2Str), "P2: %s", GESTURE_NAMES[_handResults[1]]);
        M5.Lcd.setTextColor(color2, TFT_BLACK);
        M5.Lcd.drawCentreString(p2Str, SCREEN_WIDTH / 2, 104, 1);
        drawGesture(SCREEN_WIDTH / 2, 134, _handResults[1], 21, nullptr);
    }

    // 3. 底部結算與操作指引區 (Y: 172 ~ 238)
    M5.Lcd.fillRect(0, 172, SCREEN_WIDTH, 68, TFT_BLACK);
    M5.Lcd.drawFastHLine(8, 172, SCREEN_WIDTH - 16, 0x39E7);

    if (_isSpinning) {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("SHOOTING...", SCREEN_WIDTH / 2, 184, 2);
    } else {
        if (_handCount == 2) {
            // 判定雙手勝負
            uint8_t h1 = _handResults[0];
            uint8_t h2 = _handResults[1];
            if (h1 == h2) {
                M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
                M5.Lcd.drawCentreString("DRAW! (TIE)", SCREEN_WIDTH / 2, 182, 2);
            } else if ((h1 == 0 && h2 == 1) || (h1 == 1 && h2 == 2) || (h1 == 2 && h2 == 0)) {
                M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
                M5.Lcd.drawCentreString("P1 WINS!", SCREEN_WIDTH / 2, 182, 2);
            } else {
                M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                M5.Lcd.drawCentreString("P2 WINS!", SCREEN_WIDTH / 2, 182, 2);
            }
        } else {
            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawCentreString("[PULL / SHAKE]", SCREEN_WIDTH / 2, 184, 2);
        }

        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[PULL / SHAKE TO PLAY]", SCREEN_WIDTH / 2, 208, 1);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Joy L/R: 1-2 Hands", SCREEN_WIDTH / 2, 222, 1);
    }
}
