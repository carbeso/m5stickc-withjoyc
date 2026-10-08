/**
 * @file SceneBleScanner.h
 * @brief BLE 藍牙廣播掃描儀 (BLE Scanner) 場景標頭檔
 * @details 具備旋轉雷達動畫、主執行緒 3 秒非同步掃描、設備名稱與 RSSI 階梯顯示、退出生命週期釋放射頻
 */

#pragma once

#include "Scene.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

struct BleScanResult {
    char name[20];
    char address[18];
    int16_t rssi;
};

class SceneBleScanner : public Scene {
public:
    SceneBleScanner();
    virtual ~SceneBleScanner();

    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    void exit() override;
    GameScene getSceneId() const override { return SCENE_BLE_SCANNER; }

    void onBleDeviceFound(const char* name, const char* addr, int rssi);

private:
    void startBleScan();
    void stopBleScan();
    void drawRadarAnimation(const char* title, const char* subtitle, uint16_t primaryColor);

    static const uint8_t MAX_BLE_DEVS = 15;
    bool _isScanningBle;
    bool _bleInitialized;
    uint8_t _foundBleDevs;
    BleScanResult _bleDevs[MAX_BLE_DEVS];
    int8_t _bleScrollOffset;
    uint32_t _bleScanStartTime;
    bool _needsRedraw;
};
