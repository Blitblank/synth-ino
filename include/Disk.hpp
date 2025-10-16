
#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "vector" // TODO: use fixed arrays instead

// i dont like this being here but whatever
struct WiFiNetwork {
	String ssid;
	String password;
	int priority;
};

class Disk {
public:

    Disk();
    ~Disk() = default;

    void init();
    bool parseNetworkLine(const String &line, WiFiNetwork &net);
    void editNetworkFile(const std::vector<WiFiNetwork> &nets, const char *path);

private:

    // TODO: add these pins to the arduino_pins.h file
    const uint8_t chipSelect = 6;
    const uint8_t dataOut = 4;
    const uint8_t dataIn = 5;
    const uint8_t clock = 7;
    const uint8_t cardDetect = 8;

};

