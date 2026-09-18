/**
 * @file ScenePoker.h
 * @brief 極簡大字幸運撲克抽牌場景標頭檔：支援銷牌空牌手動重置、單抽模式切換與持續推持動力學
 */

#pragma once

#include "scenes/Scene.h"

struct Card {
    uint8_t suit;   // 0: 黑桃, 1: 紅心, 2: 方塊, 3: 梅花, 4: 鬼牌
    uint8_t value;  // 1(A) ~ 13(K)
};

class ScenePoker : public Scene {
public:
    ScenePoker();
    void init() override;
    void update(InputManager& input, AudioManager& audio, LedManager& led) override;
    void draw() override;
    GameScene getSceneId() const override { return SCENE_POKER; }

private:
    bool _singleMode;       // false: 牌堆銷牌模式, true: 單抽放回模式
    bool _includeJokers;    // 是否包含鬼牌
    Card _deck[54];
    uint8_t _deckSize;
    uint8_t _deckIndex;

    Card _currentCard;
    bool _isCardRevealed;
    bool _deckEmpty;        // 牌堆已抽空
    bool _needsRedraw;

    // 動態洗牌狀態
    bool _isShuffling;
    bool _triggeredByJoy;   // 是否由搖桿推持觸發 (放開時才停止)
    uint32_t _animStartTime;
    uint32_t _lastTickTime;
    Card _tempAnimCard;

    void resetAndShuffle();
    void startShuffle(bool byJoy, AudioManager& audio, LedManager& led);
    void finalizeDraw(AudioManager& audio, LedManager& led);
};
