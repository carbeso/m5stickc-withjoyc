# M5StickC Plus & MiniJoyC 13-in-1 Handheld Fidget Toy & Sensor Suite

[English](README.md) | [繁體中文](README.zh-TW.md)

A 13-in-1 interactive handheld fidget toy and sensor exploration suite firmware specifically designed for **M5StickC Plus** paired with the **MiniJoyC HAT** extension dock.
Integrating a high-density vertical IPS display with global double-buffering zero-flicker rendering, passive buzzer haptic sound effects, SK6812 RGB ambient lighting, dual-axis analog joystick tactile feedback, MPU6886 motion gesture controls, and a multi-source physical entropy engine to deliver a rich, crisp, responsive, and immersive micro-kinetic fidgeting experience.

---

## 🎮 13 Fidget Games & Sensor Tools Overview

| # & Name | Screen & Key Features | Primary Controls | Visuals & Physical Dynamics |
| :--- | :--- | :--- | :--- |
| **1. Multi-Sided Dice Roller** | Supports 1d4 to 6d100 in single or multi-dice layouts | Joystick L/R for die type, U/D for count; Button/Shake to roll | 2.8s natural tumbling dynamic; single d20 critical hit/fail LED effects; real-time sum calculation |
| **2. Minimalist Poker Draw** | 52 standard cards + 2 Jokers in clear minimalist typography | Joystick Left to toggle mode; Button/Shake to draw; Hold to reshuffle | Supports "DECK" mode (discard) and "SINGLE" draw mode with 0.4s high-speed deck card flipper animation |
| **3. Magic 8-Ball** | Features Wikipedia standard 20 classic English fortune answers | Button press or firm device shake to reveal answer | 3s underwater churn animation with floating 8-ball & bubble particle dynamics; reveals answer in ▼ (Affirmative), ▲ (Negative), or ◆ (Neutral) |
| **4. Vertical Roulette** | European 37-pocket single-zero roulette wheel with vertical scrolling strip | Joystick pull-down (hold/flick); Button/Shake to spin | 2.5s ~ 6.0s randomized cruising duration; supports continuous spinning while held, with realistic friction deceleration |
| **5. 3×3 Slot Machine** | Classic 3x3 grid with 7 lucky symbols for pure fidget fun | Strong joystick pull-down to simulate mechanical lever; release to stop | Continuous full-speed spin while pulled down; reels stop sequentially on release (Clack, Clack, Chime!); win line & rainbow LED animation |
| **6. Multi-Coin Toss** | Supports tossing 1 to 5 coins simultaneously with detailed heads/tails engravings | Joystick L/R to change coin count; Pull-down hold/Shake to toss | 3D compressed perspective flipping with crisp clinking sound; automatically counts Heads (H) & Tails (T) for ≥2 coins |
| **7. RPS Duel (Rock-Paper-Scissors)** | Supports 1-Hand Single Player or 2-Hand Split Screen Duel | Joystick L/R to toggle mode; Pull-down hold/Shake to throw | Clean vector silhouettes for Rock ✊, Paper ✋, and Scissors ✌️; rapid cycling while pulled; automatic outcome judgment (P1/P2/TIE) |
| **8. 1A2B Bulls & Cows** | Classic 4-digit deduction puzzle with duplicate prevention and guess history | Joystick L/R to select digit, U/D to change number; Button A to submit | Guess history scroll, duplicate input guard; hitting 4A triggers golden flash and victory melody |
| **9. Standby Clock** | Dual-mode screen saver: Matrix code rain and large RTC digital clock | Joystick L/R to switch mode; Button A to cycle color themes | Auto drops backlight to 20% to save battery; 16-column falling green code rain; RTC digital clock with blinking seconds colon |
| **10. Sensor Lab** | 2D Spirit Level, 3D G-Tracker Radar, 2.4G RF Scanner, and RGB LED Studio | Joystick to navigate tabs & adjust values; Click joystick to scan | Crosshair spirit level bubble, polar G-force trajectory tracker, Wi-Fi AP scanner with RSSI bars, HSV color palette studio |
| **11. Spectrum FFT** | 64-point Radix-2 FFT audio analyzer and IMU vibration visualizer | Button B to toggle Audio/IMU mode; Button A to switch color palette | 7-band real-time bouncing bars with peak hold falloff; dynamic I2S/I2C bus isolation on GPIO 0 |
| **12. Gravity Sand Simulator** | 44×80 cellular automata particle physics simulation with up to 1400 sand grains | Joystick/Center click to spawn sand; Button A to switch theme; Shake to scatter | Gravity vector angle fluid movement with 45° angle of repose; violent shake explodes sand outward; zero flicker via double buffering |
| **13. Mini Tetris** | 10×20 classic Tetris, 7 tetrominoes with NEXT piece preview | Joystick L/R to move, pull down for soft drop; Click for hard drop; A to rotate | Wall-kick rotation support; line-clear green flash feedback; game over and restart logic |

