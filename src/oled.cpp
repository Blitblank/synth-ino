
#include "oled.hpp"

#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Oled::Oled() {
    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

void Oled::init() {

    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.display();

}

void Oled::draw(int32_t* i2sBuffer, uint32_t bufferLength) {

    // 21.5 ms per write, max 46.5 fps at i2c = 700kHz
    // too slow to run on same core as the synth but fast enough to run conncurent with io, wifi, etc
    // i was able to get i2c up to 1.4MHz for 69fps

    // these need to be externally passed through
    volatile uint32_t scopeWavelength = 578263587;
    volatile uint32_t scopeTrigger = 237734124;

    uint32_t stride = (scopeWavelength << 16) / 128;
    uint32_t phase = scopeTrigger << 16;

    uint8_t prev_x = 0;
    uint8_t prev_y = 32;

    for (uint8_t x = 0; x < 128; x++) {

        /*
        for (uint8_t y = 0; y < 64; y++) {
            ssd1306_set_pixel(device_handle, x, y, true); // sclear display
        }
        */
        display.clearDisplay();

        uint32_t val = phase >> 16; // where to index the buffer so that the screen is scaled to one period
        int32_t sample = i2sBuffer[val % 512];

        uint8_t y = 32 + (sample >> (24+2)); // scale [-2^31, 2^31-1] to [0, 63]

        if(y > 63 || y <= 0) y = 0;
        y = 63 - y; // invert vertically 
        last_y[x] = y;

        //ssd1306_set_pixel(device_handle, x, y, false);
        if(x == 0) prev_y = y;
        display.writePixel(x, y, 1);
        prev_x = x;
        prev_y = y;

        phase += stride;

    }
    display.display(); // flush buffer to device over i2c
}

void Oled::i2cInit(uint8_t address, uint8_t pinSDA, uint8_t pinSCL) {

    bool success = true;
    if(!Wire.begin()) success = false;
    if(!Wire.setClock(I2C_MASTER_FREQ_HZ)) success = false;
    if(!Wire.setPins(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO)) success = false;

    if(!success) utils::serialLog("I2C_MASTER", xTaskGetTickCount(), "I2C initialization failed.");

}
