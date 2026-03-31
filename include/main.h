#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
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

//#include "10-json-encoder.hpp"
//#include "00-prototypes.h"

#define BUFSIZE 2048
#define MODBUS_CONFIGS 50

extern char mac_str[];

// Modbus configuration entry
typedef struct {
  char tag[32];
  char ad[32];
  uint16_t rs;
  uint8_t fn, rn;
} cfg_call;

// Modbus Configuration block
struct {
  uint8_t ncalls;
  // max MODBUS_CONFIG  calls... -- see HAL.h
  cfg_call calls[MODBUS_CONFIGS];
} modbus_cfg;

//extern cfg cfg;