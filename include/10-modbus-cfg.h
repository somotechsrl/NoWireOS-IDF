#ifndef __PROTO_10_MODBUS_CFG_C__
#define __PROTO_10_MODBUS_CFG_C__
//Extracted Prototyes
// ****************************
// src/10-modbus-cfg.c prototypes
// ****************************
void add_modbus_cfg_call(const char *tag, const char *ad, uint16_t rs, uint8_t fn, uint8_t rn);
cfg_call *next_modbus_cfg_call();
#endif
