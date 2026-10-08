/**
 * @file SceneWifiScanner.cpp
 * @brief 2.4G Wi-Fi 探測雷達與掃描器 (Wi-Fi Scanner) 場景實作檔
 * @details 進入時啟動 Wi-Fi 掃描，支援搖桿上下滾動查看，退出時立即關閉 Wi-Fi 釋放記憶體
 */

#include "scenes/SceneWifiScanner.h"
#include <cmath>

static const char* getAuthModeStr(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN:            return "OPEN";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "EAP";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
        default:                        return "OTHER";
    }
}

SceneWifiScanner::SceneWifiScanner()
    : _isScanningWifi(false), _foundWifiAps(0), _wifiScrollOffset(0),
      _lastWifiScanTime(0), _needsRedraw(true) {
    for (uint8_t i = 0; i < MAX_WIFI_APS; i++) {
        _wifiAps[i].ssid[0] = '\0';
        _wifiAps[i].rssi = -100;
        _wifiAps[i].channel = 1;
        _wifiAps[i].encryption[0] = '\0';
    }
}

SceneWifiScanner::~SceneWifiScanner() {
    stopWifiScan();
}

void SceneWifiScanner::init() {
    _foundWifiAps = 0;
    _wifiScrollOffset = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    startWifiScan();
}

void SceneWifiScanner::startWifiScan() {
    _isScanningWifi = true;
    _foundWifiAps = 0;
    _wifiScrollOffset = 0;
    _lastWifiScanTime = millis();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanNetworks(true); // 非阻塞非同步掃描
}

void SceneWifiScanner::stopWifiScan() {
    if (_isScanningWifi) {
        WiFi.scanDelete();
        WiFi.mode(WIFI_OFF);
        _isScanningWifi = false;
    } else {
        WiFi.mode(WIFI_OFF);
    }
}

void SceneWifiScanner::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        stopWifiScan();
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 按下搖桿中心鍵：重新掃描
    if (input.joyBtnPressed) {
        audio.playClick();
        startWifiScan();
        _needsRedraw = true;
        return;
    }

    // 3. 掃描中狀態偵測
    if (_isScanningWifi) {
        _needsRedraw = true;
        int16_t n = WiFi.scanComplete();
        if (n >= 0) {
            _isScanningWifi = false;
            _foundWifiAps = (n > MAX_WIFI_APS) ? MAX_WIFI_APS : (uint8_t)n;

            for (uint8_t i = 0; i < _foundWifiAps; i++) {
                String s = WiFi.SSID(i);
                if (s.length() == 0) {
                    snprintf(_wifiAps[i].ssid, sizeof(_wifiAps[i].ssid), "[Hidden SSID]");
                } else {
                    snprintf(_wifiAps[i].ssid, sizeof(_wifiAps[i].ssid), "%s", s.c_str());
                }
                _wifiAps[i].rssi = WiFi.RSSI(i);
                _wifiAps[i].channel = WiFi.channel(i);
                wifi_auth_mode_t enc = WiFi.encryptionType(i);
                snprintf(_wifiAps[i].encryption, sizeof(_wifiAps[i].encryption), "%s", getAuthModeStr(enc));
            }

            // 依 RSSI 降冪排序
            for (uint8_t i = 0; i < _foundWifiAps; i++) {
                for (uint8_t j = i + 1; j < _foundWifiAps; j++) {
                    if (_wifiAps[j].rssi > _wifiAps[i].rssi) {
                        WifiScanResult tmp = _wifiAps[i];
                        _wifiAps[i] = _wifiAps[j];
                        _wifiAps[j] = tmp;
                    }
                }
            }

            WiFi.scanDelete();
            audio.playTick();
            _needsRedraw = true;
        }
        return;
    }

    // 4. 搖桿上下捲動檢視 AP
    if (_foundWifiAps > 4) {
        int maxOffset = _foundWifiAps - 4;
        static uint32_t lastScrollTick = 0;
        uint32_t now = millis();

        if (input.joyPulledDown || (input.joyY > 50 && now - lastScrollTick > 180)) {
            if (_wifiScrollOffset < maxOffset) {
                _wifiScrollOffset++;
                audio.playTick();
                _needsRedraw = true;
                lastScrollTick = now;
            }
        } else if (input.joyPushedUp || (input.joyY < -50 && now - lastScrollTick > 180)) {
            if (_wifiScrollOffset > 0) {
                _wifiScrollOffset--;
                audio.playTick();
                _needsRedraw = true;
                lastScrollTick = now;
            }
        }
    }
}

