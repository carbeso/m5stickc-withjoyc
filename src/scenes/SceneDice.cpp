/**
 * @file SceneDice.cpp
 * @brief 直式多面骰子盒實作：支援搖桿壓持漸入漸出、體感持續甩動與靜止定格動力學
 */

#include "scenes/SceneDice.h"
#include "EntropyManager.h"

const uint8_t DIE_FACES[] = {4, 6, 8, 10, 12, 20, 100};
const char* DIE_NAMES[] = {"d4", "d6", "d8", "d10", "d12", "d20", "d100"};
const uint8_t DIE_TYPE_COUNT = sizeof(DIE_FACES) / sizeof(DIE_FACES[0]);

SceneDice::SceneDice()
    : _dieTypeIdx(1), _diceCount(1),
      _isRolling(false), _triggeredByJoy(false), _rollStartTime(0), _lastTickTime(0), _needsRedraw(true) {
    for (int i = 0; i < 6; i++) _diceResults[i] = 1;
}

void SceneDice::init() {
    _needsRedraw = true;
    _isRolling = false;
    _triggeredByJoy = false;
    _nextScene = SCENE_COUNT;
    for (int i = 0; i < 6; i++) {
        _diceResults[i] = EntropyManager::random(1, DIE_FACES[_dieTypeIdx] + 1);
    }
}

void SceneDice::rollDice(bool byJoy, AudioManager& audio, LedManager& led) {
    _isRolling = true;
    _triggeredByJoy = byJoy;
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
        // 設定微調 (無推持時)
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

        // 啟動擲骰：由 Button A、搖桿中心鍵或甩動觸發（取消下拉搖桿，避免與減少顆數衝突）
        if (input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            rollDice(true, audio, led);
        }
    } else {
        // 滾動進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _rollStartTime;

        // 翻滾節奏：前 2 秒極速翻滾 (40ms)，後 0.8 秒自然減速 (45~110ms)，總長約 3 秒！
        uint32_t interval = (elapsed < 2000) ? 40 : map(elapsed, 2000, 2800, 45, 110);

        if (now - _lastTickTime > interval) {
            _lastTickTime = now;
            for (uint8_t i = 0; i < _diceCount; i++) {
                _diceResults[i] = EntropyManager::random(1, DIE_FACES[_dieTypeIdx] + 1);
            }
            audio.playDiceRoll();
            _needsRedraw = true;
        }

        // 滿 2800ms (約 3 秒) 停定定格，跟搖晃一樣，無需長拉長按！
        if (elapsed >= 2800) {
            _isRolling = false;
            for (uint8_t i = 0; i < _diceCount; i++) {
                _diceResults[i] = EntropyManager::random(1, DIE_FACES[_dieTypeIdx] + 1);
            }

            if (DIE_FACES[_dieTypeIdx] == 20 && _diceCount == 1) {
                if (_diceResults[0] == 20) {
                    audio.playCrit();
                    led.flash(0, 255, 255, 5, 50);
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

    // 方案 A：使用全域雙緩衝畫布離線繪製，杜絕多面骰滾動閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 頂部狀態列
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x2124);
    g_canvas.setTextColor(COLOR_GOLD, 0x2124);
    g_canvas.drawString("DICE", 8, 5, 2);

    char specStr[10];
    snprintf(specStr, sizeof(specStr), "%dd%d", _diceCount, DIE_FACES[_dieTypeIdx]);
    g_canvas.setTextColor(TFT_WHITE, 0x2124);
    g_canvas.drawRightString(specStr, SCREEN_WIDTH - 8, 6, 2);

    int total = 0;
    for (uint8_t i = 0; i < _diceCount; i++) total += _diceResults[i];

    // 清空動態骰子區 (Y: 28 ~ 184)
    g_canvas.fillRect(0, 28, SCREEN_WIDTH, 156, TFT_BLACK);

    if (_diceCount == 1) {
        int cx = SCREEN_WIDTH / 2;
        int cy = 98;
        int size = 40;

        g_canvas.drawRoundRect(cx - size, cy - size, size * 2, size * 2, 8, COLOR_GOLD);
        g_canvas.fillRoundRect(cx - size + 2, cy - size + 2, size * 2 - 4, size * 2 - 4, 6, TFT_BLACK);

        char numStr[8];
        snprintf(numStr, sizeof(numStr), "%d", _diceResults[0]);
        g_canvas.setTextColor(TFT_WHITE);
        g_canvas.setTextSize(2);
        g_canvas.drawCentreString(numStr, cx, cy - 22, 4);
        g_canvas.setTextSize(1);

        if (DIE_FACES[_dieTypeIdx] == 20 && !_isRolling) {
            if (_diceResults[0] == 20) {
                g_canvas.setTextColor(TFT_GREEN);
                g_canvas.drawCentreString("CRITICAL!", cx, 152, 2);
            } else if (_diceResults[0] == 1) {
                g_canvas.setTextColor(TFT_RED);
                g_canvas.drawCentreString("FUMBLE!", cx, 152, 2);
            }
        }
    } else {
        int startY = 32;
        int itemW = 56;
        int itemH = 38;

        for (uint8_t i = 0; i < _diceCount; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = (col == 0) ? 8 : (SCREEN_WIDTH - itemW - 8);
            int y = startY + row * (itemH + 6);

            g_canvas.fillRoundRect(x, y, itemW, itemH, 4, TFT_BLACK);
            g_canvas.drawRoundRect(x, y, itemW, itemH, 4, COLOR_GOLD);

            char numStr[8];
            snprintf(numStr, sizeof(numStr), "%d", _diceResults[i]);
            g_canvas.setTextColor(TFT_WHITE);
            g_canvas.drawCentreString(numStr, x + itemW / 2, y + 6, 4);
        }
    }

    // 底部總計列與操作指引
    g_canvas.fillRect(0, 186, SCREEN_WIDTH, 54, TFT_BLACK);
    g_canvas.drawFastHLine(6, 186, SCREEN_WIDTH - 12, 0x4208);
    if (_diceCount > 1) {
        char totStr[20];
        snprintf(totStr, sizeof(totStr), "TOTAL: %d", total);
        g_canvas.setTextColor(COLOR_GOLD);
        g_canvas.drawCentreString(totStr, SCREEN_WIDTH / 2, 192, 4);
    } else {
        g_canvas.setTextColor(TFT_LIGHTGREY);
        g_canvas.drawCentreString(_isRolling ? "ROLLING..." : "HOLD / SHAKE", SCREEN_WIDTH / 2, 196, 2);
    }

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Joy L/R:d#  U/D:cnt", SCREEN_WIDTH / 2, 224, 1);

    // 一次性推送畫面至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
