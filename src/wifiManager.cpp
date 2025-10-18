
#include "WifiManager.hpp"

WifiManager::WifiManager() {
    // initialize control sate mainly
    for (int i = 0; i < 3; i++) controlState.sliders[i] = 0.0f;
    for (int i = 0; i < 4; i++) controlState.dropdowns[i] = 0;
    // TODO: for some reason if the filter resonance/cutoff freq (i think its cutoff) goes to zero it kills the filter
    controlState.sliders[3] = 0.5f;
    controlState.sliders[4] = 0.5f;
}

void WifiManager::init(Disk* disk, Adafruit_MCP23X17* io) {

    mcp = io;

    setupEvents();
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    // TODO: move file related tasks to disk class
    // probably call the function and return a vector
    const char *path = "/wifi-networks.txt";
    File file = LittleFS.open(path, FILE_READ);
    if (!file) {
        Serial.println("networks file not found");
    }

    // def should have a vector<Network> networks = disk.GetNetworks(); function
    std::vector<WiFiNetwork> networks;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        WiFiNetwork net;
        if (disk->parseNetworkLine(line, net)) {
            networks.push_back(net);
        }
    }
    file.close();

    if (networks.empty()) {
        Serial.println("no valid network entries found");
        //return;
    }

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
            WiFiNetwork successful = networks[connectedIndex];
            networks.erase(networks.begin() + connectedIndex);
            networks.insert(networks.begin(), successful);

            disk->editNetworkFile(networks, path);
        }
    } else {
        Serial.println("Failed to connect to the configured networks"); // big fat failure
    }

    startWeb();
}

void WifiManager::handleWsEvent(AsyncWebSocket* serverPtr, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

    if (type == WS_EVT_CONNECT) {
        Serial.printf("WS: client #%u connected\n", client->id());
        // TODO: send current synth config values to sync with client
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WS: client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        // ws payload needs to be error checked
        std::string payload;
        payload.assign((char*)data, len);

        // parse and store
        parsePayload(payload.c_str(), payload.size());
    }
}

void WifiManager::parsePayload(const char *payload, size_t len) {
    // might change this back to char* but string was was easier
    std::string s(payload, len);
    // find separator
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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    /* example for different routes. settings.html sits in root of the FS mount
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/settings.html", "text/html");
    });
    */
    
    // bind websocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
        {
            this->handleWsEvent(server, client, type, arg, data, len);
        });
    server.addHandler(&ws);

    server.begin();
    Serial.println("http server started");
    active = true;
}

bool WifiManager::connectWiFi(const WiFiNetwork &net) {

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
                Serial.println("WiFi disconnected");
                mcp->digitalWrite(1, LOW);
                break;

            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("WiFi connected");
                break;

            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Serial.printf("Got IP: %s\n", WiFi.localIP().toString().c_str());
                mcp->digitalWrite(1, HIGH);
                break;

            default:
                break;
        }
    });
}
