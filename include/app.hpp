
#pragma once

#include <atomic>

#include "synth.hpp"
#include "state.hpp"
#include "wifiManager.hpp"
#include "oled.hpp"
#include "utils.h"
#include <driver/i2s.h>

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

    void i2sInit();

    // audio parameters
    static constexpr uint32_t i2sBufferLength = 1024;
    static constexpr uint32_t sampleBytes = sizeof(int32_t);
    static constexpr uint32_t bufferBytes = i2sBufferLength * sampleBytes;
    static constexpr uint32_t i2sSampleRate = 44100;
    static constexpr uint32_t dmaBufferCount = 2;
    i2s_port_t i2sPort = I2S_NUM_0;
    int32_t i2sBuffer[i2sBufferLength] = {0};

    // variables for sharing between classes that run on different tasks
    uint32_t scopeTrigger = 0;
    uint32_t scopeWavelength = 0;
    
    // member classes
    WifiManager wifiManager;
    Oled oled;
    Synth synth{ i2sBufferLength, i2sSampleRate };

    volatile uint32_t spinlock1 = 0;
};
