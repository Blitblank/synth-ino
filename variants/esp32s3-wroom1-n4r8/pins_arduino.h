#pragma once

#include <stdint.h>

#define PIN_LED 2

// I2C
#define SDA 11
#define SCL 12

// SPI
#define SCK 7
#define MISO 5
#define MOSI 4
#define SS 6

// I2S
#define BCK 40
#define WS 38
#define DO 39

// IO EXPANDER
#define STATUS_LED_1 1
#define STATUS_LED_2 0
#define STATUS_LED_3 8
#define STATUS_LED_4 9

static const uint8_t LED_BUILTIN = PIN_LED;
