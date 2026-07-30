// max31855.h — MAX31855K thermocouple reader on its own SPI bus (SPI3).
// Fault bits (open circuit, short to GND/VCC) surface as read failure —
// never as a bogus temperature.

#pragma once

#include <stdint.h>

namespace max31855 {

enum class Fault : uint8_t {
    NONE = 0,
    OPEN_CIRCUIT,
    SHORT_GND,
    SHORT_VCC,
    SPI_ERROR,
};

void init();

// Reads the hot-junction temperature in °F. Returns Fault::NONE on success.
Fault read_degf(float* deg_f);

} // namespace max31855
