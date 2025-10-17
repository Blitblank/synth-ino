
#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include "Disk.hpp"
#include <Adafruit_MCP23X17.h>

struct ControlState {
  float sliders[5];    // s1..s5
  uint32_t dropdowns[4]; // d1..d4
};

class WifiManager {
public:
	WifiManager();
    //WifiManager(Disk* disk_);
    ~WifiManager() = default;

    void init(Disk* disk, Adafruit_MCP23X17* io);
    void getControlState(ControlState* out);

private:

    void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void parsePayload(const char *payload, size_t len);
    void startWeb();
	void setupEvents();
    bool connectWiFi(const WiFiNetwork &net);

    const char* taskHandle = "WIFI_TASK";

	// for handling reconnects
	bool active = false;
    WiFiNetwork lastNetwork;
    bool reconnecting = false;
	bool wifiReady = false;

    // Shared state instance
    ControlState controlState;

    // Async server and websocket
    AsyncWebServer server{ 80 };
    AsyncWebSocket ws{ "/ws" };

    Adafruit_MCP23X17 mcp;

	// TODO: add an info logger in the webpage for status updates
	// a console side by side a terminal would be so cool
	// we could have a whole cli interactable through a web page 
};
