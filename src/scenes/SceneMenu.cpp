/**
 * @file SceneMenu.cpp
 * @brief 全域主選單場景實作：支援 Button B 切換靜音、搖桿左右切換亮度、3卡片平滑滾動視窗
 */

#include "scenes/SceneMenu.h"
#include "EntropyManager.h"

struct MenuItem {
    const char* name;
    const char* subtext;
    uint16_t color;
    GameScene scene;
};

const MenuItem MENU_ITEMS[] = {
    {"DICE ROLLER", "1d4 - 6d100", COLOR_GOLD, SCENE_DICE},
    {"POKER DRAW", "52 + 2 JOKERS", TFT_RED, SCENE_POKER},
    {"MAGIC 8-BALL", "20 ORACLES", COLOR_PURPLE, SCENE_EIGHT_BALL},
    {"ROULETTE", "EUROPEAN 0-36", COLOR_CYAN, SCENE_ROULETTE},
    {"SLOT 3x3", "PULL JOY DOWN", TFT_GREEN, SCENE_SLOT},
    {"COIN TOSS", "1-5 COINS", COLOR_GOLD, SCENE_COIN},
    {"ROCK PAPER SCIS", "1-2 HANDS DUEL", 0xFBE0, SCENE_RPS},
    {"1A2B PUZZLE", "GUESS 4 DIGITS", COLOR_LIGHT_BLUE, SCENE_1A2B},
    {"STANDBY CLOCK", "RAIN & RTC TIME", 0x4A69, SCENE_STANDBY},
    {"SENSOR LAB", "LEVEL/G/RF/LED", COLOR_CYAN, SCENE_SENSOR_LAB},
    {"SPECTRUM FFT", "AUDIO & IMU VIBE", TFT_MAGENTA, SCENE_SPECTRUM}
};
const uint8_t MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
const uint8_t VISIBLE_CARDS = 3; // 一頁顯示 3 張寬敞大卡片，告別文字擠壓

const uint8_t BRIGHTNESS_VALUES[] = {35, 70, 100};

SceneMenu::SceneMenu()
    : _selectedIdx(0), _topIdx(0), _needsRedraw(true), _brightnessLevel(1), _cachedMuteState(false),
      _lastBatCheckTime(0), _cachedBatPct(100), _cachedIsCharging(false) {}

void SceneMenu::applyBrightness() {
    M5.Axp.ScreenBreath(BRIGHTNESS_VALUES[_brightnessLevel]);
}

void SceneMenu::updateBatteryInfo() {
    float vbat = M5.Axp.GetBatVoltage();
    float vbus = M5.Axp.GetVBusVoltage();
    _cachedIsCharging = (vbus > 4.2f);

    uint8_t pct = 0;
    if (vbat >= 4.12f) pct = 100;
    else if (vbat >= 4.00f) pct = 90 + (uint8_t)((vbat - 4.00f) / 0.12f * 10.0f);
    else if (vbat >= 3.85f) pct = 65 + (uint8_t)((vbat - 3.85f) / 0.15f * 25.0f);
    else if (vbat >= 3.70f) pct = 30 + (uint8_t)((vbat - 3.70f) / 0.15f * 35.0f);
    else if (vbat >= 3.55f) pct = 10 + (uint8_t)((vbat - 3.55f) / 0.15f * 20.0f);
    else if (vbat >= 3.40f) pct = 2 + (uint8_t)((vbat - 3.40f) / 0.15f * 8.0f);
    else pct = 0;

    if (pct > 100) pct = 100;
    _cachedBatPct = pct;
}

void SceneMenu::drawBatteryIcon(int x, int y, uint8_t pct, bool charging) {
    // 繪製小電池外框 (寬 14px，高 8px)
    M5.Lcd.drawRoundRect(x, y, 13, 8, 1, TFT_LIGHTGREY);
    M5.Lcd.drawFastVLine(x + 13, y + 2, 4, TFT_LIGHTGREY);

    int fillW = (pct * 9) / 100;
    if (fillW < 1 && pct > 0) fillW = 1;
    if (fillW > 9) fillW = 9;

    uint16_t barColor = charging ? COLOR_CYAN :
                        (pct > 50) ? TFT_GREEN :
                        (pct > 20) ? COLOR_GOLD : TFT_RED;

    if (fillW > 0) {
        M5.Lcd.fillRect(x + 2, y + 2, fillW, 4, barColor);
    }
}

