/**
 * @file SceneSpectrum.h
 * @brief 頻譜分析儀 (Spectrum Analyzer) 與麥克風診斷面板標頭檔
 * @details 採用 SPM1423 數位 PDM 麥克風進行 16kHz 音訊取樣，支援 8 頻段 VU 方塊等化器
 *          與即時聲學示波器/診斷面板。具備 GPIO 0 / I2C 匯流排全域動態隔離防當保護。
 */

#pragma once

#include "Scene.h"

enum SpectrumView {
    SPEC_VIEW_EQUALIZER = 0,    // 8 頻段 VU 方塊跳動頻譜 + 即時 VU 能量條
    SPEC_VIEW_DIAGNOSTIC        // 麥克風聲學診斷面板 + 示波器波形 + RMS 數值 + 吹氣/拍手測試
};

class SceneSpectrum : public Scene {
public:
    SceneSpectrum();
    void init() override;
    void exit() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_SPECTRUM; }

private:
    void setupAudioI2S();
    void teardownAudioI2S();
    void sampleAudio();
    void computeFFT();

    void drawEqualizer();
    void drawDiagnostic();

    SpectrumView _viewMode;
    uint8_t _themeIdx;          // 0: 經典綠黃紅, 1: 電音霓虹, 2: 冰霜冷藍
    bool _isI2SActive;

    // 64 點實數 FFT 採樣緩衝區與示波器波形緩衝
    static const uint16_t FFT_SIZE = 64;
    float _vReal[FFT_SIZE];
    float _vImag[FFT_SIZE];
    int16_t _waveBuffer[FFT_SIZE];

    // 8 根頻段柱高度與頂部峰值
    static const uint8_t BAND_COUNT = 8;
    float _bandValues[BAND_COUNT];
    float _peakValues[BAND_COUNT];
    uint32_t _peakDropTimes[BAND_COUNT];

    // 麥克風即時診斷與音量測量數據
    float _currentRms;
    float _currentPeak;
    float _micLevel;            // 0.0f ~ 100.0f 平滑音量計
    float _peakMicLevel;        // 0.0f ~ 100.0f 峰值指標
    uint32_t _soundDetectedUntil; // 吹氣/拍手聲音偵測高亮時間戳

    uint32_t _lastFrameTime;
    bool _needsRedraw;
};
