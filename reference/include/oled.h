
#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"
#include "font_latin_8x8.h"
#include "esp_log.h"

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SCL_IO   12
#define I2C_MASTER_SDA_IO   11
#define I2C_MASTER_FREQ_HZ  400000
#define OLED_ADDR           0x3C

void i2c_master_init();
void oled_init();
void oled_update();
