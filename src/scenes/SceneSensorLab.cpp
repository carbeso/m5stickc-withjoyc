/**
 * @file SceneSensorLab.cpp
 * @brief 感測器實驗室 (Sensor Lab) 儀表板實作：水平儀、G-Force 追蹤、2.4G Wi-Fi 掃描、BLE 藍牙掃描與 LED 工作室
 */

#include "scenes/SceneSensorLab.h"
#include <cmath>

// Wi-Fi 加密類型轉文字描述
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

// HSV 轉 RGB 通用函式
static void hsvToRgb(int h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf((float)h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf = 0, gf = 0, bf = 0;

    if (h < 60) { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else { rf = c; gf = 0; bf = x; }

    r = (uint8_t)((rf + m) * 255.0f);
    g = (uint8_t)((gf + m) * 255.0f);
    b = (uint8_t)((bf + m) * 255.0f);
}

// BLE 廣播設備回呼類別
class LabBleCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
    LabBleCallbacks(SceneSensorLab* lab) : _lab(lab) {}
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (_lab) {
            std::string name = advertisedDevice.getName();
            std::string addr = advertisedDevice.getAddress().toString();
            int rssi = advertisedDevice.getRSSI();
            _lab->onBleDeviceFound(name.c_str(), addr.c_str(), rssi);
        }
    }
private:
    SceneSensorLab* _lab;
};

static LabBleCallbacks* s_pBleCallbacks = nullptr;

SceneSensorLab::SceneSensorLab()
    : _currentTab(TAB_LEVEL), _needsRedraw(true), _lastSampleTime(0),
      _bubbleX(0), _bubbleY(0), _targetBubbleX(0), _targetBubbleY(0), _isCentered(false), _lastCenterTick(0),
      _gHistIdx(0), _currentG(1.0f), _peakG(1.0f), _peakAx(0), _peakAy(0),
      _isScanningWifi(false), _foundWifiAps(0), _wifiScrollOffset(0), _lastWifiScanTime(0),
      _isScanningBle(false), _bleInitialized(false), _foundBleDevs(0), _bleScrollOffset(0), _bleScanStartTime(0),
      _hue(180), _brightness(80), _ledR(0), _ledG(200), _ledB(200) {
    for (uint8_t i = 0; i < G_HIST_SIZE; i++) _gHistory[i] = 1.0f;
    for (uint8_t i = 0; i < MAX_WIFI_APS; i++) {
        _wifiAps[i].ssid[0] = '\0';
        _wifiAps[i].rssi = -100;
        _wifiAps[i].channel = 1;
        _wifiAps[i].encryption[0] = '\0';
    }
    for (uint8_t i = 0; i < MAX_BLE_DEVS; i++) {
        _bleDevs[i].name[0] = '\0';
        _bleDevs[i].address[0] = '\0';
        _bleDevs[i].rssi = -100;
    }
}

SceneSensorLab::~SceneSensorLab() {
    stopWifiScan();
    stopBleScan();
}

void SceneSensorLab::init() {
    _currentTab = TAB_LEVEL;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _lastSampleTime = millis();
    _peakG = 1.0f;
    _isScanningWifi = false;
    _isScanningBle = false;

    // 預設關閉 Wi-Fi 射頻以節省功耗
    WiFi.mode(WIFI_OFF);
}

// ----------------------------------------------------
// 水平儀 (Level) 邏輯與繪製
// ----------------------------------------------------
void SceneSensorLab::updateLevel(InputManager& input, AudioManager& audio) {
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getAccelData(&ax, &ay, &az);

    _targetBubbleX = -ax * 42.0f;
    _targetBubbleY = ay * 42.0f;

    _bubbleX += (_targetBubbleX - _bubbleX) * 0.25f;
    _bubbleY += (_targetBubbleY - _bubbleY) * 0.25f;

    float distSq = _bubbleX * _bubbleX + _bubbleY * _bubbleY;
    bool centeredNow = (distSq < 36.0f); // 半徑 6px 內判定完美水平

    uint32_t now = millis();
    if (centeredNow && !_isCentered && (now - _lastCenterTick > 800)) {
        audio.playTick();
        _lastCenterTick = now;
    }
    _isCentered = centeredNow;
    _needsRedraw = true;
}

