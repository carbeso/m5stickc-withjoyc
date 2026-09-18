/**
 * @file SceneMenu.cpp
 * @brief 全域主選單場景實作：支援 Button B 切換靜音、搖桿左右切換亮度
 */

#include "scenes/SceneMenu.h"

struct MenuItem {
    const char* name;
    const char* subtext;
    uint16_t color;
    GameScene scene;
};

const MenuItem MENU_ITEMS[] = {
    {"DICE ROLLER", "1d4 - 6d100", COLOR_GOLD, SCENE_DICE},
    {"POKER DRAW", "52 + 2 JOKERS", TFT_RED, SCENE_POKER},
    {"MAGIC 8-BALL", "ZH-TW ORACLE", COLOR_PURPLE, SCENE_EIGHT_BALL},
    {"ROULETTE", "EUROPEAN 0-36", COLOR_CYAN, SCENE_ROULETTE},
    {"SLOT 3x3", "PULL JOY DOWN", TFT_GREEN, SCENE_SLOT}
};
const uint8_t MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

const uint8_t BRIGHTNESS_VALUES[] = {35, 70, 100};

SceneMenu::SceneMenu()
    : _selectedIdx(0), _needsRedraw(true), _brightnessLevel(1), _cachedMuteState(false) {}

void SceneMenu::applyBrightness() {
    M5.Axp.ScreenBreath(BRIGHTNESS_VALUES[_brightnessLevel]);
}

void SceneMenu::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    applyBrightness();
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneMenu::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 短按：切換聲音開關 (靜音 / 啟用)
    if (input.btnBPressed) {
        audio.toggleMute();
        _cachedMuteState = audio.isMuted();
        _needsRedraw = true;
    }

    // 2. 搖桿上下推：切換選中遊戲項目
    if (input.joyPushedUp) {
        if (_selectedIdx > 0) {
            _selectedIdx--;
        } else {
            _selectedIdx = MENU_COUNT - 1;
        }
        audio.playTick();
        _needsRedraw = true;
    } else if (input.joyPulledDown) {
        if (_selectedIdx < MENU_COUNT - 1) {
            _selectedIdx++;
        } else {
            _selectedIdx = 0;
        }
        audio.playTick();
        _needsRedraw = true;
    }

    // 3. 搖桿左右推：調整螢幕亮度 (35% -> 70% -> 100%)
    if (input.joyPushedLeft) {
        if (_brightnessLevel > 0) _brightnessLevel--;
        else _brightnessLevel = 2;
        applyBrightness();
        audio.playTick();
        _needsRedraw = true;
    } else if (input.joyPushedRight) {
        if (_brightnessLevel < 2) _brightnessLevel++;
        else _brightnessLevel = 0;
        applyBrightness();
        audio.playTick();
        _needsRedraw = true;
    }

    // 4. 晃動機身：隨機選取遊戲
    if (input.isShaken) {
        _selectedIdx = random(0, MENU_COUNT);
        audio.playDiceRoll();
        _needsRedraw = true;
    }

    // 5. 按下中心鍵或 Button A：確認進入遊戲
    if (input.joyBtnPressed || input.btnAPressed) {
        audio.playClick();
        _nextScene = MENU_ITEMS[_selectedIdx].scene;
        return;
    }

    // 設定 LED 燈色配合當前選中遊戲
    led.setHexColor(
        (_selectedIdx == 0) ? 0xFFAA00 :
        (_selectedIdx == 1) ? 0xFF2222 :
        (_selectedIdx == 2) ? 0x9900FF :
        (_selectedIdx == 3) ? 0x00D0FF :
                              0x00FF33
    );
}

void SceneMenu::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 34)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 32, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("FIDGET OS", 8, 4, 2);

    // 顯示聲音與亮度狀態 (例如 [SND 70%])
    char sysInfo[18];
    snprintf(sysInfo, sizeof(sysInfo), "%s %d%%",
             _cachedMuteState ? "[MUTE]" : "[SND]",
             BRIGHTNESS_VALUES[_brightnessLevel]);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawRightString(sysInfo, SCREEN_WIDTH - 6, 6, 1);

    // 2. 中央卡片區 (Y: 38 ~ 188)
    for (uint8_t i = 0; i < MENU_COUNT; i++) {
        int y = 38 + i * 30;
        bool isSel = (i == _selectedIdx);

        if (isSel) {
            M5.Lcd.fillRoundRect(6, y, SCREEN_WIDTH - 12, 27, 4, MENU_ITEMS[i].color);
            M5.Lcd.setTextColor(TFT_BLACK, MENU_ITEMS[i].color);
            M5.Lcd.drawString(MENU_ITEMS[i].name, 12, y + 3, 2);
            M5.Lcd.setTextColor(0x18C3, MENU_ITEMS[i].color);
            M5.Lcd.drawString(MENU_ITEMS[i].subtext, 12, y + 17, 1);

            M5.Lcd.fillTriangle(
                SCREEN_WIDTH - 16, y + 8,
                SCREEN_WIDTH - 16, y + 19,
                SCREEN_WIDTH - 9, y + 13,
                TFT_BLACK
            );
        } else {
            M5.Lcd.drawRoundRect(6, y, SCREEN_WIDTH - 12, 27, 4, 0x39E7);
            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[i].name, 12, y + 3, 2);
            M5.Lcd.setTextColor(0x7BEF, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[i].subtext, 12, y + 17, 1);
        }
    }

    // 3. 底部操作指引 (Y: 196 ~ 238)
    M5.Lcd.drawFastHLine(8, 194, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("[PRESS A / JOY] ENTER", SCREEN_WIDTH / 2, 200, 1);
    M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Lcd.drawCentreString("BtnB: Mute  Joy L/R: Bright", SCREEN_WIDTH / 2, 214, 1);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Shake to Random", SCREEN_WIDTH / 2, 226, 1);
}
