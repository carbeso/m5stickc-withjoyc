/**
 * @file SceneSand.cpp
 * @brief 重力感應流沙實作：落沙細胞自動機物理模擬與雙緩衝零閃爍渲染
 */

#include "scenes/SceneSand.h"
#include <cmath>

SceneSand::SceneSand()
    : _sandCount(0), _theme(THEME_DESERT_GOLD),
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

void SceneSand::updatePhysics(float ax, float ay, bool isShaking) {
    // 1. 體感甩動時將沙粒激盪震散噴濺 (Shake Eruption)
    if (isShaking) {
        for (int x = 0; x < GRID_W; x++) {
            for (int y = 0; y < GRID_H; y++) {
                if (_grid[x][y] != 0 && EntropyManager::random(0, 4) == 0) {
                    int ny = y - EntropyManager::random(5, 20);
                    int nx = x + EntropyManager::random(-6, 7);
                    if (ny >= 0 && nx >= 0 && nx < GRID_W && _grid[nx][ny] == 0) {
                        _grid[nx][ny] = _grid[x][y];
                        _grid[x][y] = 0;
                    }
                }
            }
        }
        return;
    }

    // 2. 確定重力方向 (直向 LCD 下：ay 向下為正，ax 向右為正)
    int8_t dy = 1;
    int8_t dx = 0;

    if (fabsf(ay) > 0.15f) {
        dy = (ay > 0) ? 1 : -1;
    }
    if (fabsf(ax) > 0.15f) {
        dx = (ax > 0) ? 1 : -1;
    }

    // 3. 自適應掃描方向，防止同一顆沙粒在單幀內重複推進
    int startY = (dy >= 0) ? (GRID_H - 1) : 0;
    int endY   = (dy >= 0) ? -1 : GRID_H;
    int stepY  = (dy >= 0) ? -1 : 1;

    int startX = (dx >= 0) ? (GRID_W - 1) : 0;
    int endX   = (dx >= 0) ? -1 : GRID_W;
    int stepX  = (dx >= 0) ? -1 : 1;

    for (int y = startY; y != endY; y += stepY) {
        for (int x = startX; x != endX; x += stepX) {
            uint8_t grain = _grid[x][y];
            if (grain == 0) continue;

            int targetX = x + dx;
            int targetY = y + dy;

            // 優先朝正重力方向墜落
            if (targetX >= 0 && targetX < GRID_W && targetY >= 0 && targetY < GRID_H && _grid[targetX][targetY] == 0) {
                _grid[targetX][targetY] = grain;
                _grid[x][y] = 0;
                continue;
            }

            // 若正下方被阻擋，嘗試向左右兩側斜下滑落 (形成 45 度安息角沙堆)
            bool preferLeft = (EntropyManager::random(0, 2) == 0);
            int8_t side1 = preferLeft ? -1 : 1;
            int8_t side2 = preferLeft ? 1 : -1;

            int s1X = x + side1;
            int s1Y = y + dy;
            if (s1X >= 0 && s1X < GRID_W && s1Y >= 0 && s1Y < GRID_H && _grid[s1X][s1Y] == 0) {
                _grid[s1X][s1Y] = grain;
                _grid[x][y] = 0;
                continue;
            }

            int s2X = x + side2;
            int s2Y = y + dy;
            if (s2X >= 0 && s2X < GRID_W && s2Y >= 0 && s2Y < GRID_H && _grid[s2X][s2Y] == 0) {
                _grid[s2X][s2Y] = grain;
                _grid[x][y] = 0;
                continue;
            }

            // 若橫向加速度強烈 (橫擺)，嘗試純水平滾移
            if (fabsf(ax) > 0.45f) {
                int hX = x + dx;
                if (hX >= 0 && hX < GRID_W && _grid[hX][y] == 0) {
                    _grid[hX][y] = grain;
                    _grid[x][y] = 0;
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

        updatePhysics(ax, ay, input.isActivelyShaking);
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
