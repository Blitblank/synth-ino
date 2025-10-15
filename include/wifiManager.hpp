
#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include "Disk.hpp"

struct ControlState {
  float sliders[5];    // s1..s5
  uint32_t dropdowns[4]; // d1..d4
};

class WifiManager {
public:
	WifiManager();
    //WifiManager(Disk* disk_);
    ~WifiManager() = default;

    void init(Disk* disk);
    void getControlState(ControlState* out);

private:

    void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void parsePayload(const char *payload, size_t len);
    void startWeb();
    bool connectWiFi(const WiFiNetwork &net);

    const char* taskHandle = "WIFI_TASK";

    // Shared state instance
    ControlState controlState;

    // Use a portMUX to make access atomic/cross-core safe
    bool active = false;

    // Async server and websocket
    AsyncWebServer server{ 80 };
    AsyncWebSocket ws{ "/ws" };

	// TODO: add an info logger in the webpage for status updates
};
