// pins.h — ALL pin assignments live here. Nothing else defines a GPIO.
//
// Bench assignments for a generic ESP32-S3 DevKitC-1. Revisit against the
// actual board/panel before wiring anything permanent. Avoids strapping pins
// (0, 3, 45, 46) and the flash/PSRAM range (26-37 on octal modules).

#pragma once

// ---- I2C bus (ADS1115 ADCs) ------------------------------------------------
#define PIN_I2C_SDA        8
#define PIN_I2C_SCL        9
// ADS1115 ALERT/RDY (optional, unused in phase 1)
#define PIN_ADS_ALERT      17

// ---- SPI2 (FSPI): display + SD card, shared bus, separate CS ---------------
#define PIN_SPI2_SCLK      12
#define PIN_SPI2_MOSI      11
#define PIN_SPI2_MISO      13   // display doesn't drive MISO; SD does

#define PIN_TFT_CS         10
#define PIN_TFT_DC         14
#define PIN_TFT_RST        21
#define PIN_TFT_BL         47   // backlight, active high

#define PIN_SD_CS          15   // phase 4

// ---- SPI3: MAX31855 thermocouple (own bus — 5 MHz max, read-only) ----------
#define PIN_EGT_SCLK       40
#define PIN_EGT_MISO       41
#define PIN_EGT_CS         42

// ---- IPR duty capture -------------------------------------------------------
// 12V PWM from the PCM tap. MUST go through a level shifter / divider+clamp
// before this GPIO. Firmware treats "no edges" as SENSOR_STALE, not 0% duty.
#define PIN_IPR_IN         16
