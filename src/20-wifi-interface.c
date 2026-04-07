#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WiFi Config";
static httpd_handle_t server = NULL;

// HTML for WiFi configuration page
const char wifi_config_html[] = R"(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Configuration</title>
    <style>
        body { font-family: Arial; margin: 20px; }
        input { padding: 8px; margin: 5px; width: 200px; }
        button { padding: 10px 20px; background: #007bff; color: white; border: none; cursor: pointer; }
        button:hover { background: #0056b3; }
    </style>
</head>
<body>
    <h1>WiFi Configuration</h1>
    <form id="wifiForm">
        <label>SSID:</label><br>
        <input type="text" id="ssid" required><br><br>
        <label>Password:</label><br>
        <input type="password" id="password" required><br><br>
        <button type="submit">Connect</button>
    </form>
    <p id="status"></p>
    <script>
        document.getElementById('wifiForm').onsubmit = async (e) => {
            e.preventDefault();
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            const response = await fetch('/api/wifi', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password })
            });
            const result = await response.json();
            document.getElementById('status').textContent = result.message;
        };
    </script>
</body>
</html>
)";

static esp_err_t wifi_config_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, wifi_config_html, strlen(wifi_config_html));
    return ESP_OK;
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    char ssid[32] = {0}, password[64] = {0};
    sscanf(buf, "{\"ssid\":\"%31[^\"]\",\"password\":\"%63[^\"]\"}", ssid, password);

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    const char *response = "{\"message\":\"Connecting...\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

void start_wifi_config_ap(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "NoWireOS-Config",
            .password = "12345678",
            .ssid_len = 0,
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 4,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = wifi_config_get_handler,
    };
    httpd_register_uri_handler(server, &uri_get);

    httpd_uri_t uri_post = {
        .uri = "/api/wifi",
        .method = HTTP_POST,
        .handler = wifi_config_post_handler,
    };
    httpd_register_uri_handler(server, &uri_post);

    ESP_LOGI(TAG, "WiFi Config AP started. Connect to 'NoWireOS-Config'");
}