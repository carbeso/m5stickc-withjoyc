/**
 * @file main.cpp
 * @brief M5StickC Plus 與 MiniJoyC 五合一掌上型紓壓玩具韌體主程式
 * @details 採用直向顯示 (135x240)，搖桿在上操控，整合 5 大紓壓遊戲：
 *          1. 骰子盒 (1d6 ~ 6d20)
 *          2. 極簡幸運撲克 (52 + 2 鬼牌)
 *          3. 直式粒子神秘八號球 (繁中占卜籤詩)
 *          4. 直向垂直幸運輪盤 (歐式 0-36)
 *          5. 3x3 搖桿下拉角子老虎機 (拉桿手勢與 8 條連線)
 */

#include <M5StickCPlus.h>
#include "Config.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "LedManager.h"

// 引入所有遊戲場景
#include "scenes/SceneMenu.h"
#include "scenes/SceneDice.h"
#include "scenes/ScenePoker.h"
#include "scenes/SceneEightBall.h"
#include "scenes/SceneRoulette.h"
#include "scenes/SceneSlot.h"

// 核心硬體與感官管理器
InputManager input;
AudioManager audio;
LedManager led(input.getJoyC());

// 各遊戲場景實體
SceneMenu sceneMenu;
SceneDice sceneDice;
ScenePoker scenePoker;
SceneEightBall sceneEightBall;
SceneRoulette sceneRoulette;
SceneSlot sceneSlot;

Scene* currentScene = &sceneMenu;
uint8_t currentRotation = 0; // 0: 直向 (搖桿在上), 2: 旋轉 180 度

void switchScene(GameScene target) {
    switch (target) {
        case SCENE_MENU:        currentScene = &sceneMenu; break;
        case SCENE_DICE:        currentScene = &sceneDice; break;
        case SCENE_POKER:       currentScene = &scenePoker; break;
        case SCENE_EIGHT_BALL:  currentScene = &sceneEightBall; break;
        case SCENE_ROULETTE:    currentScene = &sceneRoulette; break;
        case SCENE_SLOT:        currentScene = &sceneSlot; break;
        default:                currentScene = &sceneMenu; break;
    }
    currentScene->clearNextScene();
    currentScene->init();
}

void setup() {
    // 1. 初始化 M5StickC Plus
    M5.begin(true, true, false);

    // 2. 設定螢幕為直向模式 (135 寬 x 240 高，搖桿在上)
    M5.Lcd.setRotation(currentRotation);
    M5.Lcd.fillScreen(TFT_BLACK);

    // 3. 蜂鳴器硬體防呆與初始化
    audio.begin();

    // 4. 初始化頂部 MiniJoyC HAT 匯流排 (SDA=0, SCL=26)
    input.begin();

    // 5. 開機啟動畫面與音效
    M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
    M5.Lcd.drawCentreString("FIDGET TOY", SCREEN_WIDTH / 2, 85, 4);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("5-in-1 System", SCREEN_WIDTH / 2, 120, 2);
    audio.playClick();
    led.setColor(255, 200, 50);

    delay(400);

    // 6. 進入全域主選單
    switchScene(SCENE_MENU);
}

void loop() {
    // 1. 更新 M5 內部按鍵與 IMU
    M5.update();

    // 2. 更新搖桿與體感狀態
    input.update();

    // 3. Button B 短按：一鍵旋轉螢幕 180 度 (Rotation 0 <-> 2)
    if (input.btnBPressed) {
        currentRotation = (currentRotation == 0) ? 2 : 0;
        M5.Lcd.setRotation(currentRotation);
        audio.playClick();
        currentScene->init(); // 觸發重繪
    }

    // 4. 運行當前場景邏輯與繪製
    currentScene->update(input, audio, led);
    currentScene->draw();

    // 5. 檢查場景跳轉請求
    GameScene next = currentScene->getNextScene();
    if (next != SCENE_COUNT) {
        switchScene(next);
    }

    // 6. 更新非阻塞音效與 LED 燈效
    audio.update();
    led.update();

    delay(10); // 控制主循環幀率約 50 ~ 60 FPS
}
