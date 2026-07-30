#include "ads1115.h"

#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "sensors_config.h"

namespace ads1115 {

static const uint8_t REG_CONVERSION = 0x00;
static const uint8_t REG_CONFIG     = 0x01;

// PGA ±4.096 V → 125 µV/LSB
static const float LSB_VOLTS = 4.096f / 32768.0f;

static bool write_reg(uint8_t addr, uint8_t reg, uint16_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write((uint8_t)(val >> 8));
    Wire.write((uint8_t)(val & 0xFF));
    return Wire.endTransmission() == 0;
}

static bool read_reg(uint8_t addr, uint8_t reg, uint16_t* val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)2) != 2) return false;
    *val = ((uint16_t)Wire.read() << 8) | Wire.read();
    return true;
}

uint8_t init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
    uint8_t found = 0;
    for (int u = 0; u < 2; u++) {
        uint16_t cfg;
        if (read_reg(ADS1115_ADDR[u], REG_CONFIG, &cfg)) {
            found |= (1 << u);
        }
    }
    return found;
}

bool read_single(uint8_t unit, uint8_t mux, float* volts) {
    uint8_t addr = ADS1115_ADDR[unit];

    // OS=1 start | MUX=100+ch (single-ended) | PGA=001 (±4.096V) |
    // MODE=1 single-shot | DR=110 (250 SPS) | comparator disabled
    uint16_t cfg = 0x8000
                 | ((uint16_t)(0x4 | (mux & 0x3)) << 12)
                 | (0x1 << 9)
                 | (1 << 8)
                 | (0x6 << 5)
                 | 0x0003;

    if (!write_reg(addr, REG_CONFIG, cfg)) return false;

    // 250 SPS → 4 ms conversion. Poll the OS bit with a hard timeout.
    uint32_t start = millis();
    for (;;) {
        uint16_t st;
        if (!read_reg(addr, REG_CONFIG, &st)) return false;
        if (st & 0x8000) break;
        if (millis() - start > 20) return false;
        delayMicroseconds(300);
    }

    uint16_t raw;
    if (!read_reg(addr, REG_CONVERSION, &raw)) return false;
    int16_t signed_raw = (int16_t)raw;
    if (signed_raw < 0) signed_raw = 0;   // single-ended: negative is noise
    *volts = signed_raw * LSB_VOLTS;
    return true;
}

} // namespace ads1115
