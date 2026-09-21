/**
 * @file SceneSpectrum.cpp
 * @brief 頻譜分析儀實作：SPM1423 麥克風音訊 FFT 與 MPU6886 震動頻譜
 */

#include "scenes/SceneSpectrum.h"
#include <driver/i2s.h>
#include <cmath>

#define PIN_MIC_CLK  0
#define PIN_MIC_DATA 34

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
    : _mode(SPEC_MODE_AUDIO), _themeIdx(0), _isI2SActive(false),
      _lastFrameTime(0), _needsRedraw(true) {
    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        _bandValues[b] = 0.0f;
        _peakValues[b] = 0.0f;
        _peakDropTimes[b] = 0;
    }
}

void SceneSpectrum::setupAudioI2S() {
    if (_isI2SActive) return;

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
    i2s_set_clk(I2S_NUM_0, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    _isI2SActive = true;
}

void SceneSpectrum::teardownAudioI2S() {
    if (!_isI2SActive) return;

    i2s_driver_uninstall(I2S_NUM_0);
    _isI2SActive = false;

    // 釋放 GPIO 0 後重新初始化 I2C 匯流排以還原 MiniJoyC
    Wire.begin(HAT_I2C_SDA, HAT_I2C_SCL, 400000L);
}

void SceneSpectrum::init() {
    _mode = SPEC_MODE_AUDIO;
    _themeIdx = 0;
    _needsRedraw = true;
    _nextScene = SCENE_COUNT;
    _lastFrameTime = millis();

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        _bandValues[b] = 0.0f;
        _peakValues[b] = 0.0f;
        _peakDropTimes[b] = 0;
    }

    setupAudioI2S();
}

void SceneSpectrum::sampleAudio() {
    int16_t samples[FFT_SIZE];
    size_t bytesRead = 0;

    i2s_read(I2S_NUM_0, (char*)samples, sizeof(samples), &bytesRead, portMAX_DELAY);

    // 計算平均值消除 DC 偏置
    int32_t sum = 0;
    for (int i = 0; i < FFT_SIZE; i++) {
        sum += samples[i];
    }
    float mean = (float)sum / (float)FFT_SIZE;

    for (int i = 0; i < FFT_SIZE; i++) {
        _vReal[i] = (float)samples[i] - mean;
        _vImag[i] = 0.0f;
    }
}

void SceneSpectrum::sampleImu() {
    // 讀取三軸加速度與角速度高頻震動
    for (int i = 0; i < FFT_SIZE; i++) {
        float ax = 0, ay = 0, az = 0;
        M5.Imu.getAccelData(&ax, &ay, &az);
        float mag = sqrtf(ax * ax + ay * ay + az * az) - 1.0f; // 扣除 1G 重力
        _vReal[i] = mag * 1000.0f;
        _vImag[i] = 0.0f;
        delayMicroseconds(120); // 取樣間隔
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

        // 縮放並限制高度至 0 ~ 130px
        float targetH = (_mode == SPEC_MODE_AUDIO) ? (avg * 0.15f) : (avg * 0.08f);
        if (targetH > 130.0f) targetH = 130.0f;

        // 平滑阻尼 (上升極速跟隨 0.65，下降緩慢 0.3)
        if (targetH > _bandValues[b]) {
            _bandValues[b] += (targetH - _bandValues[b]) * 0.65f;
        } else {
            _bandValues[b] += (targetH - _bandValues[b]) * 0.30f;
        }

        // 頂部峰值暫留 (Peak Hold) 邏輯
        if (_bandValues[b] >= _peakValues[b]) {
            _peakValues[b] = _bandValues[b];
            _peakDropTimes[b] = now + 250; // 暫留 250ms
        } else {
            if (now >= _peakDropTimes[b]) {
                _peakValues[b] -= 2.5f; // 平滑緩降
                if (_peakValues[b] < _bandValues[b]) {
                    _peakValues[b] = _bandValues[b];
                }
            }
        }
    }
}

