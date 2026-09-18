# M5StickC Plus & MiniJoyC 韌體架構與維護指南 (Development Guide)

本文件專為接手開發者編寫，詳細剖析系統底層架構、MiniJoyC STM32 協同晶片通訊、非阻塞硬體狀態機與 IMU 體感偵測演算法，以利後續功能的無縫擴充與維護。

---

## 1. 系統架構設計哲學

### 1.1 全域零動態記憶體配置 (Zero Dynamic Allocation)
- **設計原則**：在嵌入式 ESP32 (Arduino Framework) 環境下，長時間頻繁使用 `new`、`delete` 或 `String` 物件容易造成 Heap 碎片化，導致系統無預警當機（Guru Meditation Error / LoadProhibited）。
- **實作規範**：
  - 全數場景物件（`SceneDice`, `ScenePoker`, `SceneEightBall` 等）皆於 `main.cpp` 中以靜態全域實體（Static Global Instance）宣告，生命週期常駐於 BSS 段。
  - 字串格式化一律採用 `snprintf` 搭配區域棧緩衝區（Stack Buffer），徹底杜絕記憶體洩漏。

### 1.2 非阻塞硬體狀態機 (Non-blocking State Machine)
- **設計原則**：主迴圈 `loop()` 週期嚴格維持在 10ms 左右（> 60 FPS 刷新響應），嚴禁在任何場景或按鈕事件中使用 `delay()` 進行長等待。
- **實作規範**：
  - 所有時序（輪盤巡航、老虎機煞車、八號球冒泡、硬幣翻轉）皆透過 `millis()` 記錄起始時間與當前時間差（`elapsed`）驅動狀態機演進。
  - 蜂鳴器聲音長度由 `AudioManager::update()` 自行依據時間戳記拉低 GPIO 2 關閉，避免佔用 CPU。

---

## 2. 硬體驅動與協同晶片通訊機制

### 2.1 MiniJoyC HAT 協同晶片 (STM32F030F4P6)
- **通訊介面**：硬體 I2C 匯流排，位址 `0x54`，SDA 為 GPIO 0，SCL 為 GPIO 26。
- **搖桿讀取與座標修正**：
  - STM32 協同晶片每幀回傳 5 個 Byte：
    - `Byte 0`: X 軸偏移量（signed 8-bit, -128 ~ 127）
    - `Byte 1`: Y 軸偏移量（signed 8-bit, -128 ~ 127）
    - `Byte 2`: 搖桿按鍵狀態（0: 按下, 1: 釋放）
    - `Byte 3~4`: 韌體版本 / 保留
  - **死區過濾 (Deadzone)**：
    ```cpp
    #define JOY_DEADZONE 25
    int8_t rawX = joyc.getX();
    int8_t rawY = joyc.getY();
    joyX = (abs(rawX) > JOY_DEADZONE) ? rawX : 0;
    joyY = (abs(rawY) > JOY_DEADZONE) ? rawY : 0;
    ```
  - **重要除錯經驗 (坑位防呆)**：
    MiniJoyC STM32 在未被按下時回傳值常態為 1。**嚴禁將中心鍵作為持續長按判斷依據**，長推/長拉判定必須純粹依據實體搖桿偏角（例如 `joyY > 35`）或實體按鍵（`Button A`）。

### 2.2 SK6812 全彩 RGB LED 驅動
- **通訊方式**：由 STM32 協同晶片代為驅動單顆 3535 SK6812。
- **暫存器協議**：
  - 向 I2C 位址 `0x54` 寫入暫存器 `0x20`（R）、`0x21`（G）、`0x22`（B）。
  - `LedManager` 類別內建 HSV-to-RGB 色彩空間換算與非阻塞爆閃控制器（`flash()`）。

### 2.3 MPU6886 體感甩動偵測演算法
- **感測器更新頻率**：每次 `InputManager::update()` 讀取加速度（`accX, accY, accZ`）與角速度（`gyroX, gyroY, gyroZ`）。
- **人體工學門檻設計**：
  - 避免過度敏感誤觸發，綜合判定條件：
    ```cpp
    float gyroMag = sqrt(gx*gx + gy*gy + gz*gz);
    float deltaAcc = abs(totalAcc - lastTotalAcc);
    // 門檻：總角速度 > 450 deg/s 且 加速度劇烈變動 > 3.2G 且 距離上次觸發 > 900ms
    bool isShaken = (gyroMag > 450.0f) && (deltaAcc > 3.2f) && (now - lastShakeTime > 900);
    ```
  - **靜止判定 (`isNearlyStill`)**：
    考量人手持握微顫（Physiological Tremor），靜止門檻設為 `gyroMag < 220.0 deg/s`。

---

## 3. 新增遊戲場景 SOP (How to Add a New Scene)

若回家後想新增第 8 款遊戲（例如俄羅斯方塊或計數器）：

1. **定義場景列舉**：
   - 於 `include/Config.h` 的 `enum GameScene` 中加入新場景（例如 `SCENE_TETRIS`）。
2. **建立場景類別**：
   - 在 `include/scenes/` 建立 `SceneTetris.h`，繼承 `Scene` 並實作：
     ```cpp
     void init() override;
     void update(InputManager& input, AudioManager& audio, LedManager& led) override;
     void draw() override;
     GameScene getSceneId() const override { return SCENE_TETRIS; }
     ```
   - 在 `src/scenes/` 建立 `SceneTetris.cpp`，編寫繪製與操控邏輯。
3. **註冊至主選單**：
   - 在 `src/scenes/SceneMenu.cpp` 的 `MENU_ITEMS` 陣列加入新遊戲項目：
     ```cpp
     {"TETRIS MINI", "STACK & CLEAR", TFT_ORANGE, SCENE_TETRIS}
     ```
   - 3 卡片視窗（`SceneMenu`）會自動計算總數並自適應滾動條！
4. **主程式掛載**：
   - 於 `src/main.cpp` 中 `#include "scenes/SceneTetris.h"`。
   - 實例化 `SceneTetris sceneTetris;`。
   - 在 `switchScene` 加入 `case SCENE_TETRIS: currentScene = &sceneTetris; break;`。
   - 更新開機字樣為 `8-in-1 System`。
5. **本地驗證與燒錄**：
   - 執行 `pio run` 確保 exit code 0，再執行 `pio run -t upload`。
