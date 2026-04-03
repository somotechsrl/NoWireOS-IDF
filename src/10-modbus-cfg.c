#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-json-encoder.h"
#include "10-core-mqtt.h"

// Modbus configuration entry
typedef struct {
  char tag[32];
  char ad[32];
  uint16_t rs;
  uint8_t fn, rn;
} cfg_call;

// Modbus Configuration block
typedef struct {
  uint8_t ncalls;
  // max MODBUS_CONFIG  calls... -- see HAL.h
  cfg_call calls[MODBUS_CONFIGS];
} modbus_config;

static modbus_config modbus_cfg;

void add_modbus_cfg_call(const char *tag, const char *ad, uint16_t rs, uint8_t fn, uint8_t rn) {
  if (modbus_cfg.ncalls >= MODBUS_CONFIGS) {
    ESP_LOGW(TAG, "Maximum number of Modbus calls reached");
    jsonAddObject_string("value", "Maximum number of Modbus calls reached");
    return;
  }

  // checks if tag already exists
  for (uint8_t i = 0; i < modbus_cfg.ncalls; i++) {
    if (strcmp(modbus_cfg.calls[i].tag, tag) == 0 && strcmp(modbus_cfg.calls[i].ad, ad) == 0) {
      ESP_LOGW(TAG, "Modbus call with tag '%s' and address '%s' already exists", tag, ad);
      jsonAddObject_string("value", "Modbus call with tag '%s' and address '%s' already exists", tag, ad);
      return;
    }
  }

  cfg_call *call = &modbus_cfg.calls[modbus_cfg.ncalls++];
  strncpy(call->tag, tag, sizeof(call->tag) - 1);
  strncpy(call->ad, ad, sizeof(call->ad) - 1);
  call->rs = rs;
  call->fn = fn;
  call->rn = rn;
}

cfg_call *next_modbus_cfg_call() {
  static uint8_t index = 0;
  if (index < modbus_cfg.ncalls) {
    return &modbus_cfg.calls[index++];
  } else {
    index = 0; // reset for next iteration
    return NULL;
  }
}