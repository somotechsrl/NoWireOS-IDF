#ifndef JSON_ENCODER_HPP
#define JSON_ENCODER_HPP

void jsonClear();
void jsonInit();

const char *jsonGetBuffer();
uint16_t jsonGetBufferSize();
const char *jsonGetCompressedBuffer();
const char *jsonGetEncryptedBuffer();
uint16_t jsonGetCompressedSize();

void jsonAddArray(const char *oname);
void jsonAddObject(const char *oname);
void jsonCloseObject();
void jsonCloseArray();
/*
void jsonAddValue(const char *value);
void jsonaddValue(float value);
void jsonaddValue(double value);
void jsonAddValue(uint32_t value);  
void jsonAddValue(uint16_t value);
void jsonAddValue(uint8_t value);
void jsonAddValue(bool value);

void jsonAddObject(const char *oname, float value);
void jsonAddObject(const char *oname, double value);
void jsonAddObject(const char *oname, uint32_t value);
void jsonAddObject(const char *oname, uint16_t value);
void jsonAddObject(const char *oname, uint8_t value);
void jsonAddObject(const char *oname, bool value);  
void jsonAddObject(const char *oname, const char *value);
*/
#endif
