#include "main.h"
#include "10-core-wifi.h"
#include "10-core-mqtt.h"
#include "10-modbus-tcp.h"
#include "10-modbus-rtu.h"
#include "30-utils.h"


void app_main() {

 
    esp_init();

    printf("Device Serial: %s\n", mac_str);
    printf("%s\n", "Hello, NoWireOS!");

    // Initialize Wi-Fi and MQTT
    wifi_init();
    mqtt_init();
    modbus_tcp_init();
    //modbus_rtu_init();


    // start web_server
    //start_web_server();

    // blinker
    led_blink_init();
    
 
}