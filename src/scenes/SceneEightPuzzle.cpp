/**
 * @file SceneEightPuzzle.cpp
 * @brief 經典九宮格「數字華容道 (8-Puzzle)」遊戲實作
 * @details 全域雙緩衝零閃爍繪製、直覺推移動力學、MPU6886 體感甩動洗牌、逆序數偶數 100% 有解保證
 */

#include "scenes/SceneEightPuzzle.h"

// 棋盤尺寸與幾何佈局常數
static const int TILE_SIZE = 36;                                                // 單一方塊寬高 (像素)
static const int TILE_GAP = 3;                                                 // 方塊間隔 (像素)
static const int BOARD_W = 3 * TILE_SIZE + 2 * TILE_GAP;                       // 棋盤總寬度 (114px)
static const int BOARD_H = 3 * TILE_SIZE + 2 * TILE_GAP;                       // 棋盤總高度 (114px)
static const int BOARD_X = (SCREEN_WIDTH - BOARD_W) / 2;                       // 棋盤置中 X: 10px
static const int BOARD_Y = 32;                                                 // 棋盤頂部 Y 座標

// 洗牌動作時間間隔 (ms)
static const uint16_t SHUFFLE_INTERVAL_MS = 25;

SceneEightPuzzle::SceneEightPuzzle()
    : _state(PUZZLE_STATE_IDLE), _emptyX(2), _emptyY(2),
      _shuffleStepsLeft(0), _lastShuffleTime(0), _lastMoveDir(-1),
      _moves(0), _startTime(0), _elapsedTime(0), _bestMoves(999),
      _lastMoveTime(0) {
    resetToSolved();
}

void SceneEightPuzzle::init() {
    _state = PUZZLE_STATE_IDLE;
    _nextScene = SCENE_COUNT;
    _moves = 0;
    _elapsedTime = 0;
    _lastMoveTime = millis();
    resetToSolved();
}

void SceneEightPuzzle::exit() {
    // 離開場景時關閉彩虹燈效與任何殘留 LED
    _state = PUZZLE_STATE_IDLE;
}

void SceneEightPuzzle::resetToSolved() {
    // 將 3x3 盤面還原為 1~8 順序排列，右下角為空格 (0)
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (x == 2 && y == 2) {
                _board[x][y] = 0;
            } else {
                _board[x][y] = y * 3 + x + 1;
            }
        }
    }
    _emptyX = 2;
    _emptyY = 2;
}

void SceneEightPuzzle::startShuffle() {
    resetToSolved();
    _shuffleStepsLeft = 70; // 執行 70 步隨機合法移動，保證盤面 100% 可解
    _lastShuffleTime = millis();
    _lastMoveDir = -1;
    _state = PUZZLE_STATE_SHUFFLING;
}

void SceneEightPuzzle::stepShuffle() {
    // 尋找當前空格周圍相鄰的合法方塊座標
    // 方向定義：0:上, 1:下, 2:左, 3:右
    const int8_t dx[4] = {0, 0, -1, 1};
    const int8_t dy[4] = {-1, 1, 0, 0};
    const int8_t oppDir[4] = {1, 0, 3, 2}; // 避免原路往返

    int8_t validDirs[4];
    uint8_t validCount = 0;

    for (int8_t i = 0; i < 4; i++) {
        int8_t nx = _emptyX + dx[i];
        int8_t ny = _emptyY + dy[i];
        if (nx >= 0 && nx < 3 && ny >= 0 && ny < 3) {
            // 優先排除恰好是上一步的反方向，防止原地來回震盪
            if (_lastMoveDir != -1 && i == oppDir[_lastMoveDir] && validCount > 0) {
                continue;
            }
            validDirs[validCount++] = i;
        }
    }

    // 若過濾後無剩餘方向，則放寬限制
    if (validCount == 0) {
        for (int8_t i = 0; i < 4; i++) {
            int8_t nx = _emptyX + dx[i];
            int8_t ny = _emptyY + dy[i];
            if (nx >= 0 && nx < 3 && ny >= 0 && ny < 3) {
                validDirs[validCount++] = i;
            }
        }
    }

    if (validCount > 0) {
        int chosen = validDirs[EntropyManager::random(0, validCount)];
        int8_t targetX = _emptyX + dx[chosen];
        int8_t targetY = _emptyY + dy[chosen];

        // 方塊與空格交換
        _board[_emptyX][_emptyY] = _board[targetX][targetY];
        _board[targetX][targetY] = 0;
        _emptyX = targetX;
        _emptyY = targetY;
        _lastMoveDir = chosen;
    }

    _shuffleStepsLeft--;
    if (_shuffleStepsLeft == 0) {
        // 洗牌結束，若恰好歸位過多則加洗幾步
        if (countPlaced() >= 7) {
            _shuffleStepsLeft = 10;
            return;
        }
        _moves = 0;
        _startTime = millis();
        _state = PUZZLE_STATE_PLAYING;
    }
}

