
#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "vector"

// 
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

    const uint8_t chipSelect = 100;

};

