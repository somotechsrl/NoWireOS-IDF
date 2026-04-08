#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <string.h>

#define WIFI_SSID "NoWireOS-AP"
#define WIFI_PASS "12345678"

static httpd_handle_t server = NULL;

static esp_err_t wifi_config_get_handler(httpd_req_t *req) {
    const char resp[] = "<!DOCTYPE html><html><head><title>WiFi Config</title></head><body>"
        "<h1>WiFi Configuration</h1>"
        "<form action='/wifi/config' method='post'>"
        "SSID: <input type='text' name='ssid' required><br>"
        "Password: <input type='password' name='password'><br>"
        "<input type='submit' value='Connect'>"
        "</form></body></html>";
    
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    buf[ret] = '\0';
    
    wifi_config_t wifi_config = {};
    sscanf(buf, "ssid=%255[^&]&password=%255s", 
           (char*)wifi_config.sta.ssid, 
           (char*)wifi_config.sta.password);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    httpd_resp_send(req, "Connecting...", -1);
    return ESP_OK;
}

static void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/",
        .method = HTTP_GET,
        .handler = wifi_config_get_handler
    });
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/wifi/config",
        .method = HTTP_POST,
        .handler = wifi_config_post_handler
    });
    httpd_start(&server, &config);
}

void wifi_ap_init(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
    
    start_webserver();
}