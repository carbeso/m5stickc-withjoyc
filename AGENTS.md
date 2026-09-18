# M5StickC-Plus 與 MiniJoyC 紓壓玩具專案規範 (AGENTS.md)

本專案致力於利用 **M5StickC Plus** 搭配 **MiniJoyC HAT** 擴充底座，打造一款具備豐富觸覺、聽覺、視覺互動之掌上型「紓壓玩具 (Stress Relief / Fidget Toy)」。

---

## 1. 專案基本資訊
- **硬體核心**：M5StickC Plus (ESP32-PICO-D4)
- **擴充底座**：M5Stack Hat Mini JoyC (STM32F030F4P6, I2C 0x54)
- **硬體文件目錄**：`docs/hardware/`
  - 規格統整：`docs/hardware/HARDWARE_SPECS.md`
  - 資料手冊與原理圖：`docs/hardware/datasheets/`
- **驅動與函式庫**：`lib/M5HatMiniJoyC/`

---

## 2. 開發規範與守則
- **語言與文字**：
  - 全域採用**繁體中文台灣正體**（專用術語採台灣習慣，如：程式碼、專案、預設、最佳化、端點、暫存器、蜂鳴器、搖桿）。
  - 技術識別碼保留英文原狀（如 API、GPIO、I2C、ST7789v2、AXP192、MPU6886、SK6812）。
- **Git 分支紀律**：
  - 嚴禁直接在 `main` / `master` 提交或推送。
  - 所有功能變更必須在 `dev` 或 `feature/...` 分支執行。
  - 嚴禁 `git add .` 或 `git add -A`，一律精確指定檔案路徑 `git add <file>`。
  - 提交訊息與 PR 必須清楚標註 AI 模型與代理身分。
- **程式碼註解**：
  - 程式碼必須補足清晰繁體中文註解，詳細說明腳位定義、暫存器設定與數值計算邏輯。
