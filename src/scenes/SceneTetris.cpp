/**
 * @file SceneTetris.cpp
 * @brief 極簡掌上俄羅斯方塊實作：雙緩衝零閃爍、經典 7 款方塊、NEXT 預覽與消行
 */

#include "scenes/SceneTetris.h"

// 7 種經典方塊在 4 個旋轉角度下的 4 個格子相對座標 (dx, dy)
const int8_t PIECE_COORDS[7][4][4][2] = {
    // 0: I (青)
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}},
        {{0,2}, {1,2}, {2,2}, {3,2}},
        {{1,0}, {1,1}, {1,2}, {1,3}}
    },
    // 1: O (黃)
    {
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}}
    },
    // 2: T (紫)
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    // 3: S (綠)
    {
        {{1,0}, {2,0}, {0,1}, {1,1}},
        {{1,0}, {1,1}, {2,1}, {2,2}},
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{0,0}, {0,1}, {1,1}, {1,2}}
    },
    // 4: Z (紅)
    {
        {{0,0}, {1,0}, {1,1}, {2,1}},
        {{2,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{1,0}, {0,1}, {1,1}, {0,2}}
    },
    // 5: J (藍)
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    // 6: L (橙)
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    }
};

const uint16_t PIECE_COLORS[7] = {
    0x07FF, // I: 科技青
    0xFFE0, // O: 亮金黃
    0xA01F, // T: 霓虹紫
    0x07E0, // S: 亮翠綠
    0xF800, // Z: 鮮豔紅
    0x3BFF, // J: 深天藍
    0xFD20  // L: 晨曦橙
};

SceneTetris::SceneTetris()
    : _curX(3), _curY(0), _curShape(0), _curRot(0), _nextShape(0),
      _state(TETRIS_PLAYING), _linesCleared(0),
      _lastFallTime(0), _lastMoveTime(0), _lastRotateTime(0), _needsRedraw(true) {
    for (uint8_t x = 0; x < BOARD_W; x++) {
        for (uint8_t y = 0; y < BOARD_H; y++) {
            _board[x][y] = 0;
        }
    }
}

void SceneTetris::resetGame() {
    for (uint8_t x = 0; x < BOARD_W; x++) {
        for (uint8_t y = 0; y < BOARD_H; y++) {
            _board[x][y] = 0;
        }
    }
    _linesCleared = 0;
    _state = TETRIS_PLAYING;
    _lastFallTime = millis();
    _lastMoveTime = millis();
    _lastRotateTime = millis();

    _nextShape = (uint8_t)EntropyManager::random(0, 7);
    spawnPiece();
    _needsRedraw = true;
}

void SceneTetris::init() {
    _nextScene = SCENE_COUNT;
    resetGame();
}

void SceneTetris::spawnPiece() {
    _curShape = _nextShape;
    _nextShape = (uint8_t)EntropyManager::random(0, 7);
    _curRot = 0;
    _curX = 3;
    _curY = 0;

    // 出生點重疊判定遊戲結束
    if (checkCollision(_curX, _curY, _curShape, _curRot)) {
        _state = TETRIS_GAME_OVER;
    }
}

bool SceneTetris::checkCollision(int8_t px, int8_t py, uint8_t shape, uint8_t rot) {
    for (uint8_t i = 0; i < 4; i++) {
        int8_t bx = px + PIECE_COORDS[shape][rot][i][0];
        int8_t by = py + PIECE_COORDS[shape][rot][i][1];

        if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return true;
        if (by >= 0 && _board[bx][by] != 0) return true;
    }
    return false;
}

void SceneTetris::clearLines(AudioManager& audio, LedManager& led) {
    uint8_t clearedCount = 0;

    for (int8_t y = BOARD_H - 1; y >= 0; y--) {
        bool full = true;
        for (uint8_t x = 0; x < BOARD_W; x++) {
            if (_board[x][y] == 0) {
                full = false;
                break;
            }
        }

        if (full) {
            clearedCount++;
            // 將上方所有行向下平移
            for (int8_t ny = y; ny > 0; ny--) {
                for (uint8_t x = 0; x < BOARD_W; x++) {
                    _board[x][ny] = _board[x][ny - 1];
                }
            }
            for (uint8_t x = 0; x < BOARD_W; x++) {
                _board[x][0] = 0;
            }
            y++; // 重新檢查當前行（已被上方填入）
        }
    }

    if (clearedCount > 0) {
        _linesCleared += clearedCount;
        audio.playClick();
        led.setColor(50, 255, 50); // 消行亮綠燈
    }
}

void SceneTetris::lockPiece(AudioManager& audio, LedManager& led) {
    for (uint8_t i = 0; i < 4; i++) {
        int8_t bx = _curX + PIECE_COORDS[_curShape][_curRot][i][0];
        int8_t by = _curY + PIECE_COORDS[_curShape][_curRot][i][1];
        if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
            _board[bx][by] = _curShape + 1;
        }
    }

    audio.playTick();
    clearLines(audio, led);
    spawnPiece();
    _needsRedraw = true;
}

