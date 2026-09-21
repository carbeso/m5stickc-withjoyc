/**
 * @file SceneSensorLab.h
 * @brief 感測器實驗室 (Sensor Lab) 儀表板標頭檔
 * @details 整合三軸水平儀、5 秒 G-Force 歷史峰值紀錄器、RF 訊號掃描儀與 RGB LED 調光工作室
 */

#pragma once

#include "Scene.h"
#include <WiFi.h>

enum SensorTab {
    TAB_LEVEL = 0,      // 三軸水平儀
    TAB_G_TRACKER,      // 5秒最大 G-Force 衝擊向量追蹤
    TAB_RF_SCANNER,     // Wi-Fi 探測掃描儀
    TAB_LED_STUDIO,     // RGB LED 調光工作室
    TAB_COUNT
};

struct WifiScanResult {
    char ssid[24];
    int32_t rssi;
};

class SceneSensorLab : public Scene {
public:
    SceneSensorLab();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SENSOR_LAB; }

private:
    void updateLevel(InputManager& input, AudioManager& audio);
    void drawLevel();

    void updateGTracker(InputManager& input);
    void drawGTracker();

    void startWifiScan();
    void updateRfScanner(InputManager& input, AudioManager& audio);
    void drawRfScanner();

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

    // --- RF 掃描器變數 ---
    bool _isScanningWifi;
    uint8_t _foundAps;
    WifiScanResult _topAps[3];
    uint32_t _lastScanTime;

    // --- LED Studio 變數 ---
    int16_t _hue;           // 0 ~ 359
    uint8_t _brightness;    // 0 ~ 100
    uint8_t _ledR, _ledG, _ledB;
};
