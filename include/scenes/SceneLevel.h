/**
 * @file SceneLevel.h
 * @brief 三軸電子水平儀 (Bubble Level) 場景標頭檔
 * @details 讀取 MPU6886 IMU 加速度資料，以雙緩衝畫布呈現平滑移動氣泡與完美水平聲效
 */

#pragma once

#include "Scene.h"

class SceneLevel : public Scene {
public:
    SceneLevel();
    virtual ~SceneLevel() {}

    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    void exit() override;
    GameScene getSceneId() const override { return SCENE_LEVEL; }

private:
    float _bubbleX;
    float _bubbleY;
    float _targetBubbleX;
    float _targetBubbleY;
    bool _isCentered;
    uint32_t _lastCenterTick;
    bool _needsRedraw;
};