void SceneSensorLab::drawLevel() {
    int centerX = SCREEN_WIDTH / 2;
    int centerY = 115;

    uint16_t ringColor = _isCentered ? TFT_GREEN : 0x39E7;
    g_canvas.drawCircle(centerX, centerY, 48, ringColor);
    g_canvas.drawCircle(centerX, centerY, 24, 0x2124);
    g_canvas.drawCircle(centerX, centerY, 8, _isCentered ? TFT_GREEN : 0x2965);

    g_canvas.drawFastHLine(centerX - 48, centerY, 96, 0x2124);
    g_canvas.drawFastVLine(centerX, centerY - 48, 96, 0x2124);

    int bx = centerX + (int)_bubbleX;
    int by = centerY + (int)_bubbleY;
    uint16_t bubbleColor = _isCentered ? TFT_GREEN : COLOR_CYAN;
    g_canvas.fillCircle(bx, by, 7, bubbleColor);
    g_canvas.drawCircle(bx, by, 7, TFT_WHITE);

    char infoStr[32];
    snprintf(infoStr, sizeof(infoStr), "X:%+.1f  Y:%+.1f", _bubbleX / 4.2f, _bubbleY / 4.2f);
    g_canvas.setTextColor(_isCentered ? TFT_GREEN : COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(infoStr, centerX, 175, 2);

    if (_isCentered) {
        g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
        g_canvas.drawCentreString("[ PERFECT LEVEL ]", centerX, 195, 1);
    } else {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("Tilt to center bubble", centerX, 195, 1);
    }
}

// ----------------------------------------------------
// G-Force 追蹤器邏輯與繪製
// ----------------------------------------------------
void SceneSensorLab::updateGTracker(InputManager& input) {
    uint32_t now = millis();
    if (now - _lastSampleTime >= 100) {
        _lastSampleTime = now;
        float ax = 0, ay = 0, az = 0;
        M5.Imu.getAccelData(&ax, &ay, &az);

        _currentG = sqrtf(ax * ax + ay * ay + az * az);
        _gHistory[_gHistIdx] = _currentG;
        _gHistIdx = (_gHistIdx + 1) % G_HIST_SIZE;

        float maxVal = 1.0f;
        for (uint8_t i = 0; i < G_HIST_SIZE; i++) {
            if (_gHistory[i] > maxVal) maxVal = _gHistory[i];
        }
        _peakG = maxVal;
        _peakAx = ax;
        _peakAy = ay;

        _needsRedraw = true;
    }
}

void SceneSensorLab::drawGTracker() {
    int centerX = SCREEN_WIDTH / 2;

    char gStr[16];
    snprintf(gStr, sizeof(gStr), "%.2f G", _currentG);
    uint16_t gColor = (_currentG > 2.5f) ? TFT_RED : (_currentG > 1.5f) ? COLOR_GOLD : TFT_GREEN;
    g_canvas.setTextColor(gColor, TFT_BLACK);
    g_canvas.drawCentreString(gStr, centerX, 36, 4);

    char peakStr[32];
    snprintf(peakStr, sizeof(peakStr), "5s Peak: %.2f G", _peakG);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(peakStr, centerX, 68, 2);

    int chartX = 15;
    int chartY = 95;
    int chartW = SCREEN_WIDTH - 30;
    int chartH = 65;

    g_canvas.drawRect(chartX, chartY, chartW, chartH, 0x2965);
    g_canvas.drawFastHLine(chartX, chartY + chartH - 16, chartW, 0x18C3);

    for (uint8_t i = 0; i < G_HIST_SIZE && i * 2 < chartW; i++) {
        uint8_t readIdx = (_gHistIdx + i) % G_HIST_SIZE;
        float val = _gHistory[readIdx];
        int barH = (int)((val / 4.0f) * chartH);
        if (barH > chartH) barH = chartH;
        if (barH < 1) barH = 1;

        int bx = chartX + i * 2;
        int by = chartY + chartH - barH;
        uint16_t bColor = (val > 2.5f) ? TFT_RED : (val > 1.5f) ? COLOR_GOLD : COLOR_CYAN;
        g_canvas.drawFastVLine(bx, by, barH, bColor);
    }

    char dirStr[32];
    snprintf(dirStr, sizeof(dirStr), "Vector Ax:%+.1f Ay:%+.1f", _peakAx, _peakAy);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString(dirStr, centerX, 175, 1);

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Shake or slam to test G-Force", centerX, 192, 1);
}

// ----------------------------------------------------
// 科技感 360 度旋轉雷達掃描指針與波紋動效 (通用雙緩衝)
// ----------------------------------------------------
void SceneSensorLab::drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor) {
    int centerX = SCREEN_WIDTH / 2;
    int centerY = 112;
    int radius = 45;

    // 標題與說明
    g_canvas.setTextColor(primaryColor, TFT_BLACK);
    g_canvas.drawCentreString(title, centerX, 30, 2);
    g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(subtitle, centerX, 50, 1);

    // 雙層外環與十字網格
    g_canvas.drawCircle(centerX, centerY, radius, primaryColor);
    g_canvas.drawCircle(centerX, centerY, radius - 1, 0x18C3);
    g_canvas.drawCircle(centerX, centerY, 15, 0x1183);
    g_canvas.drawCircle(centerX, centerY, 30, 0x19E3);

    g_canvas.drawFastHLine(centerX - radius + 2, centerY, (radius - 2) * 2, 0x1183);
    g_canvas.drawFastVLine(centerX, centerY - radius + 2, (radius - 2) * 2, 0x1183);

    // 動態向外擴散的同心波紋
    uint32_t tick = millis();
    int rip1 = (tick / 20) % radius;
    int rip2 = ((tick / 20) + radius / 2) % radius;
    if (rip1 > 2) g_canvas.drawCircle(centerX, centerY, rip1, 0x2286);
    if (rip2 > 2) g_canvas.drawCircle(centerX, centerY, rip2, 0x2286);

    // 360 度旋轉指針與扇形餘輝
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

    // 模擬信標亮點 (Radar Blips)
    if ((tick / 200) % 2 == 0) {
        g_canvas.fillCircle(centerX + 18, centerY - 14, 2, primaryColor);
        g_canvas.fillCircle(centerX - 16, centerY + 20, 2, TFT_WHITE);
    }
    if ((tick / 350) % 2 == 1) {
        g_canvas.fillCircle(centerX + 26, centerY + 16, 2, COLOR_GOLD);
    }

    // 底部動態搜尋文字
    int dotCount = (tick / 250) % 4;
    char dotBuf[5] = "...";
    dotBuf[dotCount] = '\0';
    char statusBuf[32];
    snprintf(statusBuf, sizeof(statusBuf), "SEARCHING%s", dotBuf);
    g_canvas.setTextColor(primaryColor, TFT_BLACK);
    g_canvas.drawCentreString(statusBuf, centerX, 168, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Please wait a moment", centerX, 182, 1);
}

// ----------------------------------------------------
// 2.4G Wi-Fi 掃描器邏輯與可捲動清單繪製
// ----------------------------------------------------
void SceneSensorLab::startWifiScan() {
    stopBleScan();
    _isScanningWifi = true;
    _foundWifiAps = 0;
    _wifiScrollOffset = 0;
    _lastWifiScanTime = millis();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanNetworks(true);
}

void SceneSensorLab::stopWifiScan() {
    if (_isScanningWifi) {
        WiFi.scanDelete();
        WiFi.mode(WIFI_OFF);
        _isScanningWifi = false;
    }
}

void SceneSensorLab::updateRfScanner(InputManager& input, AudioManager& audio) {
    if (input.joyBtnPressed) {
        audio.playClick();
        startWifiScan();
        _needsRedraw = true;
        return;
    }

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

            // 依 RSSI 降冪排序 (最強 AP 排在最前面)
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
            WiFi.mode(WIFI_OFF);
            audio.playTick();
            _needsRedraw = true;
        }
        return;
    }

    // 支援搖桿上下捲動檢視最多 15 筆 AP
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

