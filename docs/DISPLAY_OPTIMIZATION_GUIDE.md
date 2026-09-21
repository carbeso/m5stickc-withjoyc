# M5StickC Plus 螢幕防閃爍與繪圖渲染最佳化手冊 (Display Optimization Guide)

本文件專門針對 **M5StickC Plus**（ST7789v2 LCD 控制晶片）在頻繁更新畫面時的渲染效能、畫面撕裂與閃爍（Screen Flickering）問題進行原理解析，並規範兩種標準解決方案（**方案 A：雙緩衝** 與 **方案 B：局部區域更新**）供後續遊戲與應用開發者遵循。

---

## 1. 螢幕硬體特性與閃爍成因分析

### 1.1 硬體規格與架構特性
- **LCD 尺寸與解析度**：1.14 吋 Colorful TFT LCD，解析度 **135 × 240**（直向模式，Rotation 0）。
- **驅動晶片**：Sitronix **ST7789v2**，透過 SPI 介面（時脈約 20MHz ~ 27MHz）與 ESP32-PICO-D4 連接。
- **色彩深度**：16-bit 全彩（RGB565，每個像素 2 位元組）。
- **顯存容量**：全螢幕單幀記憶體大小為：
  $$135 \times 240 \times 2 \text{ 位元組} = 64,800 \text{ 位元組} \approx \mathbf{63.3\text{ KB}}$$
- **ESP32 資源評估**：ESP32 內部具備 520 KB SRAM，扣除作業系統與變數後仍有 >250 KB 可用 Heap，記憶體預算非常寬裕，完全能夠配置全螢幕 Sprite。

### 1.2 畫面閃爍（Screen Flickering）的根本原因
傳統嵌入式繪圖若直接操作 `M5.Lcd.fillScreen(TFT_BLACK)`，接著呼叫 `M5.Lcd.drawString()`、`M5.Lcd.fillRect()`，會產生嚴重的閃爍，其核心成因如下：
1. **直接寫入顯存（Direct Draw to GRAM）**：
   ST7789v2 在 M5StickC Plus 電路中未引出垂直同步訊號線（TE, Tearing Effect）。當 MCU 呼叫繪圖指令時，是直接透過 SPI 邊畫邊顯現在螢幕上。
2. **中間抹除過程暴露給人眼**：
   當呼叫 `fillScreen(TFT_BLACK)` 時，螢幕瞬間整片變黑；隨後 MCU 需要數毫秒至十幾毫秒陸續將文字與圖形畫上。在每秒 30~60 次的重繪頻率下，人眼清楚捕捉到**「整面全黑 ➔ 物件依序出現 ➔ 整面全黑」**的交替過程。
3. **黑色主題的視覺生理放大效應**：
   黑色主題並非閃爍本因，但黑色背景上通常配置高對比的亮色（白字、金黃卡片、青藍數值）。在粗暴刷黑時，這些發光區域在數十毫秒內產生激烈的「亮 ➔ 滅 ➔ 亮」亮度突波（Luminance Spikes），使人眼視網膜感受到極度刺眼的頻閃與震顫。

---

## 2. 解決方案 A：全域雙緩衝機制 (Double Buffering with TFT_eSprite)

適用於**高頻連續動畫、即時物理運算、即時音訊頻譜、粒子效果**等場景（例如：`SceneSpectrum`、`SceneSensorLab`、`SceneStandby` 矩陣雨、`SceneSlot` 滾輪）。

### 2.1 架構設計：全域單例畫布 (Zero-Fragmentation Canvas)
為避免各場景自行 `malloc` / `new` 造成記憶體碎片化，本專案在全域配置單一畫布實例：
- 定義於 `include/Config.h`：`extern TFT_eSprite g_canvas;`
- 初始化於 `src/main.cpp` 的 `setup()`：`g_canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);`

### 2.2 標準實作模式
所有背景擦除、形狀繪製、文字輸出全部在記憶體中的 `g_canvas` 執行，最後呼叫 `g_canvas.pushSprite(0, 0)` 一次性推送給螢幕。螢幕上永遠只展示繪製完成的最終幀，達成 **100% 零閃爍與極限 60 FPS 流暢度**。

