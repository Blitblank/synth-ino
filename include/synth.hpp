
#pragma once

#include <Arduino.h>

#include "oscillator.hpp"
#include "filter.hpp"

#define MIDI_NOTE_MIN 0
#define MIDI_NOTE_MAX 127
#define WAVETABLE_SIZE 1024

class Synth {
public:
    Synth() {}
    ~Synth() = default;

    void init() {}
    void update(int32_t* buffer, uint32_t bufferLength) {}

private:

    const char* taskHandle = "SYNTH_TASK";

    // wavetables
    int32_t wavetable1[WAVETABLE_SIZE];
    int32_t wavetable2[WAVETABLE_SIZE];
    int32_t wavetable3[WAVETABLE_SIZE];
    int32_t wavetable4[WAVETABLE_SIZE];
    int32_t* wavetables[4] = {wavetable1, wavetable2, wavetable3, wavetable4};
    volatile int32_t midiNote = -1;
    uint32_t phaseIncrements[128];

    volatile uint32_t wavelength = 100;
    volatile uint32_t trigger = 0;
    uint32_t triggerOffset = 0;

    Oscillator oscillator1;
    Oscillator oscillator2;
    Oscillator oscillator3;
    Filter filterLowpass;

    const float webControls[5] = {0.0f, 0.0f, 0.0f, 0.5f, 0.5f};
    uint32_t wave_selectors[4] = {0, 1, 2, 3}; 

    void initWavetables() {}
    void initPhaseTable() {}

    void i2sInit() {}
    void i2sBufferWrite(uint32_t index, int32_t sample) {}
};
