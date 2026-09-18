/**
 * @file AudioManager.h
 * @brief 蜂鳴器聲音音效管理器標頭檔
 */

#pragma once

#include <Arduino.h>
#include "Config.h"

class AudioManager {
public:
    AudioManager();
    void begin();
    void update();

    void toggleMute();
    bool isMuted() const { return _muted; }

    // 擬真音效介面
    void playClick();          // 8ms 輕脆微動按鍵聲
    void playTick();           // 10ms 滾輪/齒輪卡榫滴答聲
    void playDiceRoll();       // 擲骰骨碌聲
    void playCardDraw();       // 撲克抽牌滑順刷牌聲
    void playBubble();         // 八號球神秘液體冒泡聲
    void playJackpot();        // 老虎機中獎歡慶和弦 (非阻塞)
    void playCrit();           // d20 大成功清脆高音
    void playFumble();         // d20 大失敗低沉哀鳴

private:
    bool _muted;

    // 非阻塞中獎音樂狀態機
    bool _playingMelody;
    uint8_t _melodyStep;
    uint32_t _nextNoteTime;
};
