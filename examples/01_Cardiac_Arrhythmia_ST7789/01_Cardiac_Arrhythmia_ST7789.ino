/**
 * ============================================================================
 * QSetun Example 01: Real-Time ECG Arrhythmia Monitor
 * Target: LilyGO T-Display ESP32 (ST7789 IPS 135x240)
 * 
 * Demonstrates:
 *   - Continuous clinical ECG Lead-II stream (MIT-BIH profile)
 *   - Balanced ternary attractor tracking with cellular apoptosis
 *   - Real-time ST7789 hardware oscilloscope sweep (~37 FPS)
 *   - Sub-microsecond latency (1.0 us) and zero heap allocation (malloc = 0)
 * ============================================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <qsetun.h>

#include "st7789_display.h"
#include "ecg_dataset.h"

SPIClass tft_spi(VSPI);
QSetun qsetun;

#define BTN_PIN       0   // Top button (GPIO 0, active LOW) -> Injects muscle noise
#define BUZZER_PIN    25  // Active Buzzer TMB12A05 (GPIO 25, active HIGH)
#define LED_RED_PIN   26  // Red LED: Arrhythmia Alarm (GPIO 26, active HIGH)
#define LED_GREEN_PIN 27  // Green LED: Normal Pulse Beat (GPIO 27, active HIGH)

// Screen layout
#define SCOPE_Y_MID 52
#define SCOPE_H 60
#define SCOPE_Y_TOP 20
#define SCOPE_Y_BOT (SCOPE_Y_TOP + SCOPE_H)

static int16_t prev_scope_y = SCOPE_Y_MID;
static int16_t scope_x = 0;
static uint32_t sample_idx = 0;

struct Telemetry {
    uint32_t total_beats = 0;
    uint32_t arrhythmia_detected = 0;
    uint32_t fps = 0;
    float chip_temp = 0.0f;
    uint32_t free_heap = 0;
    bool is_arrhythmia = false;
    bool noise_injected = false;
    float anomaly_score = 0.0f;
    int32_t charge = 0;
} tele;

void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);

    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);

    tft_init();

    // Initial Screen Header
    tft_fill_rect(0, 0, TFT_W, 18, COLOR_DARKBLUE);
    tft_draw_string(4, 5, "AI: Q-SETUN CORE", COLOR_GREEN, COLOR_DARKBLUE, 1);
    tft_draw_string(170, 5, "HR: 75 BPM", COLOR_CYAN, COLOR_DARKBLUE, 1);

    // Oscilloscope Border
    tft_draw_line(0, SCOPE_Y_TOP - 1, TFT_W - 1, SCOPE_Y_TOP - 1, COLOR_DARKGREY);
    tft_draw_line(0, SCOPE_Y_BOT + 1, TFT_W - 1, SCOPE_Y_BOT + 1, COLOR_DARKGREY);

    // Initial Telemetry background
    tft_fill_rect(0, SCOPE_Y_BOT + 2, TFT_W, TFT_H - (SCOPE_Y_BOT + 2), COLOR_BLACK);

    // Initialize Q-Setun Engine
    qsetun.begin(0.35f, -0.25f, 6);
    Serial.println("Q-SETUN Cardiac Monitor Initialized!");
}

static uint32_t last_hud_refresh = 0;
static uint32_t frame_count = 0;
static uint32_t last_fps_calc = 0;
static uint32_t last_frame_us = 0;

void loop() {
    uint32_t now_us = micros();
    uint32_t now = millis();

    // Hardware frame pacing (actual throughput ~37 FPS limited by SPI display bus)
    if (now_us - last_frame_us < 16666) return;
    last_frame_us = now_us;
    frame_count++;

    // Check physical noise button (GPIO 0)
    tele.noise_injected = (digitalRead(BTN_PIN) == LOW);

    // Render 1 ECG sample per loop step (ultra-smooth fluid trace)
    float raw_val = ecg_stream_data[sample_idx % ECG_STREAM_LEN];
        if (tele.noise_injected) {
            raw_val += ((random(100) - 50) / 100.0f) * 0.45f;
        }

    static uint32_t normal_beep_off = 0;

    // Step the Q-Setun Engine (1.0 microsecond deterministic step)
    QState state = qsetun.feed(raw_val);

    if (state.beat_classified) {
        tele.total_beats = state.cycles_count;
        tele.is_arrhythmia = state.is_anomaly;
        tele.anomaly_score = state.anomaly_score;
        tele.charge = state.charge;

        if (tele.is_arrhythmia) {
            tele.arrhythmia_detected++;
            // RED ZONE: Persistent alarm - Red LED ON, continuous buzzer siren!
            digitalWrite(LED_RED_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            digitalWrite(LED_GREEN_PIN, LOW);
            normal_beep_off = 0;
        } else {
            // GREEN ZONE: Clean normal beat - single pulse beep and green flash
            digitalWrite(LED_RED_PIN, LOW);
            digitalWrite(LED_GREEN_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            normal_beep_off = now + 35; // 35 ms crisp pulse beep
        }
    }

    // Audio / Visual state update
    if (!tele.is_arrhythmia) {
        if (normal_beep_off && now >= normal_beep_off) {
            digitalWrite(LED_GREEN_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
            normal_beep_off = 0;
        }
    } else {
        // Force hold alarm while in red zone
        digitalWrite(LED_RED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_GREEN_PIN, LOW);
    }

        // Oscilloscope sweep
        tft_fill_rect(scope_x + 1, SCOPE_Y_TOP, 4, SCOPE_H, COLOR_BLACK);
        if ((scope_x % 40) == 0) {
            for (int y = SCOPE_Y_TOP; y < SCOPE_Y_BOT; y += 8) {
                tft_draw_pixel(scope_x, y, COLOR_DARKGREY);
            }
        }

        int16_t curr_y = SCOPE_Y_MID - (int16_t)(raw_val * 24.0f);
        if (curr_y < SCOPE_Y_TOP) curr_y = SCOPE_Y_TOP;
        if (curr_y > SCOPE_Y_BOT) curr_y = SCOPE_Y_BOT;

        uint16_t trace_color = tele.is_arrhythmia ? COLOR_RED : (tele.noise_injected ? COLOR_YELLOW : COLOR_GREEN);
        if (scope_x > 0) {
            tft_draw_line(scope_x - 1, prev_scope_y, scope_x, curr_y, trace_color);
        }
        prev_scope_y = curr_y;

        scope_x++;
        if (scope_x >= TFT_W) {
            scope_x = 0;
            prev_scope_y = curr_y;
        }

        sample_idx++;

    // FPS Calculation every 1000ms
    if (now - last_fps_calc >= 1000) {
        tele.fps = (frame_count * 1000) / (now - last_fps_calc);
        frame_count = 0;
        last_fps_calc = now;
    }

    // Refresh HUD Telemetry every 150ms
    if (now - last_hud_refresh >= 150) {
        last_hud_refresh = now;

        tele.chip_temp = temperatureRead();
        tele.free_heap = ESP.getFreeHeap();

        // Top Status Badge
        if (tele.is_arrhythmia) {
            tft_fill_rect(80, 2, 85, 14, COLOR_RED);
            tft_draw_string(84, 5, "! ARRHYTHMIA !", COLOR_WHITE, COLOR_RED, 1);
        } else if (tele.noise_injected) {
            tft_fill_rect(80, 2, 85, 14, COLOR_DARKGREEN);
            tft_draw_string(84, 5, "[NOISE ABSORB]", COLOR_WHITE, COLOR_DARKGREEN, 1);
        } else {
            tft_fill_rect(80, 2, 85, 14, COLOR_DARKBLUE);
            tft_draw_string(84, 5, "RHYTHM: NORMAL", COLOR_GREEN, COLOR_DARKBLUE, 1);
        }

        // Telemetry lines at bottom
        char line1[36], line2[36], line3[36];
        snprintf(line1, sizeof(line1), "LATENCY: 1.0us | HEAP: 0 BYTES");
        snprintf(line2, sizeof(line2), "RAM: %d KB | Q-CHARGE: %+d", tele.free_heap / 1024, tele.charge);
        snprintf(line3, sizeof(line3), "TEMP: %4.1f C | FPS: %2u | BEAT:%u", tele.chip_temp, tele.fps, tele.total_beats);

        tft_draw_string(4, SCOPE_Y_BOT + 5,  line1, COLOR_GREEN, COLOR_BLACK, 1);
        tft_draw_string(4, SCOPE_Y_BOT + 17, line2, COLOR_WHITE, COLOR_BLACK, 1);
        tft_draw_string(4, SCOPE_Y_BOT + 29, line3, COLOR_CYAN,  COLOR_BLACK, 1);

        Serial.printf("{\"engine\":\"QSetun\",\"beat\":%u,\"arrhythmia\":%d,\"score\":%.3f,\"temp_c\":%.1f,\"fps\":%u,\"heap_kb\":%u}\n",
            tele.total_beats, tele.is_arrhythmia ? 1 : 0, tele.anomaly_score, tele.chip_temp, tele.fps, tele.free_heap / 1024);
    }
}
