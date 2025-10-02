
#pragma once

class WifiManager {
public:
    WifiManager();
    ~WifiManager() = default;

    void init();
    void update();

private:

    const char* taskHandle = "WIFI_TASK";

    // TODO: implement http requests
    // TODO: implement websockets
    // TODO: implement reading from sd card network info

};
