#pragma once

#include <stdint.h>

// Example pin mapping for ESP32-S3
#define PIN_LED     2
#define PIN_BUTTON  0
// I2C
#define SDA 11
#define SCL 12
// SPI
#define SCK 7
#define MISO 5
#define MOSI 4
#define SS 6

static const uint8_t LED_BUILTIN = PIN_LED;
