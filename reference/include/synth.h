
#pragma once

// generates audio samples
// reads voice state to change sound generation

/* TODO FOR SOBURG:
    - read wavetables from sd card
    - scope information wrapped in a struct (also a state.h change)
    - midi info needs to be implemented from state
    - probably an array from 0-127 where the index indicates the pitch and the value indicates the velocity. zero velocity would be note-off
    - theres variables in here that function for reading midi messages but that needs to be in its own midi.h file 
    - profile speed of generating the i2s buffer
    - profile speed of 64 bit integer arithmetic because it will prevent a lot of clipping everywhere. cant do it in 32 bit because youll lose resolution
    - ^^ audio quality needs to be studied as well. 24 bit might be just fine
    - fixed point arithmetic for biquad_filter will increase performance
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/i2s.h"

#include "web.h"
#include "state.h"
#include "oscillator.h"
#include "filter.h"

// sound config
#define I2S_NUM I2S_NUM_0 // i2s port
#define BUFFER_LEN 512 // i2s buffer size in bits (increase if theres losses, decrease if high latency)
// WARNING: maximum amount of phase precision at 32-log2(wavetable_size)
#define MIDI_NOTE_MIN 0 // lowest note
#define MIDI_NOTE_MAX 127 // higest note (7 bits)

// uart config for midi
#define MIDI_UART_NUM UART_NUM_1 // uart port
#define MIDI_TXD -1 // tx pin (-1 because not used)
#define MIDI_RXD 18 // rx pin
#define MIDI_BUF_SIZE 128 // size of each message in bits

// i2s config
#define I2S_NUM 0  // Use I2S port 0
#define I2S_BCK_IO GPIO_NUM_40
#define I2S_WS_IO GPIO_NUM_38  // also known as LRCLK
#define I2S_DO_IO GPIO_NUM_39  // data out

// wavetable
static int32_t wavetable1[WAVETABLE_SIZE]; // 32 bit signed integer
static int32_t wavetable2[WAVETABLE_SIZE];
static int32_t wavetable3[WAVETABLE_SIZE];
static int32_t wavetable4[WAVETABLE_SIZE];
static int32_t* wavetables[4] = {wavetable1, wavetable2, wavetable3, wavetable4};
volatile static int midi_note = -1; // current note being played
static uint32_t phase_increments[128]; // stores timesteps for sampling the wavetable for each note
// stored in an array because the calculation is reused for every new note played

static volatile uint32_t wavelength = 100;
static volatile uint32_t trigger = 0;

static oscillator_t osc1;
static oscillator_t osc2;
static oscillator_t osc3;
static biquad_t filter_lowpass;

static uint32_t trigger_offset = 0;

// eventually this will be a struct containing all different control parameters (mostly bools[checkboxes] and uint32[sliders])
static float web_controls[5] = {0.0f, 0.0f, 0.0f, 0.5f, 0.5f};
static uint32_t wave_selectors[4] = {0, 1, 2, 3}; 

void init_wavetable();
void init_phase_table();

void synth_init();
void synth_loop();

void i2s_init();
void i2s_buffer_write(uint32_t index, int32_t sample);
