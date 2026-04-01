#include "main.h"
#include "10-core-wifi.h"
#include "10-core-mbtp.h"
#include "10-core-mqtt.h"

// mac address array and string
uint8_t mac[6];
char mac_str[20];
char serial_str[16];
esp_chip_info_t chip_info;

void app_main() {

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
    

    printf("Device Serial: %s\n", mac_str);
    printf("%s\n", "Hello, NoWireOS!");

    // Initialize Wi-Fi and MQTT
    wifi_init();
    mqtt_init();
    modbus_init();

}