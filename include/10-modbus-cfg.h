#ifndef __PROTO_10_MODBUS_CFG_C__
#define __PROTO_10_MODBUS_CFG_C__
//Extracted Prototyes
// ****************************
// src/10-modbus-cfg.c prototypes
// ****************************
void addModbusCall(const char *params);
void add_modbus_cfg_call(const char *tag, const char *ad, uint8_t fn, uint16_t rs, uint8_t rn);
cfg_call *next_modbus_cfg_call();
cfg_call *get_modbus_cfg_call();
#endif
