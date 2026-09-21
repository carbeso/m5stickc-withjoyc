/**
 * @file SceneSlot.cpp
 * @brief 3x3 搖桿下拉角子老虎機實作：拉桿拉持持續旋轉、放開依序煞車、持續甩動動力學
 */

#include "scenes/SceneSlot.h"
#include "EntropyManager.h"

const int CELL_W = 36;
const int CELL_H = 40;
const int START_X = 9;
const int START_Y = 34;
const int GAP_X = 4;
const int GAP_Y = 4;

SceneSlot::SceneSlot()
    : _spinStartTime(0), _hasWon(false), _winLinesMask(0),
      _releaseTime(0), _needsRedraw(true), _lastTickTime(0), _flashTimer(0) {
    for (int c = 0; c < 3; c++) {
        _colSpinning[c] = false;
        _colOffset[c] = 0;
        for (int r = 0; r < 3; r++) {
            _grid[c][r] = EntropyManager::random(0, SYM_COUNT);
        }
    }
}

void SceneSlot::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _hasWon = false;
    _winLinesMask = 0;
    _releaseTime = 0;
    for (int c = 0; c < 3; c++) {
        _colSpinning[c] = false;
        for (int r = 0; r < 3; r++) {
            _grid[c][r] = EntropyManager::random(0, SYM_COUNT);
        }
    }
}

void SceneSlot::pullLever(AudioManager& audio, LedManager& led) {
    for (int c = 0; c < 3; c++) _colSpinning[c] = true;
    _spinStartTime = millis();
    _releaseTime = 0;
    _hasWon = false;
    _winLinesMask = 0;

    audio.playDiceRoll();
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneSlot::checkWinLines(AudioManager& audio, LedManager& led) {
    _winLinesMask = 0;

    for (int r = 0; r < 3; r++) {
        if (_grid[0][r] == _grid[1][r] && _grid[1][r] == _grid[2][r]) {
            _winLinesMask |= (1 << r);
        }
    }

    for (int c = 0; c < 3; c++) {
        if (_grid[c][0] == _grid[c][1] && _grid[c][1] == _grid[c][2]) {
            _winLinesMask |= (1 << (3 + c));
        }
    }

    if (_grid[0][0] == _grid[1][1] && _grid[1][1] == _grid[2][2]) {
        _winLinesMask |= (1 << 6);
    }
    if (_grid[0][2] == _grid[1][1] && _grid[1][1] == _grid[2][0]) {
        _winLinesMask |= (1 << 7);
    }

    if (_winLinesMask > 0) {
        _hasWon = true;
        audio.playJackpot();
        led.flash(255, 180, 0, 8, 60);
    } else {
        _hasWon = false;
        audio.playClick();
        led.setColor(0, 100, 200);
    }
}

void SceneSlot::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);
    // 持拉判定：明確以搖桿下拉 (joyY > 35) 或持續按著 Button A 判定，杜絕中心鍵假性死鎖
    bool isPullingLever = (input.joyY > 35 || input.isBtnAHeld || input.isActivelyShaking);

    if (!anySpinning) {
        // 啟動拉桿：必須是明確按鍵、下拉或甩動脈衝觸發，絕不因常態推持誤觸
        if (input.joyPulledDown || input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            pullLever(audio, led);
        }
    } else {
        uint32_t now = millis();
        uint32_t elapsed = now - _spinStartTime;

        if (now - _lastTickTime > 40) {
            _lastTickTime = now;
            audio.playTick();
        }

        // 滾輪更新
        for (int c = 0; c < 3; c++) {
            if (_colSpinning[c]) {
                for (int r = 0; r < 3; r++) {
                    _grid[c][r] = EntropyManager::random(0, SYM_COUNT);
                }
            }
        }
        _needsRedraw = true;

        // 煞車時序控制：
        // 1. 只要玩家持續向下拉著搖桿 (joyY > 35) 或甩動，且未達 30 秒安全超時，三輪維持全速飛轉！
        if (isPullingLever && elapsed < 30000) {
            _releaseTime = 0; // 重置放開計時
            _colSpinning[0] = true;
            _colSpinning[1] = true;
            _colSpinning[2] = true;
        } else {
            // 玩家已放開拉桿 (或達到超時)
            if (_releaseTime == 0) {
                _releaseTime = now;
            }

            uint32_t timeSinceRelease = now - _releaseTime;

            // 依序煞車：保證至少旋轉滿 1600ms，放開後依序煞停，總時長約 3 秒！
            // 第 1 輪煞停：放開後滿 400ms 且總時間滿 1600ms
            if (_colSpinning[0] && timeSinceRelease > 400 && elapsed > 1600) {
                _colSpinning[0] = false;
                audio.playClick();
            }
            // 第 2 輪煞停：第 1 輪停後且放開後滿 850ms，總時間滿 2050ms
            if (_colSpinning[1] && !_colSpinning[0] && timeSinceRelease > 850 && elapsed > 2050) {
                _colSpinning[1] = false;
                audio.playClick();
            }
            // 第 3 輪煞停：第 2 輪停後且放開後滿 1350ms，總時間滿 2550ms -> 開獎！
            if (_colSpinning[2] && !_colSpinning[1] && timeSinceRelease > 1350 && elapsed > 2550) {
                _colSpinning[2] = false;
                _releaseTime = 0;
                checkWinLines(audio, led);
            }
        }
    }

    if (_hasWon) {
        _flashTimer++;
        if (_flashTimer % 8 == 0) {
            _needsRedraw = true;
        }
    }
}

