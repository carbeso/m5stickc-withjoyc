/**
 * @file main.cpp
 * @brief M5StickC Plus 與 MiniJoyC HAT 硬體功能驗證測試程式
 * @details 專為紓壓玩具專案設計之硬體檢測韌體，驗證 LCD 顯示、雙軸搖桿、微動按鍵、
 *          無源蜂鳴器、SK6812 全彩 RGB LED 以及 MPU6886 六軸姿態感測器。
 */

#include <M5StickCPlus.h>
#include "M5HatMiniJoyC.h"

// 建立 MiniJoyC 擴充底座驅動實體
M5HatMiniJoyC joyc;

// 蜂鳴器接腳定義
#define BUZZER_PIN 2

// 狀態計數器與定時器
uint32_t lastUpdate = 0;
uint8_t colorHue = 0;

/**
 * @brief 將 HSV 色彩空間轉換為 RGB888 格式
 * @param h 色相 (0 ~ 255)
 * @param s 飽和度 (0 ~ 255)
 * @param v 明度 (0 ~ 255)
 * @return 32 位元 RGB888 色彩值
 */
uint32_t hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r = 0, g = 0, b = 0;
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/**
 * @brief 發出短促的清脆按鍵音 (模擬機械鍵盤軸體聲音)
 */
void playClickSound() {
    tone(BUZZER_PIN, 2400, 15); // 2.4kHz 頻率發聲 15ms
}

void setup() {
    // 1. 初始化 M5StickC Plus (啟用 LCD、電源管理、停用內建揚聲器由 tone 代替)
    M5.begin(true, true, false);
    M5.Lcd.setRotation(1); // 橫向顯示 (240 x 135)
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

    M5.Lcd.drawString("M5StickC Plus + MiniJoyC", 10, 10, 2);
    M5.Lcd.drawString("Hardware Initializing...", 10, 30, 2);

    // 2. 初始化無源蜂鳴器接腳
    pinMode(BUZZER_PIN, OUTPUT);
    playClickSound(); // 開機提示音

    // 3. 初始化頂部 HAT 擴充槽 I2C 匯流排對接 MiniJoyC
    // MiniJoyC 接腳：SDA = GPIO 0, SCL = GPIO 26, 速率 = 400kHz, 位址 = 0x54
    bool joycSuccess = joyc.begin(&Wire, MiniJoyC_ADDR, 0, 26, 400000L);

    M5.Lcd.fillScreen(BLACK);
    if (joycSuccess) {
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.drawString("[OK] MiniJoyC Connected!", 10, 10, 2);
        // 設定初次燈光為翠綠色
        joyc.setLEDColor(0x00FF33);
    } else {
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.drawString("[WARN] JoyC Not Found @0x54", 10, 10, 2);
    }

    delay(800);
    M5.Lcd.fillScreen(BLACK);
}

void loop() {
    // 更新 M5 按鍵狀態
    M5.update();

    // 檢查 StickC 實體按鍵觸發
    if (M5.BtnA.wasPressed()) {
        playClickSound();
        joyc.setLEDColor(0xFFCC00); // 閃爍金黃光（木魚模式）
    }
    if (M5.BtnB.wasPressed()) {
        playClickSound();
        joyc.setLEDColor(0xFF0055); // 閃爍桃紅光（切換指示）
    }

    // 定時讀取搖桿與姿態資訊並更新畫面 (約 30 FPS)
    if (millis() - lastUpdate > 33) {
        lastUpdate = millis();

        // 讀取 MiniJoyC 搖桿 10-bit 位置與按鍵狀態 (注意：有號數 int16_t，中心約為 0)
        int16_t joyX = (int16_t)joyc.getPOSValue(POS_X, _10bit);
        int16_t joyY = (int16_t)joyc.getPOSValue(POS_Y, _10bit);
        bool joyBtn = joyc.getButtonStatus();

        if (joyBtn) {
            // 搖桿按鍵按下瞬間
            playClickSound();
        }

        // 讀取 MPU6886 六軸加速度計數值
        float ax = 0, ay = 0, az = 0;
        M5.IMU.getAccelData(&ax, &ay, &az);

        // 動態更新 RGB 氛圍燈 (色相環漸變)
        colorHue += 2;
        joyc.setLEDColor(hsvToRgb(colorHue, 255, 120));

        // 螢幕繪製資訊看板
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Lcd.drawString("=== Fidget Toy Diagnostic ===", 10, 5, 2);

        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.setCursor(10, 30, 2);
        M5.Lcd.printf("Joy X: %4d   Y: %4d\n", joyX, joyY);

        M5.Lcd.setCursor(10, 50, 2);
        M5.Lcd.printf("Joy Btn: %s\n", joyBtn ? "PRESSED (1)" : "RELEASE (0)");

        M5.Lcd.setCursor(10, 70, 2);
        M5.Lcd.printf("IMU Accel: X:%.2f Y:%.2f\n", ax, ay);

        M5.Lcd.setCursor(10, 90, 2);
        M5.Lcd.printf("BtnA: %s | BtnB: %s\n",
                      M5.BtnA.isPressed() ? "DOWN" : "UP  ",
                      M5.BtnB.isPressed() ? "DOWN" : "UP  ");

        // 繪製中心虛擬準心與搖桿偏移小圓點
        int centerX = 195;
        int centerY = 75;
        int maxRadius = 35;
        M5.Lcd.drawCircle(centerX, centerY, maxRadius, TFT_DARKGREY);
        M5.Lcd.drawLine(centerX - 5, centerY, centerX + 5, centerY, TFT_DARKGREY);
        M5.Lcd.drawLine(centerX, centerY - 5, centerX, centerY + 5, TFT_DARKGREY);

        // 將 -512 ~ 511 映射至圓形範圍
        int dotX = centerX + (joyX * (maxRadius - 4) / 512);
        int dotY = centerY + (joyY * (maxRadius - 4) / 512);
        M5.Lcd.fillCircle(dotX, dotY, 4, joyBtn ? TFT_RED : TFT_YELLOW);
    }
}
