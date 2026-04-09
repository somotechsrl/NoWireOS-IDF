#include "main.h"
#include "10-wifi.h"
#include "10-mqtt.h"
#include "20-modbus-master.h"
#include "20-wifi-provision.h"
#include "30-utils.h"

void app_main() {

 
    esp_init();

    printf("Device Serial: %s\n", mac_str);
    printf("%s\n", "Hello, NoWireOS!");

    wifi_init();
    //wifi_init_hard();

    // network elated
    mqtt_init();
    modbus_init();
 
    // blinker
    led_blink_init();
    
 
}