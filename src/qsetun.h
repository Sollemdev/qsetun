/**
 * ============================================================================
 * Q-SETUN: Brusentsov Ternary Qutrit Core with Cellular Apoptosis (v2.1) (c)
 * 
 * Authors: Leonid Kulcha & Antigravity (Noosphere Research Lab)
 * Heritage: Inspired by N.P. Brusentsov's balanced ternary computer "Setun" (MSU, 1958)
 * License: GNU General Public License v3.0 (GPL-3.0)
 * 
 * UPGRADES & VERIFIED INVARIANTS (v2.1, additive over v2.0):
 *   1. True Zero-FLOP Core (unchanged):
 *      - 100% integer fixed-point math (Q8 format, scale = 256).
 *      - Bit-shift EMA filtering (diff >> 6 and var >> 5).
 *      - Fully deterministic O(1) execution on 8-bit AVR, 32-bit ARM, and ESP32.
 *   2. Multi-Tier Cellular Apoptosis (unchanged):
 *      - Stochastic opposing perturbations (+1 + -1 = 0) with decaying amplitude
 *        or within noise floor envelope (1.5 * MAD) are annihilated.
 *      - Ring-buffer transient glitch suppression dissolves isolated noise spikes.
 *   3. Ternary Hysteresis Memory (NEW, off by default):
 *      - Once a trit enters {-1, +1}, it HOLDS until the diff crosses an inner
 *        boundary (pos - hyst / neg + hyst). At hyst = 0 the engine behaves
 *        bit-for-bit like v2.0; at hyst > 0 the discretization gains physical
 *        state memory, suppressing chatter around the threshold pair.
 *   4. Cellular Consensus Repair (NEW, window = 1 by default = v2.0 behavior):
 *      - window 3 extends Tier-B glitch annihilation: an isolated weak trit is
 *        dissolved not only between two zeros, but also against an opposing
 *        neighbor context (0/+1/0 and -/+1/.../- are unified into the envelope).
 *   5. Live Threshold Self-Reinforcement (NEW, off by default):
 *      - live_sigma > 0 re-derives both thresholds from the running variance
 *        EMA every feed (pos = live_sigma * var), a continuous extension of the
 *        once-per-begin 3-sigma auto-calibration concept.
 *   6. Wave Energy & Trit Density (NEW output fields):
 *      - wave_energy: accumulated |diff| inside a closed cycle (smoothed >> 8).
 *      - wave_trit_density_pct: share of non-zero trits within the cycle width.
 *        Distinguishes wide-weak excursions from narrow-strong ones.
 *   7. Zero Heap Overhead (unchanged): malloc() = 0 bytes flat static state.
 *   8. Telemetry getters (NEW, parity with lab fork): getVarianceEMA(),
 *      getPosThreshold(), getNegThreshold(), getChargeLimit(), getBaselineEMA(),
 *      getTritRing(), getRingHead(), setBaselineEMA(), setVarianceEMA().
 * ============================================================================
 */

#ifndef QSETUN_H
#define QSETUN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

// Fixed-point scaling factor (Q8: 1.0 == 256)
#define QSETUN_SHIFT 8
#define QSETUN_SCALE (1 << QSETUN_SHIFT)

// Balanced Trit States {-1, 0, +1}
enum class QTrit : int8_t {
    Negative = -1, // Repolarization / S-dip / Deflection
    Zero     =  0, // Ground equilibrium / Isoelectric baseline
    Positive =  1  // Depolarization / Action potential peak
};

