# Example 03: Noise Apoptosis Stress Test

> **Hardware:** Any Arduino-compatible board  
> **Complexity:** Beginner — no sensor required (synthetic signal)

## What It Does

Demonstrates Q-SETUN's cellular apoptosis in action:

1. Generates a clean sine wave (simulating periodic sensor signal)
2. Periodically injects aggressive high-frequency noise bursts (±0.65)
3. Shows how opposing trits (+1 followed by -1) self-annihilate to 0

**Zero false positives** under extreme noise degradation.

## Hardware Required

None — uses a synthetic signal. Any board works.

## How to Run

1. Upload to any board
2. Open Serial Monitor at 115200 baud
3. Watch the `[APOPTOSIS ANNIHILATED NOISE]` messages during noise injection windows

## Expected Output

```
    [STABLE BASIN]  InjectedNoise: NO  | Signal: 0.39 | Trit: 1 | NetCharge: 3
    [STABLE BASIN]  InjectedNoise: NO  | Signal: 0.37 | Trit: 1 | NetCharge: 4
[APOPTOSIS ANNIHILATED NOISE]     [STABLE BASIN]  InjectedNoise: YES | Signal: 0.92 | Trit: 0 | NetCharge: 4
[APOPTOSIS ANNIHILATED NOISE]     [STABLE BASIN]  InjectedNoise: YES | Signal: -0.28 | Trit: 0 | NetCharge: 4
```

## Key Concepts Demonstrated

- Tier A apoptosis: opposing direction chatter (+1 → -1) annihilated
- `noise_annihilated` flag raised without triggering `is_anomaly`
- Stable attractor basin maintained through noise injection
