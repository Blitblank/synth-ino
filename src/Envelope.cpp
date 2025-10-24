
#include "Envelope.hpp"

Envelope::Envelope(uint32_t a, uint32_t d, float s, uint32_t r, float p, float v, bool log=false):
    vAttack(a), vDecay(d), vSustain(s), vRelease(r), vPeak(p), velocityResponse(v), logarithmic(log) {
    
    attack(96);
}

void Envelope::attack(uint8_t velocity) {
    currentStage = eAttack;
    tickBegin = xTaskGetTickCount();
    tickTimer = xTaskGetTickCount();
}

void Envelope::release() {
    currentStage = eRelease;
}

void Envelope::process() {

    uint32_t currentTime = xTaskGetTickCount();
    uint32_t timeElapsed = currentTime - tickBegin;

    if(timeElapsed >= vAttack && currentStage == eAttack) {
        currentStage = eDecay;
        tickBegin = currentTime;
    } else if(timeElapsed >= vDecay && currentStage == eDecay) {
        currentStage = eSustain;
        tickBegin = currentTime;
    } // otherwise stay attack

}

float Envelope::sample() {

    process();

    uint32_t currentTime = xTaskGetTickCount() - tickBegin;

    float amplitude = 0.0f;
    float velocityFactor = (float)(velocity)/(float)velocityCenter * velocityResponse;

    switch (currentStage) {
    case eAttack: {
        amplitude = (float)currentTime/(float)vAttack * vPeak * velocityFactor;
        break;
    }
    case eDecay: { // TODO: logarithmic option (useful for filter frequency)
        float peak = vPeak * velocityFactor;
        float level = vSustain * velocityFactor;
        amplitude = (float)currentTime/(float)vDecay * (level - peak) + peak;
        break;
    }
    case eSustain: {
        amplitude = vSustain * velocityFactor;
        break;
    }
    case eRelease: {
        float level = vSustain * velocityFactor;
        amplitude = level * (1 - (float)currentTime/(float)vRelease);
        break;
    }
    default: // unreachable
        amplitude = -1.0f;
        break;
    }

    return amplitude;
}
