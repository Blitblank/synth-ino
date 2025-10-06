
#include "oscillator.hpp"

#include <stdint.h>
#include "Arduino.h"

Oscillator::Oscillator(float freq, uint32_t size, int32_t* wt1, int32_t* wt2, int32_t* wt3, uint32_t carrierInterp, uint32_t modDepth, float relVolume) :
        frequency(freq), wavetableSize(size), wavetable1(wt1), wavetable2(wt2), wavetable3(wt3), carrierInterpolation(carrierInterp), modulationDepth(modDepth), relativeVolume(relVolume) {

    phaseIncrement = (uint32_t)((freq * wavetableSize * (1 << PHASE_PRECISION)) / 44100 + 0.5); // TODO: use the phase table instead
    phase = 0; // Also TODO: 44100 needs to be in terms oif the sample rate but it'll go away if the above is set with the phase table

    wavetableMask = wavetableSize - 1;
}

int32_t Oscillator::sampleWavetable(const int32_t* wavetable, uint32_t x) {
    uint32_t index = (x >> PHASE_PRECISION) & wavetableMask; // current phase in the wavetable
    uint32_t next_index = (index + 1) & wavetableMask; // index + 1, overflow if at the end
    uint32_t frac = x & (1 << PHASE_PRECISION); // portion in between the two samples
    int32_t s1 = wavetable[index]; // for interpolation
    int32_t s2 = wavetable[next_index];
    // interpolated wavetable sample
    int64_t interp = ((int64_t)(s2 - s1) * frac) >> PHASE_PRECISION;
    return s1 + (int32_t)interp;
}

int32_t Oscillator::fsampleWavetable(const int32_t* wavetable, uint32_t x) {

    // no interpolation
    //Serial.printf("%d ", (x >> PHASE_PRECISION) & wavetableMask);
    return wavetable[(x >> PHASE_PRECISION) & wavetableMask];

    // I want to see what this looks like graphed out as well as hearing the difference
}

uint32_t Oscillator::floatToQ31(float f) {
    if (f >= 1.0f) return 0x7FFFFFFF;
    if (f <= 0.0f) return 0;
    return (uint32_t)(f * 2147483647.0f); // 2^31 - 1
}

int32_t Oscillator::sample() {
    // phase modulation
    // h(x) = f(x + m*g(x))
    // f(x) = carrier, g(x) = modulator

    // TODO: camelCase
    int32_t modulator_sample = fsampleWavetable(wavetable3, phase);
    uint32_t u_modulator_sample = (uint32_t)(modulator_sample + INT32_MAX);
    uint64_t modulation_offset = ((uint64_t)u_modulator_sample * (uint64_t)modulationDepth) >> 32;
    uint32_t x = phase + (uint32_t)modulation_offset; // modulated phase

    int32_t c_s1 = fsampleWavetable(wavetable1, x);
    int32_t c_s2 = fsampleWavetable(wavetable2, x);
    int64_t sample_64 = (int64_t)c_s1*(~carrierInterpolation) + (int64_t)c_s2*(carrierInterpolation); // interpolate between two wavetables
    int32_t pmSample = (int32_t)(sample_64 >> 32);

    return pmSample;

}
