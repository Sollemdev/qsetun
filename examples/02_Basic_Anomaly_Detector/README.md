# Example 02: Basic Anomaly Detector

> **Hardware:** Any Arduino-compatible board (Uno, Nano, Mega, ESP32, STM32, RP2040)  
> **Complexity:** Beginner — one analog sensor + LED

## What It Does

Universal anomaly detector for any analog sensor. Reads from A0, feeds to Q-SETUN, blinks the built-in LED on anomaly detection.

## Hardware Required

| Component | Pin | Notes |
|:--|:--|:--|
| Any analog sensor | A0 | Piezo, potentiometer, photoresistor, etc. |
| Built-in LED | LED_BUILTIN | Lights up on anomaly |

## How to Run

1. Wire any analog sensor to A0 (or just leave it floating for random noise)
2. Select your board and upload
3. Open Serial Monitor at 115200 baud
4. Create signal changes — fast pot turns, piezo taps, or covering a photoresistor

## Expected Output

```
============================================
 Q-SETUN: Universal Neuromorphic Anomaly Core
============================================
>>> [ANOMALY DETECTED!] Score: 0.980 | Trit Charge: 7 | Width: 16
```

## Key Concepts Demonstrated

- Float-based initialization: `qsetun.begin(0.35f, -0.25f, 6)`
- Normalized float input via `feed(float)`
- Checking `is_anomaly`, `charge`, `cycle_width`, and `noise_annihilated`
