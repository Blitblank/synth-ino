
#pragma once

// main waveform generation
// reads wavetables and applies basic phase modulation

/* TODO FOR SOBURG:
    - read wavetables from sd card
    - look at audio quality of interpolated wavetables vs non-interpolated wavetables
*/

#include <stdint.h>
#include <stddef.h>

#define MAX_OSCILLATORS 4

#define WAVETABLE_SIZE 1024 // number of samples per wavetable period
#define WAVETABLE_MASK (WAVETABLE_SIZE - 1) // bitmask for sampling wavetable
#define PHASE_PRECISION 20 // fixed point arithemetic for sampling wavetable (32-x.x) 
#define SAMPLE_RATE 44100 // i2s sample rate in hz

typedef struct {
    uint32_t phase; // internal for sampling wavetable 
    uint32_t phase_inc; // determines pitch
    // probably put the options here and calculate the pitch internally
    const int32_t *wavetable1; // carrier 1
    const int32_t *wavetable2; // carrier 2
    const int32_t *wavetable3; // modulator
    uint32_t carrier_interpolation;
    uint32_t modulation_depth;
    size_t wavetable_size;
    float rel_volume;
} oscillator_t;

// Initialize a single oscillator
void oscillator_init(oscillator_t *osc, const int32_t *wavetable, size_t size, float freq, float volume);

// Generate the next sample for one oscillator
int32_t oscillator_sample(oscillator_t *osc);

// Mix multiple oscillators into one output sample
int32_t oscillator_mix(const oscillator_t *oscillators, size_t count);

// Utility to convert float [0.0, 1.0] to Q1.31
static inline uint32_t float_to_q31(float f) {
    if (f >= 1.0f) return 0x7FFFFFFF;
    if (f <= 0.0f) return 0;
    return (uint32_t)(f * 2147483647.0f); // 2^31 - 1
}

int32_t sample_wavetable(const int32_t* wavetable, uint32_t x); // interpolates between wavetable samples 
int32_t sample_wavetable_fast(const int32_t* wavetable, uint32_t x); // floors the index (no interpolation), less precise