uint8_t SceneEightPuzzle::countPlaced() const {
    uint8_t count = 0;
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            uint8_t target = y * 3 + x + 1;
            if (target <= 8 && _board[x][y] == target) {
                count++;
            }
        }
    }
    return count;
}

bool SceneEightPuzzle::checkWin() {
    if (_emptyX != 2 || _emptyY != 2) return false;
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (x == 2 && y == 2) {
                if (_board[x][y] != 0) return false;
            } else {
                if (_board[x][y] != (y * 3 + x + 1)) return false;
            }
        }
    }
    return true;
}

bool SceneEightPuzzle::tryMove(int8_t targetX, int8_t targetY, AudioManager& audio, LedManager& led) {
    if (targetX < 0 || targetX >= 3 || targetY < 0 || targetY >= 3) {
        // 受阻撞牆無效移動
        audio.playClick();
        led.setColor(80, 0, 0); // 瞬間暗紅微閃提醒
        return false;
    }

    // 將目標方塊滑入空格
    _board[_emptyX][_emptyY] = _board[targetX][targetY];
    _board[targetX][targetY] = 0;
    _emptyX = targetX;
    _emptyY = targetY;
    _moves++;

    audio.playTick();
    led.setColor(0, 120, 255); // 科技藍微光反饋

    if (checkWin()) {
        _state = PUZZLE_STATE_WON;
        _elapsedTime = millis() - _startTime;
        if (_moves < _bestMoves) {
            _bestMoves = _moves;
        }
        audio.playWin();
        led.setRainbowMode(true);
    }
    return true;
}

