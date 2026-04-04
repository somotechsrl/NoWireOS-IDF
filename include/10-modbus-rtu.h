#ifndef __PROTO_10_MODBUS_RTU_C__
#define __PROTO_10_MODBUS_RTU_C__
//Extracted Prototyes
// ****************************
// src/10-modbus-rtu.c prototypes
// ****************************
esp_err_t modbus_receive_response(uint8_t *response, size_t *len);
void modbus_rtu_init(void);
#endif