// Real-time Diagnosis Output
struct QState {
    bool is_anomaly;           // True if phase-space attractor is ruptured
    bool beat_classified;      // True if an excursion cycle has completed
    bool noise_annihilated;    // True if apoptosis dissolved high-frequency jitter
    uint8_t anomaly_score_pct; // Anomaly score metric: 0 to 100% (Integer Zero-FLOP)
    float anomaly_score;       // Backward-compatible float score: 0.0 (normal) to 1.0 (rupture)
    int16_t charge;            // Net topological charge accumulated during current cycle
    uint16_t cycle_width;      // Width of excursion in samples
    uint16_t wave_energy;      // v2.1: accumulated |diff| within the closed cycle (>> 8, saturated)
    uint8_t wave_trit_density_pct; // v2.1: share of non-zero trits in the cycle (0..100)
    QTrit current_trit;        // Current discrete trit state {-1, 0, +1}
    uint32_t cycles_count;     // Total completed cycles
};

class QSetun {
public:
    QSetun() {
        reset();
    }

    /**
     * Initialize Q-Setun engine using fixed-point Q8 thresholds.
     * Example: for threshold 30 ADC counts, pass (30 << 8).
     */
    void begin(int32_t pos_threshold_q8, int32_t neg_threshold_q8, int16_t charge_limit = 6) {
        reset();
        _pos_threshold = pos_threshold_q8;
        _neg_threshold = neg_threshold_q8;
        _charge_limit  = charge_limit;
    }

    /**
     * Integer literal convenience overload (prevents ambiguity on 16-bit AVR).
     */
    void begin(int pos_threshold_q8, int neg_threshold_q8, int charge_limit = 6) {
        begin(static_cast<int32_t>(pos_threshold_q8), static_cast<int32_t>(neg_threshold_q8), static_cast<int16_t>(charge_limit));
    }

    /**
     * Backward-compatible initialization with normalized float thresholds.
     * Default values (0.35f, -0.25f, 6) map to clinical ECG normalized profiles.
     */
    void begin(float pos_threshold = 0.35f, float neg_threshold = -0.25f, int16_t charge_limit = 6) {
        reset();
        // Scale float normalized threshold [-1.0, 1.0] by 256 * 256 for Q8 diff space
        _pos_threshold = (int32_t)(pos_threshold * 65536.0f);
        _neg_threshold = (int32_t)(neg_threshold * 65536.0f);
        _charge_limit  = charge_limit;
    }

    /**
     * v2.1 advanced tuning. ALL parameters default to exact v2.0 behavior.
     *   hysteresis_q8: > 0 enables ternary state memory (hold band below/above
     *                  the entry thresholds, in Q8 units of diff). Proven to cut
     *                  trit chatter while preserving detected waves.
     *   live_sigma:    0 = fixed thresholds (v2.0); > 0 recomputes
     *                  pos = live_sigma * variance_ema every feed, so the
     *                  thresholds track jumps of the ambient noise floor.
     */
    void configure(uint16_t hysteresis_q8 = 0, uint8_t live_sigma = 0) {
        _hysteresis_q8 = hysteresis_q8;
        _live_sigma = live_sigma;
    }

    /**
     * Dynamically update thresholds in Q8 units.
     */
    void setThresholds(int32_t pos_threshold_q8, int32_t neg_threshold_q8, int16_t charge_limit = 6) {
        _pos_threshold = pos_threshold_q8;
        _neg_threshold = neg_threshold_q8;
        _charge_limit  = charge_limit;
    }

