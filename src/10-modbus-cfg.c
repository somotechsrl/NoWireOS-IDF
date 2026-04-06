#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-json-encoder.h"
#include "10-core-mqtt.h"
#include "10-modbus-cfg.h"
#include "10-modbus-tcp.h"
#include "10-modbus-rtu.h"  

#define TAG "MODBUS_CFG"
TaskHandle_t modbus_client_task_handle;
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

// old system -- RPC one call for each register, new system will send all calls in config in one json block for more efficient transmission and processing, this is kept for backward compatibility with old RPC system, but will be deprecated in future in favor of addModbusAggregatedCall which will add calls to current json block for batch processing and transmission
void addModbusSingleCall(const char *params) {
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

void addModbusAggregatedCall(const char *params) {
  // expects params in format: tag,ad,fn,rs,rn
  char tag[32], ad[32];
  uint8_t fn, rn;
  uint16_t rs;
  char rs_str[BUFTINY];

  // typical format for aggregated call: tag;ad;fn;rs1:rn1,rs2:rn2,rs3:rn3,... where rs is starting register and rn is number of registers to read, allows for batch processing of multiple registers in one call for more efficient transmission and processing in modbus client task loop
  // splits single aggregated call with comma separated registers into multiple calls with same tag, ad, fn, but different rs and rn, then adds each call to config for processing in modbus client task loop 
  if (sscanf(params, "%31[^;];%31[^;];%hhu;%31[^;]s", tag, ad, &fn, rs_str) != 4) {
    ESP_LOGW(TAG, "Invalid parameters for Modbus aggregated call: %s", params);
    jsonAddValue_printf("Invalid parameters for Modbus aggregated call: %s", params);
    return; 
    }
  // explodes rs_str into individual register sets, separated by comma, in format rs:rn, then adds each call to config with same tag, ad, fn, but different rs and rn for each register set, allows for batch processing of multiple registers in one call for more efficient transmission and processing in modbus client task loop
  char *token = strtok((char *)rs_str, ",");
  while (token != NULL) {
    if (sscanf(token, "%hu:%hhu", &rs, &rn) == 2) {
      add_modbus_cfg_call(tag, ad, fn, rs, rn);
    } else {
      ESP_LOGW(TAG, "Invalid register set in parameters for Modbus call: %s", token);
      jsonAddValue_printf("Invalid register set in parameters for Modbus call: %s", token);
    }
    token = strtok(NULL, ",");
  }
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

    modbus_client_task_handle = xTaskGetCurrentTaskHandle();

    static char server_type[32]; // rtu, tcp
    static char server_host[BUFTINY]; // serial,speed,sb,parity || server FQDN or IP
    static uint16_t server_port,server_unit_id; 
    static char curr_block[BUFTINY],last_block[BUFTINY]= ""; // Block Used, for aggregating responses into same block for same server in json output

    // replace loop with connector loop RTU/TCP
    while (true) {

        for(cfg_call *call = reset_modbus_cfg_call(); call != NULL; call = next_modbus_cfg_call()) {
          // calculates current block for call based on tag and ad, used for json aggregation and mqtt topic grouping
          snprintf(curr_block, sizeof(curr_block), "%s:%s", call->tag, call->ad);
          // senses block change based on tag and ad, if same block as previous call, will aggregate into same json block, if different block, will close previous block and start new block for new server
          if(strcmp(curr_block, last_block) != 0) {
            if(strlen(last_block) > 0) {
              jsonCloseAll(); // close previous block if new block is different
              // logs json, and base 64 encryptedjson
              ESP_LOGI(TAG,"%s",jsonGetBuffer()); 
              ESP_LOGI(TAG,"%s",jsonGetBase64());
              mqtt_send_up_data(jsonGetBase64());              
              }
            // init json block for new server, if same server as previous call, will aggregate into same block
            strncpy(last_block, curr_block, sizeof(last_block) - 1);
            jsonInit();
            jsonAddObject_string("DEV",call->tag);
            jsonAddObject_string("BUS",call->ad);
            jsonAddObject_string("CHN","modbus");
            jsonAddObject("data");
            }

          ESP_LOGI(TAG, "Processing Modbus TCP call: %s:%s:%d:%d:%d", call->tag, call->ad, call->fn, call->rs, call->rn);
          // extract modbus call parameters from call->ad 
          if(sscanf(call->ad, "%31[^':']:%31[^':']:%hu:%hu", server_type, server_host, &server_port, &server_unit_id) != 4) {
              ESP_LOGE(TAG, "Failed to parse Modbus TCP call address: %s", call->ad);
              jsonAddObject_printf("CFG_Error", "Failed to parse Modbus TCP call address: %s", call->ad);
              }
 
          // TCP Network call
          if(strcmp(server_type, "tcp") == 0) {
              int sock = modbus_tcp_connect(server_host, server_port);
              if (sock >= 0) {
                  modbus_tcp_read_json(sock, server_unit_id, call->fn, call->rs, call->rn);
                  modbus_tcp_disconnect(sock);
                  close(sock);
              } else {
                  ESP_LOGW(TAG, "Failed to connect to Modbus server for call: %s", call->tag);
              }
            }

          // RTU Serial call - not implemented yet, placeholder for future expansion 
          else if(strcmp(server_type, "rtu") == 0) {
              // RTU client call - not implemented yet, placeholder for future expansion
              ESP_LOGW(TAG, "Modbus RTU client not implemented yet for call: %s", call->tag);
              continue; 
            } 

          // Unknown server type
          else {
            ESP_LOGW(TAG, "Unsupported Modbus call address: %s", call->ad);
          }
        }

      // block opened in loop, should be called after processing all calls to ensure json is properly closed for mqtt transmission
      // jsonClose();
      jsonCloseAll(); // ensure all blocks are closed, in case of config errors that may cause block structure issues

      // logs json, and base 64 encrypted json
      ESP_LOGI(TAG,"%s",jsonGetBuffer());
      ESP_LOGI(TAG,"%s",jsonGetBase64());

      mqtt_send_up_data(jsonGetBase64());
      vTaskDelay(pdMS_TO_TICKS(MODBUS_TCP_RETRY_DELAY_MS));

      ESP_LOGI(TAG,"%s", "Modbus client task delay terminated, restarting loop");
    }
  }


void modbus_init(void) {
    xTaskCreate(modbus_client_task, "modbus_client", 4096, NULL, 5, NULL);
}
