
#include "WifiManager.hpp"

#include "ArduinoJson.h"

WifiManager::WifiManager() {
    // initialize control sate mainly
    for (int i = 0; i < 3; i++) controlState.sliders[i] = 0.0f;
    for (int i = 0; i < 4; i++) controlState.dropdowns[i] = 0;
    controlState.sliders[3] = 12000.0f;
    controlState.sliders[4] = 0.707f;
}

void WifiManager::init(Disk* disk, Adafruit_MCP23X17* io, uint8_t* scopeBuffer) {

    mcp = io;
    scopeFrame = scopeBuffer;

    setupEvents();
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    std::vector<WifiNetwork> networks;
    disk->getNetworks(&networks);

    // connect to wifi networks in order
    bool connected = false;
    int connectedIndex = -1;
    for (size_t i = 0; i < networks.size(); ++i) {
        if (connectWiFi(networks[i])) {
            connected = true;
            connectedIndex = i;
            Serial.printf("Connected to SSID: %s\n", networks[i].ssid.c_str());
            break;
        }
    }

    if (connected) {
        // resort the networks list
        if (connectedIndex > 0) {
            WifiNetwork successful = networks[connectedIndex];
            networks.erase(networks.begin() + connectedIndex);
            networks.insert(networks.begin(), successful);

            disk->editNetworkFile(networks);
        }
    } else {
        Serial.println("Failed to connect to the configured networks."); // big fat failure
    }

    startWeb();
}

void WifiManager::handleWsEvent(AsyncWebSocket* serverPtr, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("WS: client #%u connected\n", client->id());
            mcp->digitalWrite(STATUS_LED_3, HIGH);
            // TODO: send current synth config values to sync with client
            break;
        }
        case WS_EVT_DISCONNECT: {
            Serial.printf("WS: client #%u disconnected\n", client->id());
            if(ws.count() == 0) mcp->digitalWrite(STATUS_LED_3, LOW);
            break;
        }
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;

            if (info->final && info->index == 0 && info->len == len) { // full ws message
                if (data[0] == '{') { // this means the payload is prob a json. should maybe add a character key (say &=control data)
                    parsePayload((const char*)data, len);
                }
                // only happens when theres an active websocket, and occurs on the same timestep. 
                // calling this from a loop in a task breaks the task because the library uses millis()
                if((xTaskGetTickCount() - 50) > lastScopeTime) {
                    ws.binaryAll(scopeFrame, 128);
                    lastScopeTime = xTaskGetTickCount();
                }
                
            } 

            break;
        }
        case WS_EVT_PONG: {
            Serial.println("WS: Received PONG");
            break;
        }
    }
}

void WifiManager::parsePayload(const char *payload, size_t len) {

    StaticJsonDocument<200> controlsData;
    if(deserializeJson(controlsData, payload)) {
        return; // big fat failure
    }

    for (int i = 0; i < 5; ++i) controlState.sliders[i] = controlsData["sliders"][i];
    for (int i = 0; i < 4; ++i) controlState.dropdowns[i] = controlsData["dropdowns"][i];

}

void WifiManager::getControlState(ControlState* out) {
    if(!active) return;
    *out = this->controlState;
}

void WifiManager::startWeb() {
    // serve html
    server.serveStatic("/", LittleFS, "/index/").setDefaultFile("index.html");
    server.serveStatic("/terminal", LittleFS, "/terminal/").setDefaultFile("index.html");

    server.on("/start-sequence", HTTP_GET, [](AsyncWebServerRequest *request){
        //startNoteSequence();
        request->send(200);
    });

    server.on("/stop-sequence", HTTP_GET, [](AsyncWebServerRequest *request){
        //stopNoteSequence();
        request->send(200);
    });
    
    // bind websocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
        {
            this->handleWsEvent(server, client, type, arg, data, len);
        });
    server.addHandler(&ws);

    server.begin();
    Serial.println("Http server started.");
    active = true;
}

bool WifiManager::connectWiFi(const WifiNetwork &net) {

    Serial.printf("Connecting to SSID: %s\n", net.ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    WiFi.begin(net.ssid.c_str(), net.password.c_str());
    
    uint32_t startAttempt = xTaskGetTickCount();
    while (WiFi.status() != WL_CONNECTED && ((xTaskGetTickCount() - startAttempt) < 15000)) {
        vTaskDelay(200);
        Serial.print(".");
    }
    Serial.println();
    wl_status_t success = WiFi.status();
    if(success == WL_CONNECTED) {
        lastNetwork = net; 
    }

    return success == WL_CONNECTED;
}

void WifiManager::setupEvents() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Serial.println("WiFi disconnected.");
                mcp->digitalWrite(STATUS_LED_2, LOW);
                break;

            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("WiFi connected.");
                break;

            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                ipAddress = WiFi.localIP().toString();
                Serial.printf("Got IP: %s\n", ipAddress.c_str());
                mcp->digitalWrite(STATUS_LED_2, HIGH);
                
                // TODO: maybe flash ip address on oled screen for a few seconds or until it gets an http request
                break;

            default:
                break;
        }
    });
}
