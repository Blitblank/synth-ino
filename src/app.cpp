
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

std::atomic<int32_t> App::activeBuffer;
std::atomic<bool> App::bufferReady;
intr_handle_t App::i2sIntrHandle;

void App::init() {

    activeBuffer.store(0, std::memory_order_release);
    bufferReady.store(true, std::memory_order_release);

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

        if(bufferReady.load(std::memory_order_acquire)) {
            int32_t inactive = activeBuffer.load(std::memory_order_acquire) ^ 1;
            int32_t* activeBuffer = (inactive == 0) ? i2sBufferA : i2sBufferB;
            int32_t* inactiveBuffer = (inactive != 0) ? i2sBufferA : i2sBufferB;

            synth.generate(activeBuffer, i2sBufferLength, &scopeWavelength, &scopeTrigger);

            bufferReady.store(false, std::memory_order_release);
            size_t bytesWritten;
            i2s_write(i2sPort, inactiveBuffer, sizeof(i2sBufferA), &bytesWritten, portMAX_DELAY); // esp-idf function

            Serial.printf("bytes_written: %d \n", bytesWritten);
        } else {
            Serial.printf("buffer wasn't ready :(");
        }


        // TODO: performance profiling of the synth generation
        // first thing to see is compiler optimization for performance (-O2)
    }

}

void App::ioTask() {

    utils::serialLog(ioTaskHandle, xTaskGetTickCount(), "Io task start.");

    oled.init();

    while(1) {

        int32_t front = activeBuffer.load(std::memory_order_acquire);

        uint32_t start = xTaskGetTickCount(); // time profiling
        oled.draw((front) ? i2sBufferA : i2sBufferB, i2sBufferLength, scopeWavelength, scopeTrigger);
        uint32_t end = xTaskGetTickCount();

        //Serial.printf("time diff of oled.draw: %d \n", end-start);

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
    bufferReady.store(true, std::memory_order_release); // tag for read availability

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
        .bck_io_num = 40, // TODO: add these to the pins.h file
        .ws_io_num  = 38,
        .data_out_num = 39,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(i2sPort, &i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2sPort, &pinConfig));

    // Enable TX end-of-frame interrupt
    I2S0.int_clr.val = 0xFFFFFFFF;
    //I2S0.int_ena.out_eof = 1;
    I2S0.int_ena.tx_done = 1;
    I2S0.int_ena.out_total_eof = 1;

    // interrupt service routine 
    ESP_ERROR_CHECK(esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_IRAM, i2sDmaIsr, NULL, &i2sIntrHandle));

    // Start I²S so DMA actually runs
    ESP_ERROR_CHECK(i2s_start(i2sPort));
}