```cpp
#include "Config.h"

void SceneDemo::draw() {
    if (!_needsRedraw) return;
    _needsRedraw = false;

    // 1. 在記憶體中清空畫布（完全不影響當前螢幕畫面）
    g_canvas.fillSprite(TFT_BLACK);

    // 2. 在記憶體畫布上繪製各種幾何圖形與文字
    g_canvas.fillRect(0, 0, SCREEN_WIDTH, 26, 0x18C3);
    g_canvas.setTextColor(COLOR_GOLD, 0x18C3);
    g_canvas.drawString("RADAR TRACKER", 8, 5, 2);

    g_canvas.drawCircle(67, 120, 40, TFT_GREEN);
    g_canvas.fillCircle(67 + (int)ballX, 120 + (int)ballY, 6, COLOR_CYAN);

    // 3. 一次性推送至 ST7789v2 螢幕（耗時僅約 18ms，無中間態暴露）
    g_canvas.pushSprite(0, 0);
}
```

---

## 3. 解決方案 B：局部區域更新與背景色覆蓋 (Partial / Dirty Rectangles Update)

適用於**離散狀態機、選單卡片列表、回合制益智遊戲、數位時鐘**等大部分時間畫面靜態、僅少數數值變動的場景（例如：`SceneMenu`、`Scene1A2B`、`ScenePoker`）。

### 3.1 核心技巧一：使用 `setTextColor(fg, bg)` 自動覆蓋背景
一般呼叫 `M5.Lcd.setTextColor(TFT_WHITE);` 為透明背景，新字元會與舊字元重疊，逼得開發者必須先抹黑。
若改用 `M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);`（指定文字背景色）：
- 字型繪製引擎會在字元方框內**自動填滿背景色**並同時繪製字型筆畫。
- 新數字（例如由 `98%` 變為 `99%`）直接無縫覆蓋舊數字，**完全不需事先執行 `fillRect` 或 `fillScreen`**！

### 3.2 核心技巧二：分割區域重繪標記 (Dirty Region Flags)
將場景劃分為獨立區域（如 Header、Content/Cards、Footer），在狀態更新時只標記有變化的區域重繪：

```cpp
void SceneMenu::draw() {
    // 初次進入或全域重繪
    if (_redrawAll) {
        _redrawAll = false;
        _redrawHeader = false;
        _redrawCards = false;
        M5.Lcd.fillScreen(TFT_BLACK);
        drawHeader();
        drawCards();
        drawFooter();
        return;
    }

    // 僅電量或聲音切換：只更新頂部 34px，中央卡片與底線紋絲不動
    if (_redrawHeader) {
        _redrawHeader = false;
        drawHeader();
    }

    // 僅上下選取卡片：只更新中央卡片視窗與索引文字
    if (_redrawCards) {
        _redrawCards = false;
        drawCards();
    }
}
```

---

## 4. 開發防呆查核清單 (Developer Checklist)

在為本專案新增或維護場景時，請務必檢核以下項目：

- [ ] **嚴禁在主繪圖迴圈使用 `M5.Lcd.fillScreen()`**：
  除了場景初次載入（`_redrawAll`）或全域模式切換外，嚴禁在每幀 `draw()` 中呼叫 `M5.Lcd.fillScreen()`。
- [ ] **高頻場景優先選用 `g_canvas`**：
  若畫面有超過 15 FPS 的即時動畫、連續晃動或波動柱狀圖，一律使用 `g_canvas` 雙緩衝，並以 `g_canvas.pushSprite(0, 0)` 結尾。
- [ ] **文字輸出必須指定背景色**：
  在不使用全螢幕雙緩衝的局部更新場景中，所有文字輸出必須使用雙參數形式 `setTextColor(fgColor, bgColor)`，防止字元疊印與抹黑閃爍。
- [ ] **節制重繪頻率（Frame Rate Control）**：
  動畫邏輯應透過 `millis()` 控制幀率（例如 30 FPS 或 60 FPS），避免無意義的每秒數百次重複繪製，以節省電池電量並降低晶片發熱。
