/**
 * @file SceneEightBall.cpp
 * @brief 純英文經典神秘八號球實作：100% 移除中文能力，採用清晰居中英文與二十面體浮牌
 */

#include "scenes/SceneEightBall.h"
#include "EntropyManager.h"

// 經典英文八號球籤詩清單 (依據維基百科官方標準 20 款解答)
const ClassicFortune CLASSIC_FORTUNES[] = {
    // --- 正向肯定 (Affirmative - 10 款：淡藍色倒三角 ▼) ---
    {"IT IS", "CERTAIN", 0},
    {"IT IS", "DECIDEDLY SO", 0},
    {"WITHOUT", "A DOUBT", 0},
    {"YES,", "DEFINITELY", 0},
    {"YOU MAY", "RELY ON IT", 0},
    {"AS I SEE IT,", "YES", 0},
    {"MOST", "LIKELY", 0},
    {"OUTLOOK", "GOOD", 0},
    {"YES", "", 0},
    {"SIGNS POINT", "TO YES", 0},

    // --- 模糊中立 (Non-committal - 5 款：淡紫色菱形水晶 ◆) ---
    {"REPLY HAZY,", "TRY AGAIN", 1},
    {"ASK AGAIN", "LATER", 1},
    {"BETTER NOT", "TELL YOU NOW", 1},
    {"CANNOT", "PREDICT NOW", 1},
    {"CONCENTRATE", "& ASK AGAIN", 1},

    // --- 否定懷疑 (Negative - 5 款：淡紅色正三角 ▲) ---
    {"DON'T", "COUNT ON IT", 2},
    {"MY REPLY", "IS NO", 2},
    {"MY SOURCES", "SAY NO", 2},
    {"OUTLOOK NOT", "SO GOOD", 2},
    {"VERY", "DOUBTFUL", 2}
};

const uint8_t CLASSIC_COUNT = sizeof(CLASSIC_FORTUNES) / sizeof(CLASSIC_FORTUNES[0]);

SceneEightBall::SceneEightBall()
    : _fortuneIdx(0), _isRevealing(false), _isRevealed(false), _triggeredByBtn(false),
      _revealStartTime(0), _lastBubbleTime(0), _needsRedraw(true) {}

void SceneEightBall::init() {
    _needsRedraw = true;
    _isRevealing = false;
    _isRevealed = false;
    _triggeredByBtn = false;
    _nextScene = SCENE_COUNT;
}

void SceneEightBall::startDivination(bool byBtn, AudioManager& audio, LedManager& led) {
    _fortuneIdx = EntropyManager::random(0, CLASSIC_COUNT);
    _isRevealing = true;
    _isRevealed = false;
    _triggeredByBtn = byBtn;
    _revealStartTime = millis();
    _lastBubbleTime = millis();

    audio.playBubble();
    led.setRainbowMode(true);
    _needsRedraw = true;
}

void SceneEightBall::update(InputManager& input, AudioManager& audio, LedManager& led) {
    if (input.btnBLongPressed) {
        audio.playClick();
        _nextScene = SCENE_MENU;
        return;
    }

    if (!_isRevealing) {
        // 啟動占卜：必須是明確按鍵或甩動脈衝觸發，絕不因常態推持誤觸
        if (input.btnAPressed || input.joyBtnPressed || input.isShaken) {
            startDivination(true, audio, led);
        }
    } else {
        // 翻騰冒泡進行中
        uint32_t now = millis();
        uint32_t elapsed = now - _revealStartTime;

        // 冒泡期間每 250ms 定時發出擬真水聲與微幅波紋更新
        if (now - _lastBubbleTime > 120) {
            _lastBubbleTime = now;
            _needsRedraw = true;
            if ((now / 250) != ((now - 120) / 250)) {
                audio.playBubble();
            }
        }

        // 持續 3000ms (3 秒) 翻騰冒泡後再開籤解答！
        if (elapsed >= 3000) {
            _isRevealing = false;
            _isRevealed = true;

            uint8_t cat = CLASSIC_FORTUNES[_fortuneIdx].category;
            if (cat == 0) {
                audio.playCrit();
                led.setColor(0, 200, 255);     // 正向：冰河淡藍
            } else if (cat == 1) {
                audio.playClick();
                led.setColor(200, 50, 255);    // 模糊：神秘淡紫
            } else {
                audio.playFumble();
                led.setColor(255, 60, 60);     // 否定：淡紅
            }
            _needsRedraw = true;
        }
    }
}