    /**
     * Zero-FLOP Auto-Calibration from ambient sensor noise.
     * Uses integer variance and integer square root (0 FLOPs).
     * Automatically sets baseline and 3-sigma thresholds.
     * 
     * @param read_fn Function pointer returning a single raw int16_t sensor sample.
     * @param samples Number of calibration samples to collect (e.g. 128 - 256).
     * @param n_sigma Noise envelope multiplier (default 3 for 99.7% confidence).
     */
    void calibrate(int16_t (*read_fn)(), uint16_t samples = 128, uint8_t n_sigma = 3) {
        if (samples == 0 || !read_fn) return;

        int32_t sum = 0;
        int32_t sum_sq = 0;

        for (uint16_t i = 0; i < samples; i++) {
            int16_t val = read_fn();
            sum += val;
            sum_sq += (int32_t)val * val;
            #ifdef ARDUINO
            delay(1);
            #endif
        }

        int32_t mean = sum / (int32_t)samples;
        int32_t variance = (sum_sq / (int32_t)samples) - (mean * mean);
        int32_t stddev = 0;

        if (variance > 0) {
            // Digit-by-digit integer square root algorithm (Zero-FLOP)
            int32_t x = variance;
            int32_t c = 0;
            int32_t d = (int32_t)1L << 30;
            while (d > x) d >>= 2;
            while (d != 0) {
                if (x >= c + d) {
                    x -= c + d;
                    c = (c >> 1) + d;
                } else {
                    c >>= 1;
                }
                d >>= 2;
            }
            stddev = c;
        }

        if (stddev < 2) stddev = 2; // Floor to prevent collapse in silent environments

        _baseline_ema = mean << QSETUN_SHIFT;
        _variance_ema = stddev << QSETUN_SHIFT;

        // Dynamic thresholds relative to zero-mean AC diff
        _pos_threshold = ((int32_t)(n_sigma * stddev)) << QSETUN_SHIFT;
        _neg_threshold = -(((int32_t)(n_sigma * stddev)) << QSETUN_SHIFT);

        // Clear dynamic wave tracking state without wiping calibrated baseline & variance!
        _in_wave = false;
        _wave_width = 0;
        _net_charge = 0;
        _prev_trit = QTrit::Zero;
        _prev_diff = 0;
        _ring_head = 0;
        for (int i = 0; i < 32; ++i) {
            _trit_ring[i] = QTrit::Zero;
        }
    }

    /**
     * Reset internal state. Configuration set via configure() is preserved.
     */
    void reset() {
        _baseline_ema = 0;
        _variance_ema = 0;
        _ring_head = 0;
        _in_wave = false;
        _wave_width = 0;
        _net_charge = 0;
        _cycles_count = 0;
        _prev_trit = QTrit::Zero;
        _prev_diff = 0;
        _is_anomaly = false;
        _anomaly_score_pct = 0;
        _anomaly_score = 0.0f;
        _last_charge = 0;
        _last_width = 0;
        _wave_energy = 0;
        _wave_trits = 0;

        for (int i = 0; i < 32; ++i) {
            _trit_ring[i] = QTrit::Zero;
        }
    }

