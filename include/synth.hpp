
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
    Synth(uint32_t bufferSize, uint32_t audioRate);
    ~Synth() = default;

    void init();
    void generate(int32_t* buffer, uint32_t bufferLength, uint32_t* scopeWavelength, uint32_t* scopeTrigger);

private:

    const char* taskHandle = "SYNTH_TASK";

    // wavetables
    // when these get read from disk they might be classes instead 
    int32_t wavetable1[WAVETABLE_SIZE]; // sine
    int32_t wavetable2[WAVETABLE_SIZE]; // square
    int32_t wavetable3[WAVETABLE_SIZE]; // saw
    int32_t wavetable4[WAVETABLE_SIZE]; // triangle
    int32_t* wavetables[4] = {wavetable1, wavetable2, wavetable3, wavetable4};
    // TODO: capacity of wavetables to be defined by some const variable
    // 8 MB of psram, 1024*4B = 4KiB for each wavetable
    // 16 wavetables = 64KiB = 65.5KB (man these things have so much ram)

    // tone generation
    volatile int32_t midiNote = -1;
    uint32_t phaseIncrements[128];

    // oscilloscope parameters
    uint32_t wavelength = 0;
    uint32_t trigger = 0;
    uint32_t triggerOffset = 0;

    // voice
    Oscillator oscillator1;
    Oscillator oscillator2;
    Oscillator oscillator3;
    Filter filter1;
    // TODO: eventually oscillators and filters will be members of a voice class, but I'm only doing one voice for now
    // maybe an array of oscillators? makes looping over them easier
    // voice will need a function to mix oscillators
    // synth class will do it manually

    // control
    const float webControls[5] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f};
    uint32_t waveSelectors[4] = {0, 1, 2, 3}; 
    // TODO: synth will get passed a parameters struct when generate() is called
    // parameters will contain everything configured from the control interface

    // loads a waveform shape into each wavetable array, either mathematically generated at startup or loaded from disk
    void initWavetables();
    // calculates the phase differences at each semitone needed to produce that specific frequency
    void initPhaseTable(); 

    // audio configurations
    uint32_t sampleRate = 0;
    uint32_t bufferLength = 0; 

};
