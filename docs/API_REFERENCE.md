# Q-SETUN API Reference

> Complete reference for `qsetun.h` v2.0.0
> All methods are `O(1)` time, `0` heap allocation, `0` floating-point operations in the core path.

---

## Types

### `QTrit` — Balanced Ternary State

```cpp
enum class QTrit : int8_t {
    Negative = -1,  // Repolarization / S-dip / Deflection below baseline
    Zero     =  0,  // Ground equilibrium / Isoelectric baseline
    Positive =  1   // Depolarization / Action potential / Excitation above baseline
};
```

Cast to integer: `static_cast<int8_t>(state.current_trit)` → returns `-1`, `0`, or `+1`.

---

### `QState` — Inference Output

Returned by every call to `feed()`.

```cpp
struct QState {
    bool     is_anomaly;           // True if the last completed cycle was anomalous
    bool     beat_classified;      // True if a full excursion cycle just completed
    bool     noise_annihilated;    // True if cellular apoptosis dissolved a noise spike
    uint8_t  anomaly_score_pct;    // 0 (healthy) to 98 (anomaly). Integer, zero-FLOP.
    float    anomaly_score;        // 0.0 to 0.98. Backward-compatible float copy.
    int16_t  charge;               // Net topological charge of the last completed cycle
    uint16_t cycle_width;          // Width of last cycle in samples
    QTrit    current_trit;         // Discretized trit for the current sample
    uint32_t cycles_count;         // Total completed cycles since reset
};
```

**Key fields explained:**

| Field | When it changes | What it means |
|:--|:--|:--|
| `is_anomaly` | On cycle completion | The completed cycle had abnormal width (>14 samples) or unbalanced charge (≥`charge_limit`) |
| `beat_classified` | On cycle completion | A full positive→zero excursion just ended. Check `charge` and `cycle_width` for its shape. |
| `noise_annihilated` | Every sample | High-frequency jitter was dissolved: either opposing trits (+1 then -1) cancelled, or a 1-sample glitch was erased. |
| `anomaly_score_pct` | On cycle completion | `2` = normal, `98` = anomaly. Stays at last value between completions. |
| `current_trit` | Every sample | The filtered ternary state after apoptosis: `-1`, `0`, or `+1`. |

---

## Class: `QSetun`

### Constructor

```cpp
QSetun qsetun;
```

Creates an engine instance. Calls `reset()` internally. Memory footprint: **192 bytes** of flat static state. No heap allocation.

---

### `begin()` — Initialize Thresholds

Three overloads:

#### Float (Backward-Compatible)

```cpp
void begin(float pos_threshold = 0.35f, float neg_threshold = -0.25f, int16_t charge_limit = 6);
```

For normalized sensor values in the range `[-1.0, 1.0]`. Internally scales to Q8 fixed-point.

```cpp
// Default clinical ECG thresholds
qsetun.begin();

// Custom thresholds for a sensitive piezo
qsetun.begin(0.15f, -0.10f, 4);
```

#### Integer Q8 (Recommended for Embedded)

```cpp
void begin(int32_t pos_threshold_q8, int32_t neg_threshold_q8, int16_t charge_limit = 6);
```

Pass pre-scaled Q8 values (multiply your raw ADC threshold by 256).

```cpp
// Threshold = 30 ADC counts → 30 << 8 = 7680
qsetun.begin((int32_t)(30 << 8), (int32_t)(-20 << 8), 6);
```

#### Int Convenience (AVR-Safe)

```cpp
void begin(int pos_threshold_q8, int neg_threshold_q8, int charge_limit = 6);
```

Prevents ambiguous overload resolution on 16-bit AVR platforms.

---

### `calibrate()` — Zero-FLOP Auto-Calibration

```cpp
void calibrate(int16_t (*read_fn)(), uint16_t samples = 128, uint8_t n_sigma = 3);
```

**The recommended way to initialize Q-SETUN.** Reads `samples` ambient readings via `read_fn`, computes integer mean + integer standard deviation (digit-by-digit square root, 0 FLOPs), and sets:
- Baseline EMA = mean
- Positive threshold = +`n_sigma` × σ
- Negative threshold = −`n_sigma` × σ

**Parameters:**

| Param | Default | Description |
|:--|:--:|:--|
| `read_fn` | required | Function pointer: `int16_t myReadFunc()`. Must return one raw sensor sample. |
| `samples` | `128` | Number of calibration samples. 128 = ~130 ms on Arduino, ~0.1 ms per sample. More = more precise. |
| `n_sigma` | `3` | Noise envelope multiplier. `3` = 99.7% confidence (3-sigma rule). Use `2` for more sensitive, `4` for less sensitive. |

