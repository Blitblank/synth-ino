
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
    uint32_t count = 0;

    while(1) {
        count++;

        // this should do the double dma buffer thing like vsync but for i2s
        // yadda yadda two buffers an interrupt triggers once i2s is done reading the buffer and then swap which buffer youre writing to
        uint32_t start = xTaskGetTickCount();
        synth.update(); 
        uint32_t end = xTaskGetTickCount();

        if(count % 100 == 0) {
            utils::serialLog(synthTaskHandle, (uint32_t)xTaskGetTickCount(), "100 i2s buffers filled");
        }
        vTaskDelay(10); // ms 
    }

}

void App::ioTask() {

    utils::serialLog(ioTaskHandle, xTaskGetTickCount(), "Io task start.");

    oled.init();

    while(1) {
        oled.update();
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
