
#pragma once

#include "synth.hpp"
#include "state.hpp"
#include "wifiManager.hpp"
#include "oled.hpp"
#include "utils.h"


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

    // member classes
    WifiManager wifiManager;
    Oled oled;
    Synth synth;

};
