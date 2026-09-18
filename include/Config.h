/**
 * @file Config.h
 * @brief M5StickC Plus 與 MiniJoyC 紓壓玩具全域設定與型別定義
 */

#pragma once

#include <Arduino.h>

// --- 硬體腳位定義 ---
#define BUZZER_PIN 2         // StickC 內部無源蜂鳴器接腳
#define HAT_I2C_SDA 0        // 頂部 HAT 槽 I2C SDA
#define HAT_I2C_SCL 26       // 頂部 HAT 槽 I2C SCL
#define MINI_JOYC_ADDR 0x54  // MiniJoyC STM32 協同晶片預設 I2C 位址

// --- 螢幕尺寸定義 (直向模式) ---
#define SCREEN_WIDTH 135
#define SCREEN_HEIGHT 240

// --- 搖桿死區與門檻定義 ---
#define JOY_DEADZONE 25      // 搖桿微動靜止死區
#define JOY_TRIGGER_PULL 80  // 搖桿拉桿觸發門檻 (Y 軸強烈下拉)

// --- 遊戲場景列舉 ---
enum GameScene {
    SCENE_MENU = 0,         // 全域主選單
    SCENE_DICE,             // 遊戲一：多面骰子盒
    SCENE_POKER,            // 遊戲二：極簡幸運撲克
    SCENE_EIGHT_BALL,       // 遊戲三：神秘八號球
    SCENE_ROULETTE,         // 遊戲四：直向垂直幸運輪盤
    SCENE_SLOT,             // 遊戲五：3x3 搖桿下拉角子老虎機
    SCENE_COIN,             // 遊戲六：1~5枚擲硬幣
    SCENE_RPS,              // 遊戲七：剪刀石頭布 (Rock Paper Scissors)
    SCENE_COUNT
};

// --- 色彩常數定義 (RGB565) ---
#define COLOR_BG        0x0000 // 純黑背景
#define COLOR_GOLD      0xFEA0 // 金黃色
#define COLOR_SILVER    0xCE79 // 亮銀灰
#define COLOR_CYAN      0x07FF // 科技青
#define COLOR_PURPLE    0x915F // 神秘紫
#define COLOR_LIGHT_BLUE 0x5D3F // 淡天藍
#define COLOR_LIGHT_RED  0xFB8E // 淡粉紅
#define COLOR_CARD_RED  0xF800 // 撲克鮮紅
#define COLOR_CARD_WHT  0xFFFF // 撲克純白
#define COLOR_ROU_RED   0xD800 // 輪盤深紅
#define COLOR_ROU_BLK   0x18E3 // 輪盤深黑灰色
#define COLOR_ROU_GRN   0x05E0 // 輪盤翠綠 (0)
