/**
 * @file SceneLedStudio.h
 * @brief RGB LED 調光工作室 (LED Studio) 場景標頭檔
 * @details 透過搖桿 X/Y 調整色相與亮度，即時驅動 MiniJoyC HAT 雙 SK6812 全彩燈珠與螢幕預覽
 */

#pragma once

#include "Scene.h"

class SceneLedStudio : public Scene {
public:
    SceneLedStudio();
    virtual ~SceneLedStudio() {}

    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    void exit() override;
    GameScene getSceneId() const override { return SCENE_LED_STUDIO; }

private:
    int16_t _hue;          // 0 ~ 359
    uint8_t _brightness;   // 5 ~ 100
    uint8_t _ledR;
    uint8_t _ledG;
    uint8_t _ledB;
    bool _needsRedraw;
};
