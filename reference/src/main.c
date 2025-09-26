
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "driver/uart.h"

#include "oled.h"
#include "web.h"
#include "synth.h"

static void wifi_task(void *param) {
    wifi_init();

    vTaskDelete(NULL);
}

static void audio_task(void *param) {
    
    ESP_LOGI("AUDIO_MODULE", "Running on core %d", xPortGetCoreID());
    esp_task_wdt_add(NULL);

    synth_init();

    uint32_t count = 0;

    while (1) {

        count++;

        uint64_t start = esp_timer_get_time();

        synth_loop();

        uint64_t end = esp_timer_get_time();

        // rtos things :)
        if(count % 100 == 0) { // about every second
            //ESP_LOGI("SYNTH", "100 loops passed.");
            ESP_LOGI("SYNTH", "generating an i2s buffer of size %d took %llu ms (%llu us). Max allowed time is %.3f ms\n", BUFFER_LEN, (end - start)/1000, (end - start), (float)BUFFER_LEN/(float)SAMPLE_RATE*1000.0f);
            esp_task_wdt_reset();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // i hate freertos bro really ?? cant delay in finer increments than 10ms get outta here (your tick unit had to be an integer with length of 10ms i hate you)
        // ideally with an i2s buffer size of 512 the delay would be [buffer_size/sampling_freq*1000] = 11.61ms
    }
}

static void io_task(void *param) {

    ESP_LOGI("IO_MODULE", "Running on core %d", xPortGetCoreID());

    const TickType_t frame_delay = pdMS_TO_TICKS(50); // 50ms => 20fps
    TickType_t last_wake = xTaskGetTickCount();

    oled_init();

    const uart_port_t uart_num = UART_NUM_2;
    const int uart_buffer_size = 1024;

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Install driver with RX buffer
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t tx_byte = 0x90; // Example MIDI Note On message byte
    uint8_t rx_buf[128];


    while(1) {
        oled_update();

        // send uart via rx
        //uart_write_bytes(uart_num, (const char *)&tx_byte, 1);
        //ESP_LOGI(TAG, "Sent byte: 0x%02X", tx_byte);

        // wait for message send
        //vTaskDelay(pdMS_TO_TICKS(10));

        // read back uart via tx
        //int len = uart_read_bytes(uart_num, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
        //if (len > 0) {
        //    ESP_LOGI(TAG, "Received %d bytes:", len);
        //    for (int i = 0; i < len; i++) {
        //        ESP_LOGI(TAG, "  -> 0x%02X", rx_buf[i]);
        //    }
        //}

        vTaskDelayUntil(&last_wake, frame_delay);
    }
}

void app_main(void) {

    gpio_config_t io_config = {};
    io_config.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_config.mode = GPIO_MODE_OUTPUT; // output pin
    io_config.pin_bit_mask = (1ULL << GPIO_NUM_2); // set pins
    io_config.pull_down_en = 0; // disable pulldown
    io_config.pull_up_en = 0; //disable pullup
    gpio_config(&io_config); // apply config
    gpio_set_level(GPIO_NUM_2, 0);

    io_config.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_config.mode = GPIO_MODE_OUTPUT; // output pin
    io_config.pin_bit_mask = (1ULL << GPIO_NUM_42); // set pins
    io_config.pull_down_en = 1; // disable pulldown
    io_config.pull_up_en = 1; // enable pullup
    gpio_config(&io_config); // apply config
    gpio_set_level(GPIO_NUM_42, 1);

    io_config.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_config.mode = GPIO_MODE_OUTPUT; // output pin
    io_config.pin_bit_mask = (1ULL << GPIO_NUM_41); // set pins
    io_config.pull_down_en = 1; // disable pulldown
    io_config.pull_up_en = 1; // enable pullup
    gpio_config(&io_config); // apply config
    gpio_set_level(GPIO_NUM_41, 0);

    ESP_LOGI("MAIN", "Running");

    xTaskCreate(audio_task, "audio_task", 4096, NULL, 9, NULL);
    xTaskCreate(wifi_task, "wifi_task", 4096*2, NULL, 7, NULL);
    xTaskCreate(io_task, "io_task", 4096, NULL, 5, NULL);

}
