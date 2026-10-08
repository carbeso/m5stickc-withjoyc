#include "BleSyncManager.h"
#include <time.h>
BleSyncManager::BleSyncManager()
    : _initialized(false), _deviceConnected(false), _oldDeviceConnected(false),
      _syncSuccess(false), _pServer(nullptr), _pService(nullptr), _pCharacteristic(nullptr) {}
BleSyncManager::~BleSyncManager() { end(); }
void BleSyncManager::begin() {
    if (_initialized) return;
    BLEDevice::init(BLE_DEVICE_NAME);
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(this);
    _pService = _pServer->createService(BLE_TIME_SERVICE_UUID);
    _pCharacteristic = _pService->createCharacteristic(
        BLE_TIME_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ   |
        BLECharacteristic::PROPERTY_WRITE  |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _pCharacteristic->setCallbacks(this);
    _pCharacteristic->addDescriptor(new BLE2902());
    uint8_t initialPayload[7] = {0};
    _pCharacteristic->setValue(initialPayload, sizeof(initialPayload));
    _pService->start();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_TIME_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    _initialized = true;
    _deviceConnected = false;
    _oldDeviceConnected = false;
    _syncSuccess = false;
}
void BleSyncManager::end() {
    if (!_initialized) return;
    BLEDevice::deinit(false);
    _pServer = nullptr;
    _pService = nullptr;
    _pCharacteristic = nullptr;
    _initialized = false;
    _deviceConnected = false;
    _oldDeviceConnected = false;
}
void BleSyncManager::update() {
    if (!_initialized) return;
    if (!_deviceConnected && _oldDeviceConnected) {
        delay(20);
        _pServer->startAdvertising();
        _oldDeviceConnected = _deviceConnected;
    }
    if (_deviceConnected && !_oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
    }
}
void BleSyncManager::onConnect(BLEServer* pServer) {
    _deviceConnected = true;
}
void BleSyncManager::onDisconnect(BLEServer* pServer) {
    _deviceConnected = false;
}
void BleSyncManager::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    size_t len = value.length();
    if (len == 0) return;
    const uint8_t* data = (const uint8_t*)value.data();
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    bool valid = false;
    if (len >= 8) {
        date.Year = (data[0] << 8) | data[1];
        date.Month = data[2];
        date.Date = data[3];
        time.Hours = data[4];
        time.Minutes = data[5];
        time.Seconds = data[6];
        date.WeekDay = data[7];
        valid = true;
    } else if (len == 7) {
        date.Year = 2000 + data[0];
        date.Month = data[1];
        date.Date = data[2];
        time.Hours = data[3];
        time.Minutes = data[4];
        time.Seconds = data[5];
        date.WeekDay = data[6];
        valid = true;
    } else if (len == 4) {
        uint32_t ts = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        if (ts < 1000000000UL) {
            ts = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        }
        time_t rawtime = (time_t)(ts + 8 * 3600);
        struct tm* ti = gmtime(&rawtime);
        if (ti != nullptr) {
            date.Year = ti->tm_year + 1900;
            date.Month = ti->tm_mon + 1;
            date.Date = ti->tm_mday;
            date.WeekDay = ti->tm_wday;
            time.Hours = ti->tm_hour;
            time.Minutes = ti->tm_min;
            time.Seconds = ti->tm_sec;
            valid = true;
        }
    }
    if (valid) {
        M5.Rtc.SetDate(&date);
        M5.Rtc.SetTime(&time);
        _syncSuccess = true;
        uint8_t ack[4] = {0x01, (uint8_t)time.Hours, (uint8_t)time.Minutes, (uint8_t)time.Seconds};
        pCharacteristic->setValue(ack, sizeof(ack));
        pCharacteristic->notify();
    }
}