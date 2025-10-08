
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
    void update();

private:

    void onWsEvent(AsyncWebSocket* serverPtr, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void parse_and_store_payload(const char *payload, size_t len);
    void getControlState(ControlState &out);
    void startWeb();
    void connectWiFi();

    const char* taskHandle = "WIFI_TASK";

    const char* networkSsid = "attinternet";
    const char* networkPassword = "homeburger#sama";

    // Shared state instance
    ControlState g_controlState;

    // Use a portMUX to make access atomic/cross-core safe
    portMUX_TYPE g_controlMux = portMUX_INITIALIZER_UNLOCKED;

    // Async server and websocket
    AsyncWebServer server{ 80 };
    AsyncWebSocket ws{ "/ws" };

    // TODO: implement http requests
    // TODO: implement websockets
    // TODO: implement reading from sd card network info

    const char *slider_html =
        "<!DOCTYPE html><html><body>"
        "<h2>Phase Modulation</h2>"
        "<input type='range' min='0' max='1' step='0.01' value='0.0' id='s1'>Interpolate between C1 & C2<br>"
        "<input type='range' min='0' max='1' step='0.01' value='0.0' id='s2'>nothing<br>"
        "<input type='range' min='0' max='1' step='0.01' value='0.0' id='s3'>Phase Modulation Depth<br>"
        "<input type='range' min='0' max='1' step='0.01' value='0.5' id='s4'>Low-Pass Filter Cutoff<br>"
        "<input type='range' min='0' max='1' step='0.01' value='0.5' id='s5'>Low-Pass Filter Resonance<br>"
        "<h3>Wave Selectors</h3>"
        "<div>Carrier 1: <select id='d1'>"
        "<option value='0'>0</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option>"
        "<option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option>"
        "</select><br></div>"
        "<div>Carrier 2: <select id='d2'>"
        "<option value='0'>0</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option>"
        "<option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option>"
        "</select><br></div>"
        "<div>Modulator 1: <select id='d3'>"
        "<option value='0'>0</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option>"
        "<option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option>"
        "</select><br></div>"
        "<div>nothing<select id='d4'>"
        "<option value='0'>0</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option>"
        "<option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option>"
        "</select><br></div>"
        "<script>"
        "let ws = new WebSocket('ws://' + location.host + '/ws');"
        "function sendSliders() {"
        "  let s = [1,2,3,4,5].map(i => document.getElementById('s'+i).value).join(',');"
        "  let d = [1,2,3,4].map(i => document.getElementById('d'+i).value).join(',');"
        "  ws.send(s + ';' + d);"
        "}"
        "setInterval(sendSliders, 100);"
        "</script>"
        "</body></html>";

};
