
#include "synth.h"

void i2s_init()
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,  // Transmit only
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Interrupt level 1
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
}

void init_wavetable() { // generate a sine wave
    for (int i = 0; i < WAVETABLE_SIZE; ++i) {
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

        int64_t saw = ((int64_t)i * (INT64_C(2) * INT32_MIN)) / WAVETABLE_SIZE + INT32_MAX - 1;
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
// eventually wavetables will be generated elsewhere (maybe a python file somewhere), saved to an sd card, and loaded when selected

void init_phase_table() { // fill phase increments table

    for (uint8_t n = MIDI_NOTE_MIN; n <= MIDI_NOTE_MAX; n++) {

        double semitone_offset = (double)(n - 69); // tonal center at note=69(A4) is 440hz
        double freq = 440.0 * pow(2.0, semitone_offset / 12.0); // midi note to Hz
        double inc = freq/(double)SAMPLE_RATE*(double)WAVETABLE_SIZE;
        uint32_t inc_q =  (uint32_t)(inc * pow(2.0, (double)PHASE_PRECISION) + 0.5);
        phase_increments[n] = inc_q;

        //ESP_LOGI("AUDIO", "Phase Increment n = %u  inc = %lu", n, inc_q);
    }
}

void synth_init() {
    
    init_wavetable();
    init_phase_table();

    midi_note = 60-1; // 69 = a4
    double frequency = (double)(phase_increments[midi_note]) * (double)SAMPLE_RATE / (double)(WAVETABLE_SIZE * (1 << PHASE_PRECISION)); // all in doubles because i dont care enough
    double wavelength_constant = 1.2; // oscilloscope displays this many periods (slightly more than one)
    wavelength = (uint32_t)((double)SAMPLE_RATE * wavelength_constant / frequency + 0.5);
    trigger_offset = (uint32_t)((wavelength_constant - 1.0)/2.0 * (double)wavelength + 0.5); // padding for oscilloscope waveform

    oscillator_init(&osc1, wavetable1, WAVETABLE_SIZE, 300.0f, 1.0f);
    osc1.phase_inc = phase_increments[midi_note];

    oscillator_init(&osc2, wavetable1, WAVETABLE_SIZE, 60.0f, 1.0f);
    osc2.phase_inc = phase_increments[midi_note]*4; // 2 octaves above

    oscillator_init(&osc3, wavetable1, WAVETABLE_SIZE, 120.0f, 1.0f);
    osc3.phase_inc = phase_increments[midi_note]*4*3/2; // 2 octaves+5th above


    // setup i2s
    i2s_init();

}

void synth_loop() {

    get_slider_values(web_controls);
    get_dropdown_values(wave_selectors);

    // modify oscillator parameters based on the controls
    uint32_t carrier_interpolation = (uint32_t)(web_controls[0] * (float)UINT32_MAX);
    uint32_t modulation_depth = (uint32_t)(web_controls[2] * (1 << (32-2)));
    float lpf_cutoff = powf(web_controls[3], 4.0f) * (float)SAMPLE_RATE/2.1f; // max cutoff at nyquist freq
    float lpf_resonance = powf(web_controls[4], 1.0f) * 10.0f;
    // floating point operations are fine just not when generating each sample
    biquad_calc_lowpass(&filter_lowpass, lpf_cutoff, lpf_resonance, (float)SAMPLE_RATE);

    // consolidate this please <3
    osc1.wavetable1 = wavetables[wave_selectors[0]];
    osc1.wavetable2 = wavetables[wave_selectors[1]];
    osc1.wavetable3 = wavetables[wave_selectors[2]];
    osc1.carrier_interpolation = carrier_interpolation;
    osc1.modulation_depth = modulation_depth;
    // i leave these here because they will have different options in the future
    osc2.wavetable1 = wavetables[wave_selectors[0]];
    osc2.wavetable2 = wavetables[wave_selectors[1]];
    osc2.wavetable3 = wavetables[wave_selectors[2]];
    osc2.carrier_interpolation = carrier_interpolation;
    osc2.modulation_depth = modulation_depth;
    // just now theyre the same because i havent made the different options yet
    osc3.wavetable1 = wavetables[wave_selectors[0]];
    osc3.wavetable2 = wavetables[wave_selectors[1]];
    osc3.wavetable3 = wavetables[wave_selectors[2]];
    osc3.carrier_interpolation = carrier_interpolation;
    osc3.modulation_depth = modulation_depth;

    bool triggered = false;
    taskENTER_CRITICAL(&shared_data_mux);
    for (uint32_t i = 0; i < BUFFER_LEN; ++i) { // fill buffer with audio samples

        int32_t pm_sample1 = oscillator_sample(&osc1);
        int32_t pm_sample2 = oscillator_sample(&osc2);
        int32_t pm_sample3 = oscillator_sample(&osc3);
        // mix 3 oscillators based on their volumes
        //int32_t osc_samples = pm_sample1/2 + pm_sample2/8 + pm_sample3/16; // this is the most basic scuffed way you can do it
        int32_t osc_samples = pm_sample1/2;

        int32_t filtered_sample = biquad_process(&filter_lowpass, osc_samples);
        i2s_buffer_write(i, filtered_sample);

        // trigger the scope based on the lowest frequency oscillator
        uint32_t prev_phase = osc1.phase;
        uint32_t cycle_length = WAVETABLE_SIZE << PHASE_PRECISION;
        osc1.phase += osc1.phase_inc; // this being here is a problem
        osc2.phase += osc2.phase_inc;
        osc3.phase += osc3.phase_inc;
        if ((osc1.phase % cycle_length) < (prev_phase % cycle_length) && !triggered) {
            if(i >= trigger_offset) {
                trigger = i - trigger_offset;
                triggered = true;
            }
        }

    }


    scope_trigger = trigger;
    scope_wavelength = wavelength;

    taskEXIT_CRITICAL(&shared_data_mux);
    //set_buffer_values(buffer);
    if(!trigger) {
        // dont know why sometimes it doesnt (i suspect its when the phase overflows the uint32_t type but it should still work idk)
        //ESP_LOGI("AUDIO", "failed to trigger oscilloscope. phase = %lu. state1=%f, state2=%f", phase, last_z1, last_z2);
    }

    size_t bytes_written;
    i2s_write(I2S_NUM, i2s_buffer, sizeof(i2s_buffer), &bytes_written, portMAX_DELAY);

}

void i2s_buffer_write(uint32_t index, int32_t sample) {
    i2s_buffer[index] = sample;
}