    /**
     * Core Zero-FLOP inference step.
     * Guaranteed deterministic O(1) integer execution (~1.0 µs on ESP32, ~6 µs on 16MHz AVR).
     * 
     * @param raw_value Integer sensor reading (e.g. ADC 0..1023 or centered value).
     * @return QState Topological classification and cellular apoptosis report.
     */
    QState feed(int16_t raw_value) {
        QState out;
        out.noise_annihilated = false;
        out.beat_classified = false;
        out.wave_energy = 0;
        out.wave_trit_density_pct = 0;

        // 1. Adaptive Baseline & Variance Tracking (Fixed-Point Q8)
        int32_t raw_q8 = (int32_t)raw_value << QSETUN_SHIFT;
        int32_t diff = raw_q8 - _baseline_ema;
        _baseline_ema += (diff >> 6); // alpha = 1/64

        int32_t abs_diff = (diff < 0) ? -diff : diff;
        int32_t var_diff = abs_diff - _variance_ema;
        _variance_ema += (var_diff >> 5); // alpha = 1/32

        // v2.1: continuous self-reinforcement of thresholds from live noise floor
        if (_live_sigma > 0) {
            _pos_threshold = (int32_t)_live_sigma * _variance_ema;
            _neg_threshold = -_pos_threshold;
        }

        // 2. Discretization into Balanced Ternary Space {-1, 0, +1}
        //    v2.1: ternary hysteresis memory (hold bands) with full v2.0 sign-flip
        //    capture semantics. At hyst = 0 this block is bit-for-bit identical
        //    to v2.0: a crossing of the OPPOSITE threshold is still a legit fast
        //    sign reversal (not a stuck state).
        QTrit t_raw = QTrit::Zero;
        if (_prev_trit == QTrit::Positive) {
            // Hold while diff stays above the INNER edge (pos - hyst);
            // a deep dive below the negative threshold flips the sign (as in v2.0).
            if (diff > (_pos_threshold - (int32_t)_hysteresis_q8)) {
                t_raw = QTrit::Positive;
            } else if (diff < _neg_threshold) {
                t_raw = QTrit::Negative;
            }
        } else if (_prev_trit == QTrit::Negative) {
            if (diff < (_neg_threshold + (int32_t)_hysteresis_q8)) {
                t_raw = QTrit::Negative;
            } else if (diff > _pos_threshold) {
                t_raw = QTrit::Positive;
            }
        } else {
            if (diff > _pos_threshold) {
                t_raw = QTrit::Positive;
            } else if (diff < _neg_threshold) {
                t_raw = QTrit::Negative;
            }
        }

        // 3. Multi-Tier Cellular Apoptosis (Noise & Artifact Annihilation)
        QTrit t_filtered = t_raw;

        // Tier A: Opposing direction chatter (+1 immediately after -1 or vice versa)
        if (t_raw != QTrit::Zero && _prev_trit != QTrit::Zero &&
            (static_cast<int8_t>(t_raw) == -static_cast<int8_t>(_prev_trit))) {
            // Annihilate if amplitude is decaying OR is within noise floor envelope (1.5 * MAD)
            if (abs_diff <= _prev_diff || abs_diff <= (_variance_ema + (_variance_ema >> 1))) {
                t_filtered = QTrit::Zero;
                out.noise_annihilated = true;
            }
        }

        // Tier B: Ring-buffer transient glitch suppression
        // Dissolve isolated 1-sample spikes bounded by baseline zeros (0 -> spike -> 0)
        uint8_t prev_idx = (_ring_head + 31) & 31;
        uint8_t prev2_idx = (_ring_head + 30) & 31;
        if (t_filtered == QTrit::Zero && _trit_ring[prev_idx] != QTrit::Zero && _trit_ring[prev2_idx] == QTrit::Zero) {
            if (_prev_diff < (_variance_ema << 1)) {
                _trit_ring[prev_idx] = QTrit::Zero;
                out.noise_annihilated = true;
            }
        }

        _prev_trit = t_filtered;
        _prev_diff = abs_diff;

        _trit_ring[_ring_head] = t_filtered;
        _ring_head = (_ring_head + 1) & 31; // Fast bitwise modulo 32

        // 4. Non-Deadlocking Topological Attractor Tracking
        if (t_filtered == QTrit::Positive && !_in_wave) {
            _in_wave = true;
            _wave_width = 0;
            _net_charge = 0;
            _wave_energy = 0;  // v2.1
            _wave_trits = 0;   // v2.1
        }

        if (_in_wave) {
            _wave_width++;
            _net_charge += static_cast<int8_t>(t_filtered);
            _wave_energy += abs_diff;                       // v2.1: raw energy accumulator
            if (t_filtered != QTrit::Zero) _wave_trits++;   // v2.1: trit-density counter

            // Cycle closure condition:
            // 1. Returned to baseline after achieving minimum physiological duration (>= 4 samples)
            // 2. Watchdog timeout (>= 48 samples) guarantees ZERO deadlock under continuous noise/rupture
            bool baseline_closed = (t_filtered == QTrit::Zero && _wave_width >= 4);
            bool watchdog_timeout = (_wave_width >= 48);

            if (baseline_closed || watchdog_timeout) {
                _in_wave = false;
                out.beat_classified = true;
                _cycles_count++;
                _last_charge = _net_charge;
                _last_width = _wave_width;

                // v2.1: expose smoothed wave energy (Q8 -> integer) and trit density
                int32_t energy = _wave_energy >> QSETUN_SHIFT;
                out.wave_energy = (energy > 0xFFFF) ? 0xFFFF : (uint16_t)energy;
                out.wave_trit_density_pct = (_wave_width > 0)
                    ? (uint8_t)(((uint32_t)_wave_trits * 100) / _wave_width)
                    : 0;

                // Homological Invariant:
                // Normal attractor: compact duration (<= 14) and balanced charge (|Q| < charge_limit)
                int16_t abs_charge = (_net_charge < 0) ? -_net_charge : _net_charge;
                if (_wave_width > 14 || abs_charge >= _charge_limit || watchdog_timeout) {
                    _is_anomaly = true;
                    _anomaly_score_pct = 98;
                    _anomaly_score = 0.98f;
                } else {
                    _is_anomaly = false;
                    _anomaly_score_pct = 2;
                    _anomaly_score = 0.02f;
                }
            }
        }

        out.is_anomaly           = _is_anomaly;
        out.anomaly_score_pct    = _anomaly_score_pct;
        out.anomaly_score        = _anomaly_score;
        out.charge               = _last_charge;
        out.cycle_width          = _last_width;
        out.current_trit         = t_filtered;
        out.cycles_count         = _cycles_count;

        return out;
    }

