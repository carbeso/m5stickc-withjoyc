/**
 * @file SceneSlot.cpp
 * @brief 3x3 搖桿下拉角子老虎機實作：支援真實拉桿手勢、依序煞車、8條連線判定與 Jackpot 和弦
 */

#include "scenes/SceneSlot.h"

const char* SYM_NAMES[] = {"7", "BAR", "BEL", "CHY", "LEM", "STR", "CLV"};
const uint16_t SYM_COLORS[] = {
    TFT_RED,        // 7
    COLOR_GOLD,     // BAR
    COLOR_GOLD,     // BELL
    TFT_MAGENTA,    // CHERRY
    TFT_YELLOW,     // LEMON
    COLOR_CYAN,     // STAR
    TFT_GREEN       // CLOVER
};

const int CELL_W = 36;
const int CELL_H = 40;
const int START_X = 9;
const int START_Y = 36;
const int GAP_X = 4;
const int GAP_Y = 4;

SceneSlot::SceneSlot()
    : _spinStartTime(0), _hasWon(false), _winLinesMask(0),
      _score(100), _needsRedraw(true), _lastTickTime(0), _flashTimer(0) {
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
    _hasWon = false;
    _winLinesMask = 0;

    audio.playDiceRoll(); // 機械拉動聲
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneSlot::checkWinLines(AudioManager& audio, LedManager& led) {
    _winLinesMask = 0;

    // 1. 檢查 3 橫線
    for (int r = 0; r < 3; r++) {
        if (_grid[0][r] == _grid[1][r] && _grid[1][r] == _grid[2][r]) {
            _winLinesMask |= (1 << r);
        }
    }

    // 2. 檢查 3 直線
    for (int c = 0; c < 3; c++) {
        if (_grid[c][0] == _grid[c][1] && _grid[c][1] == _grid[c][2]) {
            _winLinesMask |= (1 << (3 + c));
        }
    }

    // 3. 檢查 2 斜對角線
    if (_grid[0][0] == _grid[1][1] && _grid[1][1] == _grid[2][2]) {
        _winLinesMask |= (1 << 6);
    }
    if (_grid[0][2] == _grid[1][1] && _grid[1][1] == _grid[2][0]) {
        _winLinesMask |= (1 << 7);
    }

    if (_winLinesMask > 0) {
        _hasWon = true;
        _score += 50;
        audio.playJackpot();
        led.flash(255, 215, 0, 8, 60); // 金光爆閃
    } else {
        _hasWon = false;
        audio.playClick();
        led.setColor(0, 100, 200);
    }
}

void SceneSlot::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);

    // 搖桿向下拉桿、按鍵 A 或中心鍵觸發拉霸
    if (!anySpinning && (input.joyPulledDown || input.btnAPressed || input.joyBtnPressed || input.isShaken)) {
        pullLever(audio, led);
    }

    if (anySpinning) {
        uint32_t elapsed = millis() - _spinStartTime;

        // 齒輪聲
        if (millis() - _lastTickTime > 40) {
            _lastTickTime = millis();
            audio.playTick();
        }

        // 依序煞車時序
        if (elapsed > 600 && _colSpinning[0]) {
            _colSpinning[0] = false;
            audio.playClick();
        }
        if (elapsed > 1000 && _colSpinning[1]) {
            _colSpinning[1] = false;
            audio.playClick();
        }
        if (elapsed > 1400 && _colSpinning[2]) {
            _colSpinning[2] = false;
            // 三輪皆停定，進行中獎核算
            checkWinLines(audio, led);
        }

        // 輪盤高速滾動置換
        for (int c = 0; c < 3; c++) {
            if (_colSpinning[c]) {
                for (int r = 0; r < 3; r++) {
                    _grid[c][r] = random(0, SYM_COUNT);
                }
            }
        }
        _needsRedraw = true;
    }

    // 中獎閃爍效果
    if (_hasWon) {
        _flashTimer++;
        if (_flashTimer % 6 == 0) {
            _needsRedraw = true;
        }
    }
}

void SceneSlot::drawSymbol(int x, int y, uint8_t sym, bool highlight) {
    uint16_t bg = highlight ? COLOR_GOLD : 0x18C3;
    uint16_t border = highlight ? TFT_WHITE : 0x39E7;
    uint16_t textColor = highlight ? TFT_BLACK : SYM_COLORS[sym];

    M5.Lcd.fillRoundRect(x, y, CELL_W, CELL_H, 4, bg);
    M5.Lcd.drawRoundRect(x, y, CELL_W, CELL_H, 4, border);

    M5.Lcd.setTextColor(textColor, bg);
    if (sym == SYM_SEVEN) {
        M5.Lcd.drawCentreString("7", x + CELL_W / 2, y + 6, 4);
    } else {
        M5.Lcd.drawCentreString(SYM_NAMES[sym], x + CELL_W / 2, y + 12, 2);
    }
}

void SceneSlot::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 28)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawString("SLOT 3x3", 8, 5, 2);

    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr), "$%d", _score);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawRightString(scoreStr, SCREEN_WIDTH - 8, 5, 2);

    // 2. 繪製 3x3 九宮格
    bool flashState = (_flashTimer / 6) % 2 == 0;
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 3; r++) {
            int x = START_X + c * (CELL_W + GAP_X);
            int y = START_Y + r * (CELL_H + GAP_Y);

            // 判斷此格是否屬於中獎線
            bool highlight = false;
            if (_hasWon && flashState) {
                if (_winLinesMask & (1 << r)) highlight = true;          // 橫線
                if (_winLinesMask & (1 << (3 + c))) highlight = true;    // 直線
                if ((_winLinesMask & (1 << 6)) && (c == r)) highlight = true; // 主對角
                if ((_winLinesMask & (1 << 7)) && (c + r == 2)) highlight = true; // 副對角
            }

            drawSymbol(x, y, _grid[c][r], highlight);
        }
    }

    // 3. 底部狀態與操作 (Y: 172 ~ 238)
    M5.Lcd.drawFastHLine(8, 174, SCREEN_WIDTH - 16, 0x39E7);

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);
    if (anySpinning) {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 184, 2);
    } else if (_hasWon) {
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.drawCentreString("*** WINNER! ***", SCREEN_WIDTH / 2, 182, 2);
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("+50 COINS!", SCREEN_WIDTH / 2, 202, 2);
    } else {
        M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
        M5.Lcd.drawCentreString("PULL JOY DOWN", SCREEN_WIDTH / 2, 184, 2);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.drawCentreString("to Spin 3x3 Slots", SCREEN_WIDTH / 2, 204, 1);
    }

    M5.Lcd.setTextColor(0x52AA, TFT_BLACK);
    M5.Lcd.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 224, 1);
}
