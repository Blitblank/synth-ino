
#include "synth.hpp"

Synth::Synth(uint32_t bufferSize, uint32_t audioRate) {

    bufferLength = bufferSize;
    sampleRate = audioRate;

    // i guess this is just some default config values
    oscillator1 = Oscillator(100.0f, WAVETABLE_SIZE, wavetable1, wavetable2, wavetable3, 0, 0, 1.0f);
    oscillator2 = Oscillator(200.0f, WAVETABLE_SIZE, wavetable1, wavetable2, wavetable3, 0, 0, 1.0f);
    oscillator3 = Oscillator(400.0f, WAVETABLE_SIZE, wavetable1, wavetable2, wavetable3, 0, 0, 1.0f);

}

void Synth::initWavetables() {

    // eventually wavetables will be generated elsewhere (maybe a python file somewhere), saved to an sd card, and loaded here

    for (int i = 0; i < WAVETABLE_SIZE; ++i) { // these basic waveforms are mathematically perfect, no harmonics
        double angle = (2.0 * 3.141592653589793 * i) / WAVETABLE_SIZE;
        int32_t sine = sin(angle) * (double)(INT32_MAX); // 1 period of a sine wave between -2^31 and 2^31-1
        wavetable1[i] = sine;

        int32_t square = 0;
        if(i >= WAVETABLE_SIZE/2) {
            square = INT32_MIN;
        } else {
            square = INT32_MAX;
        }
        wavetable2[i] = square;

        int64_t saw = ((int64_t)i * ((int64_t)2 * INT32_MIN)) / WAVETABLE_SIZE + INT32_MAX - 1;
        wavetable3[i] = (int32_t)saw;

        int64_t triangle = 0;
        if(i <= WAVETABLE_SIZE/4) {
            triangle = 4*(INT32_MAX/WAVETABLE_SIZE) * i;
        } else if(i >= 3*WAVETABLE_SIZE/4) {
            triangle = 4*(INT32_MIN) + 4*(INT32_MAX/WAVETABLE_SIZE) * i;
        } else {
            triangle = 2*INT32_MAX - 4*(INT32_MAX/WAVETABLE_SIZE) * i;
        }
        wavetable4[i] = (int32_t)triangle;
    }
}


void Synth::initPhaseTable() {

    for (uint8_t n = MIDI_NOTE_MIN; n <= MIDI_NOTE_MAX; n++) {

        double semitone_offset = (double)(n - 69);
        double freq = 440.0 * pow(2.0, semitone_offset / 12.0);
        double inc  = freq / (double)(sampleRate) * (double)(WAVETABLE_SIZE);
        uint32_t inc_q = (uint32_t)(inc * (1u << PHASE_PRECISION) + 0.5);
        phaseIncrements[n] = inc_q;

        // this implementation is equal tempered (twelfth root of two)
        // eventually I want an option for justified
        // just phases need to be configured around a specific note
    }
}

void Synth::init() {

    initWavetables();
    initPhaseTable();

    uint32_t midiNote = 60-1; // 69 = a4, 1=semitone
    double frequency = (double)(phaseIncrements[midiNote]) * (double)sampleRate / (double)(WAVETABLE_SIZE * (1 << PHASE_PRECISION)); // all in doubles because i dont care enough
    double wavelength_constant = 1.2; // oscilloscope displays this many periods (slightly more than one)
    wavelength = (uint32_t)((double)sampleRate * wavelength_constant / frequency + 0.5); // scope width
    triggerOffset = (uint32_t)((wavelength_constant - 1.0)/2.0 * (double)wavelength + 0.5); // padding for oscilloscope waveform

    oscillator1.setPhaseInc(phaseIncrements[midiNote]);
    oscillator2.setPhaseInc(phaseIncrements[midiNote]*4); // 2 octaves above
    oscillator3.setPhaseInc(phaseIncrements[midiNote]*4*3/2); // 2 octaves+5th above
}

