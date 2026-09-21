/**
 * @file SceneSand.cpp
 * @brief 重力感應流沙實作：落沙細胞自動機物理模擬與雙緩衝零閃爍渲染
 */

#include "scenes/SceneSand.h"
#include <cmath>

SceneSand::SceneSand()
    : _sandCount(0), _theme(THEME_DESERT_GOLD), _shakeBurstFrames(0),
      _lastPhysicsTime(0), _lastSpawnTime(0), _needsRedraw(true) {
    for (uint8_t x = 0; x < GRID_W; x++) {
        for (uint8_t y = 0; y < GRID_H; y++) {
            _grid[x][y] = 0;
        }
    }
}

void SceneSand::resetSand() {
    for (uint8_t x = 0; x < GRID_W; x++) {
        for (uint8_t y = 0; y < GRID_H; y++) {
            _grid[x][y] = 0;
        }
    }
    _sandCount = 0;
    _shakeBurstFrames = 0;

    // 開局在中央上方預設生成 220 顆初始沙粒
    for (int i = 0; i < 220; i++) {
        int gx = (GRID_W / 2) + EntropyManager::random(-10, 11);
        int gy = EntropyManager::random(4, 25);
        if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H && _grid[gx][gy] == 0) {
            _grid[gx][gy] = (uint8_t)EntropyManager::random(1, 4);
            _sandCount++;
        }
    }
}

void SceneSand::init() {
    _theme = THEME_DESERT_GOLD;
    _nextScene = SCENE_COUNT;
    _lastPhysicsTime = millis();
    _lastSpawnTime = millis();
    _shakeBurstFrames = 0;
    _needsRedraw = true;

    resetSand();
}

void SceneSand::spawnSand(int16_t gridX, int16_t gridY, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        int gx = gridX + EntropyManager::random(-2, 3);
        int gy = gridY + EntropyManager::random(-2, 3);
        if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
            if (_grid[gx][gy] == 0 && _sandCount < 1400) {
                _grid[gx][gy] = (uint8_t)EntropyManager::random(1, 4);
                _sandCount++;
            }
        }
    }
}

