/**
 * @file SceneStandby.cpp
 * @brief 待機畫面場景實作：Matrix Code Rain、RTC 數位時鐘與低功耗休眠
 */

#include "scenes/SceneStandby.h"

const char* WEEK_DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

SceneStandby::SceneStandby()
    : _mode(STANDBY_MATRIX), _themeIdx(0),
      _lastFrameTime(0), _lastClockCheck(0), _colonBlink(true), _needsRedraw(true), _lastBleAnimTime(0), _bleAnimStep(0), _syncFeedbackEndTime(0) {}

void SceneStandby::initMatrix() {
    for (uint8_t i = 0; i < COL_COUNT; i++) {
        _cols[i].y = (int16_t)EntropyManager::random(-160, 0);
        _cols[i].speed = EntropyManager::random(3, 8);
        _cols[i].length = EntropyManager::random(6, 14);
        _cols[i].nextDropTime = millis() + EntropyManager::random(0, 100);

        for (uint8_t j = 0; j < 20; j++) {
            _cols[i].chars[j] = (char)EntropyManager::random(33, 126);
        }
    }
}

void SceneStandby::init() {
    _mode = STANDBY_MATRIX;
    _themeIdx = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _lastFrameTime = millis();
    _lastClockCheck = 0;

    initMatrix();

    // 進入待機降低螢幕背光至 20%，大幅省電護眼
    M5.Axp.ScreenBreath(20);
    _syncFeedbackEndTime = 0;
    BleSyncManager::getInstance().begin();
}

void SceneStandby::updateMatrix() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < COL_COUNT; i++) {
        if (now >= _cols[i].nextDropTime) {
            _cols[i].nextDropTime = now + (60 - _cols[i].speed * 4);
            _cols[i].y += 8;

            // 隨機變換尾流中的某些字元，增添駭客代碼雨動態感
            if (EntropyManager::random(0, 3) == 0) {
                uint8_t mutateIdx = EntropyManager::random(0, _cols[i].length);
                _cols[i].chars[mutateIdx] = (char)EntropyManager::random(33, 126);
            }

            // 超出螢幕底部重置至上方
            if (_cols[i].y - (_cols[i].length * 8) > SCREEN_HEIGHT) {
                _cols[i].y = (int16_t)EntropyManager::random(-80, 0);
                _cols[i].speed = EntropyManager::random(3, 8);
                _cols[i].length = EntropyManager::random(6, 14);
            }
        }
    }
}

