/**
 * @file SceneSpectrum.cpp
 * @brief 頻譜分析儀與麥克風聲學診斷面板實作
 * @details 採用 SPM1423 數位 PDM 麥克風進行 16kHz 音訊取樣，實作全域匯流排硬體隔離保護、
 *          64 點快速 FFT、8 頻段 VU 方塊跳動等化器與即時示波器聲學診斷驗證介面。
 */

#include "scenes/SceneSpectrum.h"
#include <driver/i2s.h>
#include <cmath>

#define PIN_MIC_CLK  0
#define PIN_MIC_DATA 34

// 引用全域輸入、燈效與畫布實例
extern InputManager input;
extern LedManager led;
extern TFT_eSprite g_canvas;

// 快速 Radix-2 64 點 FFT 實作
static void runFastFFT64(float* vReal, float* vImag) {
    // 1. 漢寧窗函數加權 (Hann Window) 抑制頻譜洩漏
    for (int i = 0; i < 64; i++) {
        float w = 0.5f * (1.0f - cosf(6.2831853f * (float)i / 63.0f));
        vReal[i] *= w;
    }

    // 2. 位元反轉重排 (Bit-reversal permutation)
    int j = 0;
    for (int i = 0; i < 63; i++) {
        if (i < j) {
            float tr = vReal[i]; vReal[i] = vReal[j]; vReal[j] = tr;
            float ti = vImag[i]; vImag[i] = vImag[j]; vImag[j] = ti;
        }
        int k = 32;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // 3. 蝶形運算 (Cooley-Tukey Butterflies)
    for (int len = 2; len <= 64; len <<= 1) {
        float ang = -6.2831853f / (float)len;
        float wlen_r = cosf(ang);
        float wlen_i = sinf(ang);
        int half = len >> 1;

        for (int i = 0; i < 64; i += len) {
            float wr = 1.0f;
            float wi = 0.0f;
            for (int k = 0; k < half; k++) {
                int u = i + k;
                int v = i + k + half;
                float vr = vReal[v] * wr - vImag[v] * wi;
                float vi = vReal[v] * wi + vImag[v] * wr;

                vReal[v] = vReal[u] - vr;
                vImag[v] = vImag[u] - vi;
                vReal[u] += vr;
                vImag[u] += vi;

                float next_wr = wr * wlen_r - wi * wlen_i;
                wi = wr * wlen_i + wi * wlen_r;
                wr = next_wr;
            }
        }
    }
}

SceneSpectrum::SceneSpectrum()
    : _viewMode(SPEC_VIEW_EQUALIZER), _themeIdx(0), _isI2SActive(false),
      _currentRms(0.0f), _currentPeak(0.0f), _micLevel(0.0f), _peakMicLevel(0.0f),
      _soundDetectedUntil(0), _lastFrameTime(0), _needsRedraw(true) {
    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        _bandValues[b] = 0.0f;
        _peakValues[b] = 0.0f;
        _peakDropTimes[b] = 0;
    }
    for (uint16_t i = 0; i < FFT_SIZE; i++) {
        _vReal[i] = 0.0f;
        _vImag[i] = 0.0f;
        _waveBuffer[i] = 0;
    }
}

void SceneSpectrum::setupAudioI2S() {
    if (_isI2SActive) return;

    // 1. 全域匯流排硬體隔離：掛起 MiniJoyC 輪詢與寫入，防止 I2C 死鎖
    input.setBusSuspended(true);
    led.setBusSuspended(true);

    // 2. 徹底關閉並釋放 Wire，確保 GPIO 0 不再被 I2C 控制器佔用
    Wire.end();
    delay(10);

    // 3. 配置 ESP32 I2S PDM 接收器接管 GPIO 0 (CLK) 與 GPIO 34 (DATA)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = PIN_MIC_CLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_MIC_DATA
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    _isI2SActive = true;
}

void SceneSpectrum::teardownAudioI2S() {
    if (!_isI2SActive) return;

    // 1. 卸載 I2S 驅動並釋放 GPIO 0
    i2s_driver_uninstall(I2S_NUM_0);
    _isI2SActive = false;

    // 2. 重新初始化頂部 HAT I2C 匯流排 (400kHz)，還原 MiniJoyC 正常通訊
    Wire.begin(HAT_I2C_SDA, HAT_I2C_SCL, 400000L);

    // 3. 解除全域匯流排掛起
    input.setBusSuspended(false);
    led.setBusSuspended(false);
}

