/**
 * ============================================================================
 * QSetun Example 03: Cellular Apoptosis & Noise Annihilation Stress Test
 * 
 * Demonstrates:
 *   - Injection of synthetic high-frequency noise spikes (EMG / electrical jitter)
 *   - Proof of cellular apoptosis: opposing trits (+1 followed by -1) collapse to 0
 *   - Zero false-positive alarms under extreme signal degradation
 * ============================================================================
 */

#include <qsetun.h>

QSetun qsetun;

void setup() {
    Serial.begin(115200);
    delay(500);

    qsetun.begin(0.35f, -0.25f, 6);

    Serial.println("==========================================================");
    Serial.println(" Q-SETUN: Cellular Apoptosis Noise Annihilation Test      ");
    Serial.println("==========================================================");
    Serial.println("Step 1: Normal sine wave baseline");
    Serial.println("Step 2: Injecting high-frequency +/- 0.5 noise bursts");
    Serial.println("Watch: Noise is silently annihilated via (+1 + -1 -> 0)!");
    Serial.println("==========================================================");
}

void loop() {
    static float angle = 0.0f;
    static uint32_t step = 0;

    // 1. Synthetic periodic sensor signal (e.g. machine vibration)
    float clean_signal = 0.4f * sin(angle);
    angle += 0.05f;

    // 2. Every 50 samples, inject an aggressive high-frequency noise burst
    float noisy_signal = clean_signal;
    bool noise_injected = false;

    if ((step % 40) > 30) {
        // High-frequency jitter (+0.6 followed by -0.6)
        noisy_signal += ((step % 2 == 0) ? +0.65f : -0.65f);
        noise_injected = true;
    }

    // 3. Step the Q-Setun Engine
    QState state = qsetun.feed(noisy_signal);

    // 4. Output results
    if (state.noise_annihilated) {
        Serial.print("[APOPTOSIS ANNIHILATED NOISE] ");
    }

    if (state.is_anomaly) {
        Serial.print(">>> [REAL ANOMALY!] ");
    } else {
        Serial.print("    [STABLE BASIN]  ");
    }

    Serial.print("InjectedNoise: ");
    Serial.print(noise_injected ? "YES" : "NO ");
    Serial.print(" | Signal: ");
    Serial.print(noisy_signal, 2);
    Serial.print(" | Trit: ");
    Serial.print(static_cast<int8_t>(state.current_trit));
    Serial.print(" | NetCharge: ");
    Serial.println(state.charge);

    step++;
    delay(20);
}
