
#pragma once

#include <inttypes.h>

// biquad filter
typedef struct {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} biquad_t;

static float last_z1 = 0.0f;
static float last_z2 = 0.0f;

int32_t biquad_process(biquad_t *f, int32_t in);
void biquad_calc_lowpass(biquad_t *f, float cutoff, float q, float sample_rate);
