/**
 * @file ScenePoker.h
 * @brief 極簡大字幸運撲克抽牌場景標頭檔
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
    bool _includeJokers;    // 是否加入鬼牌 (預設 false)
    Card _deck[54];
    uint8_t _deckSize;
    uint8_t _deckIndex;     // 目前已抽至第幾張

    Card _currentCard;
    bool _isCardRevealed;   // 目前是否已翻開
    bool _needsRedraw;

    uint32_t _btnAPressTime;
    uint8_t _btnAClickCount;

    void shuffleDeck();
    void drawCard(AudioManager& audio, LedManager& led);
};
