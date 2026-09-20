/**
 * ============================================================================
 * QSetun Example 04: Auto-Calibration + Serial Plotter
 * 
 * ZERO WIRING REQUIRED. Runs on ANY board (Uno, Nano, ESP32, STM32, RP2040).
 * Generates a synthetic clean signal, auto-calibrates from ambient noise,
 * then periodically injects noise bursts and anomaly spikes.
 * 
 * Open Arduino Serial Plotter (Ctrl+Shift+L) at 115200 baud to see:
 *   Line 1: Raw signal value
 *   Line 2: Anomaly score (0 = normal, 98 = anomaly)
 *   Line 3: Trit state × 50 (for visibility: -50, 0, +50)
 * 
 * Memory: 84 bytes RAM core + ~200 bytes stack. malloc = 0.
 * ============================================================================
 */

#include <qsetun.h>

QSetun qsetun;

// ─── Synthetic Signal Generator ─────────────────────────────────────────────

static float g_angle = 0.0f;
static uint32_t g_step = 0;

// Callback for calibrate(): returns a quiet baseline sample
int16_t readQuietBaseline() {
    // Simulate a quiet sensor at ~512 with ±3 counts of noise
    return 512 + (int16_t)(random(-3, 4));
}

// Generate synthetic signal with periodic anomalies and noise bursts
int16_t generateSignal() {
    g_step++;

    // Base: gentle sine wave centered at 512 (simulates clean sensor)
    float clean = 512.0f + 40.0f * sin(g_angle);
    g_angle += 0.08f;
    if (g_angle > 6.2832f) g_angle -= 6.2832f;

    int16_t signal = (int16_t)clean;

    // Every 200 steps: inject a sharp anomaly spike (simulates real event)
    if (g_step % 200 >= 195 && g_step % 200 <= 198) {
        signal += 120; // Strong positive deflection
    }

    // Every 80 steps: inject high-frequency noise burst (simulates EMI/jitter)
    if (g_step % 80 >= 70 && g_step % 80 <= 75) {
        signal += (g_step % 2 == 0) ? +60 : -60; // Opposing +/- chatter
    }

    return signal;
}

// ─── Setup ──────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);

    #ifdef ARDUINO
    randomSeed(analogRead(A0)); // Seed RNG from floating pin
    #endif

    // ★ AUTO-CALIBRATION ★
    // Reads 128 quiet samples, computes integer mean + stddev,
    // sets thresholds to ±3σ of ambient noise. Zero floating-point math.
    qsetun.calibrate(readQuietBaseline, 128, 3);

    // Print header for Serial Monitor (not needed for Plotter)
    Serial.println("Signal,AnomalyScore,Trit");
}

// ─── Main Loop ──────────────────────────────────────────────────────────────

void loop() {
    // 1. Generate synthetic sensor reading
    int16_t raw = generateSignal();

    // 2. Feed to Q-SETUN core (O(1), ~1 μs, 0 FLOPs)
    QState state = qsetun.feed(raw);

    // 3. Output for Serial Plotter (comma-separated, one line per sample)
    //    Line labels are taken from the header printed in setup()
    Serial.print(raw);
    Serial.print(",");
    Serial.print(state.anomaly_score_pct);
    Serial.print(",");
    Serial.println(static_cast<int8_t>(state.current_trit) * 50);

    // 4. Optionally: print events to Serial Monitor
    //    (Comment out if using Serial Plotter only — it clutters the plot)
    /*
    if (state.beat_classified) {
        Serial.print("  >> Cycle completed: charge=");
        Serial.print(state.charge);
        Serial.print(" width=");
        Serial.print(state.cycle_width);
        Serial.println(state.is_anomaly ? " [ANOMALY!]" : " [NORMAL]");
    }
    if (state.noise_annihilated) {
        Serial.println("  >> [APOPTOSIS] Noise spike annihilated");
    }
    */

    delay(10); // 100 Hz sampling rate
}
