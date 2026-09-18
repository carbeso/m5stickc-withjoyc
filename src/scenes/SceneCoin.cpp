/**
 * @file SceneCoin.cpp
 * @brief 直式多枚擲硬幣 (Coin Toss) 場景實作：支援 1~5 枚硬幣拋擲、人頭剪影與台灣 10 元面
 */

#include "scenes/SceneCoin.h"

SceneCoin::SceneCoin()
    : _coinCount(1), _isFlipping(false), _triggeredByJoy(false),
      _flipStartTime(0), _lastTickTime(0), _animPhase(0), _needsRedraw(true) {
    for (int i = 0; i < 5; i++) {
        _coinResults[i] = 0; // 預設為人頭 (0: HEAD, 1: TAIL)
    }
}

void SceneCoin::init() {
    _needsRedraw = true;
    _isFlipping = false;
    _triggeredByJoy = false;
    _animPhase = 0;
    _nextScene = SCENE_COUNT;
    M5.Lcd.fillScreen(TFT_BLACK);
}

void SceneCoin::tossCoins(bool byJoy, AudioManager& audio, LedManager& led) {
    _isFlipping = true;
    _triggeredByJoy = byJoy;
    _flipStartTime = millis();
    _lastTickTime = millis();
    _animPhase = 0;

    // 預先決定各硬幣結果 (0: HEAD, 1: TAIL)
    for (int i = 0; i < _coinCount; i++) {
        _coinResults[i] = random(0, 2);
    }

    audio.playClick();
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneCoin::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 長按 Button B 返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isFlipping) {
        // 非拋擲狀態下，左右切換硬幣數量 (1 ~ 5 枚，單次邊緣觸發手感確實)
        if (input.joyPushedLeft) {
            if (_coinCount > 1) {
                _coinCount--;
                audio.playTick();
                _needsRedraw = true;
            }
        } else if (input.joyPushedRight) {
            if (_coinCount < 5) {
                _coinCount++;
                audio.playTick();
                _needsRedraw = true;
            }
        }

        // 觸發拋擲：搖桿下拉 (joyY > 35)、Button A、搖桿中心鍵或機身甩動
        if (input.joyY > 35 || input.btnAPressed || input.joyBtnPressed) {
            tossCoins(true, audio, led);
        } else if (input.isShaken) {
            tossCoins(false, audio, led);
        }
    } else {
        // 拋擲進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _flipStartTime;

        // 硬幣翻滾循環音效與幀更新 (每 80ms 翻轉一次)
        if (now - _lastTickTime > 80) {
            _lastTickTime = now;
            _animPhase = (_animPhase + 1) % 6;
            audio.playClick();
            _needsRedraw = true;
        }

        // 長拉維持旋轉判斷：純粹依據實體搖桿拉桿 joyY > 35 (或長按 Btn A)
        bool isHolding = _triggeredByJoy && (input.joyY > 35 || input.isBtnAHeld);

        if (isHolding) {
            // 長拉中保持重設時間，讓放開後還能優雅煞停 1.2 秒落地
            if (elapsed > 1500) {
                _flipStartTime = now - 1500;
            }
        } else {
            // 煞停與落地判定
            uint32_t minDuration = _triggeredByJoy ? 2400 : 2000;
            bool stopCondition = false;

            if (_triggeredByJoy) {
                stopCondition = (elapsed >= minDuration);
            } else {
                // 體感甩動：至少持續 2 秒且機身幾乎靜止
                stopCondition = (elapsed >= minDuration) && input.isNearlyStill;
            }

            if (stopCondition) {
                _isFlipping = false;
                audio.playCrit();
                led.flash(255, 200, 0, 2, 80); // 落地金色閃爍
                _needsRedraw = true;
            }
        }
    }
}