---

## 🛠️ Hardware Specifications & Pin Configuration

- **Master Unit**: M5StickC Plus
  - **Core MCU**: ESP32-PICO-D4 (Dual-Core 240MHz, 320KB SRAM, 4MB Flash, 2.4GHz Wi-Fi & BLE)
  - **Display**: 1.14" ST7789v2 IPS TFT LCD (135 × 240 pixels, Portrait Mode), backed by global 63.3KB double-buffering canvas (`g_canvas`)
  - **Power Management**: AXP192 (Built-in Coulomb meter, charging detection, dynamic backlight adjustment)
  - **IMU Sensor**: MPU6886 6-axis Motion Sensor (3-axis Accelerometer + 3-axis Gyroscope)
  - **Audio Input**: SPM1423 Digital PDM Microphone (CLK: GPIO 0, DATA: GPIO 34)
  - **Audio Output**: Internal Passive Buzzer on GPIO 2 (PWM audio engine)
  - **Front Button**: Button A (GPIO 37)
  - **Side Button**: Button B (GPIO 39)
- **Extension Dock**: M5Hat MiniJoyC
  - **Coprocessor**: STM32F030F4P6 (I2C Address `0x54`, SDA: GPIO 0, SCL: GPIO 26)
  - **Joystick**: Dual-axis 8-bit analog joystick (X: -128~127, Y: -128~127, deadzone set to 25)
  - **Joystick Button**: Center tactile push switch (Hard drop / Confirm)
  - **Ambient Light**: Built-in 1x SK6812 Full-Color RGB LED
  - **Auxiliary Battery**: 200mAh Lithium Polymer Battery (Total combined capacity: 320mAh)

---

## 🕹️ Global System Controls

```
                      [Boot Screen: FIDGET TOY / Ultimate Suite]
                                          │
                                          ▼
                         ┌───────────────────────────────┐
                         │    Global Main Menu (Fidget OS)│ <─── Long press Button B (> 0.5s) in any scene
                         │ 3-Card Scroll View (Joy Up/Dn)│
                         └───────────────┬───────────────┘
           ┌──────────────┬────────────┼────────────┬──────────────┐
           ▼              ▼            ▼            ▼              ▼
      🎲 Dice Box     🃏 Poker     🎱 8-Ball    🎡 Roulette    🎰 Slot Machine
           │              │            │            │              │
           ▼              ▼            ▼            ▼              ▼
      🪙 Coin Toss   ✌️ RPS Duel    🔢 1A2B     ⏱️ Standby     🔬 Sensor Lab
                                       ┌────────────┴──────────────┐
                                       ▼                           ▼
                                  📊 Spectrum FFT             ⏳ Sand / 🧱 Tetris
```

### 1. Main Menu Navigation
- **Joystick Up/Down**: Smoothly scroll through 13 cards (supports seamless loop navigation with dynamic scrollbar indicator).
- **Button A / Joystick Click**: Confirm and enter selected game or tool.
- **Joystick Left/Right**: Cycle through screen backlight brightness levels (35% → 70% → 100%).
- **Short Press Button B**: Toggle global system audio (`[SND ON]` / `[MUTE]`).
- **Firm Device Shake**: Randomly select and launch a game or tool.
- **Top Status Bar**: Real-time display of battery voltage percentage (e.g., `85%`) and charging status (e.g., `+95%`).
- **Auto Screen Saver**: Inactivity exceeding 60 seconds automatically transitions to low-power Standby mode.

### 2. Universal In-Game Gestures
- **Long Press Button B (> 500ms)**: Safely teardown peripheral drivers and return to Main Menu.
- **Pull Down & Hold Joystick (joyY > 35)**: Triggers continuous full-speed spinning in Roulette, Slot Machine, Coin Toss, and RPS Duel; accelerates block falling in Tetris.
- **Motion Shake**: Firmly shaking the device at any time triggers in-game actions (rolling, flipping, shuffling, sand scattering).

---

## 💻 Quick Start & Local Development

This project uses standard **PlatformIO** for development, dependency management, compilation, and flashing.

