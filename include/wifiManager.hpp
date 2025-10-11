
#pragma once

#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

struct ControlState {
  float sliders[5];    // s1..s5
  uint32_t dropdowns[4]; // d1..d4
};

class WifiManager {
public:
    WifiManager();
    ~WifiManager() = default;

    void init();
	void getControlState(ControlState* out);

private:

    void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void parsePayload(const char *payload, size_t len);
    void startWeb();
    void connectWiFi();

    const char* taskHandle = "WIFI_TASK";

    // TODO: read sd card for network options
    // this could have a list with networks assigned a priority and the last successful connection has highest priority
    //const char* networkSsid = "attinternet";
    //const char* networkPassword = "homeburger#sama";
    const char* networkSsid = "mcgee-2.4G";
    const char* networkPassword = "aiRey56v";

    // Shared state instance
    ControlState controlState;

    // Use a portMUX to make access atomic/cross-core safe
    bool active = false;

    // Async server and websocket
    AsyncWebServer server{ 80 };
    AsyncWebSocket ws{ "/ws" };

	// TODO: add an info logger in the webpage for status updates
};
