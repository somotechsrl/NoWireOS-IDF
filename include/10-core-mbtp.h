#ifndef __PROTO_10_CORE_MBTP_CC__
#define __PROTO_10_CORE_MBTP_CC__
//Extracted Prototyes
// ****************************
// src/10-core-mbtp.cc prototypes
// ****************************
int modbus_tcp_connect(const char *host, uint16_t port);
int modbus_tcp_disconnect(int sock);
uint16_t *modbus_read(int sock, int func, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_coils(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_discrete_inputs(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_holding_registers(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_input_registers(int sock, uint16_t start_address, uint16_t quantity);
char *modbus_read_json(int sock, int func, uint16_t start_address, uint16_t quantity);
void modbus_init(void);
#endif