void SceneMenu::init() {
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    applyBrightness();
    updateBatteryInfo();
    _lastBatCheckTime = millis();
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneMenu::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 短按：切換聲音開關 (靜音 / 啟用)
    if (input.btnBPressed) {
        audio.toggleMute();
        _cachedMuteState = audio.isMuted();
        _needsRedraw = true;
    }

    // 2. 搖桿上下推：切換選中遊戲項目與視窗滾動
    if (input.joyPushedUp) {
        if (_selectedIdx > 0) {
            _selectedIdx--;
        } else {
            _selectedIdx = MENU_COUNT - 1;
        }
        // 視窗滾動追隨
        if (_selectedIdx < _topIdx) {
            _topIdx = _selectedIdx;
        } else if (_selectedIdx == MENU_COUNT - 1) {
            _topIdx = MENU_COUNT - VISIBLE_CARDS;
        }
        audio.playTick();
        _needsRedraw = true;
    } else if (input.joyPulledDown) {
        if (_selectedIdx < MENU_COUNT - 1) {
            _selectedIdx++;
        } else {
            _selectedIdx = 0;
        }
        // 視窗滾動追隨
        if (_selectedIdx >= _topIdx + VISIBLE_CARDS) {
            _topIdx = _selectedIdx - VISIBLE_CARDS + 1;
        } else if (_selectedIdx == 0) {
            _topIdx = 0;
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
        _selectedIdx = EntropyManager::random(0, MENU_COUNT);
        if (_selectedIdx < _topIdx) {
            _topIdx = _selectedIdx;
        } else if (_selectedIdx >= _topIdx + VISIBLE_CARDS) {
            _topIdx = _selectedIdx - VISIBLE_CARDS + 1;
        }
        audio.playDiceRoll();
        _needsRedraw = true;
    }

    // 5. 按下中心鍵或 Button A：確認進入遊戲
    if (input.joyBtnPressed || input.btnAPressed) {
        audio.playClick();
        _nextScene = MENU_ITEMS[_selectedIdx].scene;
        return;
    }

    // 6. 每 1500ms 定時更新電量百分比
    uint32_t now = millis();
    if (now - _lastBatCheckTime > 1500) {
        _lastBatCheckTime = now;
        uint8_t prevPct = _cachedBatPct;
        bool prevChg = _cachedIsCharging;
        updateBatteryInfo();
        if (prevPct != _cachedBatPct || prevChg != _cachedIsCharging) {
            _needsRedraw = true;
        }
    }

    // 7. 全域無操作閒置 60 秒：自動進入待機休眠
    if (now - input.lastActivityTime > 60000) {
        _nextScene = SCENE_STANDBY;
        return;
    }

    // 設定 LED 燈色配合當前選中遊戲
    led.setHexColor(
        (_selectedIdx == 0) ? 0xFFAA00 :
        (_selectedIdx == 1) ? 0xFF2222 :
        (_selectedIdx == 2) ? 0x9900FF :
        (_selectedIdx == 3) ? 0x00D0FF :
        (_selectedIdx == 4) ? 0x00FF33 :
        (_selectedIdx == 5) ? 0xFFCC00 :
        (_selectedIdx == 6) ? 0xFF8800 :
        (_selectedIdx == 7) ? 0x00AAFF :
        (_selectedIdx == 8) ? 0x00FF88 :
        (_selectedIdx == 9) ? 0x00D0FF :
                              0xFF00AA
    );
}

void SceneMenu::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    M5.Lcd.fillScreen(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 34)
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 34, 0x18C3);

    // 第一行：左側系統名稱，右側電量百分比與圖標
    M5.Lcd.setTextColor(TFT_WHITE, 0x18C3);
    M5.Lcd.drawString("FIDGET OS", 6, 3, 2);

    char batStr[10];
    if (_cachedIsCharging) {
        snprintf(batStr, sizeof(batStr), "+%d%%", _cachedBatPct);
    } else {
        snprintf(batStr, sizeof(batStr), "%d%%", _cachedBatPct);
    }
    uint16_t batTxtColor = _cachedIsCharging ? COLOR_CYAN :
                           (_cachedBatPct > 50) ? TFT_GREEN :
                           (_cachedBatPct > 20) ? COLOR_GOLD : TFT_RED;
    M5.Lcd.setTextColor(batTxtColor, 0x18C3);
    M5.Lcd.drawRightString(batStr, SCREEN_WIDTH - 21, 3, 2);
    drawBatteryIcon(SCREEN_WIDTH - 19, 7, _cachedBatPct, _cachedIsCharging);

    // 第二行：左側聲音狀態 [SND/MUTE]，右側亮度 BRT: 70%
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawString(_cachedMuteState ? "[MUTE]" : "[SND ON]", 6, 21, 1);
    char brtStr[14];
    snprintf(brtStr, sizeof(brtStr), "BRT: %d%%", BRIGHTNESS_VALUES[_brightnessLevel]);
    M5.Lcd.drawRightString(brtStr, SCREEN_WIDTH - 6, 21, 1);

    // 2. 中央大卡片區 (Y: 39 ~ 183，3 個卡片各高 42px，間距 6px)
    int cardW = SCREEN_WIDTH - 18; // 留 6px 給右側滾動條

    for (uint8_t r = 0; r < VISIBLE_CARDS; r++) {
        uint8_t itemIdx = _topIdx + r;
        if (itemIdx >= MENU_COUNT) break;

        int y = 39 + r * 48;
        bool isSel = (itemIdx == _selectedIdx);

        if (isSel) {
            M5.Lcd.fillRoundRect(6, y, cardW, 42, 4, MENU_ITEMS[itemIdx].color);
            M5.Lcd.setTextColor(TFT_BLACK, MENU_ITEMS[itemIdx].color);
            M5.Lcd.drawString(MENU_ITEMS[itemIdx].name, 12, y + 4, 2);
            M5.Lcd.setTextColor(0x18C3, MENU_ITEMS[itemIdx].color);
            M5.Lcd.drawString(MENU_ITEMS[itemIdx].subtext, 12, y + 23, 1);

            M5.Lcd.fillTriangle(
                cardW - 8, y + 14,
                cardW - 8, y + 28,
                cardW - 2, y + 21,
                TFT_BLACK
            );
        } else {
            M5.Lcd.drawRoundRect(6, y, cardW, 42, 4, 0x39E7);
            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[itemIdx].name, 12, y + 4, 2);
            M5.Lcd.setTextColor(0x7BEF, TFT_BLACK);
            M5.Lcd.drawString(MENU_ITEMS[itemIdx].subtext, 12, y + 23, 1);
        }
    }

    // 3. 右側垂直捲動條 (Y: 39 ~ 177，總高 138px)
    int trackX = SCREEN_WIDTH - 8;
    int trackY = 39;
    int trackH = 138;
    M5.Lcd.drawFastVLine(trackX + 1, trackY, trackH, 0x2965);

    int thumbH = (trackH * VISIBLE_CARDS) / MENU_COUNT; // 約 59px
    int maxTop = MENU_COUNT - VISIBLE_CARDS;
    int thumbY = trackY + (_topIdx * (trackH - thumbH)) / maxTop;
    M5.Lcd.fillRoundRect(trackX, thumbY, 3, thumbH, 1, COLOR_CYAN);

    // 4. 底部操作指引 (Y: 190 ~ 238)
    M5.Lcd.drawFastHLine(8, 190, SCREEN_WIDTH - 16, 0x39E7);

    char enterStr[32];
    snprintf(enterStr, sizeof(enterStr), "[PRESS A / JOY] (%d/%d)", _selectedIdx + 1, MENU_COUNT);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Lcd.drawCentreString(enterStr, SCREEN_WIDTH / 2, 196, 1);

    M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Lcd.drawCentreString("BtnB: Mute  Joy L/R: Bright", SCREEN_WIDTH / 2, 210, 1);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.drawCentreString("Shake to Random", SCREEN_WIDTH / 2, 224, 1);
}
