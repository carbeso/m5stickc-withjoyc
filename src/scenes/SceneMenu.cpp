/**
 * @file SceneMenu.cpp
 * @brief 全域主選單場景實作：直式卡片瀏覽 5 大遊戲
 */

#include "scenes/SceneMenu.h"

struct MenuItem {
    const char* name;
    const char* subtext;
    uint16_t color;
    GameScene scene;
};

const MenuItem MENU_ITEMS[] = {
    {"DICE ROLLER", "1d6 - 6d20", COLOR_GOLD, SCENE_DICE},
    {"POKER DRAW", "52 + 2 JOKERS", TFT_RED, SCENE_POKER},
    {"MAGIC 8-BALL", "ZH-TW ORACLE", COLOR_PURPLE, SCENE_EIGHT_BALL},
    {"ROULETTE", "EUROPEAN 0-36", COLOR_CYAN, SCENE_ROULETTE},
    {"SLOT 3x3", "PULL JOY DOWN", TFT_GREEN, SCENE_SLOT}
};
const uint8_t MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

SceneMenu::SceneMenu()
    : _selectedIdx(0), _needsRedraw(true), _lastAnimTime(0), _pulse(0) {}

void SceneMenu::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneMenu::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 搖桿切換
    if (input.joyPushedUp || input.joyPushedLeft) {
        if (_selectedIdx > 0) {
            _selectedIdx--;
        } else {
            _selectedIdx = MENU_COUNT - 1;
        }
        audio.playTick();
        _needsRedraw = true;
    } else if (input.joyPulledDown || input.joyPushedRight) {
        if (_selectedIdx < MENU_COUNT - 1) {
            _selectedIdx++;
        } else {
            _selectedIdx = 0;
        }
        audio.playTick();
        _needsRedraw = true;
    }

    // 晃動機身：隨機選取遊戲
    if (input.isShaken) {
        _selectedIdx = random(0, MENU_COUNT);
        audio.playDiceRoll();
        _needsRedraw = true;
    }

    // 按下中心鍵或 Button A：確認進入遊戲
    if (input.joyBtnPressed || input.btnAPressed) {
        audio.playClick();
        _nextScene = MENU_ITEMS[_selectedIdx].scene;
        return;
    }

    // 設定 LED 燈色配合當前選中遊戲
    led.setHexColor(
        (_selectedIdx == 0) ? 0xFFAA00 : // 骰子 金黃
        (_selectedIdx == 1) ? 0xFF2222 : // 撲克 艷紅
        (_selectedIdx == 2) ? 0x9900FF : // 八號球 神秘紫
        (_selectedIdx == 3) ? 0x00D0FF : // 輪盤 青藍
                              0x00FF33   // 老虎機 翠綠
    );
}

void SceneMenu::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部標題區 (Y: 0 ~ 36)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 34, 0x18C3);
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawCentreString("FIDGET OS", SCREEN_WIDTH / 2, 6, 2);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, 0x18C3);
    M5.Lcd.drawCentreString("SELECT GAME", SCREEN_WIDTH / 2, 22, 1);

    // 2. 中央卡片區 (Y: 42 ~ 190)
    for (uint8_t i = 0; i < MENU_COUNT; i++) {
        int y = 42 + i * 29;
        bool isSel = (i == _selectedIdx);

        if (isSel) {
            // 選中項：突出高亮邊框與填色
            M5.Lcd.fillRoundRect(6, y, SCREEN_WIDTH - 12, 26, 4, MENU_ITEMS[i].color);
            M5.Lcd.setTextColor(TFT_BLACK, MENU_ITEMS[i].color);
            M5.Lcd.drawString(MENU_ITEMS[i].name, 12, y + 3, 2);
            M5.Lcd.setTextColor(0x18C3, MENU_ITEMS[i].color);
            M5.Lcd.drawString(MENU_ITEMS[i].subtext, 12, y + 16, 1);

            // 右側箭頭指示
            M5.Lcd.fillTriangle(
                SCREEN_WIDTH - 16, y + 8,
                SCREEN_WIDTH - 16, y + 18,
                SCREEN_WIDTH - 10, y + 13,
                TFT_BLACK
            );
        } else {
            // 未選中項：暗色線框
            M5.Lcd.drawRoundRect(6, y, SCREEN_WIDTH - 12, 26, 4, 0x39E7);
            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[i].name, 12, y + 3, 2);
            M5.Lcd.setTextColor(0x7BEF, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[i].subtext, 12, y + 16, 1);
        }
    }

    // 3. 底部操作指引 (Y: 200 ~ 238)
    M5.Lcd.drawFastHLine(8, 204, SCREEN_WIDTH - 16, 0x39E7);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString("[PRESS A / JOY]", SCREEN_WIDTH / 2, 210, 1);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("SHAKE to Random", SCREEN_WIDTH / 2, 224, 1);
}
