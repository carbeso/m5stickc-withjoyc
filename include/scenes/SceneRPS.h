/**
 * @file SceneRPS.h
 * @brief 剪刀石頭布 (Rock-Paper-Scissors) 場景標頭檔：支援 1~2 隻手、手勢切換動效、長拉旋轉與勝負判定
 */

#pragma once

#include "scenes/Scene.h"

// 手勢列舉
enum GestureType {
    GESTURE_ROCK = 0,       // 石頭
    GESTURE_SCISSORS,       // 剪刀
    GESTURE_PAPER,          // 布
    GESTURE_COUNT
};

class SceneRPS : public Scene {
public:
    SceneRPS();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_RPS; }

private:
    uint8_t _handCount;         // 1: 單手出拳, 2: 雙手對決 (上下顯示)
    uint8_t _handResults[2];    // 兩隻手當前手勢 (0: ROCK, 1: SCISSORS, 2: PAPER)

    bool _isSpinning;           // 是否正在旋轉猜拳中
    bool _triggeredByJoy;       // 是否由搖桿長拉觸發 (true: 鬆開煞停; false: 體感甩動)
    uint32_t _spinStartTime;    // 旋轉啟動時間戳記
    uint32_t _lastTickTime;     // 手勢幀切換時間戳記
    uint8_t _animCycle;         // 動畫循環計數
    bool _needsRedraw;          // 螢幕重繪旗標

    void startDuel(bool byJoy, AudioManager& audio, LedManager& led);
    void drawGesture(int cx, int cy, uint8_t gesture, int radius, const char* label);
    void drawResultBanner();
};
