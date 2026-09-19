# Changelog

All notable changes to Q-SETUN are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