void SceneSensorLab::drawRfScanner() {
    int centerX = SCREEN_WIDTH / 2;

    if (_isScanningWifi) {
        drawRadarAnimation("2.4G RF SCANNER", "Scanning Wi-Fi APs...", COLOR_GOLD);
        return;
    }

    // 標題列：呈現掃描 AP 筆數與當前頁次
    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "Wi-Fi APs: %d", _foundWifiAps);
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawString(titleBuf, 8, 28, 2);

    if (_foundWifiAps > 0) {
        char pageBuf[16];
        snprintf(pageBuf, sizeof(pageBuf), "%d-%d", _wifiScrollOffset + 1, min((int)_wifiScrollOffset + 4, (int)_foundWifiAps));
        g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
        g_canvas.drawRightString(pageBuf, SCREEN_WIDTH - 8, 30, 1);
    }

    if (_foundWifiAps == 0) {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("No APs Found", centerX, 95, 2);
        g_canvas.drawCentreString("Click Joy to Scan", centerX, 115, 1);
    } else {
        // 每頁呈現 4 筆項目
        uint8_t visibleCount = min((uint8_t)4, (uint8_t)(_foundWifiAps - _wifiScrollOffset));
        for (uint8_t i = 0; i < visibleCount; i++) {
            uint8_t idx = _wifiScrollOffset + i;
            int y = 46 + i * 38;

            g_canvas.fillRoundRect(6, y, 116, 35, 3, 0x0841);
            g_canvas.drawRoundRect(6, y, 116, 35, 3, 0x2104);

            // 行 1：SSID 名稱 (長度過長自動截斷)
            char shortSsid[16];
            snprintf(shortSsid, sizeof(shortSsid), "%s", _wifiAps[idx].ssid);
            g_canvas.setTextColor(TFT_WHITE, 0x0841);
            g_canvas.drawString(shortSsid, 10, y + 4, 1);

            // 右側 RSSI 數值
            char rssiStr[16];
            snprintf(rssiStr, sizeof(rssiStr), "%ddB", (int)_wifiAps[idx].rssi);
            uint16_t sigColor = (_wifiAps[idx].rssi > -65) ? TFT_GREEN :
                                (_wifiAps[idx].rssi > -80) ? COLOR_GOLD : TFT_RED;
            g_canvas.setTextColor(sigColor, 0x0841);
            g_canvas.drawRightString(rssiStr, 118, y + 4, 1);

            // 行 2：Channel 與加密模式 (例如 "CH6 [WPA2]")
            char metaStr[24];
            snprintf(metaStr, sizeof(metaStr), "CH%d [%s]", (int)_wifiAps[idx].channel, _wifiAps[idx].encryption);
            g_canvas.setTextColor(COLOR_CYAN, 0x0841);
            g_canvas.drawString(metaStr, 10, y + 20, 1);

            // 右下角 4 段階梯 Wi-Fi 訊號條
            int bars = (_wifiAps[idx].rssi > -55) ? 4 :
                       (_wifiAps[idx].rssi > -70) ? 3 :
                       (_wifiAps[idx].rssi > -85) ? 2 : 1;
            for (int b = 0; b < 4; b++) {
                int barH = 2 + b * 2;
                int bx = 104 + b * 4;
                int by = y + 30 - barH;
                uint16_t bc = (b < bars) ? sigColor : 0x2104;
                g_canvas.fillRect(bx, by, 3, barH, bc);
            }
        }

        // 右側捲軸指示條 (Scroll Bar)
        if (_foundWifiAps > 4) {
            int trackH = 146;
            int thumbH = max(16, trackH * 4 / _foundWifiAps);
            int thumbY = 46 + (_wifiScrollOffset * (trackH - thumbH)) / (_foundWifiAps - 4);
            g_canvas.drawFastVLine(128, 46, trackH, 0x18C3);
            g_canvas.fillRect(127, thumbY, 3, thumbH, COLOR_GOLD);
        }
    }

    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy Up/Dn:Scroll | Click:Scan", centerX, 201, 1);
}

