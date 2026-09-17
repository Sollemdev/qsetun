/**
 * ============================================================================
 * QSetun Example 02: Universal Sensor Anomaly Detector
 * 
 * Works on ANY board: Arduino Uno, Nano, Mega, STM32, ESP32, ESP8266, RP2040.
 * Memory footprint: 192 bytes RAM, 0 bytes heap (malloc = 0).
 * Speed: ~1.0 us per sample.
 * ============================================================================
 */

#include <qsetun.h>

QSetun qsetun;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Connect any sensor (piezo, vibration, analog microphone, current CT, photoresistor)
#define SENSOR_PIN A0
#define ALERT_LED  LED_BUILTIN

void setup() {
    Serial.begin(115200);
    pinMode(ALERT_LED, OUTPUT);

    // Initialize Q-Setun engine
    // Parameters: pos_threshold (0.35), neg_threshold (-0.25), charge_limit (6)
    qsetun.begin(0.35f, -0.25f, 6);

    Serial.println("============================================");
    Serial.println(" Q-SETUN: Universal Neuromorphic Anomaly Core ");
    Serial.println("============================================");
}

void loop() {
    // 1. Read analog value and normalize around zero (-1.0 to +1.0)
    int raw = analogRead(SENSOR_PIN);
    float norm_val = (raw - 512) / 512.0f;

    // 2. Feed sample to Q-Setun (O(1) deterministic step, 1 microsecond)
    QState state = qsetun.feed(norm_val);

    // 3. Check for topological attractor rupture
    if (state.is_anomaly) {
        digitalWrite(ALERT_LED, HIGH); // Light alarm LED
        Serial.print(">>> [ANOMALY DETECTED!] Score: ");
        Serial.print(state.anomaly_score, 3);
        Serial.print(" | Trit Charge: ");
        Serial.print(state.charge);
        Serial.print(" | Width: ");
        Serial.println(state.cycle_width);
    } else {
        digitalWrite(ALERT_LED, LOW);
    }

    if (state.noise_annihilated) {
        // Cellular apoptosis silently eliminated an opposing high-frequency noise spike!
        // No false positive triggered!
    }

    delay(10); // Pace to 100 Hz sampling rate
}
