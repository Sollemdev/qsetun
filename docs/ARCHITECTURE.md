# Q-SETUN Neuromorphic Architecture & Topological Attractors

> **Theoretical Foundations:** Discrete ternary phase-space attractors, Brusentsov balanced logic ($\{-1, 0, +1\}$), and multi-tier cellular apoptosis for zero-FLOP edge intelligence.  
> **Target Audience:** Microarchitecture researchers, neuromorphic systems engineers, and embedded firmware developers.

---

## Pipeline Overview

Each call to `feed()` executes a **four-stage deterministic pipeline** in O(1) time:

```
  Raw Sample (int16_t)
        │
        ▼
  ┌─────────────────────────┐
  │ 1. Adaptive Baseline    │  EMA tracking (α = 1/64)
  │    & Variance (Q8)      │  MAD tracking  (α = 1/32)
  └─────────┬───────────────┘
            │ diff = raw_q8 - baseline
            ▼
  ┌─────────────────────────┐
  │ 2. Ternary              │  diff > +threshold → +1
  │    Discretization       │  diff < -threshold → -1
  │                         │  otherwise         →  0
  └─────────┬───────────────┘
            │ t_raw ∈ {-1, 0, +1}
            ▼
  ┌─────────────────────────┐
  │ 3. Cellular Apoptosis   │  Tier A: Opposing chatter annihilation
  │    (Noise Annihilation) │  Tier B: Ring-buffer glitch suppression
  └─────────┬───────────────┘
            │ t_filtered ∈ {-1, 0, +1}
            ▼
  ┌─────────────────────────┐
  │ 4. Topological Attractor│  Charge accumulation
  │    & Cycle Closure      │  Homological invariant check
  │                         │  Watchdog anti-deadlock (48 samples)
  └─────────┬───────────────┘
            │
            ▼
      QState output
```

---

## Stage 1: Adaptive Baseline & Variance

All math is **Q8 fixed-point** (multiply by 256, `<< 8`).

```
raw_q8      = raw_value << 8
diff        = raw_q8 - baseline_ema
baseline_ema += diff >> 6          // EMA α = 1/64
variance_ema += (|diff| - variance_ema) >> 5   // MAD α = 1/32
```

**Why Q8?** Multiplying by 256 gives 8 bits of fractional precision using only integer shifts. No `float`, no `div` instruction, no FPU required. Works identically on 8-bit AVR and 32-bit ARM.

**Why EMA with bit-shift?** Exponential moving average with α = 1/2^n collapses to a single right-shift and addition — the cheapest possible filter on any CPU.

| Filter | α | Time constant | Tracks |
|:--|:--:|:--:|:--|
| Baseline | 1/64 | ~64 samples | Slow drift (temperature, DC offset) |
| Variance | 1/32 | ~32 samples | Noise floor changes (cable, EMI) |

---

## Stage 2: Ternary Discretization

The continuous diff signal is quantized into Brusentsov's balanced ternary set `{-1, 0, +1}`:

```
if   diff >  pos_threshold → QTrit::Positive (+1)
elif diff <  neg_threshold → QTrit::Negative (-1)
else                       → QTrit::Zero      (0)
```

This is the core Brusentsov insight: three states with natural symmetry around zero carry more information per trit than binary bits, and eliminate sign-handling overhead.

**Asymmetric thresholds** allow modeling signals where positive excursion (e.g., ECG R-peak) has different amplitude than negative (S-dip).

### v2.1: Ternary Hysteresis Memory & Live Thresholds (Optional)

Two opt-in extensions keep the v2.0 pipeline untouched **unless configured**:

1. **Hysteresis hold bands** (`configure(hysteresis_q8)`): when a trit is in `+1`, it is *remembered* and held while `diff` stays above the inner edge `pos − hyst` (mirror: `−1` held while `diff < neg + hyst`). Only a crossing of the *opposite* threshold still flips the sign instantly — exactly the v2.0 fast-capture semantics. At `hyst = 0` the branch is bit-for-bit v2.0:

```
state +1:  diff > (pos − hyst)  → +1   (hold)
           diff < neg           → −1   (fast flip, v2.0 capture)
           otherwise            →  0
state −1:  diff < (neg + hyst)  → −1   (hold)
           diff > pos           → +1   (fast flip)
           otherwise            →  0
idle:      diff > pos           → +1
           diff < neg           → −1
           otherwise            →  0
```

2. **Live threshold self-reinforcement** (`configure(live_sigma)`): after the variance EMA update, both thresholds are re-derived every sample as `pos = live_sigma × variance_ema`. This is the 3-sigma calibration idea turned continuous — the engine keeps pace with a rising/falling noise floor instead of waiting for the next `calibrate()`.

Both options add **zero** floating-point and **zero** heap; they only cost a few integer ops and 9 bytes of state.

---

## Stage 3: Cellular Apoptosis

*(Untouched in v2.1 — the annihilation cascade below is bit-for-bit identical to v2.0.)*

Biological inspiration: in living organisms, damaged cells self-destruct (apoptosis) to protect the organism. Q-SETUN applies the same principle to noise.

### Tier A — Opposing Trit Annihilation

When a `+1` immediately follows a `-1` (or vice versa), it's likely high-frequency jitter rather than real signal. Annihilation occurs if **either** condition holds:

