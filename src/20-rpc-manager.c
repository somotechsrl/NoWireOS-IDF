#include "main.h"
#include "30-rpc-cmd.h"
#include "30-rpc-ids.h"
#include "10-json-encoder.h"

/*******************************************************************************
   RPC Parser/Executor module
*/


#define TAG "RPC"

bool trigger = false;
static char rpcmsg[256];

static void rpcMessage(const char* msg) {
  jsonAddObject_string("message", msg);
}

static void rpcWrongParams(const char *bitname, int res) {
  snprintf(rpcmsg, sizeof(rpcmsg), "Wrong Parameters: %s -- [%d]", bitname, res);
  rpcMessage(rpcmsg);
}

// retrives command sequence if for switch/case
static int getCommandID(const char *rpcmd, const char **cmdlist) {
  for (int i = 0; cmdlist[i] != ""; i++) {
    if (!strcmp(rpcmd, cmdlist[i])) return i;
  }
  return -1;
}

// Work with string which is more efficent

void rpcManage(const char *payload, bool sync) {

  const char *OK = "OK";
  const char *KO = String(F("KO"));
  const char *SEP = "|"
  const char RESULT = "result";
  
  ESP_LOGI(TAG, "Received: %s", payload);

  // extracts ID and Command
  char *r = strtok(payload,"|");
  char *rpccommand = strtok(NULL,"|");
  char *rpcparams = strtok(NULL,"|");

  char respid[64];
  snprintf(respid, sizeof(respid), "R%s", requestid);

  // sets rpcid
  int cmdid = getCommandID(rpccommand, RPC_cmd);

  ESP_LOGI(TAG, "ID: %s Command: %s (ID: %d)", respid, rpccommand, cmdid);

  // recived something.. init json Response Buffer
  // submodule MUST NOT call jsonInit/jsonCloseAll for respnses!!!
  jsonInit();
  jsonAddObject(respid);
  jsonAddObject(RESULT);
  
  // rpcStatus default is 'OK'
  char *rpcStatus = OK;

  switch (cmdid) {
    case CFG_Debug:
      //setDebugMode(bitname.toInt());
      break;
     case CFG_Leds_Enable:
      //ledEnable();
      break;
    case CFG_Leds_Disable:
      //ledDisable();
      break;
    case CFG_Timestep:
      // timestep is received in s, converted in ms
      //if (bitname != "") timestep = bitname.toInt()*1000;
      //if (timestep < MINTSTEP*1000) {
      //  debug(DEBUG_RPC,String("Timestep too low, forced to ")+MINTSTEP);
      //  timestep = MINTSTEP*1000;
      //}
      jsonAddObject_uint32_t("value", (uint32_t)timestep/1000);
      break;

    // ************ RPC group Commands
    case RPC_Trigger:
      trigger=true;
      jsonAddObject_string("value","Datalogger Triggered");
      break;
      
    case RPC_List:
      jsonAddArray("RPC.List");
      for (int i = 0; RPC_cmd[i] != ""; i++) {
        c = RPC_cmd[i];
        c.replace("|", " <param>");
        jsonAddValue(c);

      }
      jsonClose();
      break;

    // ************ System Related Commands
    case Sys_GetInfo:
      //sysGetInfo();
      break;
    case Sys_GetStatus:
      //sysGetStatus();
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
      //addModbusCall(params));
      break;

    // ************ Unknow management
    default:
      rpcStatus = KO;
      jsonErrorNotImplemented();

  }

  // Closes response Buffer and sets response status
  jsonClose();
  jsonAddObject_string("status", rpcStatus);
  jsonCloseAll();

  ESP_LOGI(TAG, "MODE: %s", sync ? "SYNC:" : "ASYNC:");
  ESP_LOGI(TAG, "%s", jsonGetBuffer());
  if(sync) mqttRpcUp(rpcid,sync);
  jsonClear();
  
}
