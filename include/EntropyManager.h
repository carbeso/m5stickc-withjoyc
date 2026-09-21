/**
 * @file EntropyManager.h
 * @brief 物理熵源管理器 (Entropy Manager)
 * @details 擷取 ESP32 硬體 TRNG、MPU6886 IMU 姿態微動熱噪、MiniJoyC 搖桿 ADC 微噪與微秒時鐘差，
 *          透過非線性雜湊混合池生成高均勻度、物理不可預測的真隨機數。
 */

#pragma once

#include <Arduino.h>

class EntropyManager {
public:
    /**
     * @brief 初始化熵池（開機時結合 ESP32 TRNG 與初始時間戳）
     */
    static void init();

    /**
     * @brief 注入單一 32-bit 雜訊取樣
     * @param sample 雜訊數值
     */
    static void feedEntropy(uint32_t sample);

    /**
     * @brief 注入物理感測器即時微噪取樣（搖桿、加速度計、陀螺儀、時間戳）
     * @param joyX 搖桿 X 軸數值
     * @param joyY 搖桿 Y 軸數值
     * @param ax 加速度 X (G)
     * @param ay 加速度 Y (G)
     * @param az 加速度 Z (G)
     * @param gx 陀螺儀 X (deg/s)
     * @param gy 陀螺儀 Y (deg/s)
     * @param gz 陀螺儀 Z (deg/s)
     */
    static void feedPhysicalSamples(int16_t joyX, int16_t joyY,
                                    float ax, float ay, float az,
                                    float gx, float gy, float gz);

    /**
     * @brief 取得 32-bit 無號隨機整數
     * @return uint32_t 隨機數值
     */
    static uint32_t getUInt32();

    /**
     * @brief 取得指定範圍內的隨機整數 [minVal, maxVal)
     * @param minVal 最小值（包含）
     * @param maxVal 最大值（不包含）
     * @return int32_t 隨機整數
     */
    static int32_t random(int32_t minVal, int32_t maxVal);

    /**
     * @brief 取得 [0, maxVal) 範圍內的隨機整數
     * @param maxVal 最大值（不包含）
     * @return int32_t 隨機整數
     */
    static int32_t random(int32_t maxVal);

    /**
     * @brief 取得 [0.0, 1.0) 範圍內的單精度隨機浮點數
     * @return float 隨機浮點數
     */
    static float randomFloat();

private:
    static uint64_t _entropyPool; // 全域 64 位元非線性熵池
    static uint32_t _mixCounter;  // 攪拌計數器
};
