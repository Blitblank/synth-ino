
#pragma once

#include "Arduino.h"
#include <stdint.h>

namespace utils {

static void serialLog(const char* handle, uint32_t timestamp, char* message) {

    // TODO: make equal spacings
    Serial.print(handle);
    Serial.print(", ");
    Serial.print(timestamp);
    Serial.print(":  ");
    Serial.print(message);
    Serial.print("\n");

    return;
}

} // namespace utils
