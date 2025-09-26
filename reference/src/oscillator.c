
#include "oscillator.h"
#include <math.h>

int32_t sample_wavetable(const int32_t* wavetable, uint32_t x) {
    uint32_t index = (x >> PHASE_PRECISION) & WAVETABLE_MASK; // current phase in the wavetable
    uint32_t next_index = (index + 1) & WAVETABLE_MASK; // index + 1, overflow to beginning
    uint32_t frac = x & (1 << PHASE_PRECISION); // portion in between the two samples
    int32_t s1 = wavetable[index]; // for interpolation
    int32_t s2 = wavetable[next_index];
    // interpolated wavetable sample
    int64_t interp = ((int64_t)(s2 - s1) * frac) >> PHASE_PRECISION;
    return s1 + (int32_t)interp;
}

int32_t sample_wavetable_fast(const int32_t* wavetable, uint32_t x) {
    return wavetable[(x >> PHASE_PRECISION) & WAVETABLE_MASK]; // current phase in the wavetable
}

void oscillator_init(oscillator_t *osc, const int32_t *wavetable, size_t size, float freq, float volume) {
    osc->wavetable1 = wavetable;
    osc->wavetable_size = size;
    osc->rel_volume = volume;
    osc->phase_inc = (uint32_t)((freq * WAVETABLE_SIZE * (1 << PHASE_PRECISION)) / SAMPLE_RATE + 0.5);
    osc->phase = 0;
}

// this is where most of the computational effort goes because it happens 3 times per voice so optimize this the most
int32_t oscillator_sample(oscillator_t *osc) {
    // phase modulation
    // h(x) = f(x + m*g(x))
    // f(x) = carrier, g(x) = modulator

    int32_t modulator_sample = sample_wavetable_fast(osc->wavetable3, osc->phase);
    uint32_t u_modulator_sample = (uint32_t)(modulator_sample + INT32_MAX);
    uint64_t modulation_offset = ((uint64_t)u_modulator_sample * (uint64_t)osc->modulation_depth) >> 32;
    uint32_t x = osc->phase + (uint32_t)modulation_offset; // modulated phase

    int32_t c_s1 = sample_wavetable_fast(osc->wavetable1, x);
    int32_t c_s2 = sample_wavetable_fast(osc->wavetable2, x);
    int64_t sample_64 = (int64_t)c_s1*(~osc->carrier_interpolation) + (int64_t)c_s2*(osc->carrier_interpolation); // interpolate between two wavetables
    int32_t pm_sample = (int32_t)(sample_64 >> 32);

    return pm_sample;
}

int32_t oscillator_mix(const oscillator_t *oscillators, size_t count) {
    int64_t mix = 0;
    for (size_t i = 0; i < count; ++i) {
        mix += oscillator_sample((oscillator_t *)&oscillators[i]); // Cast to non-const to advance phase
    }
    if (mix > INT32_MAX) mix = INT32_MAX;
    else if (mix < INT32_MIN) mix = INT32_MIN;
    return (int32_t)mix;
}
