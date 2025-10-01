
#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SCL_IO   12
#define I2C_MASTER_SDA_IO   11
#define I2C_MASTER_FREQ_HZ  400000

class Oled {
public:
    Oled();
    ~Oled() = default;

    void init();
    void draw(int32_t* i2sBuffer, uint32_t bufferLength);

private:

    Adafruit_SSD1306 display;
    
    void i2cInit(uint8_t address, uint8_t pinSDA, uint8_t pinSCL);

    uint8_t last_y[128] = {0};

};
