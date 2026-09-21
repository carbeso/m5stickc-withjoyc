/**
 * @file AudioManager.cpp
 * @brief 蜂鳴器聲音音效管理器實作
 */

#include "AudioManager.h"
#include "EntropyManager.h"

AudioManager::AudioManager()
    : _muted(false), _playingMelody(false), _melodyStep(0), _nextNoteTime(0) {}

void AudioManager::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // 預設拉低避免雜音
}

void AudioManager::toggleMute() {
    _muted = !_muted;
    if (_muted) {
        noTone(BUZZER_PIN);
        _playingMelody = false;
    }
}

void AudioManager::playClick() {
    if (_muted) return;
    tone(BUZZER_PIN, 1800, 8); // 輕脆短促 8ms
}

void AudioManager::playTick() {
    if (_muted) return;
    tone(BUZZER_PIN, 1200, 10); // 齒輪卡榫音
}

void AudioManager::playDiceRoll() {
    if (_muted) return;
    tone(BUZZER_PIN, 800 + EntropyManager::random(0, 400), 12);
}

void AudioManager::playCardDraw() {
    if (_muted) return;
    tone(BUZZER_PIN, 2200, 20);
}

void AudioManager::playBubble() {
    if (_muted) return;
    tone(BUZZER_PIN, 950, 18);
}

void AudioManager::playCrit() {
    if (_muted) return;
    tone(BUZZER_PIN, 2600, 80);
}

void AudioManager::playFumble() {
    if (_muted) return;
    tone(BUZZER_PIN, 400, 120);
}

void AudioManager::playJackpot() {
    if (_muted) return;
    _playingMelody = true;
    _melodyStep = 0;
    _nextNoteTime = millis();
}

void AudioManager::update() {
    if (!_playingMelody || _muted) return;

    // Jackpot 勝利旋律 (C5, E5, G5, C6 琶音)
    const uint16_t notes[] = {523, 659, 784, 1046, 1318, 1568, 2093};
    const uint8_t noteCount = 7;

    if (millis() >= _nextNoteTime) {
        if (_melodyStep < noteCount) {
            tone(BUZZER_PIN, notes[_melodyStep], 60);
            _nextNoteTime = millis() + 75;
            _melodyStep++;
        } else {
            _playingMelody = false;
            noTone(BUZZER_PIN);
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
}
