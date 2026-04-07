#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include <string.h>

static httpd_handle_t server = NULL;

// WiFi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI("WIFI", "Connected to WiFi");
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI("WIFI", "Disconnected from WiFi");
        }
    }
}

// GET endpoint to scan networks
static esp_err_t scan_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {0};
    esp_wifi_scan_start(&scan_config, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    wifi_ap_record_t *ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_count, ap_list);

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", (char *)ap_list[i].ssid);
        cJSON_AddNumberToObject(item, "rssi", ap_list[i].rssi);
        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free(json_str);
    free(ap_list);
    cJSON_Delete(root);

    return ESP_OK;
}

// POST endpoint to connect to WiFi
static esp_err_t connect_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf));
    if (ret <= 0) return ESP_FAIL;

    cJSON *json = cJSON_Parse(buf);
    const char *ssid = cJSON_GetObjectItem(json, "ssid")->valuestring;
    const char *password = cJSON_GetObjectItem(json, "password")->valuestring;

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    httpd_resp_send(req, "{\"status\":\"connecting\"}", -1);
    cJSON_Delete(json);

    return ESP_OK;
}

void wifi_interface_init(void) {
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    httpd_uri_t scan_uri = {
        .uri = "/api/scan",
        .method = HTTP_GET,
        .handler = scan_handler,
    };

    httpd_uri_t connect_uri = {
        .uri = "/api/connect",
        .method = HTTP_POST,
        .handler = connect_handler,
    };

    httpd_register_uri_handler(server, &scan_uri);
    httpd_register_uri_handler(server, &connect_uri);
}