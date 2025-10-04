
#include "synth.hpp"

Synth::Synth() {
    // TODO: Implement
}

void initWavetables() {

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


void initPhaseTable() {

    for (uint8_t n = MIDI_NOTE_MIN; n <= MIDI_NOTE_MAX; n++) {

        double semitone_offset = (double)(n - 69); // tonal center at note=69(A4) is 440hz
        double freq = 440.0 * pow(2.0, semitone_offset / 12.0); // midi note to Hz
        double inc = freq/(double)SAMPLE_RATE*(double)WAVETABLE_SIZE;
        uint32_t inc_q =  (uint32_t)(inc * pow(2.0, (double)PHASE_PRECISION) + 0.5);
        phase_increments[n] = inc_q;

        // this implementation is equal tempered (twelfth root of two)
        // eventually I want an option for justified
        // just phases need to be configured for a certain key
    }
}

void Synth::init() {

    initWavetables();
    initPhaseTable();

    midi_note = 60-1; // 69 = a4, 1=semitone
    double frequency = (double)(phase_increments[midi_note]) * (double)SAMPLE_RATE / (double)(WAVETABLE_SIZE * (1 << PHASE_PRECISION)); // all in doubles because i dont care enough
    double wavelength_constant = 1.2; // oscilloscope displays this many periods (slightly more than one)
    wavelength = (uint32_t)((double)SAMPLE_RATE * wavelength_constant / frequency + 0.5); // scope width
    trigger_offset = (uint32_t)((wavelength_constant - 1.0)/2.0 * (double)wavelength + 0.5); // padding for oscilloscope waveform

    oscillator_init(&osc1, wavetable1, WAVETABLE_SIZE, 300.0f, 1.0f);
    osc1.phase_inc = phase_increments[midi_note];

    oscillator_init(&osc2, wavetable1, WAVETABLE_SIZE, 60.0f, 1.0f);
    osc2.phase_inc = phase_increments[midi_note]*4; // 2 octaves above

    oscillator_init(&osc3, wavetable1, WAVETABLE_SIZE, 120.0f, 1.0f);
    osc3.phase_inc = phase_increments[midi_note]*4*3/2; // 2 octaves+5th above
}

void  Synth::i2sBufferWrite(uint32_t index, int32_t sample) {
    i2s_buffer[index] = sample;
}

void Synth::update(int32_t* buffer, uint32_t bufferLength) {
    // TODO: pass in control parameters through a struct
    get_slider_values(web_controls);
    get_dropdown_values(wave_selectors);

    // modify oscillator parameters based on the controls
    uint32_t carrier_interpolation = (uint32_t)(web_controls[0] * (float)UINT32_MAX);
    uint32_t modulation_depth = (uint32_t)(web_controls[2] * (1 << (32-2)));
    float lpf_cutoff = powf(web_controls[3], 4.0f) * (float)SAMPLE_RATE/2.1f; // max cutoff at nyquist freq
    float lpf_resonance = powf(web_controls[4], 1.0f) * 10.0f; // the pow function gives more precision at lower frequencies
    // floating point operations are fine just not when generating each sample
    filter1.biquadCalcLowpass(&filter_lowpass, lpf_cutoff, lpf_resonance, (float)SAMPLE_RATE);
    // although it will be faster once floating point operations are gone

    // consolidate this please <3
    oscillator1.wavetable1 = wavetables[wave_selectors[0]];
    oscillator1.wavetable2 = wavetables[wave_selectors[1]];
    oscillator1.wavetable3 = wavetables[wave_selectors[2]];
    oscillator1.carrier_interpolation = carrier_interpolation;
    oscillator1.modulation_depth = modulation_depth;
    // i leave these here because they will have different options in the future
    oscillator2.wavetable1 = wavetables[wave_selectors[0]];
    oscillator2.wavetable2 = wavetables[wave_selectors[1]];
    oscillator2.wavetable3 = wavetables[wave_selectors[2]];
    oscillator2.carrier_interpolation = carrier_interpolation;
    oscillator2.modulation_depth = modulation_depth;
    // just now theyre the same because i havent made the different options yet
    oscillator3.wavetable1 = wavetables[wave_selectors[0]];
    oscillator3.wavetable2 = wavetables[wave_selectors[1]];
    oscillator3.wavetable3 = wavetables[wave_selectors[2]];
    oscillator3.carrier_interpolation = carrier_interpolation;
    oscillator3.modulation_depth = modulation_depth;

    bool triggered = false;

    for (uint32_t i = 0; i < BUFFER_LEN; ++i) { // fill buffer with audio samples

        int32_t pm_sample1 = oscillator1.sample();
        int32_t pm_sample2 = oscillator2.sample();
        int32_t pm_sample3 = oscillator3.sample();
        // mix 3 oscillators based on their volumes
        //int32_t osc_samples = pm_sample1/2 + pm_sample2/8 + pm_sample3/16; // this is the most basic scuffed way you can do it
        int32_t osc_samples = pm_sample1/2;
        // TODO: mixing function here
        // note: the voice class needs a mixing function to mix oscillators
        // and the synth class needs a mixing function to mix voices

        int32_t filtered_sample = filter1.biquadProcess(osc_samples);
        // can chain together filters here
        i2s_buffer_write(i, filtered_sample);

        // TODO: camelCase
        // trigger the scope based on the lowest frequency oscillator
        uint32_t prev_phase = oscillator1.phase;
        uint32_t cycle_length = WAVETABLE_SIZE << PHASE_PRECISION;
        oscillator1.step(); // perhaps every time the oscillator is sampled it automatically increments its phase ?
        oscillator2.step();
        oscillator3.step();
        if ((oscillator1.phase % cycle_length) < (prev_phase % cycle_length) && !triggered) {
            if(i >= trigger_offset) {
                trigger = i - trigger_offset;
                triggered = true;
            }
        }

    }

    scope_trigger = trigger;
    scope_wavelength = wavelength;

    //set_buffer_values(buffer);
    if(!trigger) {
        // dont know why sometimes it doesnt (i suspect its when the phase overflows the uint32_t type but it should still work idk)
        //ESP_LOGI("AUDIO", "failed to trigger oscilloscope. phase = %lu. state1=%f, state2=%f", phase, last_z1, last_z2);
    }

    size_t bytes_written;
    i2s_write(I2S_NUM, i2s_buffer, sizeof(i2s_buffer), &bytes_written, portMAX_DELAY); // esp-idf function

}
