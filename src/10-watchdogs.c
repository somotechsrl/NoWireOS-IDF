#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "WATCHDOG";

#define WATCHDOG_TIMEOUT_MS 30000  // 30 seconds
#define CHECK_INTERVAL_MS 5000     // Check every 5 seconds

static int64_t last_ip_update = 0;
static int64_t last_mqtt_update = 0;
static bool ip_connection_active = false;
static bool mqtt_connection_active = false;

void watchdog_feed_ip(void) {
    last_ip_update = esp_timer_get_time() / 1000;
    ip_connection_active = true;
}

void watchdog_feed_mqtt(void) {
    last_mqtt_update = esp_timer_get_time() / 1000;
    mqtt_connection_active = true;
}

static void watchdog_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
        
        int64_t now = esp_timer_get_time() / 1000;
        
        // Check IP watchdog
        if (ip_connection_active && (now - last_ip_update) > WATCHDOG_TIMEOUT_MS) {
            ESP_LOGW(TAG, "IP connection stale - timeout!");
            ip_connection_active = false;
            // TODO: Handle IP reconnection
        }
        
        // Check MQTT watchdog
        if (mqtt_connection_active && (now - last_mqtt_update) > WATCHDOG_TIMEOUT_MS) {
            ESP_LOGW(TAG, "MQTT connection stale - timeout!");
            mqtt_connection_active = false;
            // TODO: Handle MQTT reconnection
        }
    }
}

void watchdog_init(void) {
    xTaskCreate(watchdog_task, "watchdog", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Watchdog initialized");
}