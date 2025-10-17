
#include "WifiManager.hpp"

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

void WifiManager::init(Disk* disk, Adafruit_MCP23X17* io) {

    io = &mcp;
    mcp.digitalWrite(1, HIGH);

    setupEvents();
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    // TODO: move file related tasks to disk class
    // probably call the function and return a vector
    const char *path = "wifi-networks/networks.txt";
    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open networks file!");
        //return;
    }

    // fill vector according to file. might move this according to comment above
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

    // sort vector by priorty (priority 1=first)
    std::sort(networks.begin(), networks.end(), [](const WiFiNetwork &a, const WiFiNetwork &b) {
        return a.priority < b.priority;
    });

    networks.push_back(WiFiNetwork{"attinternet", "homeburger#sama", 1});

    // connect to wifi networks in order
    bool connected = false;
    int connectedIndex = -1;
    for (size_t i = 0; i < networks.size(); ++i) {
        if (connectWiFi(networks[i])) {
            connected = true;
            connectedIndex = i;
            Serial.printf("Connected to %s\n", networks[i].ssid.c_str());
            break;
        }
    }

    if (connected) {
        // Move the connected network to the top of the list
        if (connectedIndex > 0) {
        WiFiNetwork successful = networks[connectedIndex];
        networks.erase(networks.begin() + connectedIndex);
        networks.insert(networks.begin(), successful);

        // Rewrite the file to make this one highest priority next time
        disk->editNetworkFile(networks, path);
        Serial.println("Updated network priority order on SD card.");
        }
    } else {
        Serial.println("Failed to connect to any network.");
    }

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

bool WifiManager::connectWiFi(const WiFiNetwork &net) {

    Serial.printf("Trying to connect to SSID: %s\n", net.ssid.c_str());
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
        Serial.printf("\nConnected. IP: %s\n", WiFi.localIP().toString().c_str());
        lastNetwork = net; 
    }

    return success == WL_CONNECTED;
}

void WifiManager::setupEvents() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Serial.println("WiFi disconnected!");
                mcp.digitalWrite(1, LOW);
                break;

            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("WiFi connected.");
                break;

            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Serial.printf("Got IP: %s\n", WiFi.localIP().toString().c_str());
                mcp.digitalWrite(1, HIGH);
                break;

            default:
                break;
        }
    });
}
