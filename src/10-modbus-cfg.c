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
  // expects params in format: tag,ad,fn,rs,rn
  char tag[32], ad[32];
  uint8_t fn, rn;
  uint16_t rs;

  if (sscanf(params, "%31[^;];%31[^;];%hhu;%hu;%hhu", tag, ad, &fn, &rs, &rn) != 5) {
    ESP_LOGW(TAG, "Invalid parameters for Modbus call: %s", params);
    jsonAddValue_printf("Invalid parameters for Modbus call: %s", params);
    return;
  }

  add_modbus_cfg_call(tag, ad, fn, rs, rn);
}

void add_modbus_cfg_call(const char *tag, const char *ad, uint8_t fn, uint16_t rs, uint8_t rn) {

  if (modbus_cfg.ncalls >= MODBUS_CONFIGS) {
    ESP_LOGW(TAG, "Maximum number of Modbus calls reached");
    jsonAddObject_printf("CFG_Error", "Maximum number of Modbus calls reached");
    return;
  }

  // checks if tag already exists
  for (uint8_t i = 0; i < modbus_cfg.ncalls; i++) {
    if (strcmp(modbus_cfg.calls[i].tag, tag) == 0 && strcmp(modbus_cfg.calls[i].ad, ad) == 0 && modbus_cfg.calls[i].fn == fn && modbus_cfg.calls[i].rs == rs && modbus_cfg.calls[i].rn == rn) {
      ESP_LOGW(TAG, "Modbus call with tag '%s;%s;%d;%d;%d' already exists", tag, ad, fn, rs, rn);
      jsonAddObject_printf("CFG_Error", "Modbus call with tag '%s;%s;%d;%d;%d' already exists", tag, ad, fn, rs, rn);
      return;
    }
  }

  cfg_call *call = &modbus_cfg.calls[modbus_cfg.ncalls++];
  strncpy(call->tag, tag, sizeof(call->tag) - 1);
  strncpy(call->ad, ad, sizeof(call->ad) - 1);
  call->rs = rs;
  call->fn = fn;
  call->rn = rn;
  jsonAddObject_printf("CFG_Done", "Modbus call added: '%s;%s;%d;%d;%d'", tag, ad, fn, rs, rn);
}

static uint8_t mb_call_index = 0;
cfg_call *next_modbus_cfg_call() {
  if (mb_call_index < modbus_cfg.ncalls) {
    ESP_LOGI(TAG, "Getting call #%d: %s:%s;%d;%d;%d", mb_call_index, modbus_cfg.calls[mb_call_index].tag, modbus_cfg.calls[mb_call_index].ad, modbus_cfg.calls[mb_call_index].fn, modbus_cfg.calls[mb_call_index].rs, modbus_cfg.calls[mb_call_index].rn);
    return &modbus_cfg.calls[mb_call_index++];
  } else {
    mb_call_index = 0; // reset for next iteration
    return NULL;
  }
}

cfg_call *get_modbus_cfg_call() {
  mb_call_index=0; // reset index for new retrieval
  if (mb_call_index < modbus_cfg.ncalls) {
    ESP_LOGI(TAG, "Getting call #%d: %s:%s;%d;%d;%d", mb_call_index, modbus_cfg.calls[mb_call_index].tag, modbus_cfg.calls[mb_call_index].ad, modbus_cfg.calls[mb_call_index].fn, modbus_cfg.calls[mb_call_index].rs, modbus_cfg.calls[mb_call_index].rn);
    return &modbus_cfg.calls[mb_call_index++];
  } else {
    return NULL;
  }
}