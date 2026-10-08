/**
 * @file SceneGTracker.h
 * @brief G-Force 歷史峰值衝擊追蹤器 (G-Tracker) 場景標頭檔
 * @details 採樣 10Hz 5 秒 G-Force 歷史資料，呈現最大衝擊向量與即時柱狀歷史圖
 */

#pragma once

#include "Scene.h"

class SceneGTracker : public Scene {
public:
    SceneGTracker();
    virtual ~SceneGTracker() {}

    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    void exit() override;
    GameScene getSceneId() const override { return SCENE_G_TRACKER; }

private:
    static const uint8_t G_HIST_SIZE = 50; // 5 秒視窗 (10Hz 採樣)
    float _gHistory[G_HIST_SIZE];
    uint8_t _gHistIdx;
    float _currentG;
    float _peakG;
    float _peakAx;
    float _peakAy;
    uint32_t _lastSampleTime;
    bool _needsRedraw;
};
