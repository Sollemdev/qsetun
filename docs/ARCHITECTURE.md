# Q-SETUN Architecture

> Internal design of the Zero-FLOP balanced ternary inference engine.  
> Read this if you want to understand **how** Q-SETUN works, not just **how to use** it.

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

---

## Stage 3: Cellular Apoptosis

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

---

## Memory Layout

Total: **192 bytes** (including alignment padding). Zero heap.

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
───── ~76 bytes active + alignment padding to 192 ─────
```

---

## Why Not a Neural Network?

| Property | Q-SETUN | TFLite Micro CNN |
|:--|:--|:--|
| Operations per sample | ~20 integer ops | ~50,000 MAC ops |
| Memory model | Flat static 192 B | TensorArena 24+ KB |
| Determinism | Bit-exact O(1) | Data-dependent branching |
| Noise handling | Built-in apoptosis | Must be in training data |
| Interpretability | Every trit is explainable | Black-box weights |
| Minimum hardware | ATmega328P (2 KB RAM) | Cortex-M4 (64+ KB RAM) |

Q-SETUN is **not** a general-purpose ML framework. It is purpose-built for **1D quasi-periodic anomaly detection** where a topological invariant (charge balance) is more robust than learned weights.
