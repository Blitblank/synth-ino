
#pragma once

#include <stdint.h>

class Filter {
public:
    Filter();
    ~Filter() = default;

    void biquadCalculateLowpass(float cutoff, float q, float sampleRate);
    int32_t biquadProcess(int32_t in);
    // TODO: add more filter types here
    // high pass
    // band pass
    // notch
    // https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
    // instead of making different calculate functions, have an enum which specifies the filter type
    // one public calculate which the enum is passed through and a private calculate for the specific types, switch statement to choose

private:

    // biquad filter structure
    float b0, b1, b2, a1, a2;
    float z1, z2;
    float lastZ1 = 0.0f;
    float lastZ2 = 0.0f;

};
