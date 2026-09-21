/**
 * @file main.cpp
 * @brief M5StickC Plus 與 MiniJoyC 七合一掌上型紓壓玩具韌體主程式
 * @details 螢幕固定為直向顯示 (135x240，Rotation 0)，搖桿在上操控。
 */

#include <M5StickCPlus.h>
#include "Config.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "LedManager.h"

#include "scenes/SceneMenu.h"
#include "scenes/SceneDice.h"
#include "scenes/ScenePoker.h"
#include "scenes/SceneEightBall.h"
#include "scenes/SceneRoulette.h"
#include "scenes/SceneSlot.h"
#include "scenes/SceneCoin.h"
#include "scenes/SceneRPS.h"
#include "scenes/Scene1A2B.h"
#include "scenes/SceneStandby.h"

InputManager input;
AudioManager audio;
LedManager led(input.getJoyC());

SceneMenu sceneMenu;
SceneDice sceneDice;
ScenePoker scenePoker;
SceneEightBall sceneEightBall;
SceneRoulette sceneRoulette;
SceneSlot sceneSlot;
SceneCoin sceneCoin;
SceneRPS sceneRPS;
Scene1A2B scene1A2B;
SceneStandby sceneStandby;

Scene* currentScene = &sceneMenu;

void switchScene(GameScene target) {
    switch (target) {
        case SCENE_MENU:        currentScene = &sceneMenu; break;
        case SCENE_DICE:        currentScene = &sceneDice; break;
        case SCENE_POKER:       currentScene = &scenePoker; break;
        case SCENE_EIGHT_BALL:  currentScene = &sceneEightBall; break;
        case SCENE_ROULETTE:    currentScene = &sceneRoulette; break;
        case SCENE_SLOT:        currentScene = &sceneSlot; break;
        case SCENE_COIN:        currentScene = &sceneCoin; break;
        case SCENE_RPS:         currentScene = &sceneRPS; break;
        case SCENE_1A2B:        currentScene = &scene1A2B; break;
        case SCENE_STANDBY:     currentScene = &sceneStandby; break;
        default:                currentScene = &sceneMenu; break;
    }
    currentScene->clearNextScene();
    input.clearEvents(); // 清除上一場景之殘留按鍵與手勢邊緣
    currentScene->init();
}

void setup() {
    // 1. 初始化 M5StickC Plus
    M5.begin(true, true, false);

    // 2. 固定螢幕為直向模式 (135x240，搖桿在上方，無需上下反轉)
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(TFT_BLACK);

    // 3. 蜂鳴器硬體防呆與初始化
    audio.begin();

    // 4. 初始化頂部 MiniJoyC 匯流排與 IMU 陀螺儀
    input.begin();

    // 5. 預設適中螢幕亮度 (70%)
    M5.Axp.ScreenBreath(70);

    // 6. 開機畫面與音效
    M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
    M5.Lcd.drawCentreString("FIDGET TOY", SCREEN_WIDTH / 2, 85, 4);
    M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("7-in-1 System", SCREEN_WIDTH / 2, 120, 2);
    audio.playClick();
    led.setColor(255, 200, 50);

    delay(400);

    // 7. 進入全域主選單
    switchScene(SCENE_MENU);
}

void loop() {
    M5.update();
    input.update();

    // 運行當前場景邏輯與繪製
    currentScene->update(input, audio, led);
    currentScene->draw();

    // 檢查場景跳轉請求
    GameScene next = currentScene->getNextScene();
    if (next != SCENE_COUNT) {
        switchScene(next);
    }

    audio.update();
    led.update();

    delay(10);
}
