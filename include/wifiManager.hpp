
#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include "Disk.hpp"
#include <LittleFS.h>
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

    void init(Disk* disk, Adafruit_MCP23X17* io, uint8_t* scopeBuffer);
    void getControlState(ControlState* out);
    String getIp() { return ipAddress; }
    void pingClients() { ws.textAll("ping"); }

private:

    void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void parsePayload(const char *payload, size_t len);
    void startWeb();
	void setupEvents();
    bool connectWiFi(const WifiNetwork &net);

    const char* taskHandle = "WIFI_TASK";

	// for handling reconnects
	bool active = false;
    WifiNetwork lastNetwork;
    bool reconnecting = false;
	bool wifiReady = false;
    uint32_t lastPingTime = 0;
    uint32_t pingInterval = 10000;
    uint32_t lastScopeTime = 0;
    String ipAddress = "";

    // Shared state instance
    ControlState controlState;
    uint8_t* scopeFrame;

    // Async server and websocket
    AsyncWebServer server{ 80 };
    AsyncWebSocket ws{ "/ws" };

    Adafruit_MCP23X17* mcp;

	// TODO: add an info logger in the webpage for status updates
	// a console side by side a terminal would be so cool
	// we could have a whole cli interactable through a web page 
};