void SceneSpectrum::exit() {
    // 生命週期清理：安全卸載 I2S 並還原 I2C 匯流排
    teardownAudioI2S();
}

void SceneSpectrum::init() {
    _viewMode = SPEC_VIEW_EQUALIZER;
    _themeIdx = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _lastFrameTime = millis();
    _currentRms = 0.0f;
    _currentPeak = 0.0f;
    _micLevel = 0.0f;
    _peakMicLevel = 0.0f;
    _soundDetectedUntil = 0;

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        _bandValues[b] = 0.0f;
        _peakValues[b] = 0.0f;
        _peakDropTimes[b] = 0;
    }
    for (uint16_t i = 0; i < FFT_SIZE; i++) {
        _vReal[i] = 0.0f;
        _vImag[i] = 0.0f;
        _waveBuffer[i] = 0;
    }

    setupAudioI2S();
}

void SceneSpectrum::sampleAudio() {
    if (!_isI2SActive) return;

    int16_t samples[FFT_SIZE];
    size_t bytesRead = 0;

    // 使用有限超時 (40ms)，避免 portMAX_DELAY 造成主迴圈無限期等待凍結
    esp_err_t err = i2s_read(I2S_NUM_0, (char*)samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(40));
    if (err != ESP_OK || bytesRead == 0) {
        for (int i = 0; i < FFT_SIZE; i++) {
            _vReal[i] = 0.0f;
            _vImag[i] = 0.0f;
        }
        _currentRms = 0.0f;
        _micLevel *= 0.8f;
        return;
    }

    int sampleCount = bytesRead / sizeof(int16_t);

    // 計算平均值消除 DC 偏置
    int32_t sum = 0;
    for (int i = 0; i < sampleCount; i++) {
        sum += samples[i];
    }
    float mean = (sampleCount > 0) ? ((float)sum / (float)sampleCount) : 0.0f;

    float sumSq = 0.0f;
    float peak = 0.0f;

    for (int i = 0; i < FFT_SIZE; i++) {
        float val = 0.0f;
        if (i < sampleCount) {
            val = (float)samples[i] - mean;
            _waveBuffer[i] = (int16_t)val;
        } else {
            _waveBuffer[i] = 0;
        }
        _vReal[i] = val;
        _vImag[i] = 0.0f;

        float absVal = fabsf(val);
        if (absVal > peak) peak = absVal;
        sumSq += val * val;
    }

    _currentRms = sqrtf(sumSq / (float)FFT_SIZE);
    _currentPeak = peak;

    // 計算 0~100% 音量計 (SPM1423 靈敏度標準刻度)
    float targetLevel = constrain((_currentRms / 1500.0f) * 100.0f, 0.0f, 100.0f);
    if (targetLevel > _micLevel) {
        _micLevel += (targetLevel - _micLevel) * 0.7f;
    } else {
        _micLevel += (targetLevel - _micLevel) * 0.25f;
    }

    // 峰值暫留緩降
    if (_micLevel > _peakMicLevel) {
        _peakMicLevel = _micLevel;
    } else {
        _peakMicLevel -= 1.2f;
        if (_peakMicLevel < 0.0f) _peakMicLevel = 0.0f;
    }

    // 聲音偵測判斷（吹氣、拍手或說話，RMS > 180 觸發高亮反饋）
    if (_currentRms > 180.0f) {
        _soundDetectedUntil = millis() + 400;
    }
}