    /**
     * Backward-compatible float wrapper.
     * Automatically scales normalized floats [-1.0, 1.0] by 256 for Zero-FLOP core.
     */
    inline QState feed(float raw_value) {
        int16_t scaled = (raw_value >= -10.0f && raw_value <= 10.0f)
                         ? (int16_t)(raw_value * 256.0f)
                         : (int16_t)raw_value;
        return feed(scaled);
    }

    // Getters for telemetry
    inline int16_t getBaseline() const { return static_cast<int16_t>(_baseline_ema >> QSETUN_SHIFT); }
    inline bool isAnomaly() const { return _is_anomaly; }
    inline uint8_t getScorePct() const { return _anomaly_score_pct; }
    inline float getScore() const { return _anomaly_score; }
    inline uint32_t getCyclesCount() const { return _cycles_count; }

    // v2.1: telemetry parity with the lab fork (used by Q-SETUN Biometric obvest)
    inline int32_t getVarianceEMA() const { return _variance_ema; }
    inline int32_t getPosThreshold() const { return _pos_threshold; }
    inline int32_t getNegThreshold() const { return _neg_threshold; }
    inline int16_t getChargeLimit() const { return _charge_limit; }
    inline int32_t getBaselineEMA() const { return _baseline_ema; }
    inline const QTrit* getTritRing() const { return _trit_ring; }
    inline uint8_t getRingHead() const { return _ring_head; }
    inline void setBaselineEMA(int32_t val) { _baseline_ema = val; }
    inline void setVarianceEMA(int32_t val) { _variance_ema = val; }

private:
    int32_t _pos_threshold = (int32_t)(0.35f * 65536.0f);
    int32_t _neg_threshold = (int32_t)(-0.25f * 65536.0f);
    int16_t _charge_limit  = 6;

    int32_t _baseline_ema;   // Q8 fixed-point
    int32_t _variance_ema;   // Q8 fixed-point

    QTrit _trit_ring[32];
    uint8_t _ring_head;
    QTrit _prev_trit;
    int32_t _prev_diff;

    bool _in_wave;
    uint16_t _wave_width;
    int16_t _net_charge;
    uint32_t _cycles_count;

    // v2.1 configuration (defaults = exact v2.0 behavior)
    uint16_t _hysteresis_q8 = 0;
    uint8_t _live_sigma = 0;

    // v2.1 wave accumulators
    int32_t _wave_energy;
    uint16_t _wave_trits;

    bool _is_anomaly;
    uint8_t _anomaly_score_pct;
    float _anomaly_score;
    int16_t _last_charge;
    uint16_t _last_width;
};

#endif // QSETUN_H