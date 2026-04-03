#ifndef __PROTO_10_JSON_ENCODER_C__
#define __PROTO_10_JSON_ENCODER_C__
//Extracted Prototyes
// ****************************
// src/10-json-encoder.c prototypes
// ****************************
void jsonClear();
void jsonInit();
const char *jsonGetBuffer();
uint16_t jsonGetBufferSize();
const char *jsonGetCompressedBuffer();
const char *jsonGetEncryptedBuffer();
uint16_t jsonGetCompressedSize();
const char *jsonGetBase64();
void jsonClose();
void jsonCloseAll();
void jsonAddValue_int_8(int8_t value);
void jsonAddValue_int_16(int16_t value);
void jsonAddValue_int_32(int32_t value);
void jsonAddValue_char(char value);
void jsonAddValue_uint8_t(uint8_t value);
void jsonAddValue_uint16_t(uint16_t value);
void jsonAddValue_uint32_t(uint32_t value);
void jsonAddValue_float(float value);
void jsonAddValue_double  (double value);
void jsonAddValue_string(const char *value);
void jsonAddValue_printf(const char *format, ...);
void jsonAddArray(const char *oname);
void jsonAddObject(const char *oname);
void jsonAddObject_string(const char *oname, const char *value);
void jsonAddObject_printf(const char *oname, const char *format, ...);
void jsonAddObject_uint8_t(const char *oname, uint8_t value);
void jsonAddObject_bool(const char *oname, bool value);
void jsonAddObject_uint16_t(const char *oname, uint16_t value);
void jsonAddObject_uint32_t(const char *oname, uint32_t value);
void jsonAddObject_float(const char *oname, float value);
void jsonAddObject_double(const char *oname, double value);
#endif
