/**
 * @file SceneBleScanner.cpp
 * @brief BLE 藍牙廣播掃描儀 (BLE Scanner) 場景實作檔
 * @details 進入時啟動 BLE 掃描，於主執行緒 3 秒計時安全停止，杜絕 FreeRTOS BTU 堆疊崩潰，退出時徹底釋放記憶體
 */

#include "scenes/SceneBleScanner.h"
#include <cmath>

class ScannerBleCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
    ScannerBleCallbacks(SceneBleScanner* scanner) : _scanner(scanner) {}
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (_scanner) {
            std::string name = advertisedDevice.getName();
            std::string addr = advertisedDevice.getAddress().toString();
            int rssi = advertisedDevice.getRSSI();
            _scanner->onBleDeviceFound(name.c_str(), addr.c_str(), rssi);
        }
    }
private:
    SceneBleScanner* _scanner;
};

static ScannerBleCallbacks* s_pScannerCallbacks = nullptr;

SceneBleScanner::SceneBleScanner()
    : _isScanningBle(false), _bleInitialized(false), _foundBleDevs(0),
      _bleScrollOffset(0), _bleScanStartTime(0), _needsRedraw(true) {
    for (uint8_t i = 0; i < MAX_BLE_DEVS; i++) {
        _bleDevs[i].name[0] = '\0';
        _bleDevs[i].address[0] = '\0';
        _bleDevs[i].rssi = -100;
    }
}

SceneBleScanner::~SceneBleScanner() {
    stopBleScan();
    if (_bleInitialized && BLEDevice::getInitialized()) {
        BLEDevice::deinit(true);
        _bleInitialized = false;
    }
    if (s_pScannerCallbacks) {
        delete s_pScannerCallbacks;
        s_pScannerCallbacks = nullptr;
    }
}

void SceneBleScanner::init() {
    _foundBleDevs = 0;
    _bleScrollOffset = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    startBleScan();
}

void SceneBleScanner::onBleDeviceFound(const char* name, const char* addr, int rssi) {
    if (!addr || !_isScanningBle) return;
    if (_foundBleDevs >= MAX_BLE_DEVS) return;

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

    uint8_t idx = _foundBleDevs;
    if (idx < MAX_BLE_DEVS) {
        snprintf(_bleDevs[idx].address, sizeof(_bleDevs[idx].address), "%s", addr);
        if (name && name[0] != '\0') {
            snprintf(_bleDevs[idx].name, sizeof(_bleDevs[idx].name), "%s", name);
        } else {
            snprintf(_bleDevs[idx].name, sizeof(_bleDevs[idx].name), "Unknown");
        }
        _bleDevs[idx].rssi = (int16_t)rssi;
        _foundBleDevs = idx + 1;
    }
}

void SceneBleScanner::startBleScan() {
    if (!_bleInitialized || !BLEDevice::getInitialized()) {
        BLEDevice::init("M5StickC-BLE");
        _bleInitialized = true;
    }
    _isScanningBle = true;
    _foundBleDevs = 0;
    _bleScrollOffset = 0;
    _bleScanStartTime = millis();

    BLEScan* pScan = BLEDevice::getScan();
    if (pScan) {
        if (!s_pScannerCallbacks) {
            s_pScannerCallbacks = new ScannerBleCallbacks(this);
        }
        pScan->setAdvertisedDeviceCallbacks(s_pScannerCallbacks, false);
        pScan->setActiveScan(true);
        pScan->setInterval(100);
        pScan->setWindow(99);
        pScan->start(0, nullptr, false);
    }
}