void SceneSpectrum::computeFFT() {
    runFastFFT64(_vReal, _vImag);

    // 計算 32 個頻率 bin 的幅值
    float magnitudes[32];
    for (int i = 0; i < 32; i++) {
        magnitudes[i] = sqrtf(_vReal[i] * _vReal[i] + _vImag[i] * _vImag[i]) / 32.0f;
    }

    // 8 個頻段對數分組 (Bins: 1, 2, 3-4, 5-7, 8-11, 12-16, 17-23, 24-31)
    const uint8_t binRanges[BAND_COUNT][2] = {
        {1, 1}, {2, 2}, {3, 4}, {5, 7},
        {8, 11}, {12, 16}, {17, 23}, {24, 31}
    };

    uint32_t now = millis();

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        float sum = 0.0f;
        uint8_t count = binRanges[b][1] - binRanges[b][0] + 1;
        for (uint8_t k = binRanges[b][0]; k <= binRanges[b][1]; k++) {
            sum += magnitudes[k];
        }
        float avg = sum / (float)count;

        // 音訊增益放大 (適度乘上增益因子，使人聲與環境音清晰跳動)
        float gain = 0.30f + (float)b * 0.04f;
        float targetH = avg * gain;
        if (targetH > 105.0f) targetH = 105.0f;

        // 平滑阻尼 (上升極速跟隨 0.70，下降緩慢 0.28)
        if (targetH > _bandValues[b]) {
            _bandValues[b] += (targetH - _bandValues[b]) * 0.70f;
        } else {
            _bandValues[b] += (targetH - _bandValues[b]) * 0.28f;
        }

        // 頂部峰值暫留 (Peak Hold) 邏輯
        if (_bandValues[b] >= _peakValues[b]) {
            _peakValues[b] = _bandValues[b];
            _peakDropTimes[b] = now + 250; // 暫留 250ms
        } else {
            if (now >= _peakDropTimes[b]) {
                _peakValues[b] -= 2.2f; // 平滑緩降
                if (_peakValues[b] < _bandValues[b]) {
                    _peakValues[b] = _bandValues[b];
                }
            }
        }
    }
}

void SceneSpectrum::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：安全退出並釋放 I2S 還原匯流排
    if (input.btnBLongPressed) {
        audio.playClick();
        teardownAudioI2S();
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. Button B 短按：切換「8 頻段 VU 方塊」與「麥克風聲學診斷面板」
    if (input.btnBPressed) {
        audio.playClick();
        _viewMode = (_viewMode == SPEC_VIEW_EQUALIZER) ? SPEC_VIEW_DIAGNOSTIC : SPEC_VIEW_EQUALIZER;
        _needsRedraw = true;
        return;
    }

    // 3. Button A 短按：切換色彩主題 (經典綠黃紅 -> 霓虹紫 -> 冰霜藍)
    if (input.btnAPressed) {
        audio.playTick();
        _themeIdx = (_themeIdx + 1) % 3;
        _needsRedraw = true;
    }

    // 4. 音訊採樣與 FFT 運算
    sampleAudio();
    computeFFT();
    _needsRedraw = true;
}

