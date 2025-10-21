
#pragma once

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "vector" // TODO: use fixed arrays instead
#include "LittleFS.h"
#include "Adafruit_MCP23X17.h"

// i dont like this being here but whatever
struct WifiNetwork {
	String ssid;
	String password;
};

class Disk {
public:

    Disk();
    ~Disk() = default;

    // TODO: abstract file system operations for the option of using littlefs or the sd card
    void init(Adafruit_MCP23X17* io);
    void getNetworks(std::vector<WifiNetwork>* networks);
    void editNetworkFile(const std::vector<WifiNetwork> &nets);

private:

	Adafruit_MCP23X17* mcp;
	
    const char *path = "/wifi-networks.txt";

    bool parseNetworkLine(const String &line, WifiNetwork &net);
};

