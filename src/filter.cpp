
#include "filter.hpp"

#include <math.h>

Filter::Filter() {

}

void Filter::biquadCalculateLowpass(float cutoff, float q, float sampleRate) {

    if(q < 0.01f) q = 0.01f; // q needs to be clamped because you get divide by zero errors otherwise

    float omega = 2.0f * M_PI * cutoff / sampleRate;
    float alpha = sinf(omega) / (2.0f * q);
    float cos_omega = cosf(omega);

    if(omega < 0.001f) omega = 0.001f; // divide by zero garbage
    if(alpha < 0.001f && alpha >= 0.0f) {
        alpha = 0.001f;
    } else if(alpha > -0.001f && alpha < 0.0f) {
        alpha = -0.001f;
    }

    // calculate biquad coefficients
    // these particular formulas are specific to lowpass
    float b0 = (1.0f - cos_omega) / 2.0f;
    float b1 = 1.0f - cos_omega;
    float b2 = (1.0f - cos_omega) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_omega;
    float a2 = 1.0f - alpha;

    // prevent more divide by zero errors
    if(a0 < 0.001f && a0 >= 0.0f) {
        a0 = 0.001f;
    } else if(a0 > -0.001f && a0 < 0.0f) {
        a0 = -0.001f;
    }

    // TODO: if process works in fixed point then this will work too
    // scale to fixed point
    //f->b0 = (int32_t)((b0 / a0) * (1 << 30));
    //f->b1 = (int32_t)((b1 / a0) * (1 << 30));
    //f->b2 = (int32_t)((b2 / a0) * (1 << 30));
    //f->a1 = (int32_t)((a1 / a0) * (1 << 30));
    //f->a2 = (int32_t)((a2 / a0) * (1 << 30));
    //f->z1 = 0;
    //f->z2 = 0;
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;
    a1 = a1 / a0;
    a2 = a2 / a0;
    z1 = lastZ1;
    z2 = lastZ2;
}

int32_t Filter::biquadProcess(int32_t in) {
    // TODO: below is an attempt to perform the uncommented portion in fixed point format 
    //int64_t acc = ((int64_t)f->b0 * in + (int64_t)f->z1) >> 30;
    //int32_t out = (int32_t)acc;
    //f->z1 = ((int64_t)f->b1 * in - (int64_t)f->a1 * out + (int64_t)f->z2) >> 30;
    //f->z2 = ((int64_t)f->b2 * in - (int64_t)f->a2 * out) >> 30;

    float in_f = (float)in/(float)INT32_MAX;
    float out = b0 * in_f + z1;
    z1 = b1 * in_f - a1 * out + z2;
    z2 = b2 * in_f - a2 * out;
    lastZ1 = z1;
    lastZ2 = z2;
    int32_t out_32 = (int32_t)(out * (float)(INT32_MAX));
    return out_32;
    // there's about 11 fixed point operations here
}
