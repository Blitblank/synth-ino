
#include "Disk.hpp"

Disk::Disk() {

}

void Disk::init(Adafruit_MCP23X17* io) {

	mcp = io;

	//SPIClass spi = SPIClass(3);
	//spi.begin(SCK, MISO, MOSI, SS);

    if (!LittleFS.begin(false)) {
        Serial.println("Filesystem not mounted, attempting to format...");
        if (!LittleFS.begin(true)) {
            Serial.println("Failed to mount filesystem even after format!");
        } else {
            mcp->digitalWrite(STATUS_LED_1, HIGH);
        }
    } else {
        mcp->digitalWrite(STATUS_LED_1, HIGH);
    }

}

void Disk::getNetworks(std::vector<WifiNetwork>* networks) {

    File file = LittleFS.open(path, FILE_READ);
    if (!file) {
        Serial.println("networks file not found");
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        WifiNetwork net;
        if (parseNetworkLine(line, net)) {
            networks->push_back(net);
        }
    }
    file.close();

    if (networks->empty()) {
        Serial.println("no valid network entries found");
        return;
    }
    return;
}

bool Disk::parseNetworkLine(const String &line, WifiNetwork &net) {
    int ssidStart = line.indexOf("{");
    int ssidEnd = line.indexOf("}", ssidStart);
    int passStart = line.indexOf("{", ssidEnd);
    int passEnd = line.indexOf("}", passStart);

    if (ssidStart == -1 || ssidEnd == -1 || passStart == -1 || passEnd == -1) return false;

    net.ssid = line.substring(ssidStart + 1, ssidEnd);
    net.password = line.substring(passStart + 1, passEnd);
    return true;
}

void Disk::editNetworkFile(const std::vector<WifiNetwork> &nets) {
	File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing!");
        return;
    }

    // erase file and reopen
    LittleFS.remove(path);
    file = LittleFS.open(path, FILE_WRITE);

    for (const auto &net : nets) {
        file.printf("ssid: {%s} password: {%s}\n", net.ssid.c_str(), net.password.c_str());
    }
    file.close();
}
