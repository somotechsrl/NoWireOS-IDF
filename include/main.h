#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>     
#include "driver/gpio.h"
#include "sdkconfig.h"


#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include <esp_chip_info.h>
#include "revision.h"

//#include "10-json-encoder.hpp"
//#include "00-prototypes.h"

#define BUFSIZE 1024
#define BUFTINY 128
#define MODBUS_CONFIGS 50

#define OK "OK"
#define KO "KO"
#define SEP "|"
#define RESULT "result"

extern char ipaddr[16];
extern char mac_str[20];
extern char serial_str[16];
extern esp_chip_info_t chip_info;

extern bool led_blink_enabled;