void SceneEightBall::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：使用全域雙緩衝畫布離線繪製，杜絕水波微動動畫與揭曉浮籤閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 頂部狀態列：左側 8-BALL，右側 ORACLE
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    g_canvas.setTextColor(COLOR_PURPLE, 0x18C3);
    g_canvas.drawString("8-BALL", 8, 5, 2);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawRightString("ORACLE", SCREEN_WIDTH - 8, 7, 1);

    int cx = SCREEN_WIDTH / 2;
    int cy = 105;

    if (!_isRevealing && !_isRevealed) {
        int radius = 44;

        g_canvas.fillCircle(cx, cy, radius, 0x18C3);
        g_canvas.drawCircle(cx, cy, radius, COLOR_PURPLE);
        g_canvas.fillCircle(cx, cy, 18, TFT_WHITE);
        g_canvas.setTextColor(TFT_BLACK, TFT_WHITE);
        g_canvas.drawCentreString("8", cx, cy - 14, 4);

        g_canvas.setTextColor(COLOR_GOLD, TFT_BLACK);
        g_canvas.drawCentreString("SHAKE TO ASK", cx, 165, 2);
    } else if (_isRevealing) {
        // 占卜旋轉中：持續顯示八號球本身，伴隨水底氣泡與微波動效
        int radius = 44;
        int wobbleX = ((millis() / 120) % 3) - 1;
        int wobbleY = ((millis() / 160) % 3) - 1;
        int ballX = cx + wobbleX;
        int ballY = cy + wobbleY;

        g_canvas.fillCircle(ballX, ballY, radius, 0x18C3);
        g_canvas.drawCircle(ballX, ballY, radius, COLOR_CYAN);
        g_canvas.fillCircle(ballX, ballY, 18, TFT_WHITE);
        g_canvas.setTextColor(TFT_BLACK, TFT_WHITE);
        g_canvas.drawCentreString("8", ballX, ballY - 14, 4);

        // 周圍隨機微氣泡
        g_canvas.drawCircle(cx - 38, cy - 35, 3, COLOR_CYAN);
        g_canvas.drawCircle(cx + 36, cy + 30, 2, COLOR_CYAN);
        g_canvas.drawCircle(cx + 40, cy - 25, 4, COLOR_CYAN);

        g_canvas.setTextColor(COLOR_CYAN, TFT_BLACK);
        g_canvas.drawCentreString("THINKING...", cx, 165, 2);
    } else {
        // 占卜結果：依正向、否定、模糊分別繪製專屬幾何幾何浮牌
        const ClassicFortune& cf = CLASSIC_FORTUNES[_fortuneIdx];

        if (cf.category == 0) {
            // 1. 正向肯定：淡藍色倒三角形 (Inverted Triangle ▼，尖端朝下)
            int yTop = cy - 48;
            int yTip = cy + 54;
            int xSpan = 58;

            g_canvas.fillTriangle(cx - xSpan, yTop, cx + xSpan, yTop, cx, yTip, 0x09CD);
            g_canvas.drawTriangle(cx - xSpan, yTop, cx + xSpan, yTop, cx, yTip, COLOR_LIGHT_BLUE);

            g_canvas.setTextColor(COLOR_CYAN, 0x09CD);
            g_canvas.drawCentreString(cf.line1, cx, cy - 30, 2);
            g_canvas.setTextColor(TFT_WHITE, 0x09CD);
            g_canvas.drawCentreString(cf.line2, cx, cy - 10, 2);
        } else if (cf.category == 2) {
            // 2. 否定懷疑：淡紅色正三角形 (Upright Triangle ▲，尖端朝上)
            int yTip = cy - 54;
            int yBase = cy + 48;
            int xSpan = 58;

            g_canvas.fillTriangle(cx, yTip, cx - xSpan, yBase, cx + xSpan, yBase, 0x3842);
            g_canvas.drawTriangle(cx, yTip, cx - xSpan, yBase, cx + xSpan, yBase, COLOR_LIGHT_RED);

            g_canvas.setTextColor(COLOR_LIGHT_RED, 0x3842);
            g_canvas.drawCentreString(cf.line1, cx, cy + 4, 2);
            g_canvas.setTextColor(TFT_WHITE, 0x3842);
            g_canvas.drawCentreString(cf.line2, cx, cy + 24, 2);
        } else {
            // 3. 模糊中立：淡紫色菱形水晶 (Diamond ◆，神秘未知感)
            int yUp = cy - 52;
            int yDown = cy + 52;
            int xSpan = 56;

            g_canvas.fillTriangle(cx, yUp, cx - xSpan, cy, cx + xSpan, cy, 0x2128);
            g_canvas.fillTriangle(cx, yDown, cx - xSpan, cy, cx + xSpan, cy, 0x2128);
            g_canvas.drawLine(cx, yUp, cx - xSpan, cy, COLOR_PURPLE);
            g_canvas.drawLine(cx, yUp, cx + xSpan, cy, COLOR_PURPLE);
            g_canvas.drawLine(cx, yDown, cx - xSpan, cy, COLOR_PURPLE);
            g_canvas.drawLine(cx, yDown, cx + xSpan, cy, COLOR_PURPLE);

            g_canvas.setTextColor(0xDCBE, 0x2128);
            g_canvas.drawCentreString(cf.line1, cx, cy - 14, 2);
            g_canvas.setTextColor(TFT_WHITE, 0x2128);
            g_canvas.drawCentreString(cf.line2, cx, cy + 6, 2);
        }
    }

    // 底部指引
    g_canvas.drawFastHLine(8, 212, SCREEN_WIDTH - 16, 0x39E7);
    g_canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g_canvas.drawCentreString("[SHAKE TO RE-ASK]", SCREEN_WIDTH / 2, 220, 1);

    // 一次性推送畫面至 ST7789v2 螢幕
    g_canvas.pushSprite(0, 0);
}

