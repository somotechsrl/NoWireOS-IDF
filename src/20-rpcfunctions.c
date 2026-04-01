#include "main.h"
#include "10-json-encoder.h"

// Compiles data for GetInfo and Status
// Don't intialize object!!!!
void sysGetInfo(void) {

  //jsonAddObject("hw", String(BOARDID).c_str());
  jsonAddObject_string("sn",mac_str );
  jsonAddObject_string("fw", REVISION);
  jsonAddObject_string("mac", mac_str);
  jsonAddObject_uint32_t("up", (uint32_t)(millis() / 1000));
  jsonAddObject_string("ip", ipaddr);
  //String temp = String((temprature_sens_read() - 32) / 1.8);
  //jsonAddObject("te", temp.c_str());
  //jsonAddObject("mf",ESP.getFreeHeap());
}
