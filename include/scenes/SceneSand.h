/**
 * @file SceneSand.h
 * @brief 重力感應流沙 (Gravity Sand Simulator) 場景標頭檔
 * @details 基於 MPU6886 三軸加速度計之落沙細胞自動機物理模擬，支援雙緩衝零閃爍繪製
 */

#pragma once

#include "Scene.h"
#include "Config.h"
#include "EntropyManager.h"

enum SandTheme {
    THEME_DESERT_GOLD = 0, // 沙漠金
    THEME_NEON_POP,        // 幻彩霓虹
    THEME_ICE_SNOW,        // 冰原藍雪
    THEME_COUNT
};

class SceneSand : public Scene {
public:
    SceneSand();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SAND; }

private:
    void resetSand();
    void spawnSand(int16_t gridX, int16_t gridY, uint8_t count = 3);
    void updatePhysics(float ax, float ay, bool isShaking);

    static const uint8_t GRID_W = 45; // 135 / 3
    static const uint8_t GRID_H = 64; // 192 / 3
    static const uint8_t SAND_OFFSET_Y = 24;

    uint8_t _grid[GRID_W][GRID_H];    // 0: 空氣, 1~3: 沙粒色階
    uint16_t _sandCount;              // 當前場上沙粒總數
    SandTheme _theme;

    uint32_t _lastPhysicsTime;
    uint32_t _lastSpawnTime;
    bool _needsRedraw;
};
