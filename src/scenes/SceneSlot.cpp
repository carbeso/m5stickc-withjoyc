/**
 * @file SceneSlot.cpp
 * @brief 3x3 搖桿下拉角子老虎機實作：拉桿拉持持續旋轉、放開依序煞車、持續甩動動力學
 */

#include "scenes/SceneSlot.h"

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
            _grid[c][r] = random(0, SYM_COUNT);
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
            _grid[c][r] = random(0, SYM_COUNT);
        }
    }
    M5.Lcd.fillScreen(TFT_BLACK);
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
    bool stillHolding = (input.isJoyPulledDown || input.isJoyBtnHeld || input.isBtnAHeld);

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
                    _grid[c][r] = random(0, SYM_COUNT);
                }
            }
        }
        _needsRedraw = true;

        // 煞車時序控制：
        // 1. 只要玩家持續拉著搖桿或持續甩動，且未達 15 秒安全超時，三輪維持全速飛轉！
        bool isHoldingOrShaking = (stillHolding || input.isActivelyShaking);

        if (isHoldingOrShaking && elapsed < 15000) {
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

            // 依序煞車：必須同時滿足「最短轉動時間」與「放開後間隔」，確保有一兩秒以上扎實體驗
            // 第 1 輪煞停：放開後滿 350ms 且總時間滿 1400ms
            if (_colSpinning[0] && timeSinceRelease > 350 && elapsed > 1400) {
                _colSpinning[0] = false;
                audio.playClick();
            }
            // 第 2 輪煞停：第 1 輪停後且放開後滿 750ms，總時間滿 1800ms
            if (_colSpinning[1] && !_colSpinning[0] && timeSinceRelease > 750 && elapsed > 1800) {
                _colSpinning[1] = false;
                audio.playClick();
            }
            // 第 3 輪煞停：第 2 輪停後且放開後滿 1200ms，總時間滿 2250ms -> 開獎！
            if (_colSpinning[2] && !_colSpinning[1] && timeSinceRelease > 1200 && elapsed > 2250) {
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

    M5.Lcd.fillRoundRect(x, y, CELL_W, CELL_H, 4, bg);
    M5.Lcd.drawRoundRect(x, y, CELL_W, CELL_H, 4, border);

    switch (sym) {
        case SYM_SEVEN:
            M5.Lcd.setTextColor(highlight ? TFT_BLACK : TFT_RED, bg);
            M5.Lcd.drawCentreString("7", cx, cy - 14, 4);
            break;

        case SYM_BAR:
            M5.Lcd.drawRoundRect(cx - 14, cy - 8, 28, 16, 2, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.setTextColor(highlight ? TFT_BLACK : TFT_WHITE, bg);
            M5.Lcd.drawCentreString("BAR", cx, cy - 5, 1);
            break;

        case SYM_BELL:
            M5.Lcd.fillCircle(cx, cy - 6, 4, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.fillTriangle(cx - 9, cy + 6, cx + 9, cy + 6, cx, cy - 6, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.fillCircle(cx, cy + 8, 3, highlight ? TFT_BLACK : COLOR_GOLD);
            break;

        case SYM_CHERRY:
            M5.Lcd.fillCircle(cx - 5, cy + 5, 5, TFT_RED);
            M5.Lcd.fillCircle(cx + 6, cy + 3, 5, TFT_RED);
            M5.Lcd.drawLine(cx - 5, cy + 1, cx, cy - 8, TFT_GREEN);
            M5.Lcd.drawLine(cx + 6, cy - 1, cx, cy - 8, TFT_GREEN);
            break;

        case SYM_LEMON:
            M5.Lcd.fillCircle(cx, cy, 8, TFT_YELLOW);
            M5.Lcd.drawPixel(cx - 9, cy, TFT_GREEN);
            M5.Lcd.drawPixel(cx + 9, cy, TFT_GREEN);
            break;

        case SYM_STAR:
            M5.Lcd.fillCircle(cx, cy, 6, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx, cy - 10, cx - 4, cy - 2, cx + 4, cy - 2, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx - 10, cy, cx - 2, cy - 4, cx - 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx + 10, cy, cx + 2, cy - 4, cx + 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            break;

        case SYM_CLOVER:
            M5.Lcd.fillCircle(cx - 4, cy - 4, 4, TFT_GREEN);
            M5.Lcd.fillCircle(cx + 4, cy - 4, 4, TFT_GREEN);
            M5.Lcd.fillCircle(cx - 4, cy + 4, 4, TFT_GREEN);
            M5.Lcd.fillCircle(cx + 4, cy + 4, 4, TFT_GREEN);
            M5.Lcd.drawLine(cx, cy, cx, cy + 10, TFT_GREEN);
            break;
    }
}

void SceneSlot::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 頂部狀態列
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawString("SLOT 3x3", 8, 5, 2);

    if (_hasWon) {
        uint8_t winCount = 0;
        for (int i = 0; i < 8; i++) {
            if (_winLinesMask & (1 << i)) winCount++;
        }
        char winLineStr[16];
        snprintf(winLineStr, sizeof(winLineStr), "%d %s!", winCount, (winCount > 1) ? "LINES" : "LINE");
        M5.Lcd.setTextColor(TFT_GREEN, 0x18C3);
        M5.Lcd.drawRightString(winLineStr, SCREEN_WIDTH - 8, 6, 2);
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
    M5.Lcd.fillRect(0, 172, SCREEN_WIDTH, 68, TFT_BLACK);
    M5.Lcd.drawFastHLine(8, 172, SCREEN_WIDTH - 16, 0x39E7);

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);
    if (anySpinning) {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 184, 2);
    } else if (_hasWon) {
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("*** JACKPOT! ***", SCREEN_WIDTH / 2, 182, 2);
    } else {
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("PULL JOY DOWN", SCREEN_WIDTH / 2, 184, 2);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("Hold to Spin, Release", SCREEN_WIDTH / 2, 204, 1);
    }

    M5.Lcd.setTextColor(0x52AA, TFT_BLACK);
    M5.Lcd.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 224, 1);
}
