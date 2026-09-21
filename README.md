# M5StickC Plus & MiniJoyC 7-in-1 Handheld Fidget Toy

[English](README.md) | [繁體中文](README.zh-TW.md)

A 7-in-1 interactive handheld fidget toy firmware system specifically designed for **M5StickC Plus** paired with the **MiniJoyC HAT** extension module.
Integrating a high-density vertical IPS display, passive buzzer haptic sound effects, SK6812 RGB ambient lighting, dual-axis analog joystick tactile feedback, and MPU6886 motion gesture controls to deliver a rich, crisp, and immersive micro-kinetic fidgeting experience.

---

## 🎮 7 Fidget Games Overview

| Game Name | Screen & Key Features | Primary Controls | Visuals & Physical Motion |
| :--- | :--- | :--- | :--- |
| **1. Multi-Sided Dice Roller** | Supports 1d4 to 6d100 in single or multi-dice layouts | Joystick L/R for die type, U/D for count; Button/Shake to roll | 2.8s natural tumbling dynamic; single d20 critical hit/fail LED effects; real-time sum calculation |
| **2. Minimalist Poker Draw** | 52 standard cards + 2 Jokers in clear minimalist typography | Joystick Left to toggle mode; Button/Shake to draw; Hold to reshuffle | Supports "DECK" mode (discard) and "SINGLE" draw mode with 0.4s high-speed deck card flipper animation |
| **3. Magic 8-Ball** | Features Wikipedia standard 20 classic English fortune answers | Button press or firm device shake to reveal answer | 3s underwater churn animation with floating 8-ball & bubble particle dynamics; reveals answer in ▼ (Affirmative), ▲ (Negative), or ◆ (Neutral) |
| **4. Vertical Roulette** | European 37-pocket single-zero roulette wheel with vertical scrolling strip | Joystick pull-down (hold/flick); Button/Shake to spin | 2.5s ~ 6.0s randomized cruising duration; supports continuous spinning while held, with realistic friction deceleration |
| **5. 3×3 Slot Machine** | Classic 3x3 grid with 7 lucky symbols for pure fidget fun | Strong joystick pull-down to simulate mechanical lever; release to stop | Continuous full-speed spin while pulled down; reels stop sequentially on release (Clack, Clack, Chime!); win line & rainbow LED animation |
| **6. Multi-Coin Toss** | Supports tossing 1 to 5 coins simultaneously with detailed heads/tails engravings | Joystick L/R to change coin count; Pull-down hold/Shake to toss | 3D compressed perspective flipping with crisp clinking sound; automatically counts Heads (H) & Tails (T) for ≥2 coins |
| **7. RPS Duel (Rock-Paper-Scissors)** | Supports 1-Hand Single Player or 2-Hand Split Screen Duel | Joystick L/R to toggle mode; Pull-down hold/Shake to throw | Clean vector silhouettes for Rock ✊, Paper ✋, and Scissors ✌️; rapid cycling while pulled; automatic outcome judgment (P1/P2/TIE) |

---

## 🛠️ Hardware Specifications & Pin Configuration

- **Master Unit**: M5StickC Plus
  - **Core MCU**: ESP32-PICO-D4 (Dual-Core 240MHz, 320KB SRAM, 4MB Flash, 2.4GHz Wi-Fi & BLE)
  - **Display**: 1.14" ST7789v2 IPS TFT LCD (135 × 240 pixels, Portrait Mode)
  - **Power Management**: AXP192 (Built-in Coulomb meter, charging detection, dynamic backlight adjustment)
  - **IMU Sensor**: MPU6886 6-axis Motion Sensor (3-axis Accelerometer + 3-axis Gyroscope)
  - **Audio Output**: Internal Passive Buzzer on GPIO 2 (PWM audio engine)
  - **Front Button**: Button A (GPIO 37)
  - **Side Button**: Button B (GPIO 39)
- **Extension Dock**: M5Hat MiniJoyC
  - **Coprocessor**: STM32F030F4P6 (I2C Address `0x54`, SDA: GPIO 0, SCL: GPIO 26)
  - **Joystick**: Dual-axis 8-bit analog joystick (X: -128~127, Y: -128~127, deadzone set to 25)
  - **Joystick Button**: Center tactile push switch
  - **Ambient Light**: Built-in 1x SK6812 Full-Color RGB LED

---

## 🕹️ Global System Controls

```
                           [Boot Screen: 7-in-1 System]
                                      │
                                      ▼
                      ┌───────────────────────────────┐
                      │    Global Main Menu (Fidget OS)│ <─── Long press Button B (> 0.5s) in any game
                      │ 3-Card Scroll View (Joy Up/Dn)│
                      └───────────────┬───────────────┘
          ┌──────────────┬────────────┼────────────┬──────────────┐
          ▼              ▼            ▼            ▼              ▼
     🎲 Dice Box     🃏 Poker     🎱 8-Ball    🎡 Roulette    🎰 Slot Machine
                                                           ┌──────┴──────┐
                                                           ▼             ▼
                                                      🪙 Coin Toss   ✌️ RPS Duel
```

