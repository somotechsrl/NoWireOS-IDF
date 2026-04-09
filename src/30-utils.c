#include "main.h"
#include "mbedtls/base64.h"


// mac address array and string
uint8_t mac[6];
char ipaddr[16];
char mac_str[20];
char serial_str[16];
esp_chip_info_t chip_info;


void esp_init() {

    esp_chip_info(&chip_info);
 
 
     // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Reads base MAC address from eFuse ad sets
    esp_read_mac(mac, ESP_MAC_BASE);
    sprintf(mac_str,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);  
    sprintf(serial_str,
        "%02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);  
    
}

#define BLINK_GPIO 2
#define BLINK_DELAY_MS 1000
bool led_blink_enabled = true;


static void led_blink(void *pvParameters) {  
    
    // Configure the GPIO pin
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Blink loop
    while (true) {

        // Turn LED ON
        //printf("LED ON\n");
        if(!led_blink_enabled) {
            vTaskDelay(BLINK_DELAY_MS/portTICK_PERIOD_MS); // Delay 500 ms
            continue;
        }   

        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay((BLINK_DELAY_MS/10)/portTICK_PERIOD_MS); // Delay 500 ms
        
        // Turn LED OFF
        //printf("LED OFF\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS); // Delay 1 second
        }
    }

void led_blink_init(void) {
    xTaskCreate(led_blink, "led_blink", 4096, NULL, 5, NULL);
    }
