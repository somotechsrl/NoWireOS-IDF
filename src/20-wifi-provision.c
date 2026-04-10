#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"

#define WIFI_SSID_MAXLEN 32
#define WIFI_PASS_MAXLEN 64
#define AP_SSID "NoWireOS_Config"
#define AP_PASS "nowireos"
#define AP_CHANNEL 1

static const char *TAG = "wifi_prov";

static void start_webserver(void);
static void stop_webserver(void);

static httpd_handle_t server = NULL;

static esp_err_t wifi_config_post_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf));
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = 0;

    char ssid[WIFI_SSID_MAXLEN] = {0};
    char pass[WIFI_PASS_MAXLEN] = {0};
    sscanf(buf, "ssid=%31[^&]&pass=%63s", ssid, pass);

    ESP_LOGI(TAG, "Received SSID: %s, PASS: %s", ssid, pass);

    // Save credentials to NVS or use directly
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, WIFI_SSID_MAXLEN);
    strncpy((char *)wifi_config.sta.password, pass, WIFI_PASS_MAXLEN);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    httpd_resp_sendstr(req, "WiFi credentials received. Trying to connect...");
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    const char resp[] =
        "<!DOCTYPE html><html><body>"
        "<h2>WiFi Configuration</h2>"
        "<form method='POST' action='/wifi'>"
        "SSID: <input name='ssid' length=32><br>"
        "Password: <input name='pass' type='password' length=64><br>"
        "<input type='submit' value='Connect'>"
        "</form></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t uri_post = {
    .uri = "/wifi",
    .method = HTTP_POST,
    .handler = wifi_config_post_handler,
    .user_ctx = NULL
};

static void start_webserver(void) {
    ESP_LOGI(TAG, "Starting WebSever for WiFi provisioning");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");    
    }
}

static void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, starting AP fallback");
        esp_wifi_set_mode(WIFI_MODE_AP);
        wifi_config_t ap_config = {
            .ap = {
                .ssid = AP_SSID,
                .ssid_len = strlen(AP_SSID),
                .password = AP_PASS,
                .channel = AP_CHANNEL,
                .max_connection = 2,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            }
        };
        if (strlen(AP_PASS) == 0) {
            ap_config.ap.authmode = WIFI_AUTH_OPEN;
        }
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Short delay to ensure AP is up before starting server
        start_webserver();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to WiFi!");
        stop_webserver();
    }
}

void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
}