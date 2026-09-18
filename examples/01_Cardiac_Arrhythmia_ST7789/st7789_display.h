#ifndef ST7789_DISPLAY_H
#define ST7789_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>

// ST7789 Pinout for LilyGO T-Display v1.1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   16
#define TFT_RST  23
#define TFT_BL    4

#define TFT_W 240
#define TFT_H 135
#define COL_OFFSET 40
#define ROW_OFFSET 53

#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_DARKBLUE  0x0810
#define COLOR_DARKGREY  0x2104
#define COLOR_DARKGREEN 0x0320

extern SPIClass tft_spi;

static inline void tft_cmd(uint8_t c) {
    digitalWrite(TFT_DC, LOW);
    digitalWrite(TFT_CS, LOW);
    tft_spi.write(c);
    digitalWrite(TFT_CS, HIGH);
}

static inline void tft_data(uint8_t d) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    tft_spi.write(d);
    digitalWrite(TFT_CS, HIGH);
}

static inline void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint16_t xa = x0 + COL_OFFSET;
    uint16_t xb = x1 + COL_OFFSET;
    uint16_t ya = y0 + ROW_OFFSET;
    uint16_t yb = y1 + ROW_OFFSET;

    tft_cmd(0x2A);
    tft_data(xa >> 8); tft_data(xa & 0xFF);
    tft_data(xb >> 8); tft_data(xb & 0xFF);

    tft_cmd(0x2B);
    tft_data(ya >> 8); tft_data(ya & 0xFF);
    tft_data(yb >> 8); tft_data(yb & 0xFF);

    tft_cmd(0x2C);
}

static inline void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= TFT_W || y >= TFT_H || w <= 0 || h <= 0) return;
    if (x + w > TFT_W) w = TFT_W - x;
    if (y + h > TFT_H) h = TFT_H - y;

    tft_set_window(x, y, x + w - 1, y + h - 1);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t total = (uint32_t)w * h;
    for (uint32_t i = 0; i < total; ++i) {
        tft_spi.write(hi);
        tft_spi.write(lo);
    }
    digitalWrite(TFT_CS, HIGH);
}

static inline void tft_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= TFT_W || y < 0 || y >= TFT_H) return;
    tft_set_window(x, y, x, y);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    tft_spi.write(color >> 8);
    tft_spi.write(color & 0xFF);
    digitalWrite(TFT_CS, HIGH);
}

static inline void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    while (true) {
        tft_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static const uint8_t FONT5x7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // \
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
};

static inline void tft_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale) {
    if (x >= TFT_W || y >= TFT_H) return;
    if (c < 32 || c > 95) c = ' ';
    uint8_t idx = c - 32;

    int16_t w = 6 * scale;
    int16_t h = 7 * scale;
    tft_set_window(x, y, x + w - 1, y + h - 1);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    uint8_t hi_c = color >> 8, lo_c = color & 0xFF;
    uint8_t hi_b = bg >> 8,    lo_b = bg & 0xFF;

    for (int8_t j = 0; j < 7; ++j) {
        for (uint8_t sy = 0; sy < scale; ++sy) {
            for (int8_t i = 0; i < 5; ++i) {
                uint8_t line = pgm_read_byte(&FONT5x7[idx][i]);
                bool on = (line & (1 << j)) != 0;
                uint8_t hi = on ? hi_c : hi_b;
                uint8_t lo = on ? lo_c : lo_b;
                for (uint8_t sx = 0; sx < scale; ++sx) {
                    tft_spi.write(hi);
                    tft_spi.write(lo);
                }
            }
            // 6th column spacing
            for (uint8_t sx = 0; sx < scale; ++sx) {
                tft_spi.write(hi_b);
                tft_spi.write(lo_b);
            }
        }
    }
    digitalWrite(TFT_CS, HIGH);
}

static inline void tft_draw_string(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t scale) {
    while (*str) {
        tft_draw_char(x, y, *str, color, bg, scale);
        x += 6 * scale;
        str++;
    }
}

static inline void tft_init() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    digitalWrite(TFT_CS, HIGH);
    digitalWrite(TFT_DC, HIGH);

    digitalWrite(TFT_RST, HIGH); delay(10);
    digitalWrite(TFT_RST, LOW);  delay(20);
    digitalWrite(TFT_RST, HIGH); delay(50);

    tft_spi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft_spi.setFrequency(40000000);

    tft_cmd(0x01); delay(150);
    tft_cmd(0x11); delay(120);
    tft_cmd(0x3A); tft_data(0x05);
    tft_cmd(0x36); tft_data(0x70); // Landscape 240x135
    tft_cmd(0x21); // INVON
    tft_cmd(0x13);
    tft_cmd(0x29); delay(50);

    tft_fill_rect(0, 0, TFT_W, TFT_H, COLOR_BLACK);
}

#endif // ST7789_DISPLAY_H
