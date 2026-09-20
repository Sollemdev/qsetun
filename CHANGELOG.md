# Changelog

All notable changes to Q-SETUN are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [2.1.0] — 2026-09-20

### Added
- **Ternary Hysteresis Memory** (`configure(hysteresis_q8)`) — once a trit enters `{-1, +1}` it HOLDS until `diff` crosses the inner band edge; deep sign flips are still captured instantly (v2.0 semantics). At `hysteresis_q8 = 0` the engine is bit-for-bit v2.0. Verified: **−34.3% trit chatter** on a threshold-boundary drift signal with **90.0% of waves preserved**.
- **Live Threshold Self-Reinforcement** (`configure(live_sigma)`) — with `live_sigma > 0` the engine re-derives both thresholds from the running variance EMA on every `feed()` (pos = `live_sigma × variance_ema`), a continuous extension of the 3-sigma auto-calibration idea. Verified: a 16× noise-floor jump floods legacy fixed thresholds with 1530 false beats vs **4** with `live_sigma = 3`.
- `QState.wave_energy` — accumulated `|diff|` inside the closed cycle (smoothed `>> 8`, saturated `uint16_t`).
- `QState.wave_trit_density_pct` — share of non-zero trits within the cycle width (0–100 %). Together with `wave_energy` these separate **narrow-strong** from **wide-weak** excursions (measured: 39 251 vs 16 496 energy units on equivalent-width synthetic waves).
- `configure()` method — single entry point for all v2.1 options; defaults reproduce v2.0 exactly.
- Telemetry getters (parity with the lab fork): `getVarianceEMA()`, `getPosThreshold()`, `getNegThreshold()`, `getChargeLimit()`, `getBaselineEMA()`, `getTritRing()`, `getRingHead()`, `setBaselineEMA()`, `setVarianceEMA()`.
- **Regression equivalence suite** (`benchmarks/regression_equivalence.py`) — independent frozen v2.0 reference vs v2.1-with-defaults, state-compared after every sample.
- `license=GPL-3.0-only` in `library.properties` (Arduino Library Manager).

### Changed
- Static footprint: **84 bytes** (measured with avr-g++ 7.3, ATmega328P, `-Os -std=c++11`; v2.0 was 75 bytes — the v2.1 additions cost exactly the new fields plus alignment). Flash: **1758 bytes** on ATmega328P.
- `keywords.txt`, `library.json`, `library.properties`, `CITATION.cff` — version bumped to 2.1.0.
- Documentation: API Reference, Architecture and Changelog brought in line with the v2.1 API.

### Verified (unchanged guarantees)
- 100 % integer arithmetic, zero heap (`malloc = 0`), deterministic O(1) `feed()`.
- Strict backward equivalence: 104 096 samples (random + full ECG ×4) → **0 state mismatches** vs v2.0.
- Official benchmark suite (`run_all_benchmarks.py`) — all blocks PASS: AVR silicon compile, O(1) determinism, ANSI/AAMI EC57 clinical accuracy, 4/4 environmental stress tests.
- All 4 examples compile unchanged against v2.1 (no API breakage).

---

## [2.0.0] — 2026-09-17

### Added
- **Zero-FLOP Auto-Calibration** (`calibrate()`) — integer-only mean + stddev from ambient sensor noise with n-sigma thresholds.
- **Multi-Tier Cellular Apoptosis** — Tier A (opposing trit annihilation with decay/MAD check) + Tier B (ring-buffer transient glitch suppression).
- **Non-Deadlocking Topological Attractor** — watchdog timeout at 48 samples prevents infinite wave-tracking under noise or disconnection.
- **Integer Q8 overloads** for `begin()` and `setThresholds()` — zero floating-point initialization path.
- **AVR-safe int convenience overload** for `begin()` — resolves 16-bit ambiguity.
- **Float backward-compatible `feed()` wrapper** — auto-scales normalized `[-10, 10]` inputs.
- `QState.anomaly_score_pct` — integer anomaly score (0–100), zero-FLOP alternative to float.
- `QState.noise_annihilated` flag — reports when apoptosis dissolved a noise spike.
- `QState.cycles_count` — cumulative completed cycle counter.
- Telemetry getters: `getBaseline()`, `isAnomaly()`, `getScorePct()`, `getScore()`, `getCyclesCount()`.
- **Example 03** — Noise Apoptosis Stress Test with synthetic jitter injection.
- **Example 04** — Auto-Calibration Serial Plotter (zero wiring, any board).
- Full documentation suite: API Reference, Getting Started, Tuning Guide, Benchmarks.
- CITATION.cff and Zenodo DOI ([10.5281/zenodo.22813548](https://doi.org/10.5281/zenodo.22813548)).
- GitHub Actions CI benchmark workflow.

### Changed
- Core engine migrated from floating-point to **100% integer fixed-point Q8** arithmetic.
- EMA filters now use bit-shift division (`>> 6` for baseline, `>> 5` for variance) — zero division instructions.
- Ring buffer size fixed at 32 with bitwise modulo (`& 31`).
- Anomaly classification now uses homological invariant: width > 14 OR |charge| ≥ limit OR watchdog timeout.

### Removed
- All floating-point operations from the inference hot path.
- Dynamic memory allocation (`malloc`, `new`, `std::vector`).

---

## [1.0.0] — 2026-08-01

### Added
- Initial release: balanced ternary discretization with basic anomaly detection.
- Float-only API (`begin(float, float, int16_t)` and `feed(float)`).
- Example 01 — Cardiac Arrhythmia Monitor (ESP32 + ST7789).
- Example 02 — Basic Anomaly Detector (any board).