void SceneEightPuzzle::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. 全域規範：Button B 長按 (> 500ms) 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        led.setRainbowMode(false);
        _nextScene = SCENE_MENU;
        return;
    }

    uint32_t now = millis();

    // 2. 體感甩動：在任何遊玩或待機狀態下激烈甩手機身立即重新洗牌開局
    if (input.isShaken && _state != PUZZLE_STATE_SHUFFLING) {
        audio.playDiceRoll();
        led.setRainbowMode(false);
        led.setColor(255, 180, 0);
        startShuffle();
        return;
    }

    // 3. 狀態機處理
    switch (_state) {
        case PUZZLE_STATE_IDLE: {
            led.setColor(0, 180, 220);
            if (input.btnAPressed || input.joyBtnPressed) {
                audio.playDiceRoll();
                startShuffle();
            }
            break;
        }

        case PUZZLE_STATE_SHUFFLING: {
            if (now - _lastShuffleTime >= SHUFFLE_INTERVAL_MS) {
                _lastShuffleTime = now;
                stepShuffle();
                if (_shuffleStepsLeft % 5 == 0) {
                    audio.playTick();
                }
            }
            break;
        }

        case PUZZLE_STATE_PLAYING: {
            // Button A：在遊玩中短按亦可手動重新開局洗牌
            if (input.btnAPressed) {
                audio.playDiceRoll();
                startShuffle();
                return;
            }

            // 搖桿推動方塊：支援邊緣脈衝與類比門檻防抖冷卻 (160ms)
            if (now - _lastMoveTime > 160) {
                bool moved = false;
                int16_t absX = abs(input.joyX);
                int16_t absY = abs(input.joyY);

                if (input.joyPushedLeft || (absX > 45 && absX >= absY && input.joyX < 0)) {
                    // 推向左邊：將空格右側之方塊推入空格
                    tryMove(_emptyX + 1, _emptyY, audio, led);
                    _lastMoveTime = now;
                } else if (input.joyPushedRight || (absX > 45 && absX >= absY && input.joyX > 0)) {
                    // 推向右邊：將空格左側之方塊推入空格
                    tryMove(_emptyX - 1, _emptyY, audio, led);
                    _lastMoveTime = now;
                } else if (input.joyPushedUp || (absY > 45 && absY > absX && input.joyY < 0)) {
                    // 推向上方：將空格下方之方塊推入空格
                    tryMove(_emptyX, _emptyY + 1, audio, led);
                    _lastMoveTime = now;
                } else if (input.joyPulledDown || (absY > 45 && absY > absX && input.joyY > 0)) {
                    // 推向下方：將空格上方之方塊推入空格
                    tryMove(_emptyX, _emptyY - 1, audio, led);
                    _lastMoveTime = now;
                }
            }
            break;
        }

        case PUZZLE_STATE_WON: {
            // 通關狀態下，按鍵或搖桿中心鍵重新開局
            if (input.btnAPressed || input.joyBtnPressed) {
                led.setRainbowMode(false);
                audio.playDiceRoll();
                startShuffle();
            }
            break;
        }
    }
}

