
#include <inttypes.h>

#include "synth.h"

volatile int32_t i2s_buffer[512] = {0};

volatile float web_sliders[5] = {0.0f, 0.0f, 0.0f, 0.5f, 0.5f};
volatile uint32_t web_dropdowns[4] = {0, 1, 2, 3};
volatile uint32_t scope_wavelength = 120;
volatile uint32_t scope_trigger = 0;

portMUX_TYPE shared_data_mux = portMUX_INITIALIZER_UNLOCKED;

void set_slider_values(const float *values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < 5; ++i) {
        web_sliders[i] = values[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

void set_dropdown_values(const uint32_t *values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < 4; ++i) {
        web_dropdowns[i] = values[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

void set_buffer_values(const int32_t *values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < 4; ++i) {
        i2s_buffer[i] = values[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

void get_slider_values(float *out_values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < 5; ++i) {
        out_values[i] = web_sliders[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

void get_dropdown_values(uint32_t *out_values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < 4; ++i) {
        out_values[i] = web_dropdowns[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

void get_buffer_values(int32_t *out_values) {
    taskENTER_CRITICAL(&shared_data_mux);
    for (int i = 0; i < BUFFER_LEN; ++i) {
        out_values[i] = i2s_buffer[i];
    }
    taskEXIT_CRITICAL(&shared_data_mux);
}

