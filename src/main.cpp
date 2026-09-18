/**
 * @file main.cpp
 * @brief M5StickC Plus 與 MiniJoyC HAT 硬體功能驗證測試程式 (靜音/邊緣觸發版)
 * @details 修復蜂鳴器持續鳴叫問題，改為邊緣觸發 (Edge-triggered) 且預設極低音量/短促，
 *          並在螢幕顯示搖桿按鍵原始數值。
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

// 按鍵前一次狀態 (用於邊緣偵測，避免每幀重複觸發)
bool prevJoyBtn = false;
bool soundEnabled = true; // 可透過 Button B 切換靜音

/**
 * @brief 將 HSV 色彩空間轉換為 RGB888 格式
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
 * @brief 發出極輕微、極短促的按鍵音 (8ms 輕微木質點擊感)
 */
void playClickSound() {
    if (!soundEnabled) return;
    tone(BUZZER_PIN, 1800, 8); // 輕微 8ms 點擊音，不擾民
}

void setup() {
    // 1. 初始化 M5StickC Plus
    M5.begin(true, true, false);
    M5.Lcd.setRotation(1); // 橫向顯示 (240 x 135)
    M5.Lcd.fillScreen(BLACK);

    // 確保蜂鳴器先處於完全靜音狀態
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.drawString("Fidget Toy Init...", 10, 10, 2);

    // 2. 初始化頂部 HAT 擴充槽 I2C (SDA = 0, SCL = 26)
    joyc.begin(&Wire, MiniJoyC_ADDR, 0, 26, 400000L);

    // 讀取初始按鍵狀態以避免開機誤觸發
    prevJoyBtn = joyc.getButtonStatus();

    delay(300);
    M5.Lcd.fillScreen(BLACK);
}

void loop() {
    M5.update();

    // 正面 Button A：敲擊木魚測試
    if (M5.BtnA.wasPressed()) {
        playClickSound();
        joyc.setLEDColor(0xFFCC00); // 暖黃金光
    }

    // 側面 Button B：切換靜音 / 聲音模式
    if (M5.BtnB.wasPressed()) {
        soundEnabled = !soundEnabled;
        if (soundEnabled) {
            playClickSound();
        } else {
            noTone(BUZZER_PIN);
            digitalWrite(BUZZER_PIN, LOW);
        }
    }

    // 定時讀取搖桿與姿態資訊並更新畫面 (約 30 FPS)
    if (millis() - lastUpdate > 33) {
        lastUpdate = millis();

        // 讀取 MiniJoyC 搖桿數值 (有號數 int16_t，中心約為 0)
        int16_t joyX = (int16_t)joyc.getPOSValue(POS_X, _10bit);
        int16_t joyY = (int16_t)joyc.getPOSValue(POS_Y, _10bit);

        // 讀取搖桿中心按鍵
        bool currentJoyBtn = joyc.getButtonStatus();

        // 僅在「邊緣變化 (從未按到按下)」時發聲一次，絕不持續鳴叫
        if (currentJoyBtn != prevJoyBtn) {
            if (currentJoyBtn) {
                playClickSound(); // 僅在按下瞬間響一次
            }
            prevJoyBtn = currentJoyBtn;
        }

        // 讀取六軸姿態
        float ax = 0, ay = 0, az = 0;
        M5.IMU.getAccelData(&ax, &ay, &az);

        // 全彩氛圍燈柔和呼吸流光
        colorHue += 2;
        joyc.setLEDColor(hsvToRgb(colorHue, 255, 80));

        // 螢幕繪製
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Lcd.drawString("=== Fidget Toy (Quiet) ===", 10, 5, 2);

        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.setCursor(10, 28, 2);
        M5.Lcd.printf("Joy X: %4d  Y: %4d\n", joyX, joyY);

        M5.Lcd.setCursor(10, 48, 2);
        M5.Lcd.printf("Joy Btn: %d (%s)\n",
                      currentJoyBtn ? 1 : 0,
                      currentJoyBtn ? "PRESS" : "IDLE ");

        M5.Lcd.setCursor(10, 68, 2);
        M5.Lcd.printf("IMU Acc: X:%.2f Y:%.2f\n", ax, ay);

        M5.Lcd.setCursor(10, 88, 2);
        M5.Lcd.printf("Sound: %s (BtnB toggles)\n",
                      soundEnabled ? "ON [Mute:BtnB]" : "MUTED (靜音) ");

        // 繪製右側準心與搖桿小圓點
        int centerX = 195;
        int centerY = 75;
        int maxRadius = 35;
        M5.Lcd.drawCircle(centerX, centerY, maxRadius, TFT_DARKGREY);
        M5.Lcd.drawLine(centerX - 5, centerY, centerX + 5, centerY, TFT_DARKGREY);
        M5.Lcd.drawLine(centerX, centerY - 5, centerX, centerY + 5, TFT_DARKGREY);

        int dotX = centerX + (joyX * (maxRadius - 4) / 512);
        int dotY = centerY + (joyY * (maxRadius - 4) / 512);
        // 限制在圓圈內
        dotX = constrain(dotX, centerX - maxRadius + 4, centerX + maxRadius - 4);
        dotY = constrain(dotY, centerY - maxRadius + 4, centerY + maxRadius - 4);
        M5.Lcd.fillCircle(dotX, dotY, 4, currentJoyBtn ? TFT_RED : TFT_YELLOW);
    }
}
