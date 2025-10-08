
#include "wifiManager.hpp"

WifiManager::WifiManager() {
    // TODO: implement
}

void WifiManager::init() {
    // TODO: implement

    // don't put this in constructor
}

void WifiManager::update() {
    // Example: show control values every second
    static unsigned long last = 0;
    if (millis() - last > 1000) {
        last = millis();
        ControlState cs;
        getControlState(cs);
        Serial.print("State: ");
        for (int i = 0; i < 5; ++i) Serial.printf("%.3f ", cs.sliders[i]);
        Serial.print(" | ");
        for (int i = 0; i < 4; ++i) Serial.printf("%u ", cs.dropdowns[i]);
        Serial.println();
    }

}

void WifiManager::onWsEvent(AsyncWebSocket* serverPtr, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS: client #%u connected\n", client->id());
    // Optionally send current values to client on connect
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS: client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    // data may be fragmented; AsyncWebSocket gives whole message by default for text frames
    // Ensure we treat as text
    std::string payload;
    payload.assign((char*)data, len);

    // parse and store
    parse_and_store_payload(payload.c_str(), payload.size());
  }
}

void WifiManager::parse_and_store_payload(const char *payload, size_t len) {
    // copy into a null-terminated buffer (payload may not be null-terminated)
    std::string s(payload, len);
    // find separator
    size_t semi = s.find(';');
    if (semi == std::string::npos) {
        Serial.println("WS parse: no ';' found.");
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

    // commit under mutex
    portENTER_CRITICAL(&g_controlMux);
    for (int i = 0; i < 5; ++i) g_controlState.sliders[i] = temp_sliders[i];
    for (int i = 0; i < 4; ++i) g_controlState.dropdowns[i] = temp_dd[i];
    portEXIT_CRITICAL(&g_controlMux);

    // Debug print
    Serial.print("Parsed sliders: ");
    for (int i = 0; i < 5; ++i) { Serial.print(g_controlState.sliders[i], 3); Serial.print(" "); }
    Serial.print(" dropdowns: ");
    for (int i = 0; i < 4; ++i) { Serial.print(g_controlState.dropdowns[i]); Serial.print(" "); }
    Serial.println();
}

// ----- API to read the state (thread-safe) -----
void WifiManager::getControlState(ControlState &out) {
    portENTER_CRITICAL(&g_controlMux);
    out = g_controlState;
    portEXIT_CRITICAL(&g_controlMux);
}

// ----- setup & start server -----
void WifiManager::startWeb() {
    // serve html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", slider_html);
    });

    // bind websocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.begin();
    Serial.println("HTTP server started.");
}

// ----- WiFi connect helper -----
void WifiManager::connectWiFi() {
    Serial.printf("WiFi connecting to '%s' ...\n", networkSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(networkSsid, networkPassword);

    // wait for connection
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        Serial.print(".");
        if ((millis() - start) > 15000) {
            Serial.println("\nWiFi connection timeout");
            break;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("Failed to connect to WiFi.");
    }
}
