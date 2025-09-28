
#include "app.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Arduino.h"
#include "tasks.h"

App::App() {

}

void App::init() {

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

        // for determining which buffer (A or B) is currently the front
        uint8_t front = (uint8_t)atomic_load_explicit(&frontIndex, memory_order_acquire);
        int32_t* backBuffer = (front ^ 1) ? i2sBufferA : i2sBufferB; // whichever is the back buffer is the one available for writing

        uint32_t start = xTaskGetTickCount(); // time profiling
        synth.update(backBuffer, I2S_BUFFER_LENGTH); //probably rename this to "synth.writeI2S" or similar
        uint32_t end = xTaskGetTickCount();

        // Queue it to I2S DMA for playback
        size_t bytes_written;
        i2s_write(I2S_NUM, buffers[back], BUFFER_BYTES, &bytes_written, portMAX_DELAY);

        // i2s dma interrupt triggers the callback to change frontIndex to the other buffer

    }

}

void App::ioTask() {

    utils::serialLog(ioTaskHandle, xTaskGetTickCount(), "Io task start.");

    oled.init();

    while(1) {

        uint8_t front = (uint8_t)atomic_load_explicit(&frontIndex, memory_order_acquire);
        uint32_t seq  = atomic_load_explicit(&bufferSequence, memory_order_relaxed);

        // Read from buffers[front]; ISR never writes this one
        int32_t* frontBuffer = (front) ? i2sBufferA : i2sBufferB; // whichever is the front buffer is the one available for reading

        uint32_t start = xTaskGetTickCount(); // time profiling
        oled.update(frontBuffer); // TODO: need some error checking here
        uint32_t end = xTaskGetTickCount();

        vTaskDelay(10); // ms
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

void IRAM_ATTR App::i2sDmaIsr(void* args) {
    BaseType_t hp_task_woken = pdFALSE;

    // swap buffers
    uint8_t front = (uint8_t)atomic_load_explicit(&front_idx, memory_order_acquire);
    uint8_t back  = front ^ 1;

    // Here you can optionally copy/convert DMA data into buffers[back]
    // If the DMA already wrote into buffers[back], just swap

    // publish to the atomic variables
    atomic_store_explicit(&frontIndex, back, memory_order_release);
    atomic_fetch_add_explicit(&bufferSequence, 1, memory_order_relaxed);

    // Clear interrupt flags as needed by driver
    // (depends on specific I2S/DMA config)
}

void App::i2sInit() {

    i2s_config_t cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // mono (left) for now
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 2, // <-- dma buffers A and B
        .dma_buf_len = BUFFER_LEN / 2, // <-- dunno why this needs to be halved
        .use_apll = false
    };

    i2s_pin_config_t pins = {
        .bck_io_num = 26,
        .ws_io_num  = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM, &cfg, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM, &pins));

    // interrupt service routine (ISR) that 
    ESP_ERROR_CHECK(esp_intr_alloc(ETS_I2S0_INTR_SOURCE, // i2s interface number
                                   ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1, 
                                   i2sDmaIsr, // isr callback function
                                   NULL,
                                   &i2sIsrHandle)); // 
}
