
#include "app.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Arduino.h"
#include "tasks.h"
#include <driver/i2s.h>

App::App() {

}

void App::init() {

    activeBuffer = 0;
    bufferReady = true;

    Serial.begin(115200);
    utils::serialLog(appTaskHandle, xTaskGetTickCount(), "App init.");

    i2sInit();

}

void App::main() {

    xTaskCreate(&wifiTaskTrampoline, "WIFI_TASK", 4096, this, 7, NULL);
    xTaskCreate(&audioTaskTrampoline, "AUDIO_TASK", 4096, this, 9, NULL);
    xTaskCreate(&ioTaskTrampoline, "IO_TASK", 4096, this, 5, NULL);

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
        if(!bufferReady.load(std::memory_order_acquire)) continue; // in the middle of a swap or something

        int32_t inactive = activeBuffer.load(std::memory_order_acquire) ^ 1;
        synth.update((inactive == 0) ? i2sBufferA : i2sBufferB, i2sBufferLength);
        bufferReady.store(false, std::memory_order_release);

    }

}

void App::ioTask() {

    utils::serialLog(ioTaskHandle, xTaskGetTickCount(), "Io task start.");

    oled.init();

    while(1) {

        int32_t front = activeBuffer.load(std::memory_order_acquire);

        uint32_t start = xTaskGetTickCount(); // time profiling
        oled.draw((front) ? i2sBufferA : i2sBufferB, i2sBufferLength); // TODO: need some error checking here
        uint32_t end = xTaskGetTickCount();

        vTaskDelay(40); // ms
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

void IRAM_ATTR App::i2sDmaIsr(void* arg) {
    // this isr is called when the i2s dma finishes reading a buffer
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    activeBuffer.store(activeBuffer.load(std::memory_order_relaxed) ^ 1, std::memory_order_release); //swap active buffer
    bufferReady.store(true, std::memory_order_release); // tag for read capability

    // this clears interrupt in hardware
    I2S0.int_clr.tx_done = 1;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
        .bck_io_num = 26, // TODO: CHANGE THESE
        .ws_io_num  = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(i2sPort, &i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2sPort, &pinConfig));

    // interrupt service routine 
    esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_IRAM, i2sDmaIsr, NULL, &i2sIntrHandle);

    
}
