#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "driver/gpio.h"

#define TAG "WiFi-Provision"
#define RESET_BUTTON_GPIO GPIO_NUM_0
#define RESET_BUTTON_HOLD_TIME 3000  // 3 seconds

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi Started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Credentials received: SSID=%s", sta_cfg->ssid);
                break;
            }
            case WIFI_PROV_CRED_FAIL:
                ESP_LOGE(TAG, "Provisioning failed");
                break;
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning ended");
                break;
            default:
                break;
        }
    }
}

static void reset_button_task(void *arg) {
    uint32_t button_press_time = 0;
    
    while (1) {
        if (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
            button_press_time = xTaskGetTickCount();
            while (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            uint32_t hold_time = (xTaskGetTickCount() - button_press_time) * portTICK_PERIOD_MS;
            
            if (hold_time > RESET_BUTTON_HOLD_TIME) {
                ESP_LOGI(TAG, "Reset button pressed for %lu ms - resetting WiFi", hold_time);
                wifi_prov_mgr_disable_auto_stop(500);
                wifi_prov_mgr_deinit();
                esp_wifi_restore();
                nvs_flash_erase();
                esp_restart();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void wifi_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    
    // Initialize provisioning manager
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
    };
    
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    
    if (!provisioned && esp_wifi_get_mode() != WIFI_MODE_STA) {
        ESP_LOGI(TAG, "Starting provisioning...");
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, NULL, "WiFi-Prov", NULL));
    } else {
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    
    // Initialize reset button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    
    xTaskCreate(reset_button_task, "reset_btn", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

