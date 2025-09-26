
#pragma once

class Oscillator {
public:
    Oscillator() {}
    ~Oscillator() = default;

    void init() {}
    int32_t sample() {}
    int32_t mix(Oscillator* oscillators, size_t count) {}

private:

    uint32_t phase; // internal for sampling wavetable 
    uint32_t phase_inc; // determines pitch
    // probably put the options here and calculate the pitch internally
    const int32_t *wavetable1; // carrier 1
    const int32_t *wavetable2; // carrier 2
    const int32_t *wavetable3; // modulator
    uint32_t carrierInterpolation;
    uint32_t modulationDepth;
    size_t wavetableSize;
    float relativeVolume;

    uint32_t floatToQ31(float f) {}

    int32_t sampleWavetable(const int32_t* wavetable, uint32_t x) {} // interpolates between wavetable samples 
    int32_t fsampleWavetable(const int32_t* wavetable, uint32_t x) {} // floors the index (no interpolation), less precise
    
};
