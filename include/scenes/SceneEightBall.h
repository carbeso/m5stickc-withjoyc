/**
 * @file SceneEightBall.h
 * @brief 直式粒子神秘八號球場景標頭檔
 */

#pragma once

#include "scenes/Scene.h"
#include "CustomChineseFont.h"

class SceneEightBall : public Scene {
public:
    SceneEightBall();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_EIGHT_BALL; }

private:
    uint8_t _fortuneIdx;
    bool _isRevealing;      // 粒子凝聚中
    bool _isRevealed;       // 已完整浮現
    uint32_t _revealStartTime;
    uint8_t _revealStep;    // 粒子凝聚階段 (0 ~ 16)
    bool _needsRedraw;

    void startDivination(AudioManager& audio, LedManager& led);
    void drawChineseChar(int x, int y, const char* utf8Char, uint8_t maxRow, uint16_t color);
};
