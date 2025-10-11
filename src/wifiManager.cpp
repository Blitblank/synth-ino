
#include "wifiManager.hpp"

// TODO: read web page from html file on sd card
const char *slider_html =
        "<!DOCTYPE html><html><body>"
        "<h2>Phase Modulation</h2>"
        "<input type='range' min='0' max='1' step='0.01' value='0.0' id='s1'>Interpolate between C1 & C2<br>"
        "<input type='range' min='40' max='400' step='0.01' value='0.0' id='s2'>pitch (40hz -> 400hz)<br>"
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

WifiManager::WifiManager() {
    // initialize control sate mainly
    for (int i = 0; i < 3; i++) controlState.sliders[i] = 0.0f;
    for (int i = 0; i < 4; i++) controlState.dropdowns[i] = 0;
    // TODO: for some reason if the filter resonance/cutoff freq (i think its cutoff) goes to zero it kills the filter
    controlState.sliders[3] = 0.5f;
    controlState.sliders[4] = 0.5f;
}

void WifiManager::init() {
    connectWiFi();
    startWeb();
}

void WifiManager::handleWsEvent(AsyncWebSocket* serverPtr, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

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
    parsePayload(payload.c_str(), payload.size());
  }
}

void WifiManager::parsePayload(const char *payload, size_t len) {
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

    for (int i = 0; i < 5; ++i) controlState.sliders[i] = temp_sliders[i];
    for (int i = 0; i < 4; ++i) controlState.dropdowns[i] = temp_dd[i];

    /*
    Serial.print("Parsed sliders: ");
    for (int i = 0; i < 5; ++i) { Serial.print(controlState.sliders[i], 3); Serial.print(" "); }
    Serial.print(" dropdowns: ");
    for (int i = 0; i < 4; ++i) { Serial.print(controlState.dropdowns[i]); Serial.print(" "); }
    Serial.println();
    */
}

void WifiManager::getControlState(ControlState* out) {
    if(!active) return;
    *out = this->controlState;
}

void WifiManager::startWeb() {
    // serve html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", slider_html);
    });

    // bind websocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
        {
            this->handleWsEvent(server, client, type, arg, data, len);
        });
    server.addHandler(&ws);

    server.begin();
    Serial.println("HTTP server started.");
    active = true;
}

void WifiManager::connectWiFi() {
    Serial.printf("WiFi connecting to '%s' ...\n", networkSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(networkSsid, networkPassword);

    // wait for connection
    unsigned long start = xTaskGetTickCount();
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(200);
        Serial.print(".");
        if ((xTaskGetTickCount() - start) > 15000) {
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
