/**
 * @file SceneMenu.h
 * @brief 全域主選單場景標頭檔：整合聲音開關、亮度調節與直式卡片導航
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
    uint8_t _topIdx;          // 滾動視窗頂部遊戲索引 (一頁顯示 3 張舒適大卡片)
    bool _redrawAll;
    bool _redrawHeader;
    bool _redrawCards;
    uint8_t _brightnessLevel; // 0: 35%, 1: 70%, 2: 100%
    bool _cachedMuteState;

    uint32_t _lastBatCheckTime;
    uint8_t _cachedBatPct;
    bool _cachedIsCharging;

    void applyBrightness();
    void updateBatteryInfo();
    void drawHeader();
    void drawCards();
    void drawFooter();
    void drawBatteryIcon(int x, int y, uint8_t pct, bool charging);
};