**Example:**

```cpp
int16_t readSensor() {
    return (int16_t)analogRead(A0);
}

void setup() {
    qsetun.calibrate(readSensor, 256, 3);
    // Engine is now calibrated to ambient noise floor.
    // Any signal exceeding 3σ of the noise will trigger detection.
}
```

**Important:**
- `calibrate()` preserves calibrated baseline and variance — it only clears wave-tracking state.
- Keep the sensor **still and quiet** during calibration (no movement, no signal source).
- If `read_fn` is `nullptr` or `samples` is `0`, calibrate silently returns without changing state.

---

### `feed()` — Core Inference Step

```cpp
QState feed(int16_t raw_value);   // Primary: raw integer input
QState feed(float raw_value);      // Wrapper: auto-scales normalized floats
```

**The main function.** Call once per sample. Returns a `QState` with the current classification.

#### Integer Version (Recommended)

```cpp
int16_t raw = analogRead(A0);     // 0..4095 on ESP32, 0..1023 on AVR
QState state = qsetun.feed(raw);
```

Execution time: **~1.0 μs** on ESP32, **~6 μs** on 16 MHz AVR. Deterministic O(1). Zero floating-point operations.

#### Float Version (Backward-Compatible)

```cpp
float normalized = (analogRead(A0) - 2048) / 2048.0f;  // -1.0 to +1.0
QState state = qsetun.feed(normalized);
```

Internally scales by 256 and calls the integer version. Values in `[-10.0, 10.0]` are treated as normalized; outside that range, cast directly to `int16_t`.

---

### `setThresholds()` — Dynamic Threshold Update

```cpp
void setThresholds(int32_t pos_threshold_q8, int32_t neg_threshold_q8, int16_t charge_limit = 6);
```

Change thresholds at runtime without resetting the engine state. Useful for adaptive sensitivity.

```cpp
// Increase sensitivity at night (lower thresholds)
qsetun.setThresholds(5 << 8, -(5 << 8), 4);

// Decrease sensitivity during daytime noise
qsetun.setThresholds(50 << 8, -(40 << 8), 8);
```

---

### `reset()` — Full State Reset

```cpp
void reset();
```

Clears all internal state: baseline, variance, ring buffer, wave tracking, cycle count, anomaly flags. Returns the engine to construction-time defaults. Call this when switching sensor channels or after a major configuration change.

---

### Telemetry Getters

```cpp
int16_t  getBaseline()    const;  // Current adaptive baseline (raw units, de-scaled from Q8)
bool     isAnomaly()      const;  // Last anomaly flag
uint8_t  getScorePct()    const;  // Last anomaly score 0..100
float    getScore()       const;  // Last anomaly score 0.0..1.0
uint32_t getCyclesCount() const;  // Total completed cycles
```

These return the **last computed** values. They don't trigger a new inference step.

---

## Memory Layout

Total static footprint: **192 bytes**.

```
┌─────────────────────────────────────┐
│  Thresholds (pos, neg, charge_limit)│  10 bytes
│  Baseline EMA (Q8 int32_t)          │   4 bytes
│  Variance EMA (Q8 int32_t)          │   4 bytes
│  Trit Ring Buffer [32]              │  32 bytes
│  Ring head + prev_trit + prev_diff  │   6 bytes
│  Wave tracking (in_wave, width, Q)  │   5 bytes
│  Anomaly state + scores             │   7 bytes
│  Last charge + last width           │   4 bytes
│  Cycles count                       │   4 bytes
├─────────────────────────────────────┤
│  Total: ~76 bytes active state      │
│  + alignment padding to 192 bytes   │
└─────────────────────────────────────┘
```

No `malloc()`. No `new`. No `std::vector`. No heap. Ever.

---

## Platform Notes

| Platform | ADC Range | `feed()` Input | Latency |
|:--|:--|:--|:--|
| Arduino Uno (ATmega328P) | 0–1023 (10-bit) | `int16_t` | ~6 μs |
| Arduino Mega (ATmega2560) | 0–1023 (10-bit) | `int16_t` | ~6 μs |
| ESP32 | 0–4095 (12-bit) | `int16_t` | ~1.0 μs |
| ESP32-S3 | 0–4095 (12-bit) | `int16_t` | ~0.8 μs |
| STM32 BluePill (F103) | 0–4095 (12-bit) | `int16_t` | ~1.5 μs |
| STM32 BlackPill (F411) | 0–4095 (12-bit) | `int16_t` | ~0.9 μs |
| RP2040 (Pico) | 0–4095 (12-bit) | `int16_t` | ~1.2 μs |

All platforms use the same `qsetun.h` with zero `#ifdef` branching in the inference path.
