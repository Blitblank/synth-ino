
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_websocket_client.h"
#include <string.h>

#include "web.h"
#include "state.h"

esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, slider_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        httpd_resp_set_type(req, "application/octet-stream");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if (httpd_ws_recv_frame(req, &ws_pkt, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame length");
        return ESP_FAIL;
    }

    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to receive frame");
        free(ws_pkt.payload);
        return ESP_FAIL;
    }

    ((char *)ws_pkt.payload)[ws_pkt.len] = 0;
    //ESP_LOGI(TAG, "WebSocket received: %s", (char *)ws_pkt.payload);

    char* payload = strdup((char*)ws_pkt.payload);
    if(!payload) {
        free(ws_pkt.payload);
        return ESP_ERR_NO_MEM;
    }

    char *sliders_data = strtok(payload, ";");
    char *dropdowns_data = strtok(NULL, ";");

    float sliders[5];
    if (sliders_data) {
        char *token = strtok(sliders_data, ",");
        for (int i = 0; token && i < 5; ++i) {
            float val = atof(token);
            sliders[i] = val;
            token = strtok(NULL, ",");
        }
    }
    uint32_t dropdowns[4];
    if (dropdowns_data) {
        char *token = strtok(dropdowns_data, ",");
        for (int i = 0; token && i < 4; ++i) {
            dropdowns[i] = atoi(token);
            token = strtok(NULL, ",");
        }
    }
    
    set_slider_values(sliders);
    set_dropdown_values(dropdowns);

    free(ws_pkt.payload);
    return ESP_OK;
}

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

    start_webserver();
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGW(TAG, "Wi-Fi disconnected. Retrying...");
    esp_wifi_connect();
}

void start_webserver() {

    ESP_LOGI(TAG, "Starting Webserver...");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t root = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler
    };
    httpd_register_uri_handler(server, &root);

    httpd_uri_t ws = {
        .uri       = "/ws",
        .method    = HTTP_GET,
        .handler   = ws_handler,
        .is_websocket = true,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &ws);

    gpio_set_level(GPIO_NUM_2, 1); // turn led on once wifi setup is complete
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 20) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, BIT1);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, BIT0);

        start_webserver();
    }
}

void wifi_init() {

    ESP_ERROR_CHECK(nvs_flash_init());

    // https://github.com/espressif/esp-idf/blob/v5.4.2/examples/wifi/getting_started/station/main/station_example_main.c
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = "",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, BIT0 | BIT1, pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & BIT0) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
    } else if (bits & BIT1) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}
