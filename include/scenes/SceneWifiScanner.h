/**
 * @file SceneWifiScanner.h
 * @brief 2.4G Wi-Fi 探測雷達與掃描器 (Wi-Fi Scanner) 場景標頭檔
 * @details 具備旋轉雷達動畫、AP 訊號階梯強度排序、滾動清單，並於退出時徹底關閉 Wi-Fi 釋放記憶體
 */

#pragma once

#include "Scene.h"
#include <WiFi.h>

struct WifiScanResult {
    char ssid[24];
    int32_t rssi;
    int32_t channel;
    char encryption[8];
};

class SceneWifiScanner : public Scene {
public:
    SceneWifiScanner();
    virtual ~SceneWifiScanner();

    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    void exit() override;
    GameScene getSceneId() const override { return SCENE_WIFI_SCANNER; }

private:
    void startWifiScan();
    void stopWifiScan();
    void drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor);

    static const uint8_t MAX_WIFI_APS = 15;
    bool _isScanningWifi;
    uint8_t _foundWifiAps;
    WifiScanResult _wifiAps[MAX_WIFI_APS];
    int8_t _wifiScrollOffset;
    uint32_t _lastWifiScanTime;
    bool _needsRedraw;
};
