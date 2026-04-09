#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-encoder.h"
#include "10-mqtt.h"
#include "10-modbus-tcp.h"
#include "10-modbus-rtu.h"  
#include "20-modbus-master.h"

// Modbus configuration entry
#define XTAG 32
#define MODBUS_CONFIGS 50

typedef struct {
  uint16_t rs;
  uint8_t rn;
} cfg_call;

// Modbus Configuration block
typedef struct {
  char tag[XTAG];
  char ad[XTAG];
  uint8_t fn,ncalls;
  // max MODBUS_CONFIG  calls... -- see HAL.h
  cfg_call calls[MODBUS_CONFIGS];
} modbus_config;


#define TAG "MODBUS_CFG"
TaskHandle_t modbus_master_task_handle;

static int amodbus_cnt;
static char amodbus_cfg[MODBUS_CONFIGS][BUFTINY];

void addModbusAggregatedCall(char *params) {

  jsonAddObject_printf("CFG_STRING",params);

  // param is in the form dev_id;tpc:address:port:unit;function;r:n,r:n,r:n....
  // checks if we reached limit
  if(amodbus_cnt>=MODBUS_CONFIGS) {
    ESP_LOGE(TAG,"Maximum configs (%d) reached",amodbus_cnt);
    jsonAddObject_printf("CFG_RESULT","ERROR: Maximum number of configs reached: %d",amodbus_cnt);
    return;
    }

  uint8_t fn;
  char tag[32],ad[32];
  char rs_str[BUFTINY];
  
  // separates root values tag,ad,fn,registers
   // typical format for aggregated call: tag;ad;fn;rs1:rn1,rs2:rn2,rs3:rn3,... where rs is starting register and rn is number of registers to read, allows for batch processing of multiple registers in one call for more efficient transmission and processing in modbus client task loop
  // splits single aggregated call with comma separated registers into multiple calls with same tag, ad, fn, but different rs and rn, then adds each call to config for processing in modbus client task loop 
  if (sscanf(params, "%31[^;];%31[^;];%hhu;%511[^;]s", tag, ad, &fn, rs_str) != 4) {
    ESP_LOGE(TAG, "Invalid params: %s", params);
    jsonAddObject_printf("CFG_RESULT","ERROR: Invalid params: %s", params);
    return; 
    }

  // check dups
  for(int i=0;i<amodbus_cnt;i++) {
    if(strcmp(amodbus_cfg[i],params) == 0) {
      ESP_LOGW(TAG, "Duplicate configuration: %s",params);
      jsonAddObject_printf("CFG_RESULT","Duplicate configuration: %s", params);
      return;
    }
  }

  strcpy(amodbus_cfg[amodbus_cnt++],params);
  ESP_LOGW(TAG, "Added configuration: %s",params);
  jsonAddObject_printf("CFG_RESULT","Added configuration: %s", params);
  }

static modbus_config *parse_modbus_cfg(char *params) {

  char *st,rs_str[BUFTINY];
  static modbus_config conf;
  memset(&conf,0,sizeof(conf));
  
  ESP_LOGI(TAG, "Parsing params: %s", params);


  // separates root values tag,ad,fn,registers
   // typical format for aggregated call: tag;ad;fn;rs1:rn1,rs2:rn2,rs3:rn3,... where rs is starting register and rn is number of registers to read, allows for batch processing of multiple registers in one call for more efficient transmission and processing in modbus client task loop
  // splits single aggregated call with comma separated registers into multiple calls with same tag, ad, fn, but different rs and rn, then adds each call to config for processing in modbus client task loop 
  if (sscanf(params, "%31[^;];%31[^;];%hhu;%511[^;]s",conf.tag, conf.ad, &conf.fn, rs_str) != 4) {
    ESP_LOGE(TAG, "Invalid params: %s", params);
    jsonAddObject_printf("CFG_RESULT","ERROR: Invalid params: %s", params);
    return NULL; 
    }

  // explodes rs_str into individual register sets, separated by comma, in format rs:rn, then adds each call to config with same tag, ad, fn, but different rs and rn for each register set, allows for batch processing of multiple registers in one call for more efficient transmission and processing in modbus client task loop
  int i=0;
  ESP_LOGI(TAG, "Scanning and generating regset from '%s'", rs_str);
  char *token = strtok_r((char *)rs_str, ",",&st);
  while (token != NULL) {
    if (sscanf(token, "%hu:%hhu", &conf.calls[i].rs, &conf.calls[i].rn) == 2) {
      i++;
    } else {
      ESP_LOGW(TAG, "Invalid register set: %s", token);
      jsonAddValue_printf("Invalid register set: %s", token);
    }
    token = strtok_r(NULL, ",",&st);
   }
   ESP_LOGI(TAG,"Done: Generated %d calls.",i);

   conf.ncalls=i;

return &conf;

}