void SceneCoin::drawCoin(int cx, int cy, int r, uint8_t side, uint8_t phase) {
    if (_isFlipping) {
        // 翻轉中：利用壓縮 X 軸半徑呈現 3D 旋轉透視感
        // phase: 0 -> r, 1 -> r*2/3, 2 -> r/3, 3 -> 2, 4 -> r/3, 5 -> r*2/3
        int rx = r;
        if (phase == 1 || phase == 5) rx = (r * 2) / 3;
        else if (phase == 2 || phase == 4) rx = r / 3;
        else if (phase == 3) rx = 3;

        if (rx <= 3) {
            // 側面硬幣厚度 (Edge view)
            M5.Lcd.fillRoundRect(cx - 2, cy - r, 4, r * 2, 2, COLOR_SILVER);
            M5.Lcd.drawRoundRect(cx - 2, cy - r, 4, r * 2, 2, COLOR_GOLD);
        } else {
            // 正反翻轉橢圓
            uint16_t bodyColor = (phase < 3) ? COLOR_GOLD : COLOR_SILVER;
            M5.Lcd.fillEllipse(cx, cy, rx, r, bodyColor);
            M5.Lcd.drawEllipse(cx, cy, rx, r, TFT_WHITE);
            // 亮面反光線
            M5.Lcd.drawFastVLine(cx - rx / 3, cy - r / 2, r, TFT_WHITE);
        }
    } else {
        // 落地靜止面
        // 外圈雙重立體金銀邊框
        M5.Lcd.fillCircle(cx, cy, r, COLOR_GOLD);
        M5.Lcd.drawCircle(cx, cy, r, 0xD4A0);
        M5.Lcd.drawCircle(cx, cy, r - 1, 0x8280);
        M5.Lcd.drawCircle(cx, cy, r - 3, 0xD4A0);

        if (side == 0) {
            // 【正面：人頭剪影浮雕 (HEADS)】
            // 內圈底色
            M5.Lcd.fillCircle(cx, cy, r - 4, 0xCE59); // 淺金浮雕底

            // 依據半徑大小繪製人頭剪影
            if (r >= 30) {
                // 特大硬幣 (1 枚)
                int headR = 11;
                int headCy = cy - 7;
                M5.Lcd.fillCircle(cx, headCy, headR, TFT_WHITE);
                // 額頭鼻尖
                M5.Lcd.fillTriangle(cx + 4, headCy - 7, cx + 15, headCy - 2, cx + 4, headCy + 2, TFT_WHITE);
                // 頸肩浮雕
                M5.Lcd.fillTriangle(cx - 18, cy + 20, cx + 18, cy + 20, cx, cy - 1, TFT_WHITE);
                // 正面英文標註
                M5.Lcd.setTextColor(0x4208, 0xCE59);
                M5.Lcd.drawCentreString("HEAD", cx, cy + 19, 1);
            } else if (r >= 22) {
                // 中型硬幣 (2~3 枚)
                int headR = 7;
                int headCy = cy - 5;
                M5.Lcd.fillCircle(cx, headCy, headR, TFT_WHITE);
                M5.Lcd.fillTriangle(cx + 2, headCy - 4, cx + 9, headCy - 1, cx + 2, headCy + 2, TFT_WHITE);
                M5.Lcd.fillTriangle(cx - 12, cy + 14, cx + 12, cy + 14, cx, cy - 1, TFT_WHITE);
                M5.Lcd.setTextColor(0x4208, 0xCE59);
                M5.Lcd.drawCentreString("H", cx, cy + 9, 1);
            } else {
                // 小型硬幣 (4~5 枚)
                M5.Lcd.fillCircle(cx, cy - 3, 5, TFT_WHITE);
                M5.Lcd.fillTriangle(cx - 9, cy + 10, cx + 9, cy + 10, cx, cy, TFT_WHITE);
                M5.Lcd.setTextColor(0x4208, 0xCE59);
                M5.Lcd.drawCentreString("H", cx, cy + 4, 1);
            }
        } else {
            // 【反面：台灣 10 元字樣 (TAILS)】
            M5.Lcd.fillCircle(cx, cy, r - 4, 0xAD40); // 典雅金底

            if (r >= 30) {
                // 特大硬幣：粗大「10」數字與「YUAN」
                M5.Lcd.setTextColor(TFT_WHITE, 0xAD40);
                M5.Lcd.drawCentreString("10", cx, cy - 14, 4);
                M5.Lcd.setTextColor(COLOR_GOLD, 0xAD40);
                M5.Lcd.drawCentreString("YUAN", cx, cy + 13, 2);
            } else if (r >= 22) {
                // 中型硬幣
                M5.Lcd.setTextColor(TFT_WHITE, 0xAD40);
                M5.Lcd.drawCentreString("10", cx, cy - 9, 2);
                M5.Lcd.setTextColor(COLOR_GOLD, 0xAD40);
                M5.Lcd.drawCentreString("TAIL", cx, cy + 7, 1);
            } else {
                // 小型硬幣
                M5.Lcd.setTextColor(TFT_WHITE, 0xAD40);
                M5.Lcd.drawCentreString("10", cx, cy - 7, 2);
            }
        }
    }
}

