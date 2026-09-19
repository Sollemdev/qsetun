# Example 04: Auto-Calibration + Serial Plotter

> **Hardware:** Any Arduino-compatible board  
> **Complexity:** Beginner — **zero wiring required**

## What It Does

Complete demonstration of Q-SETUN's auto-calibration system:

1. Generates quiet baseline samples → `calibrate()` computes integer mean + 3σ thresholds
2. Runs a synthetic signal with periodic anomaly spikes and noise bursts
3. Outputs CSV for Arduino Serial Plotter visualization

## Hardware Required

None — fully synthetic. Works on any board.

## How to Run

1. Upload to any board
2. Open **Serial Plotter** (`Ctrl+Shift+L` in Arduino IDE) at 115200 baud
3. Three lines appear:
   - **Signal** — raw synthetic waveform (~512 ± 40)
   - **AnomalyScore** — 0 (normal) or 98 (anomaly detected)
   - **Trit** — current ternary state × 50 (for visibility: -50, 0, +50)

## What You'll See

- **Every ~200 steps:** a sharp spike (+120 counts) triggers anomaly detection → score jumps to 98
- **Every ~80 steps:** opposing ±60 noise bursts are silently annihilated by apoptosis
- **Baseline:** smooth sine wave stays at score 0

## Key Concepts Demonstrated

- `calibrate(readFn, 128, 3)` — zero-FLOP auto-calibration from ambient noise
- Integer `feed(int16_t)` — the primary recommended API
- Serial Plotter integration via CSV output