// Modbus client task, will loop through calls in config and execute them, then send json result to mqtt
static void modbus_master_task(void *pvParameters) {

    modbus_config *conf;
    modbus_master_task_handle = xTaskGetCurrentTaskHandle();

    static char server_type[32]; // rtu, tcp
    static char server_host[BUFTINY]; // serial,speed,sb,parity || server FQDN or IP
    static uint16_t server_port,server_unit_id; 

    // replace loop with connector loop RTU/TCP
    while (true) {

        for(int i=0;i<amodbus_cnt;i++) {
          
          jsonInit();
 
          // explodes in modbus_cfg
          if((conf=parse_modbus_cfg(amodbus_cfg[i])) == NULL) {
            ESP_LOGE(TAG, "ERROR: Failed to get Modbus config");
            jsonAddObject_printf("ERROR", "Failed to get modbus config");
            continue;
            }
 
          // init json block for new server, if same server as previous call, will aggregate into same block
          jsonAddObject_string("DEV",conf->tag);
          jsonAddObject_string("BUS",conf->ad);
          jsonAddObject_string("DRV","modbus");
          jsonAddObject("data");

          ESP_LOGI(TAG, "Processing Modbus TCP call: tag=%s addr=%s func=%d", conf->tag, conf->ad, conf->fn);
          // extract modbus call parameters from call->ad 
          if(sscanf(conf->ad, "%31[^':']:%31[^':']:%hu:%hu", server_type, server_host, &server_port, &server_unit_id) != 4) {
              ESP_LOGE(TAG, "Failed to parse Modbus TCP call address: %s", conf->ad);
              jsonAddObject_printf("CFG_Error", "Failed to parse Modbus TCP call address: %s", conf->ad);
              continue;
              }
 
          // TCP Network call
          if(strcmp(server_type, "tcp") == 0) {
              int sock = modbus_tcp_connect(server_host, server_port);
              if (sock >= 0) {
                for(int i=0;i<conf->ncalls;i++) {
                  modbus_tcp_read_json(sock, server_unit_id, conf->fn, conf->calls[i].rs, conf->calls[i].rn);
                  }
                modbus_tcp_disconnect(sock);
              } else {
                ESP_LOGW(TAG, "Failed to connect to Modbus server for call: %s", conf->tag);
              }
            }
          // RTU Serial call - not implemented yet, placeholder for future expansion 
          else if(strcmp(server_type, "rtu") == 0) {
              // RTU client call - not implemented yet, placeholder for future expansion
              ESP_LOGW(TAG, "Modbus RTU client not implemented yet for call: %s", conf->tag);
              continue; 
            } 
       // Unknown server type
          else {
            ESP_LOGW(TAG, "Unsupported Modbus call address: %s", conf->ad);
          }
        }

      // block opened in loop, should be called after processing all calls to ensure json is properly closed for mqtt transmission
      jsonCloseAll(); // ensure all blocks are closed, in case of config errors that may cause block structure issues

      // logs json, and base 64 encrypted json
      ESP_LOGI(TAG,"%s",jsonGetBuffer());
      //ESP_LOGI(TAG,"%s",jsonGetBase64());

      mqtt_send_up_data(jsonGetBase64());

      ESP_LOGI(TAG,"Modbus client task delay %lus",timestep/1000);
      vTaskDelay(pdMS_TO_TICKS(timestep));
      ESP_LOGI(TAG,"Modbus client task delay terminated, restarting loop");
    }
  }


void modbus_init(void) {
    xTaskCreate(modbus_master_task, "modbus_master", 4096, NULL, 5, NULL);
}
