
#include "oscillator.hpp"

#include <stdint.h>

Oscillator::Oscillator() {

    // TODO: see if init() works here

}

void Oscillator::init() {
    // TODO: implement
}

int32_t Oscillator::mix(Oscillator* oscillators, size_t count) {

    // TODO: implement
    return 0;
}

int32_t Oscillator::sampleWavetable(const int32_t* wavetable, uint32_t x) {

    // TODO: implement
    return 0;
}

int32_t Oscillator::fsampleWavetable(const int32_t* wavetable, uint32_t x) {

    // TODO: implement
    return 0;
}

uint32_t Oscillator::floatToQ31(float f) {
    if (f >= 1.0f) return 0x7FFFFFFF;
    if (f <= 0.0f) return 0;
    return (uint32_t)(f * 2147483647.0f); // 2^31 - 1
}