void Synth::generate(int32_t* buffer, uint32_t bufferLength, uint32_t* scopeWavelength, uint32_t* scopeTrigger, ControlState* controls) {

    // modify oscillator parameters based on the controls
    uint32_t carrierInterpolation = (uint32_t)(controls->sliders[1] * (float)UINT32_MAX);
    uint32_t modulationDepth = (uint32_t)(controls->sliders[2] * (1 << (32-2)));
    float lpfCutoff = controls->sliders[3] + 0.1f;
    float lpfResonance = controls->sliders[4];
    // TODO: get rid of floating point operations because it bottlenecks to 24 bit precision

    // TODO: here is where i forget the sliders and go off the envelopes
    // the todo is to get rid of this 
    lpfCutoff = filterFreqEnv.sample();
    lpfResonance = filterResEnv.sample();
    float amplitude = amplitudeEnv.sample();
    filter1.biquadCalculateLowpass(lpfCutoff, lpfResonance, (float)sampleRate);

    // this is just for a demo, remove when midi 
    float freq = controls->sliders[0];
    float inc  = freq / (float)(sampleRate) * (float)(WAVETABLE_SIZE);
    uint32_t inc_q = (uint32_t)(inc * (1u << PHASE_PRECISION) + 0.5);
    oscillator1.setPhaseInc(inc_q);

    bool sequence = true;
    if(sequence) {
        if((xTaskGetTickCount()-lastNoteTime) > noteInterval) {
            lastNoteTime = xTaskGetTickCount();
            noteIndex++;
            if(noteIndex >= sequenceLength) {
                noteIndex = 0;
            }
            amplitudeEnv.attack(96);
            filterFreqEnv.attack(96);
            filterResEnv.attack(96);
        }
        uint32_t currentPhaseInc = phaseIncrements[notes[noteIndex] + 24];
        oscillator1.setPhaseInc(currentPhaseInc);
    }

    // consolidate this please <3
    oscillator1.wavetable1 = wavetables[controls->dropdowns[0]];
    oscillator1.wavetable2 = wavetables[controls->dropdowns[1]];
    oscillator1.wavetable3 = wavetables[controls->dropdowns[2]];
    oscillator1.carrierInterpolation = carrierInterpolation;
    oscillator1.modulationDepth = modulationDepth;
    // i leave these here because they will have different options in the future
    oscillator2.wavetable1 = wavetables[controls->dropdowns[0]];
    oscillator2.wavetable2 = wavetables[controls->dropdowns[1]];
    oscillator2.wavetable3 = wavetables[controls->dropdowns[2]];
    oscillator2.carrierInterpolation = carrierInterpolation;
    oscillator2.modulationDepth = modulationDepth;
    // just now theyre the same because i havent made the different options yet
    oscillator3.wavetable1 = wavetables[controls->dropdowns[0]];
    oscillator3.wavetable2 = wavetables[controls->dropdowns[1]];
    oscillator3.wavetable3 = wavetables[controls->dropdowns[2]];
    oscillator3.carrierInterpolation = carrierInterpolation;
    oscillator3.modulationDepth = modulationDepth;

    bool triggered = false;

    for (uint32_t i = 0; i < bufferLength; ++i) { // fill buffer with audio samples

        int32_t pmSample1 = oscillator1.sample();
        //int32_t pmSample2 = oscillator2.sample();
        //int32_t pmSample3 = oscillator3.sample();
        // mix 3 oscillators based on their volumes
        //int32_t osc_samples = pm_sample1/2 + pm_sample2/8 + pm_sample3/16; // this is the most basic scuffed way you can do it
        int32_t oscSamples = pmSample1/2;
        // TODO: mixing function here
        // note: the voice class needs a mixing function to mix oscillators
        // and the synth class needs a mixing function to mix voices

        int32_t filteredSample = filter1.biquadProcess(oscSamples);
        // can chain together filters here

        // magic happens
        //filteredSample = (int32_t)((float)filteredSample*amplitude);
        buffer[i] = filteredSample;
        //Serial.printf("%d ", filteredSample);


        // trigger the scope based on the lowest frequency oscillator
        uint32_t prevPhase = oscillator1.getPhase();
        uint32_t cycleLength = WAVETABLE_SIZE << PHASE_PRECISION;
        oscillator1.step(); // perhaps every time the oscillator is sampled it automatically increments its phase ?
        oscillator2.step();
        oscillator3.step();
        if (((oscillator1.getPhase() % cycleLength) < (prevPhase % cycleLength)) && !triggered) {
            if(i >= triggerOffset) {
                trigger = i - triggerOffset;
                triggered = true;
            }
        }
    }

    *scopeTrigger = trigger;
    *scopeWavelength = wavelength;
    //Serial.printf("wavelength: %u trigger: %u \n", &scopeTrigger, &scopeWavelength);

    //set_buffer_values(buffer);
    if(!trigger) {
        // dont know why sometimes it doesnt (i suspect its when the phase overflows the uint32_t type but it should still work idk)
        ESP_LOGI("AUDIO", "failed to trigger oscilloscope. phase = %lu. state1=%f, state2=%f", phase, last_z1, last_z2);
    }

}