// ----------------------------------------------------
// BLE 藍牙掃描器邏輯與設備清單繪製
// ----------------------------------------------------
void SceneSensorLab::onBleDeviceFound(const char* name, const char* addr, int rssi) {
    if (!addr) return;

    for (uint8_t i = 0; i < _foundBleDevs; i++) {
        if (strncmp(_bleDevs[i].address, addr, sizeof(_bleDevs[i].address)) == 0) {
            _bleDevs[i].rssi = (int16_t)rssi;
            if ((_bleDevs[i].name[0] == '\0' || strcmp(_bleDevs[i].name, "Unknown") == 0) &&
                name && name[0] != '\0') {
                snprintf(_bleDevs[i].name, sizeof(_bleDevs[i].name), "%s", name);
            }
            return;
        }
    }

    if (_foundBleDevs < MAX_BLE_DEVS) {
        snprintf(_bleDevs[_foundBleDevs].address, sizeof(_bleDevs[_foundBleDevs].address), "%s", addr);
        if (name && name[0] != '\0') {
            snprintf(_bleDevs[_foundBleDevs].name, sizeof(_bleDevs[_foundBleDevs].name), "%s", name);
        } else {
            snprintf(_bleDevs[_foundBleDevs].name, sizeof(_bleDevs[_foundBleDevs].name), "Unknown");
        }
        _bleDevs[_foundBleDevs].rssi = (int16_t)rssi;
        _foundBleDevs++;
    }
}

void SceneSensorLab::startBleScan() {
    stopWifiScan(); // Wi-Fi 與 BLE 互斥共用射頻
    if (!_bleInitialized) {
        BLEDevice::init("M5StickC-Lab");
        _bleInitialized = true;
    }
    _isScanningBle = true;
    _foundBleDevs = 0;
    _bleScrollOffset = 0;
    _bleScanStartTime = millis();

    BLEScan* pScan = BLEDevice::getScan();
    if (pScan) {
        if (!s_pBleCallbacks) {
            s_pBleCallbacks = new LabBleCallbacks(this);
        }
        pScan->setAdvertisedDeviceCallbacks(s_pBleCallbacks);
        pScan->setActiveScan(true);
        pScan->setInterval(100);
        pScan->setWindow(99);
        pScan->clearResults();
        pScan->start(0, nullptr, false);
    }
}

void SceneSensorLab::stopBleScan() {
    if (_isScanningBle) {
        BLEScan* pScan = BLEDevice::getScan();
        if (pScan) {
            pScan->stop();
            pScan->clearResults();
        }
        _isScanningBle = false;

        // 依 RSSI 降冪排序 (最強設備置頂)
        for (uint8_t i = 0; i < _foundBleDevs; i++) {
            for (uint8_t j = i + 1; j < _foundBleDevs; j++) {
                if (_bleDevs[j].rssi > _bleDevs[i].rssi) {
                    BleScanResult tmp = _bleDevs[i];
                    _bleDevs[i] = _bleDevs[j];
                    _bleDevs[j] = tmp;
                }
            }
        }
    }
}

void SceneSensorLab::updateBleScanner(InputManager& input, AudioManager& audio) {
    if (input.joyBtnPressed) {
        audio.playClick();
        startBleScan();
        _needsRedraw = true;
        return;
    }

    if (_isScanningBle) {
        _needsRedraw = true;
        if (millis() - _bleScanStartTime >= 3200) {
            stopBleScan();
            audio.playTick();
            _needsRedraw = true;
        }
        return;
    }

    // 支援搖桿上下捲動檢視最多 15 筆 BLE 設備
    if (_foundBleDevs > 4) {
        int maxOffset = _foundBleDevs - 4;
        static uint32_t lastScrollTick = 0;
        uint32_t now = millis();

        if (input.joyPulledDown || (input.joyY > 50 && now - lastScrollTick > 180)) {
            if (_bleScrollOffset < maxOffset) {
                _bleScrollOffset++;
                audio.playTick();
                _needsRedraw = true;
                lastScrollTick = now;
            }
        } else if (input.joyPushedUp || (input.joyY < -50 && now - lastScrollTick > 180)) {
            if (_bleScrollOffset > 0) {
                _bleScrollOffset--;
                audio.playTick();
                _needsRedraw = true;
                lastScrollTick = now;
            }
        }
    }
}

