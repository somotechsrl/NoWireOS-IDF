#include "main.h"
#include "10-core-wifi.h"
#include "10-core-mqtt.h"
#include "20-modbus-svr.h"
#include "30-utils.h"

void app_main() {

 
    esp_init();

    printf("Device Serial: %s\n", mac_str);
    printf("%s\n", "Hello, NoWireOS!");

    wifi_init_sta();
    mqtt_init();
    modbus_init();
 
    // start web_server
    //start_web_server();

    // blinker
    led_blink_init();
    
 
}