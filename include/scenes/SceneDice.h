/**
 * @file SceneDice.h
 * @brief 直式多面骰子盒場景標頭檔
 */

#pragma once

#include "scenes/Scene.h"

class SceneDice : public Scene {
public:
    SceneDice();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_DICE; }

private:
    uint8_t _dieTypeIdx;    // 0: d4, 1: d6, 2: d8, 3: d10, 4: d12, 5: d20, 6: d100
    uint8_t _diceCount;     // 1 ~ 6 顆
    uint8_t _diceResults[6];

    bool _isRolling;
    bool _triggeredByJoy;   // 是否由搖桿/按鈕觸發 (true: 搖桿放開即停; false: 體感甩動等待靜止)
    uint32_t _rollStartTime;
    uint32_t _lastTickTime;
    bool _needsRedraw;

    void rollDice(bool byJoy, AudioManager& audio, LedManager& led);
};
