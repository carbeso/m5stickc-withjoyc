/**
 * @file Scene1A2B.h
 * @brief 1A2B 益智猜數字遊戲標頭檔
 */

#pragma once

#include "Scene.h"
#include "EntropyManager.h"

enum GameState1A2B {
    STATE_1A2B_IDLE = 0,    // 待機開局狀態
    STATE_1A2B_PLAYING,     // 猜題作答中
    STATE_1A2B_WON          // 4A 獲勝狀態
};

class Scene1A2B : public Scene {
public:
    Scene1A2B();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_1A2B; }

private:
    void generateTarget();
    void evaluateGuess(AudioManager& audio, LedManager& led);

    GameState1A2B _state;
    uint8_t _target[4];     // 謎底 4 位不重複數字
    uint8_t _guess[4];      // 當前作答 4 位數
    uint8_t _cursor;        // 當前選取位數游標 (0: 千位, 1: 百位, 2: 十位, 3: 個位)

    uint16_t _attempts;     // 累積作答次數
    uint8_t _lastA;         // 最新結果 A 數量
    uint8_t _lastB;         // 最新結果 B 數量
    bool _hasEvaluated;     // 是否已產生至少一次比對結果

    uint32_t _lastNavTime;  // 搖桿導覽防抖冷卻時間戳
    bool _needsRedraw;      // 畫面重繪標記
};
