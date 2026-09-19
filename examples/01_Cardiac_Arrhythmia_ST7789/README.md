# Example 01: Cardiac Arrhythmia Monitor

> **Hardware:** LilyGO T-Display ESP32 (ST7789 IPS 135×240)  
> **Complexity:** Advanced — requires specific board + wiring

## What It Does

Real-time ECG arrhythmia monitor with hardware oscilloscope sweep on a color IPS display:

- Renders Lead-II ECG waveform at ~37 FPS
- Detects arrhythmias via Q-SETUN ternary attractor (1.0 µs per inference)
- Visual + acoustic alerts: red LED + buzzer on anomaly, green LED on normal beat
- Live telemetry HUD: latency, heap, temperature, FPS, charge

## Hardware Required

| Component | Pin | Notes |
|:--|:--|:--|
| LilyGO T-Display ESP32 | — | Built-in ST7789 135×240 IPS |
| Active Buzzer (TMB12A05) | GPIO 25 | Active HIGH |
| Red LED | GPIO 26 | Arrhythmia alarm |
| Green LED | GPIO 27 | Normal beat flash |
| Button (built-in) | GPIO 0 | Press to inject muscle noise |

## Signal Source

Uses embedded synthetic ECG dataset (`ecg_dataset.h`) — no external sensor needed. The dataset follows a MIT-BIH arrhythmia profile with injected PVC beats.

## How to Run

1. Install [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) or use the included `st7789_display.h` direct SPI driver
2. Select board: **TTGO T-Display** in Arduino IDE (or `board = esp32dev` in PlatformIO)
3. Upload and open Serial Monitor at 115200 baud for JSON telemetry output

## Noise Injection

Press the **top button (GPIO 0)** to inject real-time muscle noise into the ECG stream. Watch Q-SETUN's cellular apoptosis dissolve the noise without triggering false alarms.
