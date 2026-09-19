# Q-SETUN Tuning Guide

> How to configure Q-SETUN for your specific sensor, signal, and application.
> If `calibrate()` works for your use case — stop reading. This guide is for when you need manual control.

---

## Decision Tree: Which Initialization Method?

```
Do you know your sensor's noise floor and signal amplitude?
│
├── NO → Use calibrate()        ← Start here. Works 80% of the time.
│         qsetun.calibrate(readSensor, 128, 3);
│
└── YES → Use begin()
          │
          ├── Normalized floats [-1.0, 1.0]?
          │   → qsetun.begin(0.35f, -0.25f, 6);
          │
          └── Raw ADC integers?
              → qsetun.begin(30 << 8, -(20 << 8), 6);
```

---

## The Three Parameters

Q-SETUN has exactly **three** tunable parameters. Everything else is adaptive.

### 1. `pos_threshold` — Positive Excitation Boundary

**What it does:** Minimum signal deviation above baseline to register as `QTrit::Positive (+1)`.

| Too low | Correct | Too high |
|:--|:--|:--|
| Everything triggers. Noise → false positives. | Normal signal = `0`, real events = `+1`. | Nothing triggers. Real anomalies missed. |

**How to set:**
- With `calibrate(n_sigma=3)`: automatically set to 3× ambient noise standard deviation.
- Manually: set to ~1.5–2× peak-to-peak noise amplitude.

```cpp
// If your sensor's noise floor is ±10 ADC counts:
// Set positive threshold to 30 ADC counts (3× noise)
qsetun.begin((int32_t)(30 << 8), ...);
```

### 2. `neg_threshold` — Negative Deflection Boundary

**What it does:** Maximum signal deviation below baseline to register as `QTrit::Negative (-1)`.

- For symmetric signals (vibration, AC current): set to `-pos_threshold`.
- For asymmetric signals (ECG, where R-peak is tall and S-dip is shallow): set `neg_threshold` closer to zero than `pos_threshold`.

```cpp
// Symmetric (vibration sensor):
qsetun.begin(0.30f, -0.30f, 6);

// Asymmetric (ECG-like):
qsetun.begin(0.35f, -0.25f, 6);
```

### 3. `charge_limit` — Topological Charge Budget

**What it does:** Maximum allowed net charge `|Q|` in a single excursion cycle before it's classified as anomalous.

| Value | Behavior | Use case |
|:--:|:--|:--|
| `3–4` | Very sensitive. Short, balanced cycles only. | Precision vibration monitoring, machine fault detection |
| `5–6` | Default. Good for biological signals. | ECG, PPG, respiration |
| `8–12` | Relaxed. Tolerates wide, asymmetric cycles. | Slow environmental sensors, current monitoring |

**How it works:** During each excursion (signal rises above baseline, then returns), Q-SETUN accumulates net charge: each `+1` trit adds +1, each `-1` subtracts 1. A balanced cycle (positive peak followed by negative dip) nets near zero. An unbalanced cycle (e.g., sustained positive deflection) accumulates charge. If `|Q| ≥ charge_limit`, the cycle is flagged anomalous.

---

## Calibration Deep Dive

### When to Use `calibrate()`

✅ Use when:
- You don't know the sensor's noise characteristics
- The noise floor may change between deployments (temperature, cable length, power supply)
- You want zero-configuration plug-and-play

