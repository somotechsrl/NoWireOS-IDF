#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-json-encoder.h"
#include "10-core-mqtt.h"
#include "10-modbus-cfg.h"

#define TAG "MODBUS_CFG"
static modbus_config modbus_cfg;

void addModbusCall(const char *params) {
  // Expected format: "tag,ad,rs,fn,rn"
  char tag[32], ad[32];
  uint16_t rs;
  uint8_t fn, rn;

  if (sscanf(params, "%31[^;];%31[^;];%hu;%hhu;%hhu", tag, ad, &rs, &fn, &rn) != 5) {
    ESP_LOGW(TAG, "Invalid parameters for Modbus call: %s", params);
    jsonAddObject_printf("value", "Invalid parameters for Modbus call: %s", params);
    return;
  }

  add_modbus_cfg_call(tag, ad, rs, fn, rn);
}

void add_modbus_cfg_call(const char *tag, const char *ad, uint8_t fn, uint16_t rs, uint8_t rn) {

  if (modbus_cfg.ncalls >= MODBUS_CONFIGS) {
    ESP_LOGW(TAG, "Maximum number of Modbus calls reached");
    jsonAddObject_string("value", "Maximum number of Modbus calls reached");
    return;
  }

  // checks if tag already exists
  for (uint8_t i = 0; i < modbus_cfg.ncalls; i++) {
    if (strcmp(modbus_cfg.calls[i].tag, tag) == 0 && strcmp(modbus_cfg.calls[i].ad, ad) == 0 && modbus_cfg.calls[i].fn == fn && modbus_cfg.calls[i].rs == rs && modbus_cfg.calls[i].rn == rn) {
      ESP_LOGW(TAG, "Modbus call with tag '%s' and address '%s' already exists", tag, ad);
      jsonAddObject_printf("value", "Modbus call with tag '%s' and address '%s' already exists", tag, ad);
      return;
    }
  }

  cfg_call *call = &modbus_cfg.calls[modbus_cfg.ncalls++];
  strncpy(call->tag, tag, sizeof(call->tag) - 1);
  strncpy(call->ad, ad, sizeof(call->ad) - 1);
  call->rs = rs;
  call->fn = fn;
  call->rn = rn;
  jsonAddObject_printf("value", "Modbus call added: tag='%s', ad='%s', rs=%u, fn=%u, rn=%u", tag, ad, rs, fn, rn);
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