void SceneSensorLab::drawBleScanner() {
    int centerX = SCREEN_WIDTH / 2;

    if (_isScanningBle) {
        drawRadarAnimation("BLE SCANNER", "Scanning Bluetooth LE...", COLOR_CYAN);
        return;
    }

    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "BLE Devs: %d", _foundBleDevs);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawString(titleBuf, 8, 28, 2);

    if (_foundBleDevs > 0) {
        char pageBuf[16];
        snprintf(pageBuf, sizeof(pageBuf), "%d-%d", _bleScrollOffset + 1, min((int)_bleScrollOffset + 4, (int)_foundBleDevs));
        g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
        g_canvas.drawRightString(pageBuf, SCREEN_WIDTH - 8, 30, 1);
    }

    if (_foundBleDevs == 0) {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("No BLE Devices", centerX, 95, 2);
        g_canvas.drawCentreString("Click Joy to Scan", centerX, 115, 1);
    } else {
        uint8_t visibleCount = min((uint8_t)4, (uint8_t)(_foundBleDevs - _bleScrollOffset));
        for (uint8_t i = 0; i < visibleCount; i++) {
            uint8_t idx = _bleScrollOffset + i;
            int y = 46 + i * 38;

            g_canvas.fillRoundRect(6, y, 116, 35, 3, 0x0841);
            g_canvas.drawRoundRect(6, y, 116, 35, 3, 0x2104);

            // 行 1：BLE 設備名稱
            char shortName[16];
            snprintf(shortName, sizeof(shortName), "%s", _bleDevs[idx].name);
            g_canvas.setTextColor(TFT_WHITE, 0x0841);
            g_canvas.drawString(shortName, 10, y + 4, 1);

            // 右側 RSSI
            char rssiStr[16];
            snprintf(rssiStr, sizeof(rssiStr), "%ddB", (int)_bleDevs[idx].rssi);
            uint16_t sigColor = (_bleDevs[idx].rssi > -65) ? TFT_GREEN :
                                (_bleDevs[idx].rssi > -80) ? COLOR_GOLD : TFT_RED;
            g_canvas.setTextColor(sigColor, 0x0841);
            g_canvas.drawRightString(rssiStr, 118, y + 4, 1);

            // 行 2：MAC 位址
            g_canvas.setTextColor(COLOR_SILVER, 0x0841);
            g_canvas.drawString(_bleDevs[idx].address, 10, y + 20, 1);

            // 右下角 4 段階梯信標強度條
            int bars = (_bleDevs[idx].rssi > -55) ? 4 :
                       (_bleDevs[idx].rssi > -70) ? 3 :
                       (_bleDevs[idx].rssi > -85) ? 2 : 1;
            for (int b = 0; b < 4; b++) {
                int barH = 2 + b * 2;
                int bx = 104 + b * 4;
                int by = y + 30 - barH;
                uint16_t bc = (b < bars) ? sigColor : 0x2104;
                g_canvas.fillRect(bx, by, 3, barH, bc);
            }
        }

        // 右側捲軸指示條
        if (_foundBleDevs > 4) {
            int trackH = 146;
            int thumbH = max(16, trackH * 4 / _foundBleDevs);
            int thumbY = 46 + (_bleScrollOffset * (trackH - thumbH)) / (_foundBleDevs - 4);
            g_canvas.drawFastVLine(128, 46, trackH, 0x18C3);
            g_canvas.fillRect(127, thumbY, 3, thumbH, COLOR_CYAN);
        }
    }

    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy Up/Dn:Scroll | Click:Scan", centerX, 201, 1);
}

// ----------------------------------------------------
// LED Studio 邏輯與繪製
// ----------------------------------------------------
void SceneSensorLab::updateLedStudio(InputManager& input, LedManager& led) {
    if (abs(input.joyX) > 30) {
        _hue += (input.joyX / 15);
        if (_hue < 0) _hue += 360;
        if (_hue >= 360) _hue -= 360;
        _needsRedraw = true;
    }
    if (abs(input.joyY) > 30) {
        int nextB = _brightness - (input.joyY / 20);
        _brightness = (uint8_t)constrain(nextB, 5, 100);
        _needsRedraw = true;
    }

    hsvToRgb(_hue, 1.0f, (float)_brightness / 100.0f, _ledR, _ledG, _ledB);
    led.setColor(_ledR, _ledG, _ledB);
}

