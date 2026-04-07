#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"

static const char *TAG = "wifi_interface";
static httpd_handle_t webserver = NULL;

static const char *wifi_config_page =
    "<!DOCTYPE html>"
    "<html>"
    "<head><meta charset=\"UTF-8\"><title>ESP WiFi Config</title></head>"
    "<body>"
    "<h2>Configure WiFi</h2>"
    "<form method=\"POST\" action=\"/configure\">"
    "SSID:<br><input type=\"text\" name=\"ssid\" maxlength=\"32\"><br>"
    "Password:<br><input type=\"password\" name=\"pass\" maxlength=\"64\"><br>"
    "<input type=\"submit\" value=\"Save\">"
    "</form>"
    "</body>"
    "</html>";

static void url_decode(const char *src, char *dst, size_t dst_size)
{
    size_t di = 0;
    while (*src && di + 1 < dst_size) {
        if (*src == '+') {
            dst[di++] = ' ';
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], '\0' };
            dst[di++] = (char) strtol(hex, NULL, 16);
            src += 2;
        } else {
            dst[di++] = *src;
        }
        src++;
    }
    dst[di] = '\0';
}

static void parse_form_field(const char *body, const char *key, char *dst, size_t dst_size)
{
    const char *pos = strstr(body, key);
    if (!pos) {
        dst[0] = '\0';
        return;
    }
    pos += strlen(key);
    if (*pos != '=') {
        dst[0] = '\0';
        return;
    }
    pos++;
    const char *end = strchr(pos, '&');
    size_t len = end ? (size_t)(end - pos) : strlen(pos);
    char tmp[128];
    if (len >= sizeof(tmp)) {
        len = sizeof(tmp) - 1;
    }
    memcpy(tmp, pos, len);
    tmp[len] = '\0';
    url_decode(tmp, dst, dst_size);
}

static esp_err_t configure_wifi_sta(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = { 0 };
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    return esp_wifi_connect();
}

static esp_err_t get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, wifi_config_page, strlen(wifi_config_page));
    return ESP_OK;
}

static esp_err_t post_handler(httpd_req_t *req)
{
    int len = req->content_len;
    if (len <= 0 || len > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }

    char *buf = calloc(1, len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int received = 0;
    while (received < len) {
        int ret = httpd_req_recv(req, buf + received, len - received);
        if (ret <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }
        received += ret;
    }

    char ssid[33];
    char pass[65];
    parse_form_field(buf, "ssid", ssid, sizeof(ssid));
    parse_form_field(buf, "pass", pass, sizeof(pass));
    free(buf);

    if (ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }

    esp_err_t err = configure_wifi_sta(ssid, pass);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFi configuration failed");
        return ESP_FAIL;
    }

    const char *resp = "<!DOCTYPE html><html><body><h2>WiFi configured</h2>"
                       "<p>Trying to connect to the network.</p>"
                       "</body></html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

static const httpd_uri_t uri_get = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t uri_post = {
    .uri       = "/configure",
    .method    = HTTP_POST,
    .handler   = post_handler,
    .user_ctx  = NULL
};

static esp_err_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;

    if (httpd_start(&webserver, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    httpd_register_uri_handler(webserver, &uri_get);
    httpd_register_uri_handler(webserver, &uri_post);
    return ESP_OK;
}

static void wifi_init_ap(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP-WIFI-Config",
            .ssid_len = 0,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
            .ssid_hidden = 0,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_interface_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_ap();
    ESP_ERROR_CHECK(start_webserver());
    ESP_LOGI(TAG, "WiFi config interface running on SSID: ESP-WIFI-Config");
}