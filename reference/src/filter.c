
#include "filter.h"

#include <math.h>

int32_t biquad_process(biquad_t *f, int32_t in) {
    //int64_t acc = ((int64_t)f->b0 * in + (int64_t)f->z1) >> 30;
    //int32_t out = (int32_t)acc;
    //f->z1 = ((int64_t)f->b1 * in - (int64_t)f->a1 * out + (int64_t)f->z2) >> 30;
    //f->z2 = ((int64_t)f->b2 * in - (int64_t)f->a2 * out) >> 30;
    float in_f = (float)in/(float)INT32_MAX;
    float out = f->b0 * in_f + f->z1;
    f->z1 = f->b1 * in_f - f->a1 * out + f->z2;
    f->z2 = f->b2 * in_f - f->a2 * out;
    last_z1 = f->z1;
    last_z2 = f->z2;
    int32_t out_32 = (int32_t)(out * (float)(INT32_MAX));
    return out_32;
}

void biquad_calc_lowpass(biquad_t *f, float cutoff, float q, float sample_rate) {

    if(q < 0.01f) q = 0.01f; // q needs to be clamped because you get divide by zero errors otherwise

    float omega = 2.0f * M_PI * cutoff / sample_rate;
    float alpha = sinf(omega) / (2.0f * q);
    float cos_omega = cosf(omega);

    if(omega < 0.001f) omega = 0.001f; // divide by zero garbage
    if(alpha < 0.001f && alpha >= 0.0f) {
        alpha = 0.001f;
    } else if(alpha > -0.001f && alpha < 0.0f) {
        alpha = -0.001f;
    }

    float b0 = (1.0f - cos_omega) / 2.0f;
    float b1 = 1.0f - cos_omega;
    float b2 = (1.0f - cos_omega) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_omega;
    float a2 = 1.0f - alpha;

    if(a0 < 0.001f && a0 >= 0.0f) {
        a0 = 0.001f;
    } else if(a0 > -0.001f && a0 < 0.0f) {
        a0 = -0.001f;
    }

    // scale to fixed point
    //f->b0 = (int32_t)((b0 / a0) * (1 << 30));
    //f->b1 = (int32_t)((b1 / a0) * (1 << 30));
    //f->b2 = (int32_t)((b2 / a0) * (1 << 30));
    //f->a1 = (int32_t)((a1 / a0) * (1 << 30));
    //f->a2 = (int32_t)((a2 / a0) * (1 << 30));
    //f->z1 = 0;
    //f->z2 = 0;
    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
    f->z1 = last_z1;
    f->z2 = last_z2;
}
