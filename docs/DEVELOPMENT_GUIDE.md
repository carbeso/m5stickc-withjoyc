# M5StickC Plus & MiniJoyC 韌體架構與維護指南 (Development Guide)

本文件專為接手開發者編寫，詳細剖析系統底層架構、MiniJoyC STM32 協同晶片通訊、非阻塞硬體狀態機、全域雙緩衝防閃爍畫布、多維物理熵源與 IMU 體感偵測演算法，以利後續功能的無縫擴充與維護。

---

## 1. 系統架構設計哲學

### 1.1 全域零動態記憶體配置 (Zero Dynamic Allocation)
- **設計原則**：在嵌入式 ESP32 (Arduino Framework) 環境下，長時間頻繁使用 `new`、`delete` 或 `String` 物件容易造成 Heap 碎片化，導致系統無預警當機（Guru Meditation Error / LoadProhibited）。
- **實作規範**：
  - 全數 13 款場景物件（`SceneDice`, `ScenePoker`, `SceneSand`, `SceneTetris` 等）皆於 `src/main.cpp` 中以靜態全域實體（Static Global Instance）宣告，生命週期常駐於 BSS 段。
  - 字串格式化一律採用 `snprintf` 搭配區域棧緩衝區（Stack Buffer），徹底杜絕記憶體洩漏。

### 1.2 非阻塞硬體狀態機 (Non-blocking State Machine)
- **設計原則**：主迴圈 `loop()` 週期嚴格維持在 10ms 左右（> 60 FPS 刷新響應），嚴禁在任何場景或按鈕事件中使用 `delay()` 進行長等待。
- **實作規範**：
  - 所有時序（輪盤巡航、老虎機煞車、流沙推進、俄羅斯方塊下落）皆透過 `millis()` 記錄起始時間與當前時間差（`elapsed`）驅動狀態機演進。
  - 蜂鳴器聲音長度由 `AudioManager::update()` 自行依據時間戳記拉低 GPIO 2 關閉，避免佔用 CPU。

### 1.3 全域雙緩衝防閃爍架構 (Double Buffering with `g_canvas`)
- **設計原則**：傳統直接呼叫 `M5.Lcd.fillScreen(TFT_BLACK)` 抹黑再畫圖會造成強烈人眼頻閃。
- **實作規範**：
  - 於 `include/Config.h` 宣告全域單例畫布：`extern TFT_eSprite g_canvas;`。
  - 於 `src/main.cpp` 的 `setup()` 中配置全螢幕 Sprite（135×240 RGB565，記憶體約 63.3KB）：`g_canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);`。
  - 高頻連續動畫場景（如 `SceneSpectrum`、`SceneSensorLab`、`SceneStandby`、`SceneSand`、`SceneTetris`）全數於 `g_canvas` 上離線繪製，最後以 `g_canvas.pushSprite(0, 0)` 一次性推送至 ST7789v2 顯存。
  - 完整繪圖指南與局部更新規範，請參閱專屬文件：[docs/DISPLAY_OPTIMIZATION_GUIDE.md](file:///c:/laragon/www/m5stickc-withjoyc/docs/DISPLAY_OPTIMIZATION_GUIDE.md)。

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

### 2.2 GPIO 0 雙匯流排動態切換與隔離機制 (I2S vs I2C)
- **硬體接腳衝突**：M5StickC Plus 內建 SPM1423 PDM 麥克風之時脈線（`PIN_MIC_CLK`）與頂部 HAT 槽的 I2C SDA 線**共同連接至 ESP32 GPIO 0**。
- **切換隔離保護**：
  - 在執行音訊頻譜採樣（`SceneSpectrum`）時，透過 `setupAudioI2S()` 掛載 I2S 驅動接管 GPIO 0。
  - 在離開音訊模式或退出場景時，**必須呼叫 `teardownAudioI2S()` 卸載 I2S 驅動**，並立即重新執行：
    ```cpp
    Wire.begin(HAT_I2C_SDA, HAT_I2C_SCL, 400000L);
    ```
    以還原硬體 I2C 匯流排，避免 MiniJoyC HAT 搖桿與 LED 鎖死離線。

### 2.3 SK6812 全彩 RGB LED 驅動
- **通訊方式**：由 STM32 協同晶片代為驅動單顆 3535 SK6812。
- **暫存器協議**：
  - 向 I2C 位址 `0x54` 寫入暫存器 `0x20`（R）、`0x21`（G）、`0x22`（B）。
  - `LedManager` 類別內建 HSV-to-RGB 色彩空間換算與非阻塞爆閃控制器（`flash()`）。

### 2.4 MPU6886 體感甩動偵測演算法
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

### 2.5 全域物理多維熵源管理器 (`EntropyManager`)
- 系統封裝 `EntropyManager`，透過混和硬體 RF 底噪、三軸 IMU 浮點微震顫、AXP192 電源熱擾動與微秒時間戳，提供全域優質亂數：
  ```cpp
  int randVal = EntropyManager::random(0, 100);
  float randF = EntropyManager::randomFloat();
  ```

---

## 3. 新增遊戲場景 SOP (How to Add a New Scene)

若後續想擴充第 14 款遊戲或工具（例如：計數器或心率監測）：

1. **定義場景列舉**：
   - 於 `include/Config.h` 的 `enum GameScene` 中加入新場景（例如 `SCENE_COUNTER`，置於 `SCENE_COUNT` 前）。
2. **建立場景類別**：
   - 在 `include/scenes/` 建立 `SceneCounter.h`，繼承 `Scene` 並宣告覆寫方法：
     ```cpp
     void init() override;
     void update(InputManager& input, AudioManager& audio, LedManager& led) override;
     void draw() override;
     GameScene getSceneId() const override { return SCENE_COUNTER; }
     ```
   - 在 `src/scenes/` 建立 `SceneCounter.cpp`，編寫繪製（建議優先使用 `g_canvas` 雙緩衝）與操控邏輯。
3. **註冊至主選單**：
   - 在 `src/scenes/SceneMenu.cpp` 的 `MENU_ITEMS` 陣列加入新項目：
     ```cpp
     {"CLICK COUNTER", "TAP TO COUNT", TFT_CYAN, SCENE_COUNTER}
     ```
   - 3 卡片可捲動視窗（`SceneMenu`）會自動依陣列長度自適應滾動條與首尾循環導航！
4. **主程式掛載**：
   - 於 `src/main.cpp` 中 `#include "scenes/SceneCounter.h"`。
   - 實例化靜態場景物件：`SceneCounter sceneCounter;`。
   - 在 `switchScene` 函式中補上路由：`case SCENE_COUNTER: currentScene = &sceneCounter; break;`。
5. **本地驗證與燒錄**：
   - 執行 `pio run` 確保編譯返回 exit code 0，再透過 `pio run -t upload` 燒錄實機驗證。
