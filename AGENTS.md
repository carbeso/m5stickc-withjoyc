# M5StickC-Plus 與 MiniJoyC 紓壓玩具專案規範 (AGENTS.md)

本專案致力於利用 **M5StickC Plus** 搭配 **MiniJoyC HAT** 擴充底座，打造一款具備豐富觸覺、聽覺、視覺互動之掌上型「13 合 1 紓壓玩具與感測工具旗艦套件 (Fidget & Sensor Suite)」。

---

## 1. 專案基本資訊與架構全貌
- **硬體核心**：M5StickC Plus (ESP32-PICO-D4 @ 240MHz, 320KB RAM, 4MB Flash)
- **擴充底座**：M5Stack Hat Mini JoyC (STM32F030F4P6, I2C `0x54`, SDA: GPIO 0, SCL: GPIO 26)
- **螢幕與渲染**：1.14" ST7789v2 IPS LCD (135 × 240 直向)，全域雙緩衝畫布 `extern TFT_eSprite g_canvas;` (63.3KB 記憶體)
- **現有 13 大場景清單**：
  1. `SCENE_DICE`: 多面骰子盒 (1d4 ~ 6d100)
  2. `SCENE_POKER`: 極簡幸運撲克 (52+2張，銷牌/單抽)
  3. `SCENE_EIGHT_BALL`: 神秘八號球 (維基百科 20 款解答)
  4. `SCENE_ROULETTE`: 垂直歐式輪盤 (37格單零，隨機旋轉時長)
  5. `SCENE_SLOT`: 3×3 搖桿下拉角子老虎機 (機械拉桿體感)
  6. `SCENE_COIN`: 多枚擲硬幣 (1~5枚 3D 透視翻轉)
  7. `SCENE_RPS`: 剪刀石頭布對決 (1手/2手互搏)
  8. `SCENE_1A2B`: 1A2B 益智猜數字 (4位不重複數字)
  9. `SCENE_STANDBY`: 待機畫面 (駭客任務代碼雨 / RTC 數位時鐘)
  10. `SCENE_SENSOR_LAB`: 感測器實驗室 (水平儀 / G-Tracker / RF掃描 / LED工坊)
  11. `SCENE_SPECTRUM`: 頻譜分析儀 (SPM1423 麥克風 64點 FFT / IMU 震動頻譜)
  12. `SCENE_SAND`: 重力感應流沙 (44x80 細胞自動機，體感甩動爆散)
  13. `SCENE_TETRIS`: 極簡掌上俄羅斯方塊 (10x20 經典方塊、踢牆、軟硬降)
- **核心子系統**：
  - `EntropyManager`: 多維實體熵源管理器 (硬體 RNG + IMU 雜訊 + AXP192 + 時間微擾)
  - `InputManager`: 雙軸搖桿死區、脈衝邊緣、體感甩動偵測狀態機
  - `AudioManager`: 非阻塞無源蜂鳴器 PWM 音效引擎
  - `LedManager`: SK6812 全彩 RGB 漸變與爆閃管理
- **文件地圖**：
  - 遊戲與應用規格書：`docs/GAME_SPECS.md`
  - 韌體架構與接手指南：`docs/DEVELOPMENT_GUIDE.md`
  - 螢幕防閃爍與雙緩衝手冊：`docs/DISPLAY_OPTIMIZATION_GUIDE.md`
  - 硬體規格與 Datasheet：`docs/hardware/HARDWARE_SPECS.md`

---

## 2. 開發規範與守則
- **語言與文字**：
  - 全域採用**繁體中文台灣正體**（專用術語採台灣習慣，如：程式碼、專案、預設、最佳化、端點、暫存器、蜂鳴器、搖桿）。
  - 技術識別碼保留英文原狀（如 API、GPIO、I2C、ST7789v2、AXP192、MPU6886、SK6812）。
- **Git 分支紀律**：
  - 嚴禁直接在 `main` / `master` 提交或推送。
  - 所有功能變更必須在 `dev` 或 `feature/...`、`docs/...` 分支執行。
  - 嚴禁 `git add .` 或 `git add -A`，一律精確指定檔案路徑 `git add <file>`。
  - 提交訊息與 PR 必須清楚標註 AI 模型與代理身分。
- **渲染與繪圖規範**：
  - 高頻連續動畫場景優先使用 `g_canvas` 全域雙緩衝離線繪製，杜絕全螢幕閃爍。
  - 局部更新場景文字輸出必須使用 `setTextColor(fg, bg)` 自動覆蓋背景。
- **硬體匯流排隔離**：
  - SPM1423 麥克風與 MiniJoyC HAT 共用 GPIO 0，音訊採樣時掛載 I2S，離開時必須卸載並以 `Wire.begin(0, 26, 400000)` 還原 I2C。
