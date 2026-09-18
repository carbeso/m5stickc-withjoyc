/**
 * @file SceneEightBall.h
 * @brief 純英文經典神秘八號球場景標頭檔 (已完全移除異常之中文點陣能力)
 */

#pragma once

#include "scenes/Scene.h"

struct ClassicFortune {
    const char* line1;     // 第一行英文 (大字)
    const char* line2;     // 第二行英文 (補充)
    uint8_t category;      // 0: 吉, 1: 惑, 2: 凶
};

class SceneEightBall : public Scene {
public:
    SceneEightBall();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_EIGHT_BALL; }

private:
    uint8_t _fortuneIdx;
    bool _isRevealing;
    bool _isRevealed;
    uint32_t _revealStartTime;
    bool _needsRedraw;

    void startDivination(AudioManager& audio, LedManager& led);
};