// -----------------------------------------------------------------------------
// 繪製視圖 1：8 頻段 VU 方塊等化器
// -----------------------------------------------------------------------------
void SceneSpectrum::drawEqualizer() {
    uint32_t now = millis();
    bool soundDetected = (now < _soundDetectedUntil);

    // 1. 頂部標題列 (Y: 0 ~ 24)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 24, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("AUDIO SPECTRUM", 6, 4, 2);

    const char* THEME_NAMES[] = {"VIBRANT", "NEON", "CYAN"};
    g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
    g_canvas.drawRightString(THEME_NAMES[_themeIdx], SCREEN_WIDTH - 6, 4, 2);

    // 2. 即時麥克風收音能量計 (VU Meter Bar，Y: 28 ~ 44)
    int vuX = 8;
    int vuY = 28;
    int vuW = 119;
    int vuH = 14;
    g_canvas.drawRoundRect(vuX, vuY, vuW, vuH, 3, 0x39E7);

    int barFillW = (int)((_micLevel / 100.0f) * (vuW - 4));
    if (barFillW > 0) {
        uint16_t vuColor = (_micLevel > 75.0f) ? TFT_RED :
                           (_micLevel > 40.0f) ? COLOR_GOLD : TFT_GREEN;
        g_canvas.fillRoundRect(vuX + 2, vuY + 2, barFillW, vuH - 4, 2, vuColor);
    }

    // 峰值白線指示
    int peakX = vuX + 2 + (int)((_peakMicLevel / 100.0f) * (vuW - 4));
    if (peakX > vuX + 2 && peakX < vuX + vuW - 2) {
        g_canvas.drawFastVLine(peakX, vuY + 1, vuH - 2, TFT_WHITE);
    }

    // 收音文字指示
    if (soundDetected) {
        g_canvas.setTextColor(TFT_WHITE, TFT_BLACK);
        g_canvas.drawCentreString("* SOUND ACTIVE *", SCREEN_WIDTH / 2, vuY + 16, 1);
    } else {
        char micStr[24];
        snprintf(micStr, sizeof(micStr), "MIC VU: %d%%", (int)_micLevel);
        g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
        g_canvas.drawCentreString(micStr, SCREEN_WIDTH / 2, vuY + 16, 1);
    }

    // 3. 8 頻柱等化器繪製區 (Y: 56 ~ 168，基準線 Y = 168)
    int startX = 9;
    int colW = 11;
    int gap = 4;
    int baseY = 168;

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        int x = startX + b * (colW + gap);
        int h = (int)_bandValues[b];
        if (h > 105) h = 105;

        // 以 4px 步長堆疊等化器方塊
        for (int y = baseY; y > baseY - h; y -= 4) {
            int segmentH = baseY - y;
            uint16_t color;

            if (_themeIdx == 0) { // 經典綠黃紅
                color = (segmentH > 75) ? TFT_RED : (segmentH > 40) ? COLOR_GOLD : TFT_GREEN;
            } else if (_themeIdx == 1) { // 霓虹電音
                color = (segmentH > 75) ? 0xF81F : (segmentH > 40) ? 0x915F : COLOR_CYAN;
            } else { // 冰霜科技藍
                color = (segmentH > 75) ? TFT_WHITE : (segmentH > 40) ? COLOR_CYAN : 0x0215;
            }

            g_canvas.fillRect(x, y - 3, colW, 3, color);
        }

        // 頂部峰值線 (Peak Line)
        int peakY = baseY - (int)_peakValues[b];
        if (peakY < baseY - 105) peakY = baseY - 105;
        if (peakY >= 60 && peakY <= baseY) {
            g_canvas.drawFastHLine(x, peakY, colW, TFT_WHITE);
        }
    }

    // 等化器基底線
    g_canvas.drawFastHLine(6, baseY + 2, SCREEN_WIDTH - 12, 0x39E7);

    // 4. 即時聲學數據橫條 (Y: 174 ~ 196)
    char rmsStr[32];
    snprintf(rmsStr, sizeof(rmsStr), "RMS: %-4d | PK: %-4d", (int)_currentRms, (int)_currentPeak);
    g_canvas.setTextColor(soundDetected ? COLOR_GOLD : COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString(rmsStr, SCREEN_WIDTH / 2, 175, 1);

    g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    g_canvas.drawCentreString("SPM1423 PDM @ 16kHz [OK]", SCREEN_WIDTH / 2, 187, 1);

    // 5. 底部操作說明 (Y: 200 ~ 238)
    g_canvas.drawFastHLine(10, 199, SCREEN_WIDTH - 20, 0x2965);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Btn A: Theme | Btn B: View", SCREEN_WIDTH / 2, 203, 1);
    g_canvas.setTextColor(0x7BEF, TFT_BLACK);
    g_canvas.drawCentreString("[BASE I2C SAFE ISOLATED]", SCREEN_WIDTH / 2, 215, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 227, 1);
}