void SceneTetris::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：退出返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 遊戲結束狀態
    if (_state == TETRIS_GAME_OVER) {
        led.setColor(200, 30, 30);
        if (input.joyBtnPressed || input.btnAPressed) {
            audio.playClick();
            resetGame();
        }
        return;
    }

    uint32_t now = millis();

    // 3. 左右平移 (搖桿微動與防抖 140ms)
    if (now - _lastMoveTime > 140) {
        if (input.joyX < -35) {
            if (!checkCollision(_curX - 1, _curY, _curShape, _curRot)) {
                _curX--;
                audio.playTick();
                _needsRedraw = true;
            }
            _lastMoveTime = now;
        } else if (input.joyX > 35) {
            if (!checkCollision(_curX + 1, _curY, _curShape, _curRot)) {
                _curX++;
                audio.playTick();
                _needsRedraw = true;
            }
            _lastMoveTime = now;
        }
    }

    // 4. 順時針旋轉 (點擊搖桿中鍵或 Button A，附 180ms 防抖)
    if (input.btnAPressed || input.joyBtnPressed) {
        if (now - _lastRotateTime > 180) {
            uint8_t nextRot = (_curRot + 1) % 4;
            // 基礎踢牆 (Wall kick) 嘗試
            if (!checkCollision(_curX, _curY, _curShape, nextRot)) {
                _curRot = nextRot;
                audio.playClick();
                _needsRedraw = true;
            } else if (!checkCollision(_curX - 1, _curY, _curShape, nextRot)) {
                _curX--;
                _curRot = nextRot;
                audio.playClick();
                _needsRedraw = true;
            } else if (!checkCollision(_curX + 1, _curY, _curShape, nextRot)) {
                _curX++;
                _curRot = nextRot;
                audio.playClick();
                _needsRedraw = true;
            }
            _lastRotateTime = now;
        }
    }

    // 5. Button B 短按：瞬間落底 (Hard Drop)
    if (input.btnBPressed) {
        while (!checkCollision(_curX, _curY + 1, _curShape, _curRot)) {
            _curY++;
        }
        lockPiece(audio, led);
        return;
    }

    // 6. 自然下落與搖桿下推軟降 (Soft Drop)
    bool isSoftDrop = (input.joyY < -40); // 搖桿向下推
    uint32_t fallInterval = isSoftDrop ? 70 : 650; // 軟降 70ms，正常舒緩 650ms (固定不加速)

    if (now - _lastFallTime >= fallInterval) {
        _lastFallTime = now;
        if (!checkCollision(_curX, _curY + 1, _curShape, _curRot)) {
            _curY++;
            _needsRedraw = true;
        } else {
            lockPiece(audio, led);
        }
    }
}

void SceneTetris::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：在全域單例雙緩衝畫布離線繪製
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 24)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 24, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("TETRIS", 6, 4, 2);

    // 2. 棋盤邊框 (X: 7 ~ 88, Y: 31 ~ 192)
    g_canvas.drawRect(BOARD_X - 1, BOARD_Y - 1, BOARD_W * CELL_SIZE + 2, BOARD_H * CELL_SIZE + 2, 0x39E7);

    // 3. 繪製棋盤既有固定方塊
    for (uint8_t x = 0; x < BOARD_W; x++) {
        for (uint8_t y = 0; y < BOARD_H; y++) {
            uint8_t type = _board[x][y];
            if (type != 0) {
                uint16_t color = PIECE_COLORS[type - 1];
                int px = BOARD_X + x * CELL_SIZE;
                int py = BOARD_Y + y * CELL_SIZE;
                g_canvas.fillRect(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, color);
                g_canvas.drawPixel(px + 1, py + 1, TFT_WHITE); // 1px 高光立體效果
            }
        }
    }

    // 4. 繪製當前下落中的活動方塊
    if (_state == TETRIS_PLAYING) {
        uint16_t curColor = PIECE_COLORS[_curShape];
        for (uint8_t i = 0; i < 4; i++) {
            int8_t bx = _curX + PIECE_COORDS[_curShape][_curRot][i][0];
            int8_t by = _curY + PIECE_COORDS[_curShape][_curRot][i][1];
            if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
                int px = BOARD_X + bx * CELL_SIZE;
                int py = BOARD_Y + by * CELL_SIZE;
                g_canvas.fillRect(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, curColor);
                g_canvas.drawPixel(px + 1, py + 1, TFT_WHITE);
            }
        }
    }

    // 5. 右側資訊欄 (X: 92 ~ 130)
    // NEXT 預覽小窗
    g_canvas.drawRect(93, 31, 38, 38, 0x2965);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("NEXT", 112, 34, 1);

    uint16_t nextColor = PIECE_COLORS[_nextShape];
    for (uint8_t i = 0; i < 4; i++) {
        int8_t nx = PIECE_COORDS[_nextShape][0][i][0];
        int8_t ny = PIECE_COORDS[_nextShape][0][i][1];
        int px = 97 + nx * 6;
        int py = 46 + ny * 6;
        g_canvas.fillRect(px, py, 5, 5, nextColor);
    }

    // LINES 計數
    g_canvas.drawRect(93, 80, 38, 48, 0x2965);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("LINES", 112, 84, 1);

    char lineStr[8];
    snprintf(lineStr, sizeof(lineStr), "%d", _linesCleared);
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString(lineStr, 112, 102, 2);

    // 6. 遊戲結束畫面提示
    if (_state == TETRIS_GAME_OVER) {
        g_canvas.fillRect(10, 85, 76, 50, 0x18C3);
        g_canvas.drawRect(10, 85, 76, 50, TFT_RED);
        g_canvas.setTextColor(TFT_RED, 0x18C3);
        g_canvas.drawCentreString("GAME OVER", 48, 92, 1);
        g_canvas.setTextColor(TFT_WHITE, 0x18C3);
        g_canvas.drawCentreString("Press Joy", 48, 108, 1);
        g_canvas.drawCentreString("to Restart", 48, 120, 1);
    }

    // 7. 底部操作指示 (Y: 200 ~ 238)
    g_canvas.drawFastHLine(6, 198, SCREEN_WIDTH - 12, 0x2965);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("Joy: Move/Down  A: Rot", SCREEN_WIDTH / 2, 203, 1);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Btn B: Drop  Hold: Exit", SCREEN_WIDTH / 2, 216, 1);

    // 一次性推送至螢幕
    g_canvas.pushSprite(0, 0);
}