void SceneBleScanner::stopBleScan() {
    if (_isScanningBle) {
        _isScanningBle = false;
        BLEScan* pScan = BLEDevice::getScan();
        if (pScan) {
            pScan->stop();
        }

        // 依 RSSI 降冪排序 (最強設備置頂，於主執行緒安全執行)
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

void SceneBleScanner::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        stopBleScan();
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 按下搖桿中心鍵：重新掃描
    if (input.joyBtnPressed) {
        audio.playClick();
        startBleScan();
        _needsRedraw = true;
        return;
    }

    // 3. 掃描中倒數 3 秒安全結束
    if (_isScanningBle) {
        _needsRedraw = true;
        if (millis() - _bleScanStartTime >= 3000) {
            stopBleScan();
            audio.playTick();
            _needsRedraw = true;
        }
        return;
    }

    // 4. 搖桿上下捲動檢視設備
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

void SceneBleScanner::drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor) {
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
    g_canvas.drawCentreString("Detecting BLE Beacons...", centerX, 184, 1);
}

void SceneBleScanner::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    g_canvas.fillSprite(TFT_BLACK);

    if (_isScanningBle) {
        drawRadarAnimation("BLE SCANNER", "Scanning Bluetooth LE...", COLOR_LIGHT_BLUE);
        g_canvas.pushSprite(0, 0);
        return;
    }

    int centerX = SCREEN_WIDTH / 2;

    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "BLE Devs: %d", _foundBleDevs);
    g_canvas.setTextColor(COLOR_LIGHT_BLUE, TFT_BLACK);
    g_canvas.drawString(titleBuf, 8, 10, 2);

    if (_foundBleDevs > 0) {
        char pageBuf[16];
        snprintf(pageBuf, sizeof(pageBuf), "%d-%d", _bleScrollOffset + 1, min((int)_bleScrollOffset + 4, (int)_foundBleDevs));
        g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
        g_canvas.drawRightString(pageBuf, SCREEN_WIDTH - 8, 12, 1);
    }

    if (_foundBleDevs == 0) {
        g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        g_canvas.drawCentreString("No BLE Devices", centerX, 95, 2);
        g_canvas.drawCentreString("Click Joy to Scan", centerX, 115, 1);
    } else {
        uint8_t visibleCount = min((uint8_t)4, (uint8_t)(_foundBleDevs - _bleScrollOffset));
        for (uint8_t i = 0; i < visibleCount; i++) {
            uint8_t idx = _bleScrollOffset + i;
            int y = 32 + i * 42;

            g_canvas.fillRoundRect(6, y, 116, 38, 3, 0x0841);
            g_canvas.drawRoundRect(6, y, 116, 38, 3, 0x2104);

            char shortName[16];
            snprintf(shortName, sizeof(shortName), "%.13s", _bleDevs[idx].name);
            g_canvas.setTextColor(TFT_WHITE, 0x0841);
            g_canvas.drawString(shortName, 10, y + 4, 1);

            char rssiStr[16];
            snprintf(rssiStr, sizeof(rssiStr), "%ddB", (int)_bleDevs[idx].rssi);
            uint16_t sigColor = (_bleDevs[idx].rssi > -65) ? TFT_GREEN :
                                (_bleDevs[idx].rssi > -80) ? COLOR_GOLD : TFT_RED;
            g_canvas.setTextColor(sigColor, 0x0841);
            g_canvas.drawRightString(rssiStr, 118, y + 4, 1);

            g_canvas.setTextColor(COLOR_SILVER, 0x0841);
            g_canvas.drawString(_bleDevs[idx].address, 10, y + 22, 1);

            int bars = (_bleDevs[idx].rssi > -55) ? 4 :
                       (_bleDevs[idx].rssi > -70) ? 3 :
                       (_bleDevs[idx].rssi > -85) ? 2 : 1;
            for (int b = 0; b < 4; b++) {
                int barH = 2 + b * 2;
                int bx = 104 + b * 4;
                int by = y + 33 - barH;
                uint16_t bc = (b < bars) ? sigColor : 0x2104;
                g_canvas.fillRect(bx, by, 3, barH, bc);
            }
        }

        if (_foundBleDevs > 4) {
            int trackH = 160;
            int thumbH = max(16, trackH * 4 / _foundBleDevs);
            int thumbY = 32 + (_bleScrollOffset * (trackH - thumbH)) / (_foundBleDevs - 4);
            g_canvas.drawFastVLine(128, 32, trackH, 0x18C3);
            g_canvas.fillRect(127, thumbY, 3, thumbH, COLOR_LIGHT_BLUE);
        }
    }

    g_canvas.drawFastHLine(10, 206, SCREEN_WIDTH - 20, 0x18C3);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy Click: Re-Scan", centerX, 210, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", centerX, 224, 1);

    g_canvas.pushSprite(0, 0);
}

void SceneBleScanner::exit() {
    stopBleScan();
    // 徹底釋放 BLE 記憶體
    if (_bleInitialized && BLEDevice::getInitialized()) {
        BLEDevice::deinit(true);
        _bleInitialized = false;
    }
}
