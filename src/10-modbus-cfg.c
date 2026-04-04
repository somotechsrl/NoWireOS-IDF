#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-json-encoder.h"
#include "10-core-mqtt.h"

#define TAG "MODBUS_CFG"
static modbus_config modbus_cfg;

static void add_modbus_cfg_call(const char *tag, const char *ad, uint8_t fn, uint16_t rs, uint8_t rn) {

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


static uint8_t mb_call_index = 0;

static cfg_call *reset_modbus_cfg_call() {
  mb_call_index=0; // reset index for new retrieval
  if (mb_call_index < modbus_cfg.ncalls) {
    ESP_LOGI(TAG, "Getting call #%d: %s:%s;%d;%d;%d", mb_call_index, modbus_cfg.calls[mb_call_index].tag, modbus_cfg.calls[mb_call_index].ad, modbus_cfg.calls[mb_call_index].fn, modbus_cfg.calls[mb_call_index].rs, modbus_cfg.calls[mb_call_index].rn);
    return &modbus_cfg.calls[mb_call_index++];
  } else {
    return NULL;
  }
}

static cfg_call *next_modbus_cfg_call() {
  if (mb_call_index < modbus_cfg.ncalls) {
    ESP_LOGI(TAG, "Getting call #%d: %s:%s;%d;%d;%d", mb_call_index, modbus_cfg.calls[mb_call_index].tag, modbus_cfg.calls[mb_call_index].ad, modbus_cfg.calls[mb_call_index].fn, modbus_cfg.calls[mb_call_index].rs, modbus_cfg.calls[mb_call_index].rn);
    return &modbus_cfg.calls[mb_call_index++];
  } else {
    mb_call_index = 0; // reset for next iteration
    return NULL;
  }
}

// Modbus client task, will loop through calls in config and execute them, then send json result to mqtt
static void modbus_client_task(void *pvParameters) {

    static char server_type[32]; // rtu, tcp
    static char server_host[BUFTINY]; // serial,speed,sb,parity || server FQDN or IP
    static uint16_t server_port,server_mbaddr; 

    // replace loop with connector loop RTU/TCP
    while (true) {

        jsonInit();
        jsonAddObject_string("DEV","contrel-emm");
        jsonAddObject_string("BUS",MODBUS_TCP_DEFAULT_HOST);
        jsonAddObject_string("CHN","modbus");
        jsonAddObject("data");

        for(cfg_call *call = reset_modbus_cfg_call(); call != NULL; call = next_modbus_cfg_call()) {
 
            ESP_LOGI(TAG, "Processing Modbus TCP call: %s:%s:%d:%d:%d", call->tag, call->ad, call->fn, call->rs, call->rn);
            // extract modbus call parameters from call->ad 
            if(sscanf(call->ad, "%s:%s:%hu:%hu", server_type, server_host, &server_port, &server_mbaddr) != 4) {
                ESP_LOGE(TAG, "Failed to parse Modbus TCP call address: %s", call->ad);
                continue;
                }

            // TCP Network call
            if(strcmp(server_type, "tcp") == 0) {
                int sock = modbus_tcp_connect(server_host, server_port);
                if (sock >= 0) {
                    modbus_tcp_read_json(sock, call->fn, call->rs, call->rn);
                    modbus_tcp_disconnect(sock);
                } else {
                    ESP_LOGW(TAG, "Failed to connect to Modbus server for call: %s", call->tag);
                }
                close(sock);
                continue;

            // RTU Serial call - not implemented yet, placeholder for future expansion
            } if(strcmp(server_type, "rtu") == 0) {
                // RTU client call - not implemented yet, placeholder for future expansion
                ESP_LOGW(TAG, "Modbus RTU client not implemented yet for call: %s", call->tag);
                continue; 
            }

            ESP_LOGW(TAG, "Unsupported Modbus call address: %s", call->ad);
            }
        }

        jsonCloseAll();        

        // logs json, and base 64 encrypted json
        ESP_LOGI(TAG,"%s",jsonGetBuffer());
        ESP_LOGI(TAG,"%s",jsonGetBase64());

        mqtt_send_up_data(jsonGetBase64());
        vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_RETRY_DELAY_MS));
    }
}

void modbus_init(void) {
    xTaskCreate(modbus_client_task, "modbus_client", 4096, NULL, 5, NULL);
}
