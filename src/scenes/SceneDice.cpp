/**
 * @file SceneDice.cpp
 * @brief 直式多面骰子盒實作：修復骰子下框被覆蓋問題、強化局部刷新
 */

#include "scenes/SceneDice.h"

const uint8_t DIE_FACES[] = {4, 6, 8, 10, 12, 20, 100};
const char* DIE_NAMES[] = {"d4", "d6", "d8", "d10", "d12", "d20", "d100"};
const uint8_t DIE_TYPE_COUNT = sizeof(DIE_FACES) / sizeof(DIE_FACES[0]);

SceneDice::SceneDice()
    : _dieTypeIdx(1), _diceCount(1),
      _isRolling(false), _rollStartTime(0), _lastTickTime(0), _needsRedraw(true) {
    for (int i = 0; i < 6; i++) _diceResults[i] = 1;
}

void SceneDice::init() {
    _needsRedraw = true;
    _isRolling = false;
    _nextScene = SCENE_COUNT;
    for (int i = 0; i < 6; i++) {
        _diceResults[i] = random(1, DIE_FACES[_dieTypeIdx] + 1);
    }
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneDice::rollDice(AudioManager& audio, LedManager& led) {
    _isRolling = true;
    _rollStartTime = millis();
    audio.playDiceRoll();
    led.setRainbowMode(true);
}

void SceneDice::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isRolling) {
        if (input.joyPushedLeft) {
            if (_dieTypeIdx > 0) _dieTypeIdx--;
            else _dieTypeIdx = DIE_TYPE_COUNT - 1;
            audio.playTick();
            _needsRedraw = true;
        } else if (input.joyPushedRight) {
            if (_dieTypeIdx < DIE_TYPE_COUNT - 1) _dieTypeIdx++;
            else _dieTypeIdx = 0;
            audio.playTick();
            _needsRedraw = true;
        }

        if (input.joyPushedUp) {
            if (_diceCount < 6) _diceCount++;
            audio.playTick();
            _needsRedraw = true;
        } else if (input.joyPulledDown) {
            if (_diceCount > 1) _diceCount--;
            audio.playTick();
            _needsRedraw = true;
        }

        if (input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            rollDice(audio, led);
        }
    } else {
        uint32_t now = millis();
        if (now - _lastTickTime > 45) {
            _lastTickTime = now;
            for (uint8_t i = 0; i < _diceCount; i++) {
                _diceResults[i] = random(1, DIE_FACES[_dieTypeIdx] + 1);
            }
            audio.playDiceRoll();
            _needsRedraw = true;
        }

        if (now - _rollStartTime > 550) {
            _isRolling = false;
            for (uint8_t i = 0; i < _diceCount; i++) {
                _diceResults[i] = random(1, DIE_FACES[_dieTypeIdx] + 1);
            }

            if (DIE_FACES[_dieTypeIdx] == 20 && _diceCount == 1) {
                if (_diceResults[0] == 20) {
                    audio.playCrit();
                    led.flash(0, 255, 255, 5, 50); // 冰河青爆閃替代白光
                } else if (_diceResults[0] == 1) {
                    audio.playFumble();
                    led.flash(255, 0, 0, 3, 100);
                } else {
                    audio.playClick();
                    led.setColor(255, 180, 0);
                }
            } else {
                audio.playClick();
                led.setColor(255, 180, 0);
            }
            _needsRedraw = true;
        }
    }
}

void SceneDice::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    static uint8_t lastCount = 255;
    static uint8_t lastType = 255;
    if (lastCount != _diceCount || lastType != _dieTypeIdx) {
        lastCount = _diceCount;
        lastType = _dieTypeIdx;
        M5.Lcd.fillScreen(TFT_BLACK);

        M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 30, 0x2124);
        M5.Lcd.setTextColor(COLOR_GOLD, 0x2124);
        M5.Lcd.drawString("DICE ROLLER", 8, 4, 2);

        char specStr[16];
        snprintf(specStr, sizeof(specStr), "%dd%d", _diceCount, DIE_FACES[_dieTypeIdx]);
        M5.Lcd.setTextColor(TFT_WHITE, 0x2124);
        M5.Lcd.drawRightString(specStr, SCREEN_WIDTH - 8, 8, 2);

        M5.Lcd.drawFastHLine(6, 186, SCREEN_WIDTH - 12, 0x4208);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Joy L/R:d#  U/D:cnt", SCREEN_WIDTH / 2, 224, 1);
    }

    int total = 0;
    for (uint8_t i = 0; i < _diceCount; i++) total += _diceResults[i];

    if (_diceCount == 1) {
        int cx = SCREEN_WIDTH / 2;
        int cy = 104;
        int size = 42; // 下邊緣在 cy + size = 146

        // 繪製完整外框
        M5.Lcd.drawRoundRect(cx - size, cy - size, size * 2, size * 2, 8, COLOR_GOLD);
        // 只在內部填黑，絕對不破壞下框
        M5.Lcd.fillRoundRect(cx - size + 2, cy - size + 2, size * 2 - 4, size * 2 - 4, 6, TFT_BLACK);

        char numStr[8];
        snprintf(numStr, sizeof(numStr), "%d", _diceResults[0]);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        // 數字置中 (字型 4 放大或使用乾淨文字)
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawCentreString(numStr, cx, cy - 24, 4);
        M5.Lcd.setTextSize(1);

        // 清除下方提示區 (避開 cy + size，從 y = 152 開始清除，絕不擦掉下框！)
        M5.Lcd.fillRect(0, 152, SCREEN_WIDTH, 26, TFT_BLACK);
        if (DIE_FACES[_dieTypeIdx] == 20 && !_isRolling) {
            if (_diceResults[0] == 20) {
                M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                M5.Lcd.drawCentreString("CRITICAL!", cx, 154, 2);
            } else if (_diceResults[0] == 1) {
                M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                M5.Lcd.drawCentreString("FUMBLE!", cx, 154, 2);
            }
        }
    } else {
        int startY = 38;
        int itemW = 56;
        int itemH = 34;

        for (uint8_t i = 0; i < _diceCount; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = (col == 0) ? 8 : (SCREEN_WIDTH - itemW - 8);
            int y = startY + row * (itemH + 6);

            M5.Lcd.fillRoundRect(x + 2, y + 2, itemW - 4, itemH - 4, 3, TFT_BLACK);
            M5.Lcd.drawRoundRect(x, y, itemW, itemH, 4, COLOR_GOLD);

            char numStr[8];
            snprintf(numStr, sizeof(numStr), "%d", _diceResults[i]);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Lcd.drawCentreString(numStr, x + itemW / 2, y + 8, 4);
        }
    }

    // 局部更新底部總計
    M5.Lcd.fillRect(0, 190, SCREEN_WIDTH, 30, TFT_BLACK);
    if (_diceCount > 1) {
        char totStr[20];
        snprintf(totStr, sizeof(totStr), "TOTAL: %d", total);
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString(totStr, SCREEN_WIDTH / 2, 192, 4);
    } else {
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("[SHAKE / PRESS]", SCREEN_WIDTH / 2, 196, 2);
    }
}
