
#include "app.hpp"

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

    i2sInit();

}

void App::main() {

    xTaskCreatePinnedToCore(&wifiTaskTrampoline, "WIFI_TASK", 4096, this, 7, NULL, 0);
    xTaskCreatePinnedToCore(&audioTaskTrampoline, "AUDIO_TASK", 4096, this, 9, NULL, 1);
    xTaskCreatePinnedToCore(&ioTaskTrampoline, "IO_TASK", 4096, this, 5, NULL, 0);

}

void App::wifiTask() {

    utils::serialLog(wifiTaskHandle, xTaskGetTickCount(), "Wifi task start.");

    wifiManager.update();

    /*
    while(1) {    
        vTaskDelay(10); // ms
    }
    */
    
    vTaskDelete(NULL);

}

void App::audioTask() {

    utils::serialLog(synthTaskHandle, xTaskGetTickCount(), "Audio task start.");

    synth.init();

    while(1) {

        vTaskDelay(1);

        spinlock1 = 1;

        synth.generate(i2sBuffer, i2sBufferLength, &scopeWavelength, &scopeTrigger);
        //Serial.printf("wavelength: %u trigger: %u \n", scopeTrigger, scopeWavelength);

        spinlock1 = 0;

        size_t bytesWritten;
        i2s_write(i2sPort, i2sBuffer, sizeof(i2sBuffer), &bytesWritten, portMAX_DELAY); // esp-idf function

        //Serial.printf("bytes_written: %d \n", bytesWritten);


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

        vTaskDelay(20); // ms
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
        .bck_io_num = 40, // TODO: add these to the pins.h file
        .ws_io_num  = 38,
        .data_out_num = 39,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(i2sPort, &i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2sPort, &pinConfig));
    ESP_ERROR_CHECK(i2s_start(i2sPort));
}
