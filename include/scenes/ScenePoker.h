/**
 * @file ScenePoker.h
 * @brief 極簡大字幸運撲克抽牌場景標頭檔：加入抽牌跳動期待感動畫與標題防重疊
 */

#pragma once

#include "scenes/Scene.h"

struct Card {
    uint8_t suit;   // 0: 黑桃, 1: 紅心, 2: 方塊, 3: 梅花, 4: 鬼牌
    uint8_t value;  // 1(A) ~ 13(K), 鬼牌時 1:小鬼, 2:大鬼
};

class ScenePoker : public Scene {
public:
    ScenePoker();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_POKER; }

private:
    bool _includeJokers;
    Card _deck[54];
    uint8_t _deckSize;
    uint8_t _deckIndex;

    Card _currentCard;
    bool _isCardRevealed;
    bool _needsRedraw;

    // 抽牌跳動期待感動畫
    bool _isDrawingAnim;
    uint32_t _animStartTime;
    uint32_t _lastTickTime;
    Card _tempAnimCard;

    void shuffleDeck();
    void startDrawCard(AudioManager& audio, LedManager& led);
};
