#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64
#define PROVISION_STORAGE_NS "wifi_provision"

typedef struct {
    char ssid[WIFI_SSID_MAX];
    char password[WIFI_PASS_MAX];
    bool provisioned;
} wifi_config_t;

static wifi_config_t wifi_cfg = {0};

static esp_err_t provision_handler(httpd_req_t *req) {
    if (req->method == HTTP_POST) {
        char buffer[256];
        int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
            return ESP_FAIL;
        }

        buffer[ret] = '\0';
        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }

        cJSON *ssid_item = cJSON_GetObjectItem(json, "ssid");
        cJSON *pass_item = cJSON_GetObjectItem(json, "password");

        if (ssid_item && pass_item) {
            strncpy(wifi_cfg.ssid, ssid_item->valuestring, WIFI_SSID_MAX - 1);
            strncpy(wifi_cfg.password, pass_item->valuestring, WIFI_PASS_MAX - 1);
            wifi_cfg.provisioned = true;

            wifi_provision_save_config();
            wifi_provision_connect();

            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, "{\"status\":\"success\"}");
        } else {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing credentials");
        }

        cJSON_Delete(json);
        return ESP_OK;
    }

    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
    return ESP_FAIL;
}

void wifi_provision_save_config(void) {
    nvs_handle_t handle;
    nvs_open(PROVISION_STORAGE_NS, NVS_READWRITE, &handle);
    nvs_set_str(handle, "ssid", wifi_cfg.ssid);
    nvs_set_str(handle, "password", wifi_cfg.password);
    nvs_commit(handle);
    nvs_close(handle);
}

void wifi_provision_load_config(void) {
    nvs_handle_t handle;
    if (nvs_open(PROVISION_STORAGE_NS, NVS_READONLY, &handle) == ESP_OK) {
        size_t size = WIFI_SSID_MAX;
        nvs_get_str(handle, "ssid", wifi_cfg.ssid, &size);
        size = WIFI_PASS_MAX;
        nvs_get_str(handle, "password", wifi_cfg.password, &size);
        nvs_close(handle);
        wifi_cfg.provisioned = (strlen(wifi_cfg.ssid) > 0);
    }
}

void wifi_provision_connect(void) {
    wifi_sta_config_t sta_cfg = {0};
    strncpy((char *)sta_cfg.ssid, wifi_cfg.ssid, sizeof(sta_cfg.ssid) - 1);
    strncpy((char *)sta_cfg.password, wifi_cfg.password, sizeof(sta_cfg.password) - 1);

    wifi_config_t cfg = {
        .sta = sta_cfg,
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg);
    esp_wifi_connect();
}

httpd_handle_t wifi_provision_start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t provision_uri = {
            .uri = "/api/provision",
            .method = HTTP_POST,
            .handler = provision_handler,
        };
        httpd_register_uri_handler(server, &provision_uri);
    }

    return server;
}

void wifi_provision_init(void) {
    nvs_flash_init();
    wifi_provision_load_config();
    wifi_provision_start_webserver();

    if (wifi_cfg.provisioned) {
        wifi_provision_connect();
    }
}