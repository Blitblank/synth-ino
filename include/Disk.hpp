
#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "vector" // TODO: use fixed arrays instead
#include "LittleFS.h"
#include "Adafruit_MCP23X17.h"

// i dont like this being here but whatever
struct WiFiNetwork {
	String ssid;
	String password;
};

class Disk {
public:

    Disk();
    ~Disk() = default;

    // TODO: abstract file system operations for the option of using littlefs or the sd card
    void init(Adafruit_MCP23X17* io);
    bool parseNetworkLine(const String &line, WiFiNetwork &net);
    void editNetworkFile(const std::vector<WiFiNetwork> &nets, const char *path);

private:

	Adafruit_MCP23X17* mcp;
	

};

