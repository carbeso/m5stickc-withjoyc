/**
 * @file SceneSlot.cpp
 * @brief 3x3 搖桿下拉角子老虎機實作：完整 7 款圖案幾何繪製、中獎局部無閃爍高亮
 */

#include "scenes/SceneSlot.h"

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

    audio.playDiceRoll();
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
        led.flash(255, 215, 0, 8, 60);
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

    // 搖桿向下拉桿 (joyY > 0)、按鍵 A、中心鍵或晃動
    if (!anySpinning && (input.joyPulledDown || input.btnAPressed || input.joyBtnPressed || input.isShaken)) {
        pullLever(audio, led);
    }

    if (anySpinning) {
        uint32_t elapsed = millis() - _spinStartTime;

        if (millis() - _lastTickTime > 40) {
            _lastTickTime = millis();
            audio.playTick();
        }

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
            checkWinLines(audio, led);
        }

        for (int c = 0; c < 3; c++) {
            if (_colSpinning[c]) {
                for (int r = 0; r < 3; r++) {
                    _grid[c][r] = random(0, SYM_COUNT);
                }
            }
        }
        _needsRedraw = true;
    }

    // 中獎閃爍：局部更新
    if (_hasWon) {
        _flashTimer++;
        if (_flashTimer % 8 == 0) {
            _needsRedraw = true;
        }
    }
}

/**
 * @brief 繪製老虎機 7 種專屬生動圖案 (Icon)
 */
void SceneSlot::drawSymbol(int x, int y, uint8_t sym, bool highlight) {
    uint16_t bg = highlight ? COLOR_GOLD : 0x18C3;
    uint16_t border = highlight ? TFT_WHITE : 0x39E7;
    int cx = x + CELL_W / 2;
    int cy = y + CELL_H / 2;

    M5.Lcd.fillRoundRect(x, y, CELL_W, CELL_H, 4, bg);
    M5.Lcd.drawRoundRect(x, y, CELL_W, CELL_H, 4, border);

    switch (sym) {
        case SYM_SEVEN: // 7️⃣ Lucky 7 (鮮紅大 7)
            M5.Lcd.setTextColor(highlight ? TFT_BLACK : TFT_RED, bg);
            M5.Lcd.drawCentreString("7", cx, cy - 14, 4);
            break;

        case SYM_BAR: // 金磚 BAR
            M5.Lcd.drawRoundRect(cx - 14, cy - 8, 28, 16, 2, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.setTextColor(highlight ? TFT_BLACK : TFT_WHITE, bg);
            M5.Lcd.drawCentreString("BAR", cx, cy - 5, 1);
            break;

        case SYM_BELL: // 🔔 金鈴 (金黃鐘形)
            M5.Lcd.fillCircle(cx, cy - 6, 4, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.fillTriangle(cx - 9, cy + 6, cx + 9, cy + 6, cx, cy - 6, highlight ? TFT_BLACK : COLOR_GOLD);
            M5.Lcd.fillCircle(cx, cy + 8, 3, highlight ? TFT_BLACK : COLOR_GOLD);
            break;

        case SYM_CHERRY: // 🍒 雙櫻桃 (紅圓 + 綠枝)
            M5.Lcd.fillCircle(cx - 5, cy + 5, 5, TFT_RED);
            M5.Lcd.fillCircle(cx + 6, cy + 3, 5, TFT_RED);
            M5.Lcd.drawLine(cx - 5, cy + 1, cx, cy - 8, TFT_GREEN);
            M5.Lcd.drawLine(cx + 6, cy - 1, cx, cy - 8, TFT_GREEN);
            break;

        case SYM_LEMON: // 🍋 檸檬 (鮮黃橢圓)
            M5.Lcd.fillCircle(cx, cy, 8, TFT_YELLOW);
            M5.Lcd.drawPixel(cx - 9, cy, TFT_GREEN);
            M5.Lcd.drawPixel(cx + 9, cy, TFT_GREEN);
            break;

        case SYM_STAR: // ⭐ 金星 (五角金星)
            M5.Lcd.fillCircle(cx, cy, 6, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx, cy - 10, cx - 4, cy - 2, cx + 4, cy - 2, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx - 10, cy, cx - 2, cy - 4, cx - 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            M5.Lcd.fillTriangle(cx + 10, cy, cx + 2, cy - 4, cx + 2, cy + 4, highlight ? TFT_BLACK : COLOR_CYAN);
            break;

        case SYM_CLOVER: // 🍀 幸運草 (四葉綠圓)
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

    // 局部重繪：頂部狀態列
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 28, 0x18C3);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawString("SLOT 3x3", 8, 6, 2);

    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr), "$%d", _score);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawRightString(scoreStr, SCREEN_WIDTH - 8, 6, 2);

    // 繪製 3x3 九宮格單元 (局部刷新，完全不閃爍)
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

    // 底部狀態 (局部清空重繪)
    M5.Lcd.fillRect(0, 172, SCREEN_WIDTH, 68, TFT_BLACK);
    M5.Lcd.drawFastHLine(8, 172, SCREEN_WIDTH - 16, 0x39E7);

    bool anySpinning = (_colSpinning[0] || _colSpinning[1] || _colSpinning[2]);
    if (anySpinning) {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("SPINNING...", SCREEN_WIDTH / 2, 184, 2);
    } else if (_hasWon) {
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.drawCentreString("*** WINNER! ***", SCREEN_WIDTH / 2, 180, 2);
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
