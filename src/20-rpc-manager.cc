#include "main.h"
#include "30-rpc-cmd.h"
#include "30-rpc-ids.h"
#include "10-json-encoder.h"

/*******************************************************************************
   RPC Parser/Executor module
*/
/*
#define DEBUG_RPC "RPC"

bool trigger = false;
static char rpcmsg[256];

static void rpcMessage(const char* msg) {
  jsonAddObject("message", msg);
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

    String OK = String(F("OK"));
  String KO = String(F("KO"));
  String invPin = String(F("Invalid PIN"));

  debug(DEBUG_RPC, "Received:" + payload);

  // Switch Used Variables
  int pin, res;
  String c, rpcresp, rpcresult = "";

  // format is always ID|command[|data] (data is optional)
  const char *sep = "|";

  // extracts ID and Command
  int pos = payload.indexOf(sep);
  if (pos < 0) return;

  String rpcid = String("R") + payload.substring(0, pos);
  String rpcmd = payload.substring(pos + 1);

  // Calculates subpos for parameter
  int sub = rpcmd.indexOf(sep) + 1;
  String bitname = sub > 1 ? rpcmd.substring(sub) : "";
  String cmdname = sub > 1 ? rpcmd.substring(0, sub - 1) : rpcmd;

  // sets rpcid
  int cmdid = getCommandID(rpcmd, RPC_cmd);

  debug(DEBUG_RPC,String("ID:")+rpcid);
  debug(DEBUG_RPC,rpcmd + " (ID: " + cmdid + ")");

  // recived something.. init json Response Buffer
  // submodule MUST NOT call jsonInit/jsonCloseAll for respnses!!!
  jsonInit();
  jsonAddObject(rpcid.c_str());
  jsonAddObject("result");
  //jsonAddObject(cmdname.c_str());

  // rpcStatus default is 'OK'
  String rpcStatus = OK;

  switch (cmdid) {
    case CFG_Debug:
      setDebugMode(bitname.toInt());
      break;
     case CFG_Leds_Enable:
      ledEnable();
      break;
    case CFG_Leds_Disable:
      ledDisable();
      break;
    case CFG_Timestep:
      // timestep is received in s, converted in ms
      if (bitname != "") timestep = bitname.toInt()*1000;
      if (timestep < MINTSTEP*1000) {
        debug(DEBUG_RPC,String("Timestep too low, forced to ")+MINTSTEP);
        timestep = MINTSTEP*1000;
      }
      jsonAddObject("value", (uint32_t)timestep/1000);
      break;

    // ************ RPC group Commands
    case RPC_Trigger:
      trigger=true;
      jsonAddObject("value","Datalogger Triggered");
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
      sysGetInfo();
      break;
    case Sys_GetStatus:
      sysGetStatus();
      break;
    case Sys_Reboot:
      enableReboot();
      break;
    case Sys_Cancel_Reboot:
      cancelReboot();
      break;
    case Sys_Identify:
      identifyPixel();
      break;

#if defined(__MODBUS_TCP__)
    case CFG_Modbus_AddCall:
      addModbusCall(bitname.c_str());
      break;
#endif

    // ************ Unknow management
    default:
      rpcStatus = KO;
      jsonErrorNotImplemented();

  }

  // Closes response Buffer and sets response status
  jsonClose();
  jsonAddObject("status", rpcStatus);
  jsonCloseAll();

  debug(DEBUG_RPC,(sync ? String("SYNC:") : String("ASYNC:"))+jsonGetBuffer());
  if(sync) mqttRpcUp(rpcid,sync);
  jsonClear();
  
}

void rpcManage(const char *payload, bool sync) {
  String d = String(payload);
  return rpcManage(d, sync);
}
*/