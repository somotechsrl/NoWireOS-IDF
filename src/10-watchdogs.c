#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <mqtt_client.h>
#include "esp_task_wdt.h"

static const char *TAG = "WATCHDOG";

#define PING_HOST "8.8.8.8"
#define MQTT_HOST "rpc.somotech.it"
#define WATCHDOG_INTERVAL_MS 300000 // 5 minutes
#define WATCHDOG_MAX_FAILURES 5

static int mqtt_failures=0,ping_failures=0;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            mqtt_failures=0;
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            mqtt_failures++;
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        default:
            break;
    }
}

static bool check_ping(const char *host) {

    struct sockaddr_in addr;
    struct hostent *he = gethostbyname(host);
    if (!he) return false;
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    bool result = (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    close(sock);

    if(!result) 
        ping_failures++;
    else   
        ping_failures=0;

    return result;
}

static void watchdog_task(void *pvParameters) {
    while (1) {
        bool ping_ok = check_ping(PING_HOST);
        ESP_LOGI(TAG, "Ping 8.8.8.8: %s", ping_ok ? "OK" : "FAILED");
        ESP_LOGI(TAG, "MQTT: %s", mqtt_connected ? "CONNECTED" : "DISCONNECTED");
        
        if (!ping_ok || !mqtt_connected) {
            ESP_LOGW(TAG, "Watchdog alert: Connectivity issue detected");
        }

        if(mqtt_failures>WATCHDOG_MAX_FAILURES || ping_failures>WATCHDOG_MAX_FAILURES) {
            ESP_LOGW(TAG, "Max failures reached mqtt=%d ping=%d",mqtt_failures,ping_failures);
            }
            
        // resets on hardware stale
        //esp_task_wdt_reset();
     
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_INTERVAL_MS));
    }
}

void watchdog_init(esp_mqtt_client_handle_t client) {
    mqtt_client = client;
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    xTaskCreate(watchdog_task, "watchdog", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Watchdog system initialized");

    // enable hardware watchdog
    /*
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = WATCHDOG_INTERVAL_MS,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Watch all cores
        .trigger_panic = true,                              // Reset on timeout
        };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    ESP_LOGI(TAG, "Hardware watchdog enabled");
    */
}