void SceneSensorLab::drawLedStudio() {
    int centerX = SCREEN_WIDTH / 2;

    uint16_t previewColor = g_canvas.color565(_ledR, _ledG, _ledB);
    g_canvas.fillRoundRect(15, 35, SCREEN_WIDTH - 30, 80, 6, previewColor);
    g_canvas.drawRoundRect(15, 35, SCREEN_WIDTH - 30, 80, 6, TFT_WHITE);

    char hexStr[16];
    snprintf(hexStr, sizeof(hexStr), "#%02X%02X%02X", _ledR, _ledG, _ledB);
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString(hexStr, centerX, 126, 4);

    char rgbStr[32];
    snprintf(rgbStr, sizeof(rgbStr), "R:%d G:%d B:%d (Brt:%d%%)", _ledR, _ledG, _ledB, _brightness);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(rgbStr, centerX, 156, 1);

    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy X: Hue (Color)", centerX, 180, 1);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString("Joy Y: Brightness", centerX, 195, 1);
}

// ----------------------------------------------------
// 主邏輯更新與分頁切換管理
// ----------------------------------------------------
void SceneSensorLab::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回全域主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        stopWifiScan();
        stopBleScan();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. Button A 短按：循環切換 5 個子分頁
    if (input.btnAPressed) {
        SensorTab prevTab = _currentTab;
        _currentTab = (SensorTab)((_currentTab + 1) % TAB_COUNT);
        audio.playClick();

        // 切離 Wi-Fi 或 BLE 分頁時確保關閉射頻
        if (prevTab == TAB_RF_SCANNER && _currentTab != TAB_RF_SCANNER) {
            stopWifiScan();
        }
        if (prevTab == TAB_BLE_SCANNER && _currentTab != TAB_BLE_SCANNER) {
            stopBleScan();
        }

        // 切入時自動啟動掃描
        if (_currentTab == TAB_RF_SCANNER) {
            startWifiScan();
        } else if (_currentTab == TAB_BLE_SCANNER) {
            startBleScan();
        }

        _needsRedraw = true;
        return;
    }

    // 3. 子分頁更新邏輯
    switch (_currentTab) {
        case TAB_LEVEL:       updateLevel(input, audio); break;
        case TAB_G_TRACKER:   updateGTracker(input); break;
        case TAB_RF_SCANNER:  updateRfScanner(input, audio); break;
        case TAB_BLE_SCANNER: updateBleScanner(input, audio); break;
        case TAB_LED_STUDIO:  updateLedStudio(input, led); break;
        default: break;
    }
}

// ----------------------------------------------------
// 主繪圖函式 (雙緩衝 g_canvas 零閃爍渲染)
// ----------------------------------------------------
void SceneSensorLab::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    // 頂部 5 分頁導覽列 (Y: 0 ~ 24, 寬度 135 / 5 = 27px)
    const char* TAB_NAMES[] = {"LVL", "G-F", "WIFI", "BLE", "LED"};
    int tabW = SCREEN_WIDTH / TAB_COUNT; // 27px

    for (uint8_t i = 0; i < TAB_COUNT; i++) {
        int tx = i * tabW;
        bool isSel = (i == _currentTab);
        if (isSel) {
            g_canvas.fillRect(tx, 0, tabW, 23, COLOR_CYAN);
            g_canvas.setTextColor(TFT_BLACK, COLOR_CYAN);
        } else {
            g_canvas.fillRect(tx, 0, tabW, 23, 0x18C3);
            g_canvas.setTextColor(TFT_LIGHTGREY, 0x18C3);
        }
        g_canvas.drawCentreString(TAB_NAMES[i], tx + tabW / 2, 4, 1);
    }

    // 繪製對應子分頁
    switch (_currentTab) {
        case TAB_LEVEL:       drawLevel(); break;
        case TAB_G_TRACKER:   drawGTracker(); break;
        case TAB_RF_SCANNER:  drawRfScanner(); break;
        case TAB_BLE_SCANNER: drawBleScanner(); break;
        case TAB_LED_STUDIO:  drawLedStudio(); break;
        default: break;
    }

    // 底部全域分頁提示 (Y: 215 ~ 238)
    g_canvas.drawFastHLine(8, 214, SCREEN_WIDTH - 16, 0x2965);
    g_canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("[Btn A]: Next Tab", SCREEN_WIDTH / 2, 218, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 228, 1);

    // 統一推送到 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}