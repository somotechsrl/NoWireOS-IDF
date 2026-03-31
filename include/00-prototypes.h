#define __PROTO__
//Extracted Prototyes
// ****************************
// ****************************
void app_main();
// ****************************
// ****************************
static bool modbus_receive_all(int sock, uint8_t *buffer, size_t length);
int modbus_tcp_connect(const char *host, uint16_t port);
int modbus_tcp_disconnect(int sock);
uint16_t *modbus_read(int sock, int func, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_coils(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_discrete_inputs(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_holding_registers(int sock, uint16_t start_address, uint16_t quantity);
uint16_t *modbus_read_input_registers(int sock, uint16_t start_address, uint16_t quantity);
static void modbus_client_task(void *pvParameters);
void modbus_init(void);
// ****************************
// ****************************
// ****************************
// ****************************
static esp_err_t config_handler(httpd_req_t *req);
void wifi_init();
void start_web_server();
// ****************************
// ****************************
void jsonClear();
void jsonInit();
static const char *jsonComma();
const char *jsonGetBuffer();
uint16_t jsonGetBufferSize();
const char *jsonGetCompressedBuffer();
const char *jsonGetEncryptedBuffer();
uint16_t jsonGetCompressedSize();
static void bpAddValue(const char vtype, const void *vvalue, const uint8_t vsize);
static void jsonOpen(const char *oname, char bracket, char cbracket);
void jsonClose();
void jsonCloseAll();
void jsonAddValue(int8_t value);
void jsonAddValue(int16_t value);
void jsonAddValue(int32_t value);
void jsonAddValue(char value);
void jsonAddValue(uint8_t value);
void jsonAddValue(uint16_t value);
void jsonAddValue(uint32_t value);
void jsonAddValue(float value);
void jsonAddValue(double value);
void jsonAddValue(const char *value);
void jsonAddArray(const char *oname);
void jsonAddObject(const char *oname);
void jsonAddObject(const char *oname, const char *value);
void jsonAddObject(const char *oname, uint8_t value);
void jsonAddObject(const char *oname, bool value);
void jsonAddObject(const char *oname, uint16_t value);
void jsonAddObject(const char *oname, uint32_t value);
void jsonAddObject(const char *oname, float value);
void jsonAddObject(const char *oname, double value);
#endif
