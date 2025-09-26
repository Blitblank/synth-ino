
#pragma once

#include <inttypes.h>
#include "freertos/portmacro.h"
#include "freertos/task.h"

// holds shared state variables used by different tasks on different cores
// basically thread-safe global variables here

extern volatile int32_t i2s_buffer[512];

extern volatile float web_sliders[5];
extern volatile uint32_t web_dropdowns[4];
extern volatile uint32_t scope_wavelength;
extern volatile uint32_t scope_trigger;

extern portMUX_TYPE shared_data_mux;

void set_slider_values(const float *values);
void set_dropdown_values(const uint32_t *values);
void set_buffer_values(const int32_t *values);

void get_slider_values(float *out_values);
void get_dropdown_values(uint32_t *out_values);
void get_buffer_values(int32_t *out_values);
