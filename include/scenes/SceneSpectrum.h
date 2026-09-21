/**
 * @file SceneSpectrum.h
 * @brief 頻譜分析儀 (Spectrum Analyzer) 場景標頭檔
 * @details 支援 SPM1423 聲音音訊 FFT 頻譜與 MPU6886 體感微震動頻譜雙模式，
 *          具備 GPIO 0 匯流排動態時序隔離保護、8 頻段 VU 方塊跳動與頂部峰值暫留緩降。
 */

#pragma once

#include "Scene.h"

enum SpectrumMode {
    SPEC_MODE_AUDIO = 0,    // SPM1423 麥克風音訊 FFT 頻譜 (GPIO 0 隔離模式)
    SPEC_MODE_IMU           // MPU6886 體感微震動頻譜 (全周邊共存模式)
};

class SceneSpectrum : public Scene {
public:
    SceneSpectrum();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SPECTRUM; }

private:
    void setupAudioI2S();
    void teardownAudioI2S();
    void sampleAudio();
    void sampleImu();
    void computeFFT();

    SpectrumMode _mode;
    uint8_t _themeIdx;      // 0: 經典綠黃紅, 1: 電音霓虹, 2: 冰霜冷藍
    bool _isI2SActive;

    // 64 點實數 FFT 採樣緩衝區
    static const uint16_t FFT_SIZE = 64;
    float _vReal[FFT_SIZE];
    float _vImag[FFT_SIZE];

    // 8 根頻段柱高度與頂部峰值
    static const uint8_t BAND_COUNT = 8;
    float _bandValues[BAND_COUNT];
    float _peakValues[BAND_COUNT];
    uint32_t _peakDropTimes[BAND_COUNT];

    uint32_t _lastFrameTime;
    bool _needsRedraw;
};
