/**
 * @file SceneCoin.h
 * @brief 直式多枚擲硬幣 (Coin Toss) 場景標頭檔：支援 1~5 枚硬幣拋擲、人頭與 10 元面、長拉翻滾與統計
 */

#pragma once

#include "scenes/Scene.h"

class SceneCoin : public Scene {
public:
    SceneCoin();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_COIN; }

private:
    uint8_t _coinCount;         // 1 ~ 5 枚硬幣
    uint8_t _coinResults[5];    // 各硬幣結果 (0: HEADS 人頭, 1: TAILS 10元)

    bool _isFlipping;           // 是否正在翻滾拋擲中
    bool _triggeredByJoy;       // 是否由搖桿長拉觸發 (true: 搖桿放開才開始煞停; false: 體感甩動等待靜止)
    uint32_t _flipStartTime;    // 拋擲啟動時間戳記
    uint32_t _lastTickTime;     // 翻滾音效與動畫更新時間戳記
    uint8_t _animPhase;         // 動畫旋轉階段 (0~3 橢圓寬度壓縮)
    bool _needsRedraw;          // 螢幕重繪旗標

    void tossCoins(bool byJoy, AudioManager& audio, LedManager& led);
    void drawCoin(int cx, int cy, int r, uint8_t side, uint8_t phase);
};