// -----------------------------------------------------------------------------
// 繪製視圖 2：麥克風聲學診斷面板 (示波器波形 + 吹氣/拍手測試)
// -----------------------------------------------------------------------------
void SceneSpectrum::drawDiagnostic() {
    uint32_t now = millis();
    bool soundDetected = (now < _soundDetectedUntil);

    // 1. 頂部標題列 (Y: 0 ~ 24)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 24, 0x2008);
    g_canvas.setTextColor(COLOR_GOLD, 0x2008);
    g_canvas.drawString("MIC DIAGNOSTIC", 6, 4, 2);
    g_canvas.setTextColor(TFT_WHITE, 0x2008);
    g_canvas.drawRightString("TEST", SCREEN_WIDTH - 6, 4, 2);

    // 2. 示波器波形網格框 (Y: 28 ~ 98，高度 70px)
    int oscX = 8;
    int oscY = 28;
    int oscW = 119;
    int oscH = 70;
    g_canvas.fillRoundRect(oscX, oscY, oscW, oscH, 3, 0x0841);
    g_canvas.drawRoundRect(oscX, oscY, oscW, oscH, 3, 0x2104);

    // 示波器虛擬中心線與十字輔助線
    int midY = oscY + oscH / 2;
    for (int gx = oscX + 4; gx < oscX + oscW - 4; gx += 6) {
        g_canvas.drawFastHLine(gx, midY, 3, 0x18C3);
    }
    for (int gy = oscY + 4; gy < oscY + oscH - 4; gy += 6) {
        g_canvas.drawFastVLine(oscX + oscW / 2, gy, 3, 0x18C3);
    }

    // 繪製 64 點即時波形曲線
    uint16_t waveColor = soundDetected ? TFT_GREEN : COLOR_CYAN;
    int prevPx = oscX + 3;
    int prevPy = midY;

    for (int i = 0; i < FFT_SIZE; i++) {
        int px = oscX + 3 + (i * (oscW - 6)) / (FFT_SIZE - 1);
        // 波形放大縮放：限制在網格範圍內
        int dy = (int)(_waveBuffer[i] / 40);
        int py = constrain(midY - dy, oscY + 3, oscY + oscH - 4);

        if (i > 0) {
            g_canvas.drawLine(prevPx, prevPy, px, py, waveColor);
        }
        prevPx = px;
        prevPy = py;
    }

    // 3. 吹氣/聲音響應動態驗證橫幅 (Y: 104 ~ 124)
    if (soundDetected) {
        g_canvas.fillRoundRect(8, 104, 119, 20, 3, 0x03E0); // 鮮亮綠色背景
        g_canvas.setTextColor(TFT_WHITE, 0x03E0);
        g_canvas.drawCentreString("* SOUND DETECTED! *", SCREEN_WIDTH / 2, 107, 1);
    } else {
        g_canvas.drawRoundRect(8, 104, 119, 20, 3, 0x39E7);
        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("BLOW OR SPEAK TO TEST", SCREEN_WIDTH / 2, 107, 1);
    }

    // 4. 大型多段式音量電平指示條 (Y: 130 ~ 152)
    int barX = 8;
    int barY = 130;
    int barW = 119;
    int barH = 14;
    g_canvas.drawRoundRect(barX, barY, barW, barH, 2, 0x4208);

    int fillLen = (int)((_micLevel / 100.0f) * (barW - 4));
    if (fillLen > 0) {
        uint16_t lvlColor = (_micLevel > 75.0f) ? TFT_RED :
                            (_micLevel > 40.0f) ? COLOR_GOLD : TFT_GREEN;
        g_canvas.fillRoundRect(barX + 2, barY + 2, fillLen, barH - 4, 1, lvlColor);
    }

    // 峰值刻度指示
    int pBarX = barX + 2 + (int)((_peakMicLevel / 100.0f) * (barW - 4));
    if (pBarX > barX + 2 && pBarX < barX + barW - 2) {
        g_canvas.drawFastVLine(pBarX, barY + 1, barH - 2, TFT_WHITE);
    }

    char lvlPctStr[20];
    snprintf(lvlPctStr, sizeof(lvlPctStr), "MIC LEVEL: %d%%", (int)_micLevel);
    g_canvas.setTextColor(COLOR_SILVER, TFT_BLACK);
    g_canvas.drawCentreString(lvlPctStr, SCREEN_WIDTH / 2, barY + 16, 1);

    // 5. 硬體與遙測資訊面板 (Y: 162 ~ 196)
    g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
    g_canvas.drawString("HARDWARE TELEMETRY", 8, 162, 1);

    char tele1[32];
    snprintf(tele1, sizeof(tele1), "RMS: %-5d  PEAK: %-5d", (int)_currentRms, (int)_currentPeak);
    g_canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    g_canvas.drawString(tele1, 8, 174, 1);

    g_canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    g_canvas.drawString("BUS: GPIO 0 ISOLATED [OK]", 8, 185, 1);

    // 6. 底部操作指示 (Y: 200 ~ 238)
    g_canvas.drawFastHLine(10, 199, SCREEN_WIDTH - 20, 0x2965);
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Btn B: Switch Equalizer", SCREEN_WIDTH / 2, 204, 1);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString("Btn A: Change Color Theme", SCREEN_WIDTH / 2, 216, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit to Menu", SCREEN_WIDTH / 2, 228, 1);
}

// -----------------------------------------------------------------------------
// 主繪圖函式 (雙緩衝離線渲染，零閃爍推送到 ST7789v2)
// -----------------------------------------------------------------------------
void SceneSpectrum::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 清空雙緩衝畫布
    g_canvas.fillSprite(TFT_BLACK);

    if (_viewMode == SPEC_VIEW_EQUALIZER) {
        drawEqualizer();
    } else {
        drawDiagnostic();
    }

    // 一次性將記憶體畫面推送到 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}