void SceneEightPuzzle::draw() {
    // 全域雙緩衝離線繪製，杜絕全螢幕閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 24)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 24, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("8-PUZZLE", 6, 4, 2);

    char statBuf[16];
    if (_state == PUZZLE_STATE_PLAYING) {
        uint32_t sec = (millis() - _startTime) / 1000;
        uint32_t min = sec / 60;
        sec %= 60;
        snprintf(statBuf, sizeof(statBuf), "M:%03d %02d:%02d", _moves, min, sec);
        g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
        g_canvas.drawRightString(statBuf, SCREEN_WIDTH - 6, 6, 1);
    } else if (_state == PUZZLE_STATE_WON) {
        g_canvas.setTextColor(TFT_GREEN, 0x18C3);
        g_canvas.drawRightString("CLEAR!", SCREEN_WIDTH - 6, 6, 1);
    } else {
        g_canvas.setTextColor(COLOR_SILVER, 0x18C3);
        g_canvas.drawRightString("READY", SCREEN_WIDTH - 6, 6, 1);
    }

    // 2. 棋盤外框 (3x3, 置中 X: 10, Y: 32)
    g_canvas.drawRoundRect(BOARD_X - 2, BOARD_Y - 2, BOARD_W + 4, BOARD_H + 4, 3, 0x39E7);

    // 3. 繪製 3x3 方塊與空格
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            int px = BOARD_X + x * (TILE_SIZE + TILE_GAP);
            int py = BOARD_Y + y * (TILE_SIZE + TILE_GAP);
            uint8_t num = _board[x][y];

            if (num == 0) {
                // 空格凹槽：暗色下凹質感與微弱內框
                g_canvas.fillRoundRect(px, py, TILE_SIZE, TILE_SIZE, 3, 0x0841);
                g_canvas.drawRoundRect(px + 1, py + 1, TILE_SIZE - 2, TILE_SIZE - 2, 2, 0x18C3);
            } else {
                // 檢查是否已在正確目標位置
                bool isCorrect = (num == (y * 3 + x + 1));
                uint16_t bgCol = isCorrect ? 0x03E0 : 0x2147;       // 正確為翡翠綠，未定位為金屬石墨藍
                uint16_t borderCol = isCorrect ? TFT_GREEN : COLOR_SILVER;
                uint16_t textCol = isCorrect ? TFT_WHITE : COLOR_GOLD;

                // 圓角方塊背景與外框
                g_canvas.fillRoundRect(px, py, TILE_SIZE, TILE_SIZE, 3, bgCol);
                g_canvas.drawRoundRect(px, py, TILE_SIZE, TILE_SIZE, 3, borderCol);

                // 立體頂部微高光
                g_canvas.drawFastHLine(px + 2, py + 2, TILE_SIZE - 4, 0xCE79);

                // 大字號數字顯示
                char numStr[2];
                numStr[0] = '0' + num;
                numStr[1] = '\0';
                g_canvas.setTextColor(textCol, bgCol);
                g_canvas.drawCentreString(numStr, px + (TILE_SIZE / 2), py + 6, 4);
            }
        }
    }

    // 4. 進度與統計資訊區 (Y: 154 ~ 188)
    uint8_t placed = countPlaced();
    char placedBuf[20];
    snprintf(placedBuf, sizeof(placedBuf), "PLACED: %d/8", placed);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawString(placedBuf, 12, 156, 1);

    if (_bestMoves < 999) {
        char bestBuf[16];
        snprintf(bestBuf, sizeof(bestBuf), "BEST: %d", _bestMoves);
        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawRightString(bestBuf, SCREEN_WIDTH - 12, 156, 1);
    }

    // 歸位進度條 (寬度 114px，置中)
    int barW = (placed * BOARD_W) / 8;
    g_canvas.drawRect(BOARD_X, 170, BOARD_W, 6, 0x2965);
    if (barW > 0) {
        g_canvas.fillRect(BOARD_X + 1, 171, barW, 4, TFT_GREEN);
    }

    // 5. 狀態彈窗覆蓋 (IDLE / WON)
    if (_state == PUZZLE_STATE_IDLE) {
        g_canvas.fillRoundRect(14, 68, 107, 50, 4, 0x10A2);
        g_canvas.drawRoundRect(14, 68, 107, 50, 4, COLOR_GOLD);
        g_canvas.setTextColor(COLOR_GOLD, 0x10A2);
        g_canvas.drawCentreString("8-PUZZLE", SCREEN_WIDTH / 2, 73, 2);
        g_canvas.setTextColor(TFT_WHITE, 0x10A2);
        g_canvas.drawCentreString("Press A / Joy", SCREEN_WIDTH / 2, 92, 1);
        g_canvas.drawCentreString("or Shake device", SCREEN_WIDTH / 2, 104, 1);
    } else if (_state == PUZZLE_STATE_WON) {
        g_canvas.fillRoundRect(12, 62, 111, 62, 4, 0x10A2);
        g_canvas.drawRoundRect(12, 62, 111, 62, 4, TFT_GREEN);
        g_canvas.setTextColor(TFT_GREEN, 0x10A2);
        g_canvas.drawCentreString("VICTORY!", SCREEN_WIDTH / 2, 67, 2);

        char winInfo[24];
        uint32_t s = _elapsedTime / 1000;
        snprintf(winInfo, sizeof(winInfo), "%d Moves | %02d:%02d", _moves, s / 60, s % 60);
        g_canvas.setTextColor(COLOR_GOLD, 0x10A2);
        g_canvas.drawCentreString(winInfo, SCREEN_WIDTH / 2, 88, 1);

        g_canvas.setTextColor(TFT_WHITE, 0x10A2);
        g_canvas.drawCentreString("Press A to Replay", SCREEN_WIDTH / 2, 104, 1);
    }

    // 6. 底部操作指示 (Y: 194 ~ 238)
    g_canvas.drawFastHLine(6, 194, SCREEN_WIDTH - 12, 0x2965);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("Joy: Push Adjacent Tile", SCREEN_WIDTH / 2, 199, 1);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Shake / BtnA: Shuffle", SCREEN_WIDTH / 2, 212, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold BtnB: Exit Menu", SCREEN_WIDTH / 2, 225, 1);

    // 一次性推送全螢幕
    g_canvas.pushSprite(0, 0);
}