❌ Don't use when:
- You need deterministic, identical thresholds across devices
- The sensor is actively producing signal during boot (can't get a quiet baseline)
- You're running on a battery and need to skip the 128-sample warm-up delay

### The `n_sigma` Parameter

| `n_sigma` | Confidence | Sensitivity | False positive risk |
|:--:|:--:|:--|:--|
| `2` | 95.4% | **High** — catches subtle anomalies | Higher — more noise triggers |
| `3` | 99.7% | **Default** — balanced | Low |
| `4` | 99.99% | **Low** — only strong anomalies | Very low |
| `5` | 99.9999% | **Very low** — only extreme events | Near zero |

```cpp
// High sensitivity (noisy environment, you'll filter downstream)
qsetun.calibrate(readSensor, 128, 2);

// Default (most applications)
qsetun.calibrate(readSensor, 128, 3);

// Low sensitivity (industrial environment, lots of background vibration)
qsetun.calibrate(readSensor, 256, 4);
```

### Sample Count

More samples = more accurate baseline, but longer startup delay.

| `samples` | Duration (1 ms/sample) | Use case |
|:--:|:--:|:--|
| `64` | ~64 ms | Fast boot, rough calibration |
| `128` | ~128 ms | Default. Good balance. |
| `256` | ~256 ms | High precision, stable environment |
| `512` | ~512 ms | Laboratory-grade calibration |

---

## Tuning for Common Sensors

### ECG / Pulse Sensor (MAX30102, AD8232)

```cpp
qsetun.begin(0.35f, -0.25f, 6);
// Or auto-calibrate with patient at rest:
qsetun.calibrate(readSensor, 256, 3);
```
- Asymmetric thresholds: R-peak is tall (+0.35), S-dip is shallow (-0.25)
- `charge_limit = 6`: Normal QRS complex has charge ~3–5

### Vibration / Accelerometer (ADXL345, MPU6050, Piezo)

```cpp
qsetun.begin(0.20f, -0.20f, 4);
```
- Symmetric thresholds: vibration is bidirectional
- `charge_limit = 4`: Machine faults produce unbalanced shock waves

### Current Sensing (ACS712, INA219)

```cpp
qsetun.calibrate(readSensor, 256, 3);
```
- Use `calibrate()` — mains hum creates a predictable baseline
- Any deviation beyond 3σ indicates overcurrent or arc fault

### Microphone / Audio Envelope

```cpp
qsetun.begin(0.15f, -0.15f, 8);
```
- Low thresholds: audio signals have wide dynamic range
- High `charge_limit`: audio cycles are naturally wide

### Temperature (Thermistor, DS18B20)

```cpp
qsetun.begin(5 << 8, -(5 << 8), 12);
```
- Integer thresholds in ADC counts: temperature changes slowly
- High `charge_limit`: thermal cycles are very long

---

## Troubleshooting

### "Everything is flagged as anomaly"

**Cause:** Thresholds too low relative to noise floor.

**Fix:**
```cpp
// Option 1: Increase n_sigma
qsetun.calibrate(readSensor, 128, 4);  // was 3, now 4

// Option 2: Manually raise thresholds
qsetun.begin(0.50f, -0.40f, 8);       // was 0.35, -0.25, 6
```

### "Nothing is ever detected"

**Cause:** Thresholds too high, or signal never crosses baseline back to zero.

**Fix:**
```cpp
// Option 1: Lower n_sigma
qsetun.calibrate(readSensor, 128, 2);  // was 3, now 2

// Option 2: Lower thresholds
qsetun.begin(0.15f, -0.10f, 4);

// Option 3: Check if signal is DC-offset (never crosses zero)
// → Subtract baseline manually before feeding:
int16_t raw = analogRead(A0);
int16_t centered = raw - qsetun.getBaseline();
QState state = qsetun.feed(centered);
```

### "Watchdog timeout fires every 48 samples"

**Cause:** Signal stays above threshold for >48 consecutive samples without returning to baseline.

**What's happening:** The wave tracker entered an excursion but never saw a return to `QTrit::Zero`. The 48-sample watchdog forcibly closes the cycle to prevent deadlock.

**Fix:**
- Lower sampling rate (increase `delay()`) so cycles fit within 48 samples
- Raise `pos_threshold` so the signal doesn't stay above it for so long
- This is normal for very slow signals — the watchdog is a safety net, not an error

### "Noise annihilation kills my real signal"

**Cause:** Real signal has rapid polarity reversals that look like noise.

**Fix:** This means your signal's dominant frequency is close to the sampling rate. Either:
- Increase sampling rate (reduce `delay()`)
- Disable apoptosis is not possible in v2.0 — but raising thresholds above your signal amplitude will prevent trits from triggering at all, achieving the same effect

---

## Runtime Adaptation

Q-SETUN's baseline and variance EMA filters are **always running**. Even after `begin()` or `calibrate()`, the engine continuously adapts:

- **Baseline** tracks slow drift with α = 1/64 (~1.6% per sample)
- **Variance** tracks noise floor changes with α = 1/32 (~3.1% per sample)

This means:
- Gradual temperature drift → baseline follows, no false alarms
- Sudden sensor offset → temporary anomaly detection, then re-adaptation in ~50–100 samples
- If you need a **fixed** baseline (no drift), call `calibrate()` and then stop calling `feed()` until you're ready for inference

---

## Performance Cheat Sheet

| What you want | Do this |
|:--|:--|
| Plug and play, no tuning | `qsetun.calibrate(readSensor)` |
| Known signal, deterministic thresholds | `qsetun.begin(pos, neg, charge)` |
| Change sensitivity at runtime | `qsetun.setThresholds(pos, neg, charge)` |
| Reset after sensor swap | `qsetun.reset()` then `calibrate()` again |
| Maximum sensitivity | `n_sigma = 2`, `charge_limit = 3` |
| Maximum noise rejection | `n_sigma = 4`, `charge_limit = 10` |
| Fastest possible inference | Already O(1), ~1 μs. Can't go faster. |
