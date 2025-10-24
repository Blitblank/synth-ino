
#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Arduino.h"

enum EnvelopeStage {
    eAttack = 0,
    eDecay,
    eSustain,
    eRelease
};

class Envelope {
public:

    Envelope(uint32_t a, uint32_t d, float s, uint32_t r, float p, float v);
    ~Envelope() = default;

    void attack(uint8_t velocity);
    void release();
    float sample();

private:
    void process();

    uint32_t vAttack = 10; // time from start to peak, ms
    uint32_t vDecay = 50; // time from peak to sustain level, ms
    float vSustain = 1.0f; // amplitude value of sustain level
    uint32_t vRelease = 1000; // time from release to 0, ms
    float vPeak = 1.0f; // amplitude value of peak
    float velocityResponse = 0.5f; // response to velocity at attack and sustain. 0 = no change in envelope to velocity, 1 = full sensitivity, 0.5 = 50% sensitivity
    // assuming 96 is the default velocity (75% fullscale)
    // 100% sensitivity: 0 velocity -> 0% peak & sustain, 96 velocity -> 100% peak & sustain, 127 velocity -> 132%
    // 50% sensitivity: 0 velocity -> 50% peak & sustain, 96 velocity -> 100% peak & sustain, 127 velocity -> 116% peak & sustain

    uint32_t tickBegin = 0;
    uint32_t tickTimer = 0;
    uint8_t currentStage = eAttack;
    uint8_t velocity = 96;

    const int8_t velocityCenter = 96;

};