### 1. Main Menu Navigation
- **Joystick Up/Down**: Smoothly scroll through 7 game cards (supports loop navigation with dynamic scrollbar indicator).
- **Button A / Joystick Click**: Confirm and enter selected game.
- **Joystick Left/Right**: Cycle through screen backlight brightness levels (35% → 70% → 100%).
- **Short Press Button B**: Toggle global system audio (`[SND ON]` / `[MUTE]`).
- **Firm Device Shake**: Randomly select and launch a game.
- **Top Status Bar**: Real-time display of battery voltage converted percentage (e.g., `85%`) and charging status (e.g., `+95%`).

### 2. Universal In-Game Gestures
- **Long Press Button B (> 500ms)**: Instantly exit current game and return to Main Menu.
- **Pull Down & Hold Joystick (joyY > 35)**: Triggers continuous full-speed spinning in Roulette, Slot Machine, Coin Toss, and RPS Duel; releasing joystick starts natural deceleration and braking sequence.
- **Motion Shake**: Firmly shaking the device at any time triggers in-game actions (rolling, flipping, shuffling).

---

## 💻 Quick Start & Local Development

This project uses standard **PlatformIO** for development, dependency management, compilation, and flashing.

### 1. Prerequisites
- Install [VS Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension (or standalone PlatformIO Core CLI).
- Connect Hardware: Connect M5StickC Plus via a USB Type-C cable and verify the serial port (`CH9102` or `CP210x`, e.g., `COM3`) appears in Device Manager.

### 2. Clone Repository
```bash
git clone https://github.com/carbeso/m5stickc-withjoyc.git
cd m5stickc-withjoyc
git checkout dev  # Recommended branch for feature development
```

### 3. Compilation & Flashing Commands
Open terminal in the project root directory:

```bash
# 1. Compilation check (verify syntax and dependencies)
pio run

# 2. Build and flash to device (auto-detect port or specify)
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
│   ├── Config.h               # Global pins, scene enums, color constants
│   ├── InputManager.h         # Joystick, button, IMU state machine
│   ├── AudioManager.h         # Non-blocking passive buzzer audio manager
│   ├── LedManager.h           # SK6812 RGB ambient light manager
│   └── scenes/                # Game scene class headers
│       ├── Scene.h            # Base abstract scene interface
│       ├── SceneMenu.h        # 3-card scrolling main menu
│       ├── SceneDice.h        # Multi-sided dice roller
│       ├── ScenePoker.h       # Minimalist poker draw
│       ├── SceneEightBall.h   # Classic Magic 8-Ball
│       ├── SceneRoulette.h    # Vertical European roulette
│       ├── SceneSlot.h        # 3x3 Slot machine
│       ├── SceneCoin.h        # Multi-coin toss
│       └── SceneRPS.h         # Rock-Paper-Scissors duel
├── src/                       # Implementation source code
│   ├── main.cpp               # Entry point, setup/loop, scene scheduler
│   ├── InputManager.cpp       # Joystick deadzone, pulse trigger, shake detection
│   ├── AudioManager.cpp       # PWM tone generator and non-blocking timers
│   ├── LedManager.cpp         # RGB/HSV gradient and flashing effects
│   └── scenes/                # Game scene logic and rendering implementation
│       ├── SceneMenu.cpp
│       ├── SceneDice.cpp
│       ├── ScenePoker.cpp
│       ├── SceneEightBall.cpp
│       ├── SceneRoulette.cpp
│       ├── SceneSlot.cpp
│       ├── SceneCoin.cpp
│       └── SceneRPS.cpp
├── lib/                       # Local driver libraries
│   └── M5HatMiniJoyC/         # MiniJoyC HAT coprocessor I2C driver
├── docs/                      # Specifications and manuals
│   ├── GAME_SPECS.md          # Complete 7-in-1 game specification
│   ├── DEVELOPMENT_GUIDE.md   # System architecture and maintenance guide
│   └── hardware/              # Hardware specifications and datasheets
├── platformio.ini             # PlatformIO configuration file
├── README.zh-TW.md            # Traditional Chinese README
└── README.md                  # Main English README (this document)
```

---

## 📜 Development Guidelines

- **Branch Discipline**:
  - Daily feature development must take place on `dev` or `feature/...` branches.
  - The `main` branch remains stable and ready for release.
  - Always run `pio run` locally and verify exit code is 0 before committing.
  - Avoid `git add .`; explicitly specify file paths when staging (e.g., `git add README.md README.zh-TW.md`).