void SceneSpectrum::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：安全退出並釋放匯流排
    if (input.btnBLongPressed) {
        audio.playClick();
        teardownAudioI2S();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. 模式切換：在非 I2S 或透過 Button B 短按切換 Audio / IMU 模式
    if (input.btnBPressed) {
        audio.playClick();
        if (_mode == SPEC_MODE_AUDIO) {
            teardownAudioI2S();
            _mode = SPEC_MODE_IMU;
        } else {
            _mode = SPEC_MODE_AUDIO;
            setupAudioI2S();
        }
        _needsRedraw = true;
        return;
    }

    // 3. Button A：切換色彩主題 (綠黃紅 -> 霓虹紫 -> 冰霜藍)
    if (input.btnAPressed) {
        _themeIdx = (_themeIdx + 1) % 3;
        audio.playTick();
        _needsRedraw = true;
    }

    // 4. 採樣與 FFT 運算
    if (_mode == SPEC_MODE_AUDIO) {
        sampleAudio();
    } else {
        sampleImu();
        // IMU 模式連動底座 RGB
        uint8_t energy = (uint8_t)constrain(_bandValues[1] * 2.0f, 0.0f, 255.0f);
        led.setColor(energy, 0, 255 - energy);
    }

    computeFFT();
    _needsRedraw = true;
}

void SceneSpectrum::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布在記憶體中繪製，避免 SPI 逐像素擦除造成的閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部標題列 (Y: 0 ~ 26)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString((_mode == SPEC_MODE_AUDIO) ? "AUDIO FFT" : "IMU VIBE", 6, 5, 2);

    const char* THEME_NAMES[] = {"VIBRANT", "NEON", "CYAN"};
    g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
    g_canvas.drawRightString(THEME_NAMES[_themeIdx], SCREEN_WIDTH - 6, 5, 2);

    // 2. 8 頻柱等化器繪製區 (Y: 40 ~ 175，基準線 Y = 175)
    int startX = 6;
    int colW = 12;
    int gap = 4;
    int baseY = 175;

    for (uint8_t b = 0; b < BAND_COUNT; b++) {
        int x = startX + b * (colW + gap);
        int h = (int)_bandValues[b];
        if (h > 130) h = 130;

        // 以 4px 步長堆疊方塊
        for (int y = baseY; y > baseY - h; y -= 4) {
            int segmentH = baseY - y;
            uint16_t color;

            if (_themeIdx == 0) { // 經典綠黃紅
                color = (segmentH > 95) ? TFT_RED : (segmentH > 55) ? COLOR_GOLD : TFT_GREEN;
            } else if (_themeIdx == 1) { // 霓虹電音
                color = (segmentH > 95) ? 0xF81F : (segmentH > 55) ? 0x915F : COLOR_CYAN;
            } else { // 冰霜科技藍
                color = (segmentH > 95) ? TFT_WHITE : (segmentH > 55) ? COLOR_CYAN : 0x0113;
            }

            g_canvas.fillRect(x, y - 3, colW, 3, color);
        }

        // 頂部峰值線 (Peak Line)
        int peakY = baseY - (int)_peakValues[b];
        if (peakY < baseY - 130) peakY = baseY - 130;
        if (peakY >= 35 && peakY <= baseY) {
            g_canvas.drawFastHLine(x, peakY, colW, TFT_WHITE);
        }
    }

    // 等化器基底線
    g_canvas.drawFastHLine(4, baseY + 2, SCREEN_WIDTH - 8, 0x39E7);

    // 3. 底部操作指示 (Y: 195 ~ 238)
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString("Btn A: Theme  Btn B: Mode", SCREEN_WIDTH / 2, 198, 1);
    g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
    g_canvas.drawCentreString((_mode == SPEC_MODE_AUDIO) ? "[MIC ACTIVE: BUS ISOLATED]" : "[IMU ACTIVE: FULL JOY]", SCREEN_WIDTH / 2, 212, 1);
    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Hold Btn B: Exit", SCREEN_WIDTH / 2, 226, 1);

    // 一次性將記憶體幀推送到 ST7789v2 螢幕，達成 0 閃爍 60FPS 視覺效果
    g_canvas.pushSprite(0, 0);
}
