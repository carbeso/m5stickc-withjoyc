/**
 * @file SceneSensorLab.cpp
 * @brief 感測器實驗室 (Sensor Lab) 儀表板實作：水平儀、G-Force 追蹤、RF 掃描與 LED 工作室
 */

#include "scenes/SceneSensorLab.h"
#include <cmath>

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

SceneSensorLab::SceneSensorLab()
    : _currentTab(TAB_LEVEL), _needsRedraw(true), _lastSampleTime(0),
      _bubbleX(0), _bubbleY(0), _targetBubbleX(0), _targetBubbleY(0), _isCentered(false), _lastCenterTick(0),
      _gHistIdx(0), _currentG(1.0f), _peakG(1.0f), _peakAx(0), _peakAy(0),
      _isScanningWifi(false), _foundAps(0), _lastScanTime(0),
      _hue(180), _brightness(80), _ledR(0), _ledG(200), _ledB(200) {
    for (uint8_t i = 0; i < G_HIST_SIZE; i++) _gHistory[i] = 1.0f;
    for (uint8_t i = 0; i < 3; i++) {
        _topAps[i].ssid[0] = '\0';
        _topAps[i].rssi = -100;
    }
}

void SceneSensorLab::init() {
    _currentTab = TAB_LEVEL;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _lastSampleTime = millis();
    _peakG = 1.0f;
    _isScanningWifi = false;

    // 確保 Wi-Fi 預設關閉省電
    WiFi.mode(WIFI_OFF);
}