1. **Amplitude is decaying:** `|diff_current| ≤ |diff_previous|`  
   → The "bounce" is getting weaker — characteristic of ringing, not signal.

2. **Within noise envelope:** `|diff_current| ≤ 1.5 × MAD`  
   → The amplitude is within the noise floor (Mean Absolute Deviation × 1.5).

If triggered: `t_filtered = 0`, and `noise_annihilated = true`.

### Tier B — Ring Buffer Glitch Suppression

A 32-element circular buffer stores recent trit values. Pattern detection:

```
trit[n-2] = 0,  trit[n-1] ≠ 0,  trit[n] = 0
→ Isolated 1-sample spike surrounded by baseline
```

If the spike's amplitude was below `2 × MAD`, the middle trit is retroactively set to 0.

**Why 32?** Power of two allows bitwise modulo: `head = (head + 1) & 31` — zero division.

---

## Stage 4: Topological Attractor

The engine tracks **excursion cycles** — contiguous sequences of non-zero trits that begin with `+1` and end when the signal returns to `0`.

### State Machine

```
          ┌──────────────────────┐
          │     IDLE             │
          │  (_in_wave = false)  │
          └──────────┬───────────┘
                     │ t_filtered == +1
                     ▼
          ┌──────────────────────┐
          │     IN WAVE          │
          │  charge += trit      │◄─── each sample
          │  width++             │
          └──────────┬───────────┘
                     │ closure condition
                     ▼
          ┌──────────────────────┐
          │   CLASSIFY CYCLE     │
          │  check invariants    │
          └──────────┬───────────┘
                     │
                     ▼
               back to IDLE
```

### Closure Conditions

A cycle closes when **either**:

1. **Baseline return:** `t_filtered == 0` AND `width ≥ 4` samples  
   → Signal completed a physiologically meaningful excursion and returned to equilibrium.

2. **Watchdog timeout:** `width ≥ 48` samples  
   → Anti-deadlock safety. Prevents permanent lock in wave-tracking if sensor disconnects or signal saturates.

### Homological Invariant

On cycle closure, the engine checks:

```
ANOMALY if:  width > 14  OR  |net_charge| ≥ charge_limit  OR  watchdog_fired
NORMAL  if:  width ≤ 14  AND |net_charge| <  charge_limit
```

**Intuition:** A normal cycle is compact (short duration) and balanced (equal positive and negative trits → net charge near zero). An anomalous cycle is either too long, too asymmetric, or forcibly closed by the watchdog.

### v2.1: Wave Energy & Trit Density

While in wave, the engine additionally accumulates:

```
wave_energy += |diff|        // raw energetic content of the excursion (int32_t, Q8)
wave_trits  += (trit ≠ 0)    // non-zero trit counter
```

On cycle closure these are exposed as `QState.wave_energy` (smoothed `>> 8`, saturated) and `QState.wave_trit_density_pct` (`100 × wave_trits / width`). They add a *shape dimension* to the binary normal/anomaly verdict: a narrow-strong spike and a wide-weak drift can carry the same charge yet differ sharply in energy and density — which matters for tremor/force-scale biometrics.

---

## Memory Layout

Total: **84 bytes** (v2.1; actual, measured with avr-g++ 7.3 `-Os` on ATmega328P). Zero heap.

```
Offset  Size  Field
──────  ────  ─────────────────────────
0x00     4    _pos_threshold (int32_t)
0x04     4    _neg_threshold (int32_t)
0x08     2    _charge_limit  (int16_t)
0x0A     4    _baseline_ema  (int32_t, Q8)
0x0E     4    _variance_ema  (int32_t, Q8)
0x12    32    _trit_ring[32] (int8_t × 32)
0x32     1    _ring_head     (uint8_t)
0x33     1    _prev_trit     (QTrit / int8_t)
0x34     4    _prev_diff     (int32_t)
0x38     1    _in_wave       (bool)
0x39     2    _wave_width    (uint16_t)
0x3B     2    _net_charge    (int16_t)
0x3D     4    _cycles_count  (uint32_t)
0x41     1    _is_anomaly    (bool)
0x42     1    _anomaly_score_pct (uint8_t)
0x43     4    _anomaly_score (float)
0x47     2    _last_charge   (int16_t)
0x49     2    _last_width    (uint16_t)
─────── v2.1 additions (schematic offsets) ───────
       2    _hysteresis_q8  (uint16_t)
       1    _live_sigma     (uint8_t)
       4    _wave_energy    (int32_t, Q8)
       2    _wave_trits     (uint16_t)
───── 84 bytes actual (v2.1) ─────
```

---

## Why Not a Neural Network?

| Property | Q-SETUN | TFLite Micro CNN |
|:--|:--|:--|
| Operations per sample | ~20 integer ops | ~50,000 MAC ops |
| Memory model | Flat static 84 B | TensorArena 24+ KB |
| Determinism | Bit-exact O(1) | Data-dependent branching |
| Noise handling | Built-in apoptosis | Must be in training data |
| Interpretability | Every trit is explainable | Black-box weights |
| Minimum hardware | ATmega328P (2 KB RAM) | Cortex-M4 (64+ KB RAM) |

Q-SETUN is **not** a general-purpose ML framework. It is purpose-built for **1D quasi-periodic anomaly detection** where a topological invariant (charge balance) is more robust than learned weights.