void SceneWifiScanner::drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor) {
    int centerX = SCREEN_WIDTH / 2;
    int centerY = 112;
    int radius = 45;

    g_canvas.setTextColor(primaryColor, TFT_BLACK);
    g_canvas.drawCentreString(title, centerX, 30, 2);
    g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(subtitle, centerX, 50, 1);

    g_canvas.drawCircle(centerX, centerY, radius, primaryColor);
    g_canvas.drawCircle(centerX, centerY, radius - 1, 0x18C3);
    g_canvas.drawCircle(centerX, centerY, 15, 0x1183);
    g_canvas.drawCircle(centerX, centerY, 30, 0x19E3);

    g_canvas.drawFastHLine(centerX - radius + 2, centerY, (radius - 2) * 2, 0x1183);
    g_canvas.drawFastVLine(centerX, centerY - radius + 2, (radius - 2) * 2, 0x1183);

    uint32_t tick = millis();
    int rip1 = (tick / 20) % radius;
    int rip2 = ((tick / 20) + radius / 2) % radius;
    if (rip1 > 2) g_canvas.drawCircle(centerX, centerY, rip1, 0x2286);
    if (rip2 > 2) g_canvas.drawCircle(centerX, centerY, rip2, 0x2286);

    float currentDeg = (float)((tick / 4) % 360);
    float rad = currentDeg * 0.0174532925f;

    for (int i = 3; i >= 1; i--) {
        float trailRad = (currentDeg - i * 3.5f) * 0.0174532925f;
        int tx = centerX + (int)(cosf(trailRad) * (radius - 3));
        int ty = centerY + (int)(sinf(trailRad) * (radius - 3));
        uint16_t fadeColor = (i == 1) ? 0x23E6 : (i == 2) ? 0x1344 : 0x0982;
        g_canvas.drawLine(centerX, centerY, tx, ty, fadeColor);
    }

    int endX = centerX + (int)(cosf(rad) * (radius - 2));
    int endY = centerY + (int)(sinf(rad) * (radius - 2));
    g_canvas.drawLine(centerX, centerY, endX, endY, TFT_WHITE);
    g_canvas.drawPixel(endX, endY, primaryColor);

    if ((tick / 200) % 2 == 0) {
        g_canvas.fillCircle(centerX + 18, centerY - 14, 2, primaryColor);
        g_canvas.fillCircle(centerX - 16, centerY + 20, 2, TFT_WHITE);
    }

    int dotCount = (tick / 250) % 4;
    char dotBuf[5] = "...";
    dotBuf[dotCount] = '\0';
    char statusBuf[32];
    snprintf(statusBuf, sizeof(statusBuf), "SCANNING%s", dotBuf);
    g_canvas.setTextColor(primaryColor, TFT_BLACK);
    g_canvas.drawCentreString(statusBuf, centerX, 168, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Detecting 2.4G APs...", centerX, 184, 1);
}

void SceneWifiScanner::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    if (_isScanningWifi) {
        drawRadarAnimation("WIFI SCANNER", "Scanning 2.4GHz Band...", TFT_GREEN);
        g_canvas.pushSprite(0, 0);
        return;
    }

    int centerX = SCREEN_WIDTH / 2;

    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "Wi-Fi APs: %d", _foundWifiAps);
    g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    g_canvas.drawString(titleBuf, 8, 10, 2);

    if (_foundWifiAps > 0) {
        char pageBuf[16];
        snprintf(pageBuf, sizeof(pageBuf), "%d-%d", _wifiScrollOffset + 1, min((int)_wifiScrollOffset + 4, (int)_foundWifiAps));
        g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
        g_canvas.drawRightString(pageBuf, SCREEN_WIDTH - 8, 12, 1);
    }

    if (_foundWifiAps == 0) {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("No Networks Found", centerX, 95, 2);
        g_canvas.drawCentreString("Click Joy to Scan", centerX, 115, 1);
    } else {
        uint8_t visibleCount = min((uint8_t)4, (uint8_t)(_foundWifiAps - _wifiScrollOffset));
        for (uint8_t i = 0; i < visibleCount; i++) {
            uint8_t idx = _wifiScrollOffset + i;
            int y = 32 + i * 42;

            g_canvas.fillRoundRect(6, y, 116, 38, 3, 0x0841);
            g_canvas.drawRoundRect(6, y, 116, 38, 3, 0x2104);

            char shortSsid[16];
            snprintf(shortSsid, sizeof(shortSsid), "%.13s", _wifiAps[idx].ssid);
            g_canvas.setTextColor(TFT_WHITE, 0x0841);
            g_canvas.drawString(shortSsid, 10, y + 4, 1);

            char rssiStr[16];
            snprintf(rssiStr, sizeof(rssiStr), "%ddB", (int)_wifiAps[idx].rssi);
            uint16_t sigColor = (_wifiAps[idx].rssi > -60) ? TFT_GREEN :
                                (_wifiAps[idx].rssi > -75) ? COLOR_GOLD : TFT_RED;
            g_canvas.setTextColor(sigColor, 0x0841);
            g_canvas.drawRightString(rssiStr, 118, y + 4, 1);

            char detailStr[24];
            snprintf(detailStr, sizeof(detailStr), "CH:%d [%s]", (int)_wifiAps[idx].channel, _wifiAps[idx].encryption);
            g_canvas.setTextColor(COLOR_SILVER, 0x0841);
            g_canvas.drawString(detailStr, 10, y + 22, 1);

            int bars = (_wifiAps[idx].rssi > -55) ? 4 :
                       (_wifiAps[idx].rssi > -70) ? 3 :
                       (_wifiAps[idx].rssi > -85) ? 2 : 1;
            for (int b = 0; b < 4; b++) {
                int barH = 2 + b * 2;
                int bx = 104 + b * 4;
                int by = y + 33 - barH;
                uint16_t bc = (b < bars) ? sigColor : 0x2104;
                g_canvas.fillRect(bx, by, 3, barH, bc);
            }
        }

        if (_foundWifiAps > 4) {
            int trackH = 160;
            int thumbH = max(16, trackH * 4 / _foundWifiAps);
            int thumbY = 32 + (_wifiScrollOffset * (trackH - thumbH)) / (_foundWifiAps - 4);
            g_canvas.drawFastVLine(128, 32, trackH, 0x18C3);
            g_canvas.fillRect(127, thumbY, 3, thumbH, COLOR_GOLD);
        }
    }

    g_canvas.drawFastHLine(10, 206, SCREEN_WIDTH - 20, 0x18C3);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy Click: Re-Scan", centerX, 210, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", centerX, 224, 1);

    g_canvas.pushSprite(0, 0);
}

void SceneWifiScanner::exit() {
    stopWifiScan();
}
