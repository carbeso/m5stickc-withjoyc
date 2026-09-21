#pragma once

#include <Arduino.h>
#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BLE_TIME_SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define BLE_TIME_CHAR_UUID           "0000ffe1-0000-1000-8000-00805f9b34fb"
#define BLE_DEVICE_NAME              "M5StickC-Fidget"

class BleSyncManager : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    static BleSyncManager& getInstance() {
        static BleSyncManager instance;
        return instance;
    }

    void begin();
    void end();
    void update();

    bool isConnected() const { return _deviceConnected; }
    bool hasJustSynced() {
        bool synced = _syncSuccess;
        _syncSuccess = false;
        return synced;
    }

    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

    void onWrite(BLECharacteristic* pCharacteristic) override;

private:
    BleSyncManager();
    ~BleSyncManager();
    BleSyncManager(const BleSyncManager&) = delete;
    BleSyncManager& operator=(const BleSyncManager&) = delete;

    bool _initialized;
    bool _deviceConnected;
    bool _oldDeviceConnected;
    bool _syncSuccess;

    BLEServer* _pServer;
    BLEService* _pService;
    BLECharacteristic* _pCharacteristic;
};
