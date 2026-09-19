# Getting Started with Q-SETUN

> **Time to first result: 5 minutes.**
> No display required. No external libraries. Any Arduino-compatible board.

---

## Step 1: Install the Library

### Option A: Arduino IDE
1. Download the [latest release ZIP](https://github.com/Sollemdev/qsetun/releases/tag/v2.0.0).
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library...** → select the file.

### Option B: PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
    https://github.com/Sollemdev/qsetun.git
```

### Option C: Manual
Copy `src/qsetun.h` into your project folder. That's it — it's a single header.

---

## Step 2: Wire a Sensor

Any analog sensor works. Here are three options, pick one:

### Option 1: Potentiometer (Simplest — No Soldering)
```
    [3.3V] ──── POT Pin 1
    [A0]  ──── POT Pin 2 (Wiper)
    [GND] ──── POT Pin 3
```
Turn the knob to simulate signal changes.

### Option 2: Piezo Disc (Vibration / Knock Detection)
```
    [A0]  ──── Piezo (+)
    [GND] ──── Piezo (-)
    [A0]  ──┬─ 1MΩ resistor ──── [GND]
            └─ (optional 5.1V Zener to GND for protection)
```
Tap the piezo to generate anomaly spikes.

### Option 3: No Sensor At All
Use `Example 04` below — it generates a synthetic signal internally with `calibrate()` + noise injection. No wiring needed.

---

## Step 3: Upload Your First Sketch

Create a new sketch and paste:

```cpp
#include <qsetun.h>

QSetun qsetun;

// Callback for auto-calibration: reads one raw sample
int16_t readSensor() {
    return (int16_t)analogRead(A0);
}

void setup() {
    Serial.begin(115200);

    // Auto-calibrate from 128 ambient samples (learns noise floor + baseline)
    qsetun.calibrate(readSensor, 128, 3);

    Serial.println("Q-SETUN calibrated. Open Serial Plotter (Ctrl+Shift+L).");
}

void loop() {
    int16_t raw = analogRead(A0);
    QState state = qsetun.feed(raw);

    // Output for Serial Plotter (comma-separated values)
    Serial.print(raw);
    Serial.print(",");
    Serial.print(state.anomaly_score_pct);
    Serial.print(",");
    Serial.println(static_cast<int8_t>(state.current_trit) * 100);

    delay(10); // 100 Hz sampling
}
```

### Upload:
1. Select your board in **Tools → Board** (Arduino Uno, ESP32, STM32, etc.)
2. Select your COM port in **Tools → Port**
3. Click **Upload** (→)

---

## Step 4: See It Work

### Serial Plotter (Recommended)
1. Open **Tools → Serial Plotter** (or press `Ctrl+Shift+L`).
2. Set baud rate to **115200**.
3. You'll see three lines:
   - **Blue:** Raw sensor value
   - **Red:** Anomaly score (0–100%)
   - **Green:** Current trit state (×100 for visibility: -100, 0, +100)

### What to do:
- **With a potentiometer:** Turn the knob slowly — the signal is stable (score ~2%). Turn it **fast** — Q-SETUN detects the anomaly (score jumps to 98%).
- **With a piezo:** Tap the disc — each tap produces a spike that Q-SETUN classifies as a beat cycle.
- **With nothing connected:** The floating A0 pin generates random noise. Q-SETUN auto-calibrates to it and then tracks deviations.

### Expected output in Serial Monitor:
```
Q-SETUN calibrated. Open Serial Plotter (Ctrl+Shift+L).
512,2,0
513,2,100
510,2,-100
515,2,0
480,98,0       ← anomaly detected!
```

---

## Step 5: Understand the Output

Every call to `qsetun.feed()` returns a `QState` struct:

| Field | Type | Meaning |
|:--|:--|:--|
| `is_anomaly` | `bool` | `true` if an abnormal cycle was detected |
| `anomaly_score_pct` | `uint8_t` | 0 = healthy, 98 = anomaly (integer, no floats) |
| `current_trit` | `QTrit` | Current balanced trit: `-1`, `0`, or `+1` |
| `charge` | `int16_t` | Net topological charge of the last cycle |
| `cycle_width` | `uint16_t` | Duration of last cycle in samples |
| `noise_annihilated` | `bool` | `true` if apoptosis killed a noise spike this step |
| `beat_classified` | `bool` | `true` if a full excursion cycle just completed |
| `cycles_count` | `uint32_t` | Total completed cycles since boot |

---

## What's Next?

- **[API Reference](API_REFERENCE.md)** — Full documentation of every method.
- **[Tuning Guide](TUNING_GUIDE.md)** — How to adjust thresholds, charge limits, and calibration for your specific sensor.
- **[Benchmarks](BENCHMARKS.md)** — Performance data on real silicon.
- **[Example 01](../examples/01_Cardiac_Arrhythmia_ST7789/)** — Full cardiac arrhythmia monitor with color display (ESP32 + ST7789).
- **[Example 04](../examples/04_AutoCalibrate_SerialPlotter/)** — Auto-calibration demo with synthetic signal and noise injection (any board, no sensor needed).
