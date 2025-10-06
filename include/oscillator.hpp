
#pragma once

#include <stdint.h>
#include <cstddef>

#define PHASE_PRECISION 20 // fixed point arithemetic for sampling wavetable (32-x.x) 
// PHASE_PRECISION must be <= 32 - log2(WAVETABLE_SIZE)

class Oscillator {
public:
    Oscillator() {}
    Oscillator(float freq, uint32_t size, int32_t* wt1, int32_t* wt2, int32_t* wt3, uint32_t carrierInterp, uint32_t modDepth, float relVolume);
    ~Oscillator() = default;
    
    // TODO: profile performance on making these inline
    int32_t sampleWavetable(const int32_t* wavetable, uint32_t x); // interpolates between wavetable samples 
    int32_t fsampleWavetable(const int32_t* wavetable, uint32_t x); // fast version. floors the index (no interpolation), less precise
    // if you want you can save generated audio to a .wav file on the sd card to qualify the difference 
    // the precise version might be too slow to generate live (especially with multiple oscillators on multiple voices)

    int32_t sample(); // returns a single audio sample at the current phase
    void step() { phase += phaseIncrement; }; // increments the phase by the set phase increment value
    void setPhaseInc(uint32_t phaseInc) { phaseIncrement = phaseInc; } // TODO: keep a pointer to the phase table and instead set phaseInc by the midiNote
    uint32_t getPhase() { return phase; }

    const int32_t *wavetable1; // carrier 1
    const int32_t *wavetable2; // carrier 2
    const int32_t *wavetable3; // modulator
    uint32_t carrierInterpolation; // amount to mix carrier 1 and carrier2 linearly
    uint32_t modulationDepth; // amount that the modulator affects the sampling phase of the carrier

private:

    float frequency;
    uint32_t phase; // current wavetable sampling position
    uint32_t phaseIncrement; // amount to move forward when sampling wavetable, determines pitch
    uint32_t wavetableSize;
    uint32_t wavetableMask;
    float relativeVolume; // for mixing

    uint32_t floatToQ31(float f);
    
};
