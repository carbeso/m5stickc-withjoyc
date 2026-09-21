/**
 * @file EntropyManager.cpp
 * @brief 物理熵源管理器實作
 */

#include "EntropyManager.h"
#include <esp_system.h>
#include <cstring>

// 初始熵池魔術種子 (黃金分割比衍生常數)
uint64_t EntropyManager::_entropyPool = 0x9E3779B97F4A7C15ULL;
uint32_t EntropyManager::_mixCounter = 0;

void EntropyManager::init() {
    // 結合開機時的 ESP32 硬體 TRNG 與微秒時鐘
    uint64_t r1 = esp_random();
    uint64_t r2 = esp_random();
    _entropyPool = (r1 << 32) | r2;
    feedEntropy(micros());
}

void EntropyManager::feedEntropy(uint32_t sample) {
    // SplitMix64 變形雜湊攪拌演算法
    _mixCounter++;
    _entropyPool ^= ((uint64_t)sample << 32) | (sample ^ _mixCounter);
    _entropyPool += 0x9E3779B97F4A7C15ULL;
    _entropyPool = (_entropyPool ^ (_entropyPool >> 30)) * 0xBF58476D1CE4E5B9ULL;
    _entropyPool = (_entropyPool ^ (_entropyPool >> 27)) * 0x94D049BB133111EBULL;
    _entropyPool ^= (_entropyPool >> 31);
}

void EntropyManager::feedPhysicalSamples(int16_t joyX, int16_t joyY,
                                        float ax, float ay, float az,
                                        float gx, float gy, float gz) {
    // 1. 擷取搖桿類比微噪
    uint32_t joyBits = ((uint16_t)joyX << 16) | (uint16_t)joyY;

    // 2. 擷取 IMU 浮點數底層二進位讀數（微動熱噪集中在尾數 Mantissa 最低位元）
    uint32_t aBits = 0, gBits = 0;
    uint32_t tmp;
    memcpy(&tmp, &ax, 4); aBits ^= tmp;
    memcpy(&tmp, &ay, 4); aBits = (aBits << 5) | (aBits >> 27); aBits ^= tmp;
    memcpy(&tmp, &az, 4); aBits = (aBits << 5) | (aBits >> 27); aBits ^= tmp;

    memcpy(&tmp, &gx, 4); gBits ^= tmp;
    memcpy(&tmp, &gy, 4); gBits = (gBits << 7) | (gBits >> 25); gBits ^= tmp;
    memcpy(&tmp, &gz, 4); gBits = (gBits << 7) | (gBits >> 25); gBits ^= tmp;

    // 3. 連同微秒時鐘一起注入熵池
    uint32_t mixed = joyBits ^ aBits ^ gBits ^ (uint32_t)micros();
    feedEntropy(mixed);
}

uint32_t EntropyManager::getUInt32() {
    // 攪拌一次並混合 ESP32 硬體 TRNG，確保不可預測性
    feedEntropy(esp_random());
    uint64_t z = (_entropyPool += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return (uint32_t)(z ^ (z >> 31));
}

int32_t EntropyManager::random(int32_t minVal, int32_t maxVal) {
    if (minVal >= maxVal) return minVal;
    uint32_t range = (uint32_t)(maxVal - minVal);
    uint32_t val = getUInt32();
    return minVal + (int32_t)(val % range);
}

int32_t EntropyManager::random(int32_t maxVal) {
    if (maxVal <= 0) return 0;
    return (int32_t)(getUInt32() % (uint32_t)maxVal);
}

float EntropyManager::randomFloat() {
    // 輸出 [0.0, 1.0)
    return (float)(getUInt32() >> 8) * (1.0f / 16777216.0f);
}
