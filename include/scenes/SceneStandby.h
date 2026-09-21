/**
 * @file SceneStandby.h
 * @brief 待機畫面場景標頭檔：支援 Matrix Code Rain、RTC 數位時鐘與自動低功耗調光
 */

#pragma once

#include "Scene.h"
#include "EntropyManager.h"
#include "BleSyncManager.h"

enum StandbyMode {
    STANDBY_MATRIX = 0,     // 黑客帝國代碼雨動態螢幕保護
    STANDBY_CLOCK           // 桌面 RTC 數位時鐘
};

struct MatrixColumn {
    int16_t y;              // 串流頭部當前 Y 像素位置
    uint8_t speed;          // 下落步長速度 (像素/次)
    uint8_t length;         // 尾翼字元長度
    uint32_t nextDropTime;  // 下次步進時間戳
    char chars[20];         // 串流內暫存隨機字元
};

class SceneStandby : public Scene {
public:
    SceneStandby();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_STANDBY; }

private:
    void initMatrix();
    void updateMatrix();
    void drawMatrix();
    void drawClock();

    StandbyMode _mode;
    uint8_t _themeIdx;      // Matrix 色彩主題 (0: 經典綠, 1: 科技藍, 2: 霓虹紫)
    
    // Matrix Rain 狀態
    static const uint8_t COL_COUNT = 16;
    MatrixColumn _cols[COL_COUNT];
    uint32_t _lastFrameTime;

    // Clock 狀態
    RTC_TimeTypeDef _time;
    RTC_DateTypeDef _date;
    uint32_t _lastClockCheck;
    bool _colonBlink;

    bool _needsRedraw;
    uint32_t _lastBleAnimTime;
    uint8_t _bleAnimStep;
    uint32_t _syncFeedbackEndTime;
};
