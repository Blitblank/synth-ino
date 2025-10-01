
#pragma once

#include <atomic>

#include "synth.hpp"
#include "state.hpp"
#include "wifiManager.hpp"
#include "oled.hpp"
#include "utils.h"
#include <driver/i2s.h>

// TODO: I'd like this to be somewhere else, maybe an i2s manager class. next option is in the synth class


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

    static void IRAM_ATTR i2sDmaIsr(void* arg); // buffer swap reporter
    void i2sInit(); // TODO: also move this somewhere else I think

    // member classes
    WifiManager wifiManager;
    Oled oled;
    Synth synth;

    static std::atomic<int32_t> activeBuffer; // current i2s buffer to read
    static std::atomic<bool> bufferReady;

    // audio parameters
    static constexpr uint32_t i2sBufferLength = 512;
    static constexpr uint32_t sampleBytes = sizeof(int32_t);
    static constexpr uint32_t bufferBytes = i2sBufferLength * sampleBytes;
    static constexpr uint32_t i2sSampleRate = 44100;
    static constexpr uint32_t dmaBufferCount = 2;
    static intr_handle_t i2sIntrHandle;

    i2s_port_t i2sPort = I2S_NUM_0;

    // for i2s. synth class writes to this and the oled class reads
    int32_t i2sBufferA[i2sBufferLength] = {0};
    int32_t i2sBufferB[i2sBufferLength] = {0};
    // if i2s gets moved move this elsewhere too

};
