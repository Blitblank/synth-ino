
#pragma once

// starts wifi -> connects to network
// http api that receives voice parameters
// changes voice state

/* TODO FOR SOBURG:
    - read html page from sd card
    - read wifi network ssid and password from sd card
    - two way websocket binding (hardware changes sliders on the web interfac)
    - led that shows active webserver needs to be accurate
*/

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_websocket_client.h"
#include <string.h>

#include "synth.h"

#define WIFI_SSID "attinternet"
#define WIFI_PASS "homeburger#sama"

static const char *TAG = "WEB";

static httpd_handle_t server = NULL;
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static const char *slider_html =
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

void wifi_init();

void start_webserver();

esp_err_t ws_handler(httpd_req_t *req);
esp_err_t root_get_handler(httpd_req_t *req);