void SceneSlot::drawSymbol(int x, int y, uint8_t sym, bool highlight) {
    uint16_t bg = highlight ? COLOR_GOLD : 0x18C3;
    uint16_t border = highlight ? TFT_WHITE : 0x39E7;
    int cx = x + CELL_W / 2;
    int cy = y + CELL_H / 2;

    g_canvas.fillRoundRect(x, y, CELL_W, CELL_H, 4, bg);
    g_canvas.drawRoundRect(x, y, CELL_W, CELL_H, 4, border);

    switch (sym) {
        case SYM_SEVEN:
            g_canvas.setTextColor(highlight ? TFT_BLACK : TFT_RED, bg);
            g_canvas.drawCentreString("7", cx, cy - 14, 4);
            break;

        case SYM_BAR:
            g_canvas.drawRoundRect(cx - 14, cy - 8, 28, 16, 2, highlight ? TFT_BLACK : COLOR_GOLD);
            g_canvas.setTextColor(highlight ? TFT_BLACK : TFT_WHITE, bg);
            g_canvas.drawCentreString("BAR", cx, cy - 5, 1);
            break;

        case SYM_BELL:
            g_canvas.fillCircle(cx, cy - 6, 4, highlight ? TFT_BLACK : COLOR_GOLD);
            g_canvas.fillTriangle(cx - 9, cy + 6, cx + 9, cy + 6, cx, cy - 6, highlight ? TFT_BLACK : COLOR_GOLD);
            g_canvas.fillCircle(cx, cy + 8, 3, highlight ? TFT_BLACK : COLOR_GOLD);
            break;

        case SYM_CHERRY:
            g_canvas.fillCircle(cx - 5, cy + 5, 5, TFT_RED);
            g_canvas.fillCircle(cx + 6, cy + 3, 5, TFT_RED);
            g_canvas.drawLine(cx - 5, cy + 1, cx, cy - 8, TFT_GREEN);
            g_canvas.drawLine(cx + 6, cy - 1, cx, cy - 8, TFT_GREEN);
            break;

        case SYM_LEMON:
            g_canvas.fillCircle(cx, cy, 8, TFT_YELLOW);
            g_canvas.drawPixel(cx - 9, cy, TFT_GREEN);
            g_canvas.drawPixel(cx + 9, cy, TFT_GREEN);
            break;

        case SYM_STAR:
            g_canvas.fillCircle(cx, cy, 6, highlight ? TFT_BLACK : COLOR_CYAN);
            g_canvas.fillTriangle(cx, cy - 10, cx - 4, cy - 2, cx + 4, cy - 2, highlight ? TFT_BLACK : COLOR_CYAN);
            g_canvas.fillTriangle(cx - 10, cy, cx - 2, cy - 4, cx - 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            g_canvas.fillTriangle(cx + 10, cy, cx + 2, cy - 4, cx + 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            break;

        case SYM_CLOVER:
            g_canvas.fillCircle(cx - 4, cy - 4, 4, TFT_GREEN);
            g_canvas.fillCircle(cx + 4, cy - 4, 4, TFT_GREEN);
            g_canvas.fillCircle(cx - 4, cy + 4, 4, TFT_GREEN);
            g_canvas.fillCircle(cx + 4, cy + 4, 4, TFT_GREEN);
            g_canvas.drawLine(cx, cy, cx, cy + 10, TFT_GREEN);
            break;
    }
}

void SceneSlot::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布離線繪圖，消除拉霸轉輪與底部文字閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 頂部狀態列
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("SLOT 3x3", 8, 5, 2);

    if (_hasWon) {
        uint8_t winCount = 0;
        for (int i = 0; i < 8; i++) {
            if (_winLinesMask & (1 << i)) winCount++;
        }
        char winLineStr[16];
        snprintf(winLineStr, sizeof(winLineStr), "%d %s!", winCount, (winCount > 1) ? "LINES" : "LINE");
        g_canvas.setTextColor(TFT_GREEN, 0x18C3);
        g_canvas.drawRightString(winLineStr, SCREEN_WIDTH - 8, 6, 2);
    }

    // 繪製 3x3 九宮格
    bool flashState = (_flashTimer / 8) % 2 == 0;
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 3; r++) {
            int x = START_X + c * (CELL_W + GAP_X);
            int y = START_Y + r * (CELL_H + GAP_Y);

            bool highlight = false;
            if (_hasWon && flashState) {
                if (_winLinesMask & (1 << r)) highlight = true;
                if (_winLinesMask & (1 << (3 + c))) highlight = true;
                if ((_winLinesMask & (1 << 6)) && (c == r)) highlight = true;
                if ((_winLinesMask & (1 << 7)) && (c + r == 2)) highlight = true;
            }

            drawSymbol(x, y, _grid[c][r], highlight);
        }
    }

    // 底部狀態
    g_canvas.fillRect(0, 172, SCREEN_WIDTH, 68, TFT_BLACK);
    g_canvas.drawFastHLine(8, 172, SCREEN_WIDTH - 16, 0x39E7);

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);
    if (anySpinning) {
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 184, 2);
    } else if (_hasWon) {
        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawCentreString("*** JACKPOT! ***", SCREEN_WIDTH / 2, 182, 2);
    } else {
        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawCentreString("PULL JOY DOWN", SCREEN_WIDTH / 2, 184, 2);
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("Hold to Spin, Release", SCREEN_WIDTH / 2, 204, 1);
    }

    g_canvas.setTextColor(0x52AA, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 224, 1);

    // 一次性推送畫面至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