void SceneCoin::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 頂部狀態列
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    M5.Lcd.setTextColor(COLOR_GOLD, 0x18C3);
    M5.Lcd.drawString("COIN TOSS", 8, 5, 2);

    char countStr[8];
    snprintf(countStr, sizeof(countStr), "x%d", _coinCount);
    M5.Lcd.setTextColor(COLOR_CYAN, 0x18C3);
    M5.Lcd.drawRightString(countStr, SCREEN_WIDTH - 8, 7, 1);

    // 清空硬幣主活動區
    M5.Lcd.fillRect(0, 26, SCREEN_WIDTH, 172, TFT_BLACK);

    // 依據硬幣數量計算中心位置與半徑
    if (_coinCount == 1) {
        drawCoin(SCREEN_WIDTH / 2, 112, 38, _coinResults[0], _animPhase);
    } else if (_coinCount == 2) {
        drawCoin(SCREEN_WIDTH / 2, 70, 28, _coinResults[0], _animPhase);
        drawCoin(SCREEN_WIDTH / 2, 145, 28, _coinResults[1], _animPhase);
    } else if (_coinCount == 3) {
        drawCoin(SCREEN_WIDTH / 2, 55, 22, _coinResults[0], _animPhase);
        drawCoin(SCREEN_WIDTH / 2, 112, 22, _coinResults[1], _animPhase);
        drawCoin(SCREEN_WIDTH / 2, 169, 22, _coinResults[2], _animPhase);
    } else if (_coinCount == 4) {
        int x1 = 38, x2 = 97;
        int y1 = 66, y2 = 145;
        drawCoin(x1, y1, 22, _coinResults[0], _animPhase);
        drawCoin(x2, y1, 22, _coinResults[1], _animPhase);
        drawCoin(x1, y2, 22, _coinResults[2], _animPhase);
        drawCoin(x2, y2, 22, _coinResults[3], _animPhase);
    } else if (_coinCount == 5) {
        // 五梅花幾何排列
        int x1 = 36, x2 = 99, cx = SCREEN_WIDTH / 2;
        int y1 = 58, y2 = 160, cy = 109;
        drawCoin(x1, y1, 20, _coinResults[0], _animPhase);
        drawCoin(x2, y1, 20, _coinResults[1], _animPhase);
        drawCoin(cx, cy, 20, _coinResults[2], _animPhase);
        drawCoin(x1, y2, 20, _coinResults[3], _animPhase);
        drawCoin(x2, y2, 20, _coinResults[4], _animPhase);
    }

    // 底部結算與指引區
    M5.Lcd.fillRect(0, 198, SCREEN_WIDTH, 42, TFT_BLACK);
    M5.Lcd.drawFastHLine(8, 200, SCREEN_WIDTH - 16, 0x39E7);

    if (!_isFlipping) {
        if (_coinCount > 1) {
            // 結算正面與反面數量
            int heads = 0, tails = 0;
            for (int i = 0; i < _coinCount; i++) {
                if (_coinResults[i] == 0) heads++;
                else tails++;
            }
            char sumStr[32];
            snprintf(sumStr, sizeof(sumStr), "%dH %dT", heads, tails);
            M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
            M5.Lcd.drawCentreString(sumStr, SCREEN_WIDTH / 2, 205, 2);

            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawCentreString("[PULL / SHAKE]", SCREEN_WIDTH / 2, 224, 1);
        } else {
            // 單枚硬幣結果大字顯示
            const char* resStr = (_coinResults[0] == 0) ? "HEAD (OBVERSE)" : "10 YUAN (REVERSE)";
            M5.Lcd.setTextColor(COLOR_GOLD, TFT_BLACK);
            M5.Lcd.drawCentreString(resStr, SCREEN_WIDTH / 2, 205, 1);

            M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            M5.Lcd.drawCentreString("[PULL / SHAKE]", SCREEN_WIDTH / 2, 222, 1);
        }
    } else {
        M5.Lcd.setTextColor(COLOR_CYAN, TFT_BLACK);
        M5.Lcd.drawCentreString("FLIPPING...", SCREEN_WIDTH / 2, 212, 2);
    }
}
