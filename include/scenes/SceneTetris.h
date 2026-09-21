/**
 * @file SceneTetris.h
 * @brief 極簡掌上俄羅斯方塊 (Mini Tetris) 場景標頭檔
 * @details 10x20 棋盤、右側 NEXT 預覽與消行計數，支援全域雙緩衝零閃爍渲染
 */

#pragma once

#include "Scene.h"
#include "Config.h"
#include "EntropyManager.h"

enum TetrisState {
    TETRIS_PLAYING = 0,
    TETRIS_GAME_OVER
};

class SceneTetris : public Scene {
public:
    SceneTetris();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_TETRIS; }

private:
    void resetGame();
    void spawnPiece();
    bool checkCollision(int8_t px, int8_t py, uint8_t shape, uint8_t rot);
    void lockPiece(AudioManager& audio, LedManager& led);
    void clearLines(AudioManager& audio, LedManager& led);

    static const uint8_t BOARD_W = 10;
    static const uint8_t BOARD_H = 20;
    static const uint8_t CELL_SIZE = 8;
    static const uint8_t BOARD_X = 8;
    static const uint8_t BOARD_Y = 32;

    uint8_t _board[BOARD_W][BOARD_H]; // 0: 空, 1~7: 方塊類型

    // 當前下落中方塊
    int8_t _curX, _curY;
    uint8_t _curShape;  // 0~6
    uint8_t _curRot;    // 0~3

    // 下一個方塊
    uint8_t _nextShape;

    TetrisState _state;
    uint16_t _linesCleared;

    uint32_t _lastFallTime;
    uint32_t _lastMoveTime;
    uint32_t _lastRotateTime;
    bool _needsRedraw;
};
