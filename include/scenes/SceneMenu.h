/**
 * @file SceneMenu.h
 * @brief 全域主選單場景標頭檔
 */

#pragma once

#include "scenes/Scene.h"

class SceneMenu : public Scene {
public:
    SceneMenu();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_MENU; }

private:
    uint8_t _selectedIdx;
    bool _needsRedraw;
    uint32_t _lastAnimTime;
    uint8_t _pulse;

    void drawMenuItem(int idx, int y, bool isSelected);
};
