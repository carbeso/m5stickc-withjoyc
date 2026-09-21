/**
 * @file Scene.h
 * @brief 遊戲場景抽象介面
 */

#pragma once

#include <Arduino.h>
#include <M5StickCPlus.h>
#include "Config.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "LedManager.h"

class Scene {
public:
    virtual ~Scene() {}
    virtual void init() = 0;
    virtual void exit() {}
    virtual void update(InputManager& input, AudioManager& audio, LedManager& led) = 0;
    virtual void draw() = 0;
    virtual GameScene getSceneId() const = 0;

    // 檢查是否要求切換至其他場景 (SCENE_COUNT 表示無切換)
    virtual GameScene getNextScene() { return _nextScene; }
    virtual void clearNextScene() { _nextScene = SCENE_COUNT; }

protected:
    GameScene _nextScene = SCENE_COUNT;
};
