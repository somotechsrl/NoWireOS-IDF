#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"

static const char *TAG = "wifi_interface";
static const char *NVS_NAMESPACE = "wifi_cfg";
static const char *DEFAULT_AP_SSID = "NoWireOS_Config";
static const char *DEFAULT_AP_PASS = ""; // open AP

static esp_http_server_handle_t server = NULL;

static esp_err_t load_wifi_config(char *ssid, size_t ssid_size, char *password, size_t pass_size)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(handle, "ssid", ssid, &ssid_size);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, "password", password, &pass_size);
    }
    nvs_close(handle);
    return err;
}

static esp_err_t save_wifi_config(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_str(handle, "ssid", ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "password", password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static esp_err_t reset_wifi_config(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_key(handle, "ssid");
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_erase_key(handle, "password");
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && isxdigit(a) && isxdigit(b)) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    }
}

static void start_wifi_ap(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, NULL);

    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = strlen(DEFAULT_AP_SSID),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    memcpy(ap_config.ap.ssid, DEFAULT_AP_SSID, strlen(DEFAULT_AP_SSID));
    if (strlen(DEFAULT_AP_PASS) > 0) {
        ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        strcpy((char *)ap_config.ap.password, DEFAULT_AP_PASS);
    }

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Wi-Fi AP started. SSID:%s", DEFAULT_AP_SSID);
}

static void start_wifi_sta_if_saved(void)
{
    char ssid[64];
    char password[64];
    if (load_wifi_config(ssid, sizeof(ssid), password, sizeof(password)) != ESP_OK) {
        ESP_LOGI(TAG, "No saved Wi-Fi credentials found");
        return;
    }

    esp_netif_create_default_wifi_sta();
    wifi_config_t sta_config = {};
    strncpy((char *)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char *)sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);

    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    esp_wifi_connect();
    ESP_LOGI(TAG, "Attempting STA connection to SSID:%s", ssid);
}

static esp_err_t send_html_page(httpd_req_t *req)
{
    char ssid[64] = "";
    char password[64] = "";
    load_wifi_config(ssid, sizeof(ssid), password, sizeof(password));

    char page[1024];
    int len = snprintf(page, sizeof(page),
        "<!DOCTYPE html>"
        "<html><head><meta charset='utf-8'><title>Wi-Fi Setup</title></head>"
        "<body>"
        "<h1>Wi-Fi Configuration</h1>"
        "<form method='POST' action='/save'>"
        "SSID:<br><input type='text' name='ssid' value='%s'><br>"
        "Password:<br><input type='password' name='password' value='%s'><br><br>"
        "<input type='submit' value='Save'>"
        "</form>"
        "<form method='GET' action='/reset' style='margin-top:20px;'>"
        "<input type='submit' value='Reset to default'>"
        "</form>"
        "<p>Connect to the AP \"%s\" to manage Wi-Fi settings.</p>"
        "</body></html>",
        ssid, password, DEFAULT_AP_SSID);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, page, len);
}

static esp_err_t root_handler(httpd_req_t *req)
{
    return send_html_page(req);
}

static esp_err_t save_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    if (total_len <= 0 || total_len > 512) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }
    char content[512] = {0};
    int ret = httpd_req_recv(req, content, total_len);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
        return ESP_FAIL;
    }
    content[ret] = '\0';

    char *ssid_ptr = strstr(content, "ssid=");
    char *pass_ptr = strstr(content, "password=");
    if (!ssid_ptr || !pass_ptr) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing fields");
        return ESP_FAIL;
    }

    char ssid[64] = {0};
    char password[64] = {0};
    strncpy(ssid, ssid_ptr + 5, sizeof(ssid) - 1);
    char *amp = strchr(ssid, '&');
    if (amp) *amp = '\0';
    strncpy(password, pass_ptr + 9, sizeof(password) - 1);

    url_decode(ssid, ssid);
    url_decode(password, password);

    if (save_wifi_config(ssid, password) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save");
        return ESP_FAIL;
    }

    const char *msg = "<html><body><h1>Saved</h1><p>Rebooting device...</p></body></html>";
    httpd_resp_send(req, msg, strlen(msg));
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t reset_handler(httpd_req_t *req)
{
    if (reset_wifi_config() != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Reset failed");
        return ESP_FAIL;
    }

    const char *msg = "<html><body><h1>Reset</h1><p>Defaults restored. Rebooting...</p></body></html>";
    httpd_resp_send(req, msg, strlen(msg));
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t hd = NULL;
    if (httpd_start(&hd, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return NULL;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(hd, &root);

    httpd_uri_t save = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(hd, &save);

    httpd_uri_t reset = {
        .uri = "/reset",
        .method = HTTP_GET,
        .handler = reset_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(hd, &reset);

    return hd;
}

void wifi_init_config(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    start_wifi_ap();
    start_wifi_sta_if_saved();
    server = start_webserver();
    if (server == NULL) {
        ESP_LOGE(TAG, "Web server failed to start");
    } else {
        ESP_LOGI(TAG, "Configuration portal running");
    }
}