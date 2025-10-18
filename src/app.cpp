
#include "App.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Arduino.h"
#include "tasks.h"
#include <driver/i2s.h>
#include "soc/i2s_struct.h"

App::App() {

}

void App::init() {

    Serial.begin(115200);
    utils::serialLog(appTaskHandle, xTaskGetTickCount(), "App init.");

    Wire.begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    if (!mcp.begin_I2C()) {
        Serial.println("mcp i2c init failure"); // TODO: standardize output logging
    } else {
        mcp.pinMode(STATUS_LED_1, OUTPUT);
        mcp.pinMode(STATUS_LED_2, OUTPUT);
        mcp.pinMode(STATUS_LED_3, OUTPUT);
        mcp.pinMode(STATUS_LED_4, OUTPUT);
    }
    mcp.digitalWrite(STATUS_LED_1, LOW);
    mcp.digitalWrite(STATUS_LED_2, LOW);
    mcp.digitalWrite(STATUS_LED_3, LOW);
    mcp.digitalWrite(STATUS_LED_4, LOW);
    // might make a class for the mcp too 

    i2sInit();
    disk.init(&mcp);
}

void App::main() {

    xTaskCreatePinnedToCore(&wifiTaskTrampoline, "WIFI_TASK", 4096, this, 7, NULL, 0);
    xTaskCreatePinnedToCore(&audioTaskTrampoline, "AUDIO_TASK", 4096, this, 9, NULL, 1);
    xTaskCreatePinnedToCore(&ioTaskTrampoline, "IO_TASK", 4096, this, 5, NULL, 0);

}

void App::wifiTask() {

    utils::serialLog(wifiTaskHandle, xTaskGetTickCount(), "Wifi task start.");

    wifiManager.init(&disk, &mcp);
    
    vTaskDelete(NULL);

}

void App::audioTask() {

    utils::serialLog(synthTaskHandle, xTaskGetTickCount(), "Audio task start.");

    ControlState controls = {{0.0f, 0.0f, 0.0f, 0.5f, 0.5f}, {1, 2, 3, 4}};
    synth.init();

    while(1) {

        vTaskDelay(1);

        wifiManager.getControlState(&controls);

        spinlock1 = 1;
        synth.generate(i2sBuffer, i2sBufferLength, &scopeWavelength, &scopeTrigger, &controls);
        spinlock1 = 0;

        size_t bytesWritten;
        i2s_write(i2sPort, i2sBuffer, sizeof(i2sBuffer), &bytesWritten, portMAX_DELAY); // esp-idf function

        // TODO: performance profiling of the synth generation
        // first thing to see is compiler optimization for performance (-O2)
    }

}

void App::ioTask() {

    utils::serialLog(ioTaskHandle, xTaskGetTickCount(), "Io task start.");

    oled.init();

    while(1) {

        
        while(spinlock1 != 0) vTaskDelay(1); // no mutex causes the flickering

        uint32_t start = xTaskGetTickCount(); // time profiling
        oled.draw(i2sBuffer, i2sBufferLength, scopeWavelength, scopeTrigger);
        uint32_t end = xTaskGetTickCount();

        //Serial.printf("time diff of oled.draw: %d \n", end-start);

        vTaskDelay(50); // ms
    }

}

void App::wifiTaskTrampoline(void* args) {
    App* self = static_cast<App*>(args);
    self->wifiTask();
}

void App::audioTaskTrampoline(void* args) {
    App* self = static_cast<App*>(args);
    self->audioTask();
}

void App::ioTaskTrampoline(void* args) {
    App* self = static_cast<App*>(args);
    self->ioTask();
}

void App::i2sInit() {

    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = i2sSampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // mono (left) for now
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = dmaBufferCount,
        .dma_buf_len = i2sBufferLength, 
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pinConfig = {
        .bck_io_num = BCK,
        .ws_io_num  = WS,
        .data_out_num = DO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(i2sPort, &i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2sPort, &pinConfig));
    ESP_ERROR_CHECK(i2s_start(i2sPort));
}