void SceneSensorLab::updateLevel(InputManager& input, AudioManager& audio) {
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getAccelData(&ax, &ay, &az);

    // 映射至中央靶盤 (-42 ~ 42px)
    _targetBubbleX = -ax * 42.0f;
    _targetBubbleY = ay * 42.0f;

    // 物理阻尼低通濾波
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

    // 繪製水平儀外圓與內十字準星
    uint16_t ringColor = _isCentered ? TFT_GREEN : 0x39E7;
    g_canvas.drawCircle(centerX, centerY, 48, ringColor);
    g_canvas.drawCircle(centerX, centerY, 24, 0x2124);
    g_canvas.drawCircle(centerX, centerY, 8, _isCentered ? TFT_GREEN : 0x2965);

    g_canvas.drawFastHLine(centerX - 48, centerY, 96, 0x2124);
    g_canvas.drawFastVLine(centerX, centerY - 48, 96, 0x2124);

    // 繪製氣泡 (實心圓)
    int bx = centerX + (int)_bubbleX;
    int by = centerY + (int)_bubbleY;
    uint16_t bubbleColor = _isCentered ? TFT_GREEN : COLOR_CYAN;
    g_canvas.fillCircle(bx, by, 7, bubbleColor);
    g_canvas.drawCircle(bx, by, 7, TFT_WHITE);

    // 數值面板
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

void SceneSensorLab::updateGTracker(InputManager& input) {
    uint32_t now = millis();
    if (now - _lastSampleTime >= 100) { // 10Hz 取樣
        _lastSampleTime = now;
        float ax = 0, ay = 0, az = 0;
        M5.Imu.getAccelData(&ax, &ay, &az);

        _currentG = sqrtf(ax * ax + ay * ay + az * az);
        _gHistory[_gHistIdx] = _currentG;
        _gHistIdx = (_gHistIdx + 1) % G_HIST_SIZE;

        // 計算 5 秒歷史峰值
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

    // 1. 即時 G 與峰值顯示 (Y: 36 ~ 75)
    char gStr[16];
    snprintf(gStr, sizeof(gStr), "%.2f G", _currentG);
    uint16_t gColor = (_currentG > 2.5f) ? TFT_RED : (_currentG > 1.5f) ? COLOR_GOLD : TFT_GREEN;
    g_canvas.setTextColor(gColor, TFT_BLACK);
    g_canvas.drawCentreString(gStr, centerX, 36, 4);

    char peakStr[32];
    snprintf(peakStr, sizeof(peakStr), "5s Peak: %.2f G", _peakG);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(peakStr, centerX, 68, 2);

    // 2. 5 秒滑動歷史長條圖 (Y: 95 ~ 165，高 70px)
    int chartX = 15;
    int chartY = 95;
    int chartW = SCREEN_WIDTH - 30; // 105px
    int chartH = 65;

    g_canvas.drawRect(chartX, chartY, chartW, chartH, 0x2965);
    g_canvas.drawFastHLine(chartX, chartY + chartH - 16, chartW, 0x18C3); // 1.0G 基準線

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

    // 3. 受力方向與提示 (Y: 175 ~ 205)
    char dirStr[32];
    snprintf(dirStr, sizeof(dirStr), "Vector Ax:%+.1f Ay:%+.1f", _peakAx, _peakAy);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString(dirStr, centerX, 175, 1);

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Shake or slam to test G-Force", centerX, 192, 1);
}

void SceneSensorLab::startWifiScan() {
    _isScanningWifi = true;
    _lastScanTime = millis();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanNetworks(true); // 非同步背景掃描
}

void SceneSensorLab::updateRfScanner(InputManager& input, AudioManager& audio) {
    if (input.joyBtnPressed) {
        audio.playClick();
        startWifiScan();
        _needsRedraw = true;
        return;
    }

    if (_isScanningWifi) {
        int16_t n = WiFi.scanComplete();
        if (n >= 0) {
            _isScanningWifi = false;
            _foundAps = (n > 3) ? 3 : n;

            // 擷取訊號最強的前 3 個 AP
            for (uint8_t i = 0; i < _foundAps; i++) {
                String s = WiFi.SSID(i);
                snprintf(_topAps[i].ssid, sizeof(_topAps[i].ssid), "%s", s.c_str());
                _topAps[i].rssi = WiFi.RSSI(i);
            }
            WiFi.scanDelete();
            WiFi.mode(WIFI_OFF); // 掃描完成立即釋放射頻
            audio.playTick();
            _needsRedraw = true;
        }
    }
}

void SceneSensorLab::drawRfScanner() {
    int centerX = SCREEN_WIDTH / 2;

    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString("2.4G RF SCANNER", centerX, 35, 2);

    if (_isScanningWifi) {
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("SCANNING RF...", centerX, 95, 2);
        g_canvas.drawRoundRect(20, 125, SCREEN_WIDTH - 40, 6, 2, 0x39E7);
        int dotX = 20 + ((millis() / 50) % (SCREEN_WIDTH - 44));
        g_canvas.fillRect(dotX, 126, 8, 4, TFT_GREEN);
    } else {
        if (_foundAps == 0) {
            g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
            g_canvas.drawCentreString("No APs / Click Joy", centerX, 95, 2);
            g_canvas.drawCentreString("to start Scan", centerX, 115, 1);
        } else {
            for (uint8_t i = 0; i < _foundAps; i++) {
                int y = 65 + i * 40;
                g_canvas.drawRoundRect(10, y, SCREEN_WIDTH - 20, 34, 3, 0x2965);

                // SSID
                g_canvas.setTextColor(TFT_WHITE, TFT_BLACK);
                g_canvas.drawString(_topAps[i].ssid, 16, y + 5, 1);

                // RSSI 與訊號強度條
                char rssiStr[16];
                snprintf(rssiStr, sizeof(rssiStr), "%d dBm", (int)_topAps[i].rssi);
                uint16_t sigColor = (_topAps[i].rssi > -60) ? TFT_GREEN :
                                    (_topAps[i].rssi > -75) ? COLOR_GOLD : TFT_RED;
                g_canvas.setTextColor(sigColor, TFT_BLACK);
                g_canvas.drawRightString(rssiStr, SCREEN_WIDTH - 16, y + 5, 1);

                // 訊號進度條
                int barW = map(constrain(_topAps[i].rssi, -95, -35), -95, -35, 4, SCREEN_WIDTH - 36);
                g_canvas.fillRect(16, y + 22, barW, 4, sigColor);
            }
        }
    }

    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Click Joy to Re-Scan", centerX, 195, 1);
}

void SceneSensorLab::updateLedStudio(InputManager& input, LedManager& led) {
    if (abs(input.joyX) > 30) {
        _hue += (input.joyX / 15);
        if (_hue < 0) _hue += 360;
        if (_hue >= 360) _hue -= 360;
        _needsRedraw = true;
    }
    if (abs(input.joyY) > 30) {
        // input.joyY < 0 (向上推) 時亮度增加，input.joyY > 0 (向下推) 時亮度減少
        int nextB = _brightness - (input.joyY / 20);
        _brightness = (uint8_t)constrain(nextB, 5, 100);
        _needsRedraw = true;
    }

    hsvToRgb(_hue, 1.0f, (float)_brightness / 100.0f, _ledR, _ledG, _ledB);
    led.setColor(_ledR, _ledG, _ledB);
}

void SceneSensorLab::drawLedStudio() {
    int centerX = SCREEN_WIDTH / 2;

    // 1. 全幅調光預覽色塊 (Y: 35 ~ 115)
    uint16_t previewColor = g_canvas.color565(_ledR, _ledG, _ledB);
    g_canvas.fillRoundRect(15, 35, SCREEN_WIDTH - 30, 80, 6, previewColor);
    g_canvas.drawRoundRect(15, 35, SCREEN_WIDTH - 30, 80, 6, TFT_WHITE);

    // 2. HEX 色碼與 RGB 數值 (Y: 125 ~ 170)
    char hexStr[16];
    snprintf(hexStr, sizeof(hexStr), "#%02X%02X%02X", _ledR, _ledG, _ledB);
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawCentreString(hexStr, centerX, 126, 4);

    char rgbStr[32];
    snprintf(rgbStr, sizeof(rgbStr), "R:%d G:%d B:%d (Brt:%d%%)", _ledR, _ledG, _ledB, _brightness);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(rgbStr, centerX, 156, 1);

    // 3. 底部操作提示 (Y: 180 ~ 205)
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Joy X: Hue (Color)", centerX, 180, 1);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString("Joy Y: Brightness", centerX, 195, 1);
}

void SceneSensorLab::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：返回全域主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        WiFi.mode(WIFI_OFF);
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. Button A 短按：循環切換分頁
    if (input.btnAPressed) {
        _currentTab = (SensorTab)((_currentTab + 1) % TAB_COUNT);
        audio.playClick();
        if (_currentTab != TAB_RF_SCANNER) {
            WiFi.mode(WIFI_OFF); // 離開 RF 分頁關閉 Wi-Fi
        } else {
            startWifiScan();
        }
        _needsRedraw = true;
        return;
    }

    // 3. 各分頁邏輯更新
    switch (_currentTab) {
        case TAB_LEVEL:      updateLevel(input, audio); break;
        case TAB_G_TRACKER:  updateGTracker(input); break;
        case TAB_RF_SCANNER: updateRfScanner(input, audio); break;
        case TAB_LED_STUDIO: updateLedStudio(input, led); break;
        default: break;
    }
}

void SceneSensorLab::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布離線渲染，徹底杜絕畫面撕裂與閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 頂部分頁導覽列 (Y: 0 ~ 26)
    const char* TAB_NAMES[] = {"LEVEL", "G-TRK", "RF", "LED"};
    int tabW = SCREEN_WIDTH / TAB_COUNT; // 約 33px

    for (uint8_t i = 0; i < TAB_COUNT; i++) {
        int tx = i * tabW;
        bool isSel = (i == _currentTab);
        if (isSel) {
            g_canvas.fillRect(tx, 0, tabW, 24, COLOR_CYAN);
            g_canvas.setTextColor(TFT_BLACK, COLOR_CYAN);
        } else {
            g_canvas.fillRect(tx, 0, tabW, 24, 0x18C3);
            g_canvas.setTextColor(TFT_LIGHTGREY, 0x18C3);
        }
        g_canvas.drawCentreString(TAB_NAMES[i], tx + tabW / 2, 4, 1);
    }

    // 繪製當前分頁內容至畫布
    switch (_currentTab) {
        case TAB_LEVEL:      drawLevel(); break;
        case TAB_G_TRACKER:  drawGTracker(); break;
        case TAB_RF_SCANNER: drawRfScanner(); break;
        case TAB_LED_STUDIO: drawLedStudio(); break;
        default: break;
    }

    // 底部全域分頁提示 (Y: 215 ~ 238)
    g_canvas.drawFastHLine(8, 214, SCREEN_WIDTH - 16, 0x2965);
    g_canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("[Btn A]: Next Tab", SCREEN_WIDTH / 2, 218, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 228, 1);

    // 一次性將記憶體畫面推送到螢幕
    g_canvas.pushSprite(0, 0);
}
