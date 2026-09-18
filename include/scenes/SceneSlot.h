/**
 * @file SceneSlot.h
 * @brief 3x3 搖桿下拉角子老虎機標頭檔
 */

#pragma once

#include "scenes/Scene.h"

// 老虎機符號定義
enum SlotSymbol {
    SYM_SEVEN = 0,  // 7 (大獎)
    SYM_BAR,        // BAR
    SYM_BELL,       // 金鈴
    SYM_CHERRY,     // 櫻桃
    SYM_LEMON,      // 檸檬
    SYM_STAR,       // 金星
    SYM_CLOVER,     // 幸運草
    SYM_COUNT
};

class SceneSlot : public Scene {
public:
    SceneSlot();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SLOT; }

private:
    uint8_t _grid[3][3];    // [col][row], 3 欄 x 3 列
    bool _colSpinning[3];   // 3 欄各別旋轉狀態
    float _colOffset[3];    // 3 欄滾動像素偏移
    uint32_t _spinStartTime;

    bool _hasWon;
    uint8_t _winLinesMask;  // 8 條線中獎遮罩 (位元 0~7)
    uint32_t _releaseTime;  // 放開拉桿的時間戳記 (非靜態，防止狀態殘留)
    bool _needsRedraw;
    uint32_t _lastTickTime;
    uint8_t _flashTimer;

    void pullLever(AudioManager& audio, LedManager& led);
    void checkWinLines(AudioManager& audio, LedManager& led);
    void drawSymbol(int x, int y, uint8_t sym, bool highlight);
};
