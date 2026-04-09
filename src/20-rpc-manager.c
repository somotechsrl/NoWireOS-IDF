#include "main.h"
#include <string.h>
#include "30-rpc-cmd.h"
#include "30-rpc-ids.h"
#include "10-encoder.h"
#include "10-mqtt.h"
#include "20-rpc-utils.h"
#include "20-modbus-master.h"
#include "30-utils.h"

#define TAG "RPC"
bool trigger = false;
uint32_t timestep=300000;


// retrives command sequence if for switch/case
static int getCommandID(const char *rpcmd) {
  int size=sizeof(RPC_cmd)/sizeof(RPC_cmd[0]);
  for (int i = 0; i<size;i++) {
    if (!strncmp(rpcmd, RPC_cmd[i], strlen(RPC_cmd[i]))) return i;
  }
  return -1;
}

// Work with string which is more efficent
static char *p,rpcb[BUFTINY];
void rpcManage(const char *payload, bool sync) {

  ESP_LOGI(TAG, "Received: %s", payload);

  // trigger task handle to call AFTER RPC execution, to allow sync/async response management
  TaskHandle_t trigger_task_handle = NULL;

  // extracts ID and Command
  char *st;
  strcpy(rpcb, payload);

  char *request_id = (p=strtok_r(rpcb,"|",&st))!=NULL ? p : "";
  char *rpccommand = (p=strtok_r(NULL,"|",&st))!=NULL ? p : "";
  char *rpc_params = (p=strtok_r(NULL,"|",&st))!=NULL ? p : "";

  char respid[BUFTINY];
  snprintf(respid, sizeof(respid), "R%s", request_id);
  ESP_LOGI(TAG, "Splitting: %s :: %s :: %s --> %s", request_id,rpccommand,rpc_params, respid);

  // sets rpcid
  int cmdid = getCommandID(rpccommand);
  ESP_LOGI(TAG, "ID: %s Command: %s (ID: %d)", respid, rpccommand, cmdid);

  // recived something.. init json Response Buffer
  // submodule MUST NOT call jsonInit/jsonCloseAll for respnses!!!
  jsonInit();
  jsonAddObject(respid);
  jsonAddObject(RESULT);
  
  // rpcStatus default is 'OK'
  char *rpcStatus = OK;
  char result[BUFSIZE] = "Executed successfully";

  switch (cmdid) {
    case CFG_Debug:
      //setDebugMode(bitname.toInt());
      break;
    case CFG_Leds_Enable:
      led_blink_enabled = true;
      jsonAddObject_string("value","HB LED Enabled");
      break;
    case CFG_Leds_Disable:
      led_blink_enabled = false;
      jsonAddObject_string("value","HB LED Disabled");
    break;
       case CFG_Timestep:
      // timestep is received in s, converted in ms
      if (*rpc_params) timestep = atoi(rpc_params)*1000;
      jsonAddObject_uint32_t("value", (uint32_t)timestep/1000);
      break;
      case CFG_LOG_Mqtt:
        logger_mqtt();
        jsonAddObject_string("value","MQTT Logger Enabled");
        break;
      case CFG_LOG_Local:
        logger_default();
        jsonAddObject_string("value","Local Logger Enabled");
        break;
      case CFG_LOG_Off:
      case CFG_LOG_None:
        logger_off();
        jsonAddObject_string("value","Logging Disabled");
        break;

    case RPC_Trigger:
      if(!strcmp(rpc_params,"modbus")) {
        jsonAddObject_string("value","OK: Modbus Triggered");
        trigger_task_handle = modbus_master_task_handle;
        }
      else {
        jsonAddObject_string("value","ERROR:Unknown Trigger");
        }
      break;
      
    case RPC_List:
      jsonAddArray("RPC.List");
      int size=sizeof(RPC_cmd)/sizeof(RPC_cmd[0]);
      for (int i = 0; i<size;i++) {
        jsonAddValue_string(RPC_cmd[i]);
      }
      jsonClose();
      break;

    // ************ System Related Commands
    case Sys_GetInfo:
    case Sys_GetStatus:
      sysGetInfo();
      break;
    case Sys_Reboot:
      //enableReboot();
      break;
    case Sys_Cancel_Reboot:
      //cancelReboot();
      break;
    case Sys_Identify:
      //identifyPixel();
      break;

    case CFG_Modbus_AddCall:
      addModbusAggregatedCall(rpc_params);
      break;
  
    // ************ Unknow management
    default:
      rpcStatus = KO;
      snprintf(result, sizeof(result), "%s(%s): %s", rpccommand,rpc_params, "not implemented");
      jsonAddObject_string("value",result);
 
  }

  // Closes response Buffer and sets response status
  jsonClose();
  jsonAddObject_string("status", rpcStatus);
  jsonCloseAll();

  ESP_LOGI(TAG, "MODE: %s", sync ? "SYNC:" : "ASYNC:");
  ESP_LOGI(TAG, "%s", jsonGetBuffer());
  if(sync) mqtt_send_rpc_response(respid);

  // After RPC calls evetal trigger
  if(trigger_task_handle) {
    vTaskDelay(100);
    ESP_LOGI(TAG, "Triggering task handle: %p", trigger_task_handle);
    xTaskAbortDelay(trigger_task_handle);
    }
 }
  

