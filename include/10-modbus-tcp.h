#ifndef __PROTO_10_MODBUS_TCP_C__
#define __PROTO_10_MODBUS_TCP_C__
//Extracted Prototyes
// ****************************
// src/10-modbus-tcp.c prototypes
// ****************************
int modbus_tcp_connect(const char *host, uint16_t port);
int modbus_tcp_disconnect(int sock);
uint16_t *modbus_read_coils(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_discrete_inputs(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_holding_registers(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_input_registers(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_tcp_read_json(int sock,uint8_t unit_id, uint8_t func, uint16_t start_address, uint16_t quantity);
#endif
