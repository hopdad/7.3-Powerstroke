#include "max31855.h"

#include <Arduino.h>
#include <SPI.h>
#include "pins.h"

namespace max31855 {

static SPIClass spi(HSPI);   // SPI3 host on S3 — display/SD keep SPI2

void init() {
    pinMode(PIN_EGT_CS, OUTPUT);
    digitalWrite(PIN_EGT_CS, HIGH);
    spi.begin(PIN_EGT_SCLK, PIN_EGT_MISO, -1 /* no MOSI, read-only */, PIN_EGT_CS);
}

Fault read_degf(float* deg_f) {
    spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_EGT_CS, LOW);
    uint32_t raw = ((uint32_t)spi.transfer16(0) << 16) | spi.transfer16(0);
    digitalWrite(PIN_EGT_CS, HIGH);
    spi.endTransaction();

    if (raw == 0 || raw == 0xFFFFFFFF) return Fault::SPI_ERROR;

    if (raw & 0x00010000) {   // fault flag
        if (raw & 0x1) return Fault::OPEN_CIRCUIT;
        if (raw & 0x2) return Fault::SHORT_GND;
        if (raw & 0x4) return Fault::SHORT_VCC;
        return Fault::SPI_ERROR;
    }

    // Bits 31:18 = signed 14-bit thermocouple temp, 0.25 °C/LSB
    int16_t tc = (int16_t)(raw >> 16) >> 2;
    float deg_c = tc * 0.25f;
    *deg_f = deg_c * 9.0f / 5.0f + 32.0f;
    return Fault::NONE;
}

} // namespace max31855