void SceneSand::updatePhysics(float ax, float ay, bool isShaking, AudioManager& audio) {
    // 1. 搖晃觸發噴灑噴泉動效 (Dynamic Shake Eruption)
    if (isShaking) {
        if (_shakeBurstFrames == 0) {
            audio.playClick();
        }
        _shakeBurstFrames = 18; // 賦予 18 幀連續噴灑動量
    }

    // 噴灑爆發期：將沙堆表面與內部沙粒向上連續揚起飛散，形成真實泉湧拋灑視覺
    if (_shakeBurstFrames > 0) {
        _shakeBurstFrames--;
        uint8_t burstCount = 0;
        // 隨機在沙盤中抽樣揚起沙粒
        for (int tries = 0; tries < 80 && burstCount < 25; tries++) {
            int rx = EntropyManager::random(0, GRID_W);
            int ry = EntropyManager::random(10, GRID_H);
            if (_grid[rx][ry] != 0) {
                int liftY = ry - EntropyManager::random(1, 4);      // 連續向上飛升 1~3 格
                int scatterX = rx + EntropyManager::random(-2, 3);   // 左右隨機散射
                if (liftY >= 0 && scatterX >= 0 && scatterX < GRID_W && _grid[scatterX][liftY] == 0) {
                    _grid[scatterX][liftY] = _grid[rx][ry];
                    _grid[rx][ry] = 0;
                    burstCount++;
                }
            }
        }
    }

    // 2. 確定連續重力向量 (M5StickC 直向：ax > 0 為左傾，gx = -ax 為向左流；ay > 0 為立起向下流)
    float gx = -ax;
    float gy = ay;
    float gMag = sqrtf(gx * gx + gy * gy);

    // 3. 水平靜止死區判定：當機身平放水平放置於桌上且無噴灑時，沙粒完全靜止
    if (gMag < 0.20f && _shakeBurstFrames == 0) {
        return;
    }

    // 4. 計算單位重力方向向量與軸向權重
    float ux = (gMag > 0.001f) ? (gx / gMag) : 0.0f;
    float uy = (gMag > 0.001f) ? (gy / gMag) : 1.0f;

    float absX = fabsf(ux);
    float absY = fabsf(uy);
    int8_t signX = (ux >= 0.0f) ? 1 : -1;
    int8_t signY = (uy >= 0.0f) ? 1 : -1;

    // 5. 依據重力方向自適應反向掃描 (Reverse Scan Order)，確保單幀單步推進，杜絕穿透瞬移
    int startY = (gy >= 0.0f) ? (GRID_H - 1) : 0;
    int endY   = (gy >= 0.0f) ? -1 : GRID_H;
    int stepY  = (gy >= 0.0f) ? -1 : 1;

    int startX = (gx >= 0.0f) ? (GRID_W - 1) : 0;
    int endX   = (gx >= 0.0f) ? -1 : GRID_W;
    int stepX  = (gx >= 0.0f) ? -1 : 1;

    for (int y = startY; y != endY; y += stepY) {
        for (int x = startX; x != endX; x += stepX) {
            uint8_t grain = _grid[x][y];
            if (grain == 0) continue;

            // 6. 360 度連續重力自然運動邏輯 (Vector-driven Sand Simulation)
            if (absY >= absX) {
                // A. 垂直分量佔優 (正立、倒立或斜向)
                // 若傾斜角顯著，機率性直接沿對角線滑落（解決向上或向下時左右沒反應問題）
                if (absX > 0.20f && (EntropyManager::random(0, 100) < (int)(absX * 80))) {
                    int diagX = x + signX;
                    int diagY = y + signY;
                    if (diagX >= 0 && diagX < GRID_W && diagY >= 0 && diagY < GRID_H && _grid[diagX][diagY] == 0) {
                        _grid[diagX][diagY] = grain;
                        _grid[x][y] = 0;
                        continue;
                    }
                }

                // 優先直落
                int straightY = y + signY;
                if (straightY >= 0 && straightY < GRID_H && _grid[x][straightY] == 0) {
                    _grid[x][straightY] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                // 直落受阻，嘗試斜向下落 (優先向重力方向側滑，次選反向側滑形成安息角)
                bool preferGravitySide = (absX > 0.15f) ? true : (EntropyManager::random(0, 2) == 0);
                int8_t s1 = preferGravitySide ? signX : -signX;
                int8_t s2 = -s1;

                int s1X = x + s1;
                if (s1X >= 0 && s1X < GRID_W && straightY >= 0 && straightY < GRID_H && _grid[s1X][straightY] == 0) {
                    _grid[s1X][straightY] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                int s2X = x + s2;
                if (s2X >= 0 && s2X < GRID_W && straightY >= 0 && straightY < GRID_H && _grid[s2X][straightY] == 0) {
                    _grid[s2X][straightY] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                // 若下方完全堵塞且側向重力強烈，允許純水平滾動
                if (absX > 0.35f) {
                    int horizX = x + signX;
                    if (horizX >= 0 && horizX < GRID_W && _grid[horizX][y] == 0) {
                        _grid[horizX][y] = grain;
                        _grid[x][y] = 0;
                    }
                }
            } else {
                // B. 水平分量佔優 (側立橫置或近水平橫擺)
                // 若垂直分量顯著，機率性直接沿對角線滑落
                if (absY > 0.20f && (EntropyManager::random(0, 100) < (int)(absY * 80))) {
                    int diagX = x + signX;
                    int diagY = y + signY;
                    if (diagX >= 0 && diagX < GRID_W && diagY >= 0 && diagY < GRID_H && _grid[diagX][diagY] == 0) {
                        _grid[diagX][diagY] = grain;
                        _grid[x][y] = 0;
                        continue;
                    }
                }

                // 優先橫移
                int straightX = x + signX;
                if (straightX >= 0 && straightX < GRID_W && _grid[straightX][y] == 0) {
                    _grid[straightX][y] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                // 橫移受阻，嘗試斜向側滑
                bool preferGravitySide = (absY > 0.15f) ? true : (EntropyManager::random(0, 2) == 0);
                int8_t s1 = preferGravitySide ? signY : -signY;
                int8_t s2 = -s1;

                int s1Y = y + s1;
                if (straightX >= 0 && straightX < GRID_W && s1Y >= 0 && s1Y < GRID_H && _grid[straightX][s1Y] == 0) {
                    _grid[straightX][s1Y] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                int s2Y = y + s2;
                if (straightX >= 0 && straightX < GRID_W && s2Y >= 0 && s2Y < GRID_H && _grid[straightX][s2Y] == 0) {
                    _grid[straightX][s2Y] = grain;
                    _grid[x][y] = 0;
                    continue;
                }

                // 若橫向完全堵塞且垂直重力顯著，允許垂直滑移
                if (absY > 0.35f) {
                    int vertY = y + signY;
                    if (vertY >= 0 && vertY < GRID_H && _grid[x][vertY] == 0) {
                        _grid[x][vertY] = grain;
                        _grid[x][y] = 0;
                    }
                }
            }
        }
    }
}

void SceneSand::update(InputManager& input, AudioManager& audio, LedManager& led) {
    // 1. Button B 長按：退出返回主選單
    if (input.btnBLongPressed) {
        audio.playClick();
        led.setColor(0, 0, 0);
        _nextScene = SCENE_MENU;
        return;
    }

    // 2. Button A 短按：循環切換沙色主題
    if (input.btnAPressed) {
        _theme = (SandTheme)((_theme + 1) % THEME_COUNT);
        audio.playClick();
        _needsRedraw = true;
    }

    // 3. 搖桿或按鍵加沙
    uint32_t now = millis();
    int joyX = input.joyX;
    int joyY = input.joyY;
    bool isAddingSand = (abs(joyX) > 30 || abs(joyY) > 30 || input.joyBtnPressed || input.isBtnAHeld);

    if (isAddingSand && (now - _lastSpawnTime > 75)) {
        _lastSpawnTime = now;
        // 映射搖桿座標至網格
        int spawnGx = map(joyX, -100, 100, 8, GRID_W - 9);
        int spawnGy = map(joyY, -100, 100, GRID_H - 12, 6);
        spawnSand(spawnGx, spawnGy, 5);
        _needsRedraw = true;
    }

    // 4. 定時物理模擬推進 (30ms 週期，約 33 FPS)
    if (now - _lastPhysicsTime >= 30) {
        _lastPhysicsTime = now;
        float ax = 0, ay = 0, az = 0;
        M5.Imu.getAccelData(&ax, &ay, &az);

        updatePhysics(ax, ay, input.isActivelyShaking, audio);
        _needsRedraw = true;
    }

    // 5. LED 燈色連動
    if (_theme == THEME_DESERT_GOLD) {
        led.setColor(255, 180, 20);
    } else if (_theme == THEME_NEON_POP) {
        led.setColor(0, 240, 255);
    } else {
        led.setColor(180, 230, 255);
    }
}

void SceneSand::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 方案 A：在全域單例雙緩衝畫布 g_canvas 離線繪製，杜絕閃爍
    g_canvas.fillSprite(TFT_BLACK);

    // 1. 頂部狀態列 (Y: 0 ~ 24)
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 24, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("SAND TOY", 6, 4, 2);

    char countStr[16];
    snprintf(countStr, sizeof(countStr), "#%d", _sandCount);
    g_canvas.setTextColor(COLOR_CYAN, 0x18C3);
    g_canvas.drawRightString(countStr, SCREEN_WIDTH - 6, 4, 2);

    // 2. 沙盤邊界線 (Y: 24 ~ 216)
    g_canvas.drawFastHLine(0, SAND_OFFSET_Y - 1, SCREEN_WIDTH, 0x2965);
    g_canvas.drawFastHLine(0, SAND_OFFSET_Y + GRID_H * 3, SCREEN_WIDTH, 0x2965);

    // 3. 繪製所有沙粒方塊 (3x3 像素)
    for (uint8_t x = 0; x < GRID_W; x++) {
        for (uint8_t y = 0; y < GRID_H; y++) {
            uint8_t val = _grid[x][y];
            if (val == 0) continue;

            uint16_t color;
            if (_theme == THEME_DESERT_GOLD) {
                color = (val == 1) ? 0xFEA0 : (val == 2) ? 0xFD60 : 0xDC00; // 金黃色階
            } else if (_theme == THEME_NEON_POP) {
                color = (val == 1) ? 0x07FF : (val == 2) ? 0xF81F : 0x07E0; // 青/洋紅/翠綠
            } else {
                color = (val == 1) ? TFT_WHITE : (val == 2) ? COLOR_LIGHT_BLUE : 0x029B; // 藍雪白
            }

            g_canvas.fillRect(x * 3, SAND_OFFSET_Y + y * 3, 3, 3, color);
        }
    }

    // 4. 底部操作指引 (Y: 218 ~ 238)
    const char* THEME_NAMES[] = {"DESERT GOLD", "NEON POP", "ICE SNOW"};
    g_canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    g_canvas.drawCentreString(THEME_NAMES[_theme], SCREEN_WIDTH / 2, 218, 1);

    g_canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g_canvas.drawCentreString("Tilt: Flow  Joy: Add  Shake: Up", SCREEN_WIDTH / 2, 229, 1);

    // 一次性推送畫面至螢幕
    g_canvas.pushSprite(0, 0);
}