### 1. Prerequisites
- Install [VS Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension.
- Connect Hardware: Connect M5StickC Plus via a USB Type-C cable and verify the serial port (`CH9102` or `CP210x`, e.g., `COM3`) appears in Device Manager.

### 2. Clone Repository
```bash
git clone https://github.com/carbeso/m5stickc-withjoyc.git
cd m5stickc-withjoyc
git checkout dev  # Recommended branch for development
```

### 3. Compilation & Flashing Commands
Open terminal in the project root directory:

```bash
# 1. Compilation check (verify syntax and dependencies, exit code 0)
pio run

# 2. Build and flash to device (auto-detect serial port)
pio run -t upload

# 3. Flash to specific serial port
pio run -t upload --upload-port COM3

# 4. Open Serial Monitor (Baud rate: 115200)
pio device monitor -b 115200
```

---

## 📂 Project Structure

```text
m5stickc-withjoyc/
├── include/                   # Header files
│   ├── Config.h               # Global pins, 13 scene enums, color constants, g_canvas declaration
│   ├── EntropyManager.h       # Multi-source physical entropy manager (RNG + IMU noise + AXP perturbation)
│   ├── InputManager.h         # Joystick deadzone, pulse edges, shake detection state machine
│   ├── AudioManager.h         # Non-blocking passive buzzer PWM audio engine
│   ├── LedManager.h           # SK6812 RGB ambient lighting manager
│   └── scenes/                # Scene class headers
│       ├── Scene.h            # Base abstract scene interface
│       ├── SceneMenu.h        # 3-card scrolling main menu
│       ├── SceneDice.h        # Multi-sided dice roller
│       ├── ScenePoker.h       # Minimalist poker draw
│       ├── SceneEightBall.h   # Classic Magic 8-Ball
│       ├── SceneRoulette.h    # Vertical European roulette
│       ├── SceneSlot.h        # 3x3 Slot machine
│       ├── SceneCoin.h        # Multi-coin toss
│       ├── SceneRPS.h         # Rock-Paper-Scissors duel
│       ├── Scene1A2B.h        # 1A2B Bulls and Cows puzzle
│       ├── SceneStandby.h     # Standby screen (Code rain / RTC clock)
│       ├── SceneSensorLab.h   # Sensor lab (Spirit level / G-Tracker / RF / LED)
│       ├── SceneSpectrum.h    # Audio FFT and IMU spectrum analyzer
│       ├── SceneSand.h        # Gravity sand simulator (44x80 cellular automata)
│       └── SceneTetris.h      # Mini Tetris (10x20 classic blocks)
├── src/                       # Source implementation code
│   ├── main.cpp               # Entry point, setup/loop, global g_canvas, scene dispatcher
│   ├── EntropyManager.cpp     # Multi-source entropy generator implementation
│   ├── InputManager.cpp       # Joystick deadzone, pulse trigger, shake detection
│   ├── AudioManager.cpp       # PWM tone generator and non-blocking timers
│   ├── LedManager.cpp         # RGB/HSV gradient and flashing effects
│   └── scenes/                # 13 scene logic and rendering implementation
│       ├── SceneMenu.cpp
│       ├── SceneDice.cpp
│       ├── ScenePoker.cpp
│       ├── SceneEightBall.cpp
│       ├── SceneRoulette.cpp
│       ├── SceneSlot.cpp
│       ├── SceneCoin.cpp
│       ├── SceneRPS.cpp
│       ├── Scene1A2B.cpp
│       ├── SceneStandby.cpp
│       ├── SceneSensorLab.cpp
│       ├── SceneSpectrum.cpp
│       ├── SceneSand.cpp
│       └── SceneTetris.cpp
├── lib/                       # Local driver libraries
│   └── M5HatMiniJoyC/         # MiniJoyC HAT coprocessor I2C driver
├── docs/                      # Specifications and manuals
│   ├── GAME_SPECS.md          # Complete 13-in-1 suite specification
│   ├── DEVELOPMENT_GUIDE.md   # Architecture, double buffering, and developer guide
│   ├── DISPLAY_OPTIMIZATION_GUIDE.md # Zero-flicker display optimization guide
│   └── hardware/              # Hardware specifications and datasheets
├── platformio.ini             # PlatformIO configuration file
├── README.zh-TW.md            # Traditional Chinese README
└── README.md                  # Main English README (this document)
```

---

## 📜 Development Guidelines

- **Branch Discipline**:
  - Direct commits and pushes to `main` / `master` branches are strictly prohibited.
  - Development and documentation updates must be conducted on `dev`, `feature/...`, or `docs/...` branches.
  - Always run `pio run` locally and verify exit code is 0 before committing.
  - Avoid `git add .`; explicitly specify file paths when staging (e.g., `git add README.md README.zh-TW.md`).
