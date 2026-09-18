/**
 * @file SceneRoulette.h
 * @brief 直向垂直幸運輪盤場景標頭檔
 */

#pragma once

#include "scenes/Scene.h"

struct RoulettePocket {
    uint8_t number;     // 0 ~ 36
    uint8_t colorType;  // 0: 綠 (0), 1: 紅, 2: 黑
};

class SceneRoulette : public Scene {
public:
    SceneRoulette();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_ROULETTE; }

private:
    float _stripPos;        // 垂直滾動帶位置 (浮點數以利平滑阻尼)
    float _stripSpeed;      // 滾動速度
    bool _isSpinning;
    uint8_t _targetIndex;
    bool _needsRedraw;
    uint32_t _lastTickTime;
    uint32_t _spinStartTime;
    uint32_t _cruiseDuration; // 隨機巡航時長 (使得總旋轉時長在 2.5s ~ 6.0s 隨機不可預測)

    void spinRoulette(AudioManager& audio, LedManager& led);
};
