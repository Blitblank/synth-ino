
#include "WifiManager.hpp"

WifiManager::WifiManager() {
    // initialize control sate mainly
    for (int i = 0; i < 3; i++) controlState.sliders[i] = 0.0f;
    for (int i = 0; i < 4; i++) controlState.dropdowns[i] = 0;
    controlState.sliders[3] = 0.5f;
    controlState.sliders[4] = 0.5f;
}

void WifiManager::init(Disk* disk, Adafruit_MCP23X17* io) {

    mcp = io;

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
    case WS_EVT_CONNECT:
        Serial.printf("WS: client #%u connected\n", client->id());
        mcp->digitalWrite(STATUS_LED_3, HIGH;
        // TODO: send current synth config values to sync with client
        break;

    case WS_EVT_DISCONNECT:
        Serial.printf("WS: client #%u disconnected\n", client->id());
        mcp->digitalWrite(STATUS_LED_3, LOW);
        break;

    case WS_EVT_DATA:
        AwsFrameInfo *info = (AwsFrameInfo*)arg;

        if (info->final && info->index == 0 && info->len == len) { // full ws message
            std::string payload((char*)data, len);
            parsePayload(payload.c_str(), payload.size());
        } 
        else { // multi-frame message needs to be assembled
            static std::string buffer;
            if (info->index == 0) buffer.clear();
            buffer.append((char*)data, len);
            if (info->final) {
                parsePayload(buffer.c_str(), buffer.size());
                buffer.clear();
            }
        }
        break;

    case WS_EVT_PONG:
        Serial.println("WS: Received PONG");
        break;
        
    }
}

// TODO: i think having the payload be a json and using a json parsing library would make this a lot cleaner
void WifiManager::parsePayload(const char *payload, size_t len) {

    std::string s(payload, len);
    // example payload: "50,50,50,50,50;1,2,3,4"
    size_t semi = s.find(';');
    if (semi == std::string::npos) {
        Serial.println("WS parse: no ';' found."); // payload malformed (i stole this)
        return;
    }
    std::string s_part = s.substr(0, semi);
    std::string d_part = s.substr(semi + 1);

    // parse sliders
    float temp_sliders[5] = {0};
    {
        size_t pos = 0;
        size_t idx = 0;
        while (pos < s_part.size() && idx < 5) {
            size_t comma = s_part.find(',', pos);
            std::string token;
            if (comma == std::string::npos) {
                token = s_part.substr(pos);
                pos = s_part.size();
            } else {
                token = s_part.substr(pos, comma - pos);
                pos = comma + 1;
            }
            // try parse float
            char *endptr = nullptr;
            float v = strtof(token.c_str(), &endptr);
            if (endptr == token.c_str()) {
                v = 0.0f; // parse error
            }
            temp_sliders[idx++] = v;
        }
    }

    // parse dropdowns
    uint32_t temp_dd[4] = {0};
    {
        size_t pos = 0;
        size_t idx = 0;
        while (pos < d_part.size() && idx < 4) {
            size_t comma = d_part.find(',', pos);
            std::string token;
            if (comma == std::string::npos) {
                token = d_part.substr(pos);
                pos = d_part.size();
            } else {
                token = d_part.substr(pos, comma - pos);
                pos = comma + 1;
            }
            uint32_t v = (uint32_t)strtoul(token.c_str(), nullptr, 10);
            temp_dd[idx++] = v;
        }
    }

    for (int i = 0; i < 5; ++i) controlState.sliders[i] = temp_sliders[i];
    for (int i = 0; i < 4; ++i) controlState.dropdowns[i] = temp_dd[i];

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

    // TODO: when the web client returns an error on accessing the websocket, it should send an http req to re-open it
    // it can be an endpoint added here

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
                Serial.printf("Got IP: %s\n", ipAddress);
                mcp->digitalWrite(STATUS_LED_2, HIGH);
                
                // TODO: maybe flash ip address on oled screen for a few seconds or until it gets an http request
                break;

            default:
                break;
        }
    });
}
