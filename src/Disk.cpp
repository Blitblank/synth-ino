
#include "Disk.hpp"

Disk::Disk() {

}

void Disk::init(Adafruit_MCP23X17* io) {

	mcp = io;

	SPIClass spi = SPIClass(3);
	spi.begin(clock, dataOut, dataIn, chipSelect);
	if (!SD.begin(chipSelect, spi, 1000000)) {
		Serial.println("SD init failed");
	} else {
		mcp->digitalWrite(0, HIGH);
	}
	// TODO: notify some led that this component isnt functioning

}

bool Disk::parseNetworkLine(const String &line, WiFiNetwork &net) {
    int ssidStart = line.indexOf("{");
    int ssidEnd = line.indexOf("}", ssidStart);
    int passStart = line.indexOf("{", ssidEnd);
    int passEnd = line.indexOf("}", passStart);
    int priorityStart = line.indexOf("priority:");

    if (ssidStart == -1 || ssidEnd == -1 || passStart == -1 || passEnd == -1 || priorityStart == -1) return false;

    net.ssid = line.substring(ssidStart + 1, ssidEnd);
    net.password = line.substring(passStart + 1, passEnd);
    net.priority = line.substring(priorityStart + 9).toInt();
    return true;
}

void Disk::editNetworkFile(const std::vector<WiFiNetwork> &nets, const char *path) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing!");
        return;
    }

    // erase file and reopen
    SD.remove(path);
    file = SD.open(path, FILE_WRITE);

    for (const auto &net : nets) {
    file.printf("ssid: {%s} password: {%s} priority: %d\n",
                net.ssid.c_str(), net.password.c_str(), net.priority);
    }
    file.close();
}
