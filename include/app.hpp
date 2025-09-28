
#pragma once

#include <stdatomic.h>

#include "synth.hpp"
#include "state.hpp"
#include "wifiManager.hpp"
#include "oled.hpp"
#include "utils.h"

// TODO: I'd like this to be somewhere else, maybe an i2s manager class. next option is in the synth class
#define I2S_BUFFER_LENGTH 512
#define SAMPLE_BYTES sizeof(int32_t)
#define BUFFER_BYTES (I2S_BUFFER_LEN * SAMPLE_BYTES)
#define I2S_NUM 0
#define I2S_SAMPLE_RATE 44100

class App {
public:
    App();
    ~App() = default;

    void init();
    void main();
    
    // task functions
    void wifiTask();
    void audioTask();
    void ioTask();
    // aboves have to be public because freertos's task api is c-based

private:

    // static wrappers for the non-static above functions
    static void wifiTaskTrampoline(void* args);
    static void audioTaskTrampoline(void* args);
    static void ioTaskTrampoline(void* args);

    static void IRAM_ATTR i2sDmaIsr(void* args); // buffer swap reporter
    void i2sInit(); // TODO: also move this somewhere else I think

    // member classes
    WifiManager wifiManager;
    Oled oled;
    Synth synth;

    // for i2s. synth class writes to this and the oled class reads
    int32_t i2sBufferA[I2S_BUFFER_LENGTH] = {0};
    int32_t i2sBufferB[I2S_BUFFER_LENGTH] = {0};
    // if i2s gets moved move this elsewhere too

    static constexpr atomic_uint_fast8_t frontIndex = 0; // current i2s buffer to read
    static constexpr atomic_uint_fast32_t bufferSequence = 0;
    static constexpr TaskHandle_t oledTaskHandle = NULL; // for reading
    static constexpr intr_handle_t i2sIsrHandle = NULL; // for reporting buffer swaps

};
