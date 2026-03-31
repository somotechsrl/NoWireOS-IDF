#ifndef __PROTO_]]u/10_]]]]_p]u]pp]_uu__
#define __PROTO_]]u/10_]]]]_p]u]pp]_uu__
//Extracted Prototyes
void jsonClear();
void jsonInit();
const char *jsonGetBuffer();
uint16_t jsonGetBufferSize();
const char *jsonGetCompressedBuffer();
const char *jsonGetEncryptedBuffer();
uint16_t jsonGetCompressedSize();
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
