/**
 * @file SceneSensorLab.h
 * @brief 感測器實驗室 (Sensor Lab) 儀表板標頭檔
 * @details 整合三軸水平儀、5 秒 G-Force 歷史峰值紀錄器、2.4G Wi-Fi 掃描儀、BLE 藍牙掃描儀與 RGB LED 調光工作室
 */

#pragma once

#include "Scene.h"
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

enum SensorTab {
    TAB_LEVEL = 0,      // 三軸水平儀
    TAB_G_TRACKER,      // 5 秒最大 G-Force 衝擊向量追蹤
    TAB_RF_SCANNER,     // 2.4G Wi-Fi 探測掃描儀
    TAB_BLE_SCANNER,    // BLE 藍牙廣播掃描儀
    TAB_LED_STUDIO,     // RGB LED 調光工作室
    TAB_COUNT
};

struct WifiScanResult {
    char ssid[24];
    int32_t rssi;
    int32_t channel;
    char encryption[8];
};

struct BleScanResult {
    char name[20];
    char address[18];
    int16_t rssi;
};

class SceneSensorLab : public Scene {
public:
    SceneSensorLab();
    virtual ~SceneSensorLab();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SENSOR_LAB; }

    // BLE 設備發現回呼
    void onBleDeviceFound(const char* name, const char* addr, int rssi);

private:
    void updateLevel(InputManager& input, AudioManager& audio);
    void drawLevel();

    void updateGTracker(InputManager& input);
    void drawGTracker();

    void startWifiScan();
    void stopWifiScan();
    void updateRfScanner(InputManager& input, AudioManager& audio);
    void drawRfScanner();

    void startBleScan();
    void stopBleScan();
    void updateBleScanner(InputManager& input, AudioManager& audio);
    void drawBleScanner();

    // 科技感 360 度旋轉雷達掃描動畫（雙緩衝 g_canvas 零閃爍）
    void drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor);

    void updateLedStudio(InputManager& input, LedManager& led);
    void drawLedStudio();

    SensorTab _currentTab;
    bool _needsRedraw;
    uint32_t _lastSampleTime;

    // --- 水平儀變數 ---
    float _bubbleX, _bubbleY;
    float _targetBubbleX, _targetBubbleY;
    bool _isCentered;
    uint32_t _lastCenterTick;

    // --- G-Tracker 變數 ---
    static const uint8_t G_HIST_SIZE = 50; // 5 秒視窗 (10Hz 採樣)
    float _gHistory[G_HIST_SIZE];
    uint8_t _gHistIdx;
    float _currentG;
    float _peakG;
    float _peakAx, _peakAy;

    // --- 2.4G Wi-Fi 掃描器變數 ---
    static const uint8_t MAX_WIFI_APS = 15;
    bool _isScanningWifi;
    uint8_t _foundWifiAps;
    WifiScanResult _wifiAps[MAX_WIFI_APS];
    int8_t _wifiScrollOffset;
    uint32_t _lastWifiScanTime;

    // --- BLE 藍牙掃描器變數 ---
    static const uint8_t MAX_BLE_DEVS = 15;
    bool _isScanningBle;
    bool _bleInitialized;
    uint8_t _foundBleDevs;
    BleScanResult _bleDevs[MAX_BLE_DEVS];
    int8_t _bleScrollOffset;
    uint32_t _bleScanStartTime;

    // --- LED Studio 變數 ---
    int16_t _hue;           // 0 ~ 359
    uint8_t _brightness;    // 0 ~ 100
    uint8_t _ledR, _ledG, _ledB;
};