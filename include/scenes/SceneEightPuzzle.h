/**
 * @file SceneEightPuzzle.h
 * @brief 經典九宮格「數字華容道 (8-Puzzle)」遊戲標頭檔
 * @details 支援雙軸搖桿推移方塊、體感甩動洗牌、逆序數偶數/模擬還原 100% 有解保證
 */

#pragma once

#include "Scene.h"
#include "EntropyManager.h"

enum PuzzleState {
    PUZZLE_STATE_IDLE = 0,      // 待機開局狀態
    PUZZLE_STATE_SHUFFLING,     // 洗牌動畫進行中
    PUZZLE_STATE_PLAYING,       // 計時與解謎作答中
    PUZZLE_STATE_WON            // 拼盤完成通關狀態
};

class SceneEightPuzzle : public Scene {
public:
    SceneEightPuzzle();
    void init() override;
    void exit() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_EIGHT_PUZZLE; }

private:
    void resetToSolved();
    void startShuffle();
    void stepShuffle();
    bool tryMove(int8_t targetX, int8_t targetY, AudioManager& audio, LedManager& led);
    bool checkWin();
    uint8_t countPlaced() const;

    PuzzleState _state;
    uint8_t _board[3][3];       // 3x3 棋盤 (0 代表空格，1~8 為數字方塊)
    int8_t _emptyX;             // 當前空格 X 座標 (0~2)
    int8_t _emptyY;             // 當前空格 Y 座標 (0~2)

    // 洗牌狀態
    uint8_t _shuffleStepsLeft;  // 洗牌剩餘步數
    uint32_t _lastShuffleTime;  // 洗牌單步計時
    int8_t _lastMoveDir;        // 上一次洗牌反向防回跳 (0:上, 1:下, 2:左, 3:右, -1:無)

    // 遊玩統計與計時
    uint16_t _moves;            // 當前移動步數
    uint32_t _startTime;        // 開局時間戳記
    uint32_t _elapsedTime;      // 通關總耗時 (ms)
    uint16_t _bestMoves;        // 最佳通關步數

    // 搖桿防抖與控制冷卻
    uint32_t _lastMoveTime;     // 搖桿推動冷卻時間戳
};
