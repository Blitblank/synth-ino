
#include "Filter.hpp"

#include <math.h>
#include "Arduino.h"

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
    float x_b0 = (1.0f - cos_omega) / 2.0f;
    float x_b1 = 1.0f - cos_omega;
    float x_b2 = (1.0f - cos_omega) / 2.0f;
    float x_a0 = 1.0f + alpha;
    float x_a1 = -2.0f * cos_omega;
    float x_a2 = 1.0f - alpha;

    // prevent more divide by zero errors
    if(x_a0 < 0.001f && x_a0 >= 0.0f) {
        x_a0 = 0.001f;
    } else if(x_a0 > -0.001f && x_a0 < 0.0f) {
        x_a0 = -0.001f;
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
    b0 = x_b0 / x_a0;
    b1 = x_b1 / x_a0;
    b2 = x_b2 / x_a0;
    a1 = x_a1 / x_a0;
    a2 = x_a2 / x_a0;
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

    //Serial.printf("coefficients: b0=%f b1=%f b2=%f a1=%f a2=%f z1=%f z2=%f \n", b0, b1, b2, a1, a2, z1, z2);
    //Serial.printf("FilterProcess: input: %d output: %d  in_f=%f out_f=%f z1=%f z2=%f \n\n", in, out_32, in_f, out, z1, z2);

    return out_32;
    // there's about 11 floating point operations here
}