void SceneStandby::update(InputManager& input, AudioManager& audio, LedManager& led) {
    BleSyncManager::getInstance().update();
    if (BleSyncManager::getInstance().hasJustSynced()) {
        audio.playCrit();
        led.flash(0, 255, 0, 3, 70);
        _syncFeedbackEndTime = millis() + 3000;
        M5.Rtc.GetTime(&_time);
        M5.Rtc.GetDate(&_date);
        _needsRedraw = true;
    }
    // 1. 退出機制：按下搖桿中鍵、Button B 或長按均可退出回主選單
    if (input.joyBtnPressed || input.btnBPressed || input.btnBLongPressed) {
        audio.playClick();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 搖桿左右推：切換 Matrix Rain 與 RTC Clock 模式
    if (input.joyPushedLeft || input.joyPushedRight) {
        _mode = (_mode == STANDBY_MATRIX) ? STANDBY_CLOCK : STANDBY_MATRIX;
        audio.playTick();
        _needsRedraw = true;
        return;
    }

    // 3. Button A：切換色彩主題 (Matrix 模式)
    if (input.btnAPressed) {
        if (_mode == STANDBY_MATRIX) {
            _themeIdx = (_themeIdx + 1) % 3;
            audio.playClick();
            _needsRedraw = true;
        }
    }

    uint32_t now = millis();

    if (_mode == STANDBY_MATRIX) {
        // 設定微弱氛圍燈
        if (_themeIdx == 0) led.setColor(0, 40, 0);
        else if (_themeIdx == 1) led.setColor(0, 30, 60);
        else led.setColor(40, 0, 40);

        if (now - _lastFrameTime >= 35) {
            _lastFrameTime = now;
            updateMatrix();
            _needsRedraw = true;
        }
    } else {
        // 時鐘模式：每 500ms 閃爍秒點並檢查時間
        led.setColor(20, 20, 20);
        if (now - _lastClockCheck >= 500) {
            _lastClockCheck = now;
            _colonBlink = !_colonBlink;
            M5.Rtc.GetTime(&_time);
            M5.Rtc.GetDate(&_date);
            _needsRedraw = true;
        }
    }
}

void SceneStandby::drawMatrix() {
    g_canvas.fillSprite(TFT_BLACK);

    uint16_t headColor = TFT_WHITE;
    uint16_t c1, c2, c3, c4;

    if (_themeIdx == 0) { // 經典矩陣綠
        c1 = 0x07E0; c2 = 0x05E0; c3 = 0x03E0; c4 = 0x0200;
    } else if (_themeIdx == 1) { // 冰霜科技藍
        c1 = 0x07FF; c2 = 0x04DF; c3 = 0x029B; c4 = 0x0113;
    } else { // 霓虹龐克紫
        c1 = 0xF81F; c2 = 0xB81B; c3 = 0x7813; c4 = 0x380B;
    }

    for (uint8_t i = 0; i < COL_COUNT; i++) {
        int x = i * 8 + 4;
        int headY = _cols[i].y;

        for (int8_t step = 0; step < _cols[i].length; step++) {
            int y = headY - (step * 8);
            if (y >= 0 && y <= SCREEN_HEIGHT - 8) {
                uint16_t color;
                if (step == 0) color = headColor;
                else if (step < 3) color = c1;
                else if (step < 7) color = c2;
                else if (step < 10) color = c3;
                else color = c4;

                char ch = _cols[i].chars[step % 20];
                g_canvas.setTextColor(color, TFT_BLACK);
                g_canvas.drawChar(ch, x, y, 1);
            }
        }
    }

    // 底部浮水印提示
    g_canvas.setTextColor(0x4208, TFT_BLACK);
    g_canvas.drawCentreString("[STANDBY] PRESS JOY TO WAKE", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, 1);
}

void SceneStandby::drawClock() {
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部裝飾條與日期 (Y: 25 ~ 50)
    g_canvas.fillRect(10, 25, SCREEN_WIDTH - 20, 24, 0x18C3);
    char dateStr[24];
    snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d %s",
             _date.Year, _date.Month, _date.Date,
             WEEK_DAYS[_date.WeekDay % 7]);
    g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
    g_canvas.drawCentreString(dateStr, SCREEN_WIDTH / 2, 30, 2);

    // 2. 中央大字體時間 (Y: 75 ~ 125)
    char timeStr[10];
    if (_colonBlink) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", _time.Hours, _time.Minutes);
    } else {
        snprintf(timeStr, sizeof(timeStr), "%02d %02d", _time.Hours, _time.Minutes);
    }
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString(timeStr, SCREEN_WIDTH / 2, 80, 6);

    // 3. 秒數進度條與小秒數 (Y: 135 ~ 160)
    char secStr[10];
    snprintf(secStr, sizeof(secStr), ".%02ds", _time.Seconds);
    g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(secStr, SCREEN_WIDTH / 2, 138, 2);

    // 圓角進度條 (60 秒平滑推進)
    int barW = SCREEN_WIDTH - 30;
    int progressW = (barW * _time.Seconds) / 60;
    g_canvas.drawRoundRect(15, 158, barW, 6, 2, 0x39E7);
    if (progressW > 0) {
        g_canvas.fillRoundRect(15, 158, progressW, 6, 2, TFT_GREEN);
    }

    // 4. 電量、BLE 與時間同步狀態 (Y: 172 ~ 205)
    float vbat = M5.Axp.GetBatVoltage();
    float vbus = M5.Axp.GetVBusVoltage();
    bool isChg = (vbus > 4.2f);
    char pwrStr[24];
    if (isChg) {
        snprintf(pwrStr, sizeof(pwrStr), "BAT: %.2fV [CHG]", vbat);
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    } else {
        snprintf(pwrStr, sizeof(pwrStr), "BAT: %.2fV", vbat);
        g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    }
    g_canvas.drawCentreString(pwrStr, SCREEN_WIDTH / 2, 174, 1);

    // 藍牙同步狀態條
    uint32_t now = millis();
    if (_syncFeedbackEndTime > 0 && now < _syncFeedbackEndTime) {
        g_canvas.fillRoundRect(12, 188, SCREEN_WIDTH - 24, 16, 3, 0x03E0);
        g_canvas.setTextColor(TFT_WHITE, 0x03E0);
        g_canvas.drawCentreString("BLE TIME SYNCED!", SCREEN_WIDTH / 2, 192, 1);
    } else {
        bool bleConn = BleSyncManager::getInstance().isConnected();
        if (bleConn) {
            g_canvas.fillRoundRect(12, 188, SCREEN_WIDTH - 24, 16, 3, 0x0215);
            g_canvas.setTextColor(COLOR_CYAN, 0x0215);
            g_canvas.drawCentreString("BLE CONNECTED", SCREEN_WIDTH / 2, 192, 1);
        } else {
            g_canvas.setTextColor(0x7BEF, TFT_BLACK);
            g_canvas.drawCentreString("BLE: M5StickC-Fidget", SCREEN_WIDTH / 2, 192, 1);
        }
    }
    // 5. 底部操作說明 (Y: 210 ~ 235)
    g_canvas.drawFastHLine(10, 208, SCREEN_WIDTH - 20, 0x2965);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy L/R: Matrix / Clock", SCREEN_WIDTH / 2, 214, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Press Joy to Exit", SCREEN_WIDTH / 2, 226, 1);
}

void SceneStandby::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    if (_mode == STANDBY_MATRIX) {
        drawMatrix();
    } else {
        drawClock();
    }

    // 一次性推送整幀畫面至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
