
#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "vector" // TODO: use fixed arrays instead
#include "SPIFFS.h"
#include "Adafruit_MCP23X17.h"

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

    void init(Adafruit_MCP23X17* io);
    bool parseNetworkLine(const String &line, WiFiNetwork &net);
    void editNetworkFile(const std::vector<WiFiNetwork> &nets, const char *path);

private:

	Adafruit_MCP23X17* mcp;
	

};

