#include "main.h"
#include "10-json-encoder.h"
#include "mbedtls/base64.h"



// Json formatting utilities
#define TAG "JSON"
#define JLEVELS 10
static char r[BUFSIZE], *rp;
static char s[BUFSIZE];
static char jclose[JLEVELS];
uint16_t comma[JLEVELS], level;

// Parallel binary structure. Emulates json
// and compresses data. keep only brackets but removes
// all info names (only values)
// every info is prefixed by a char wich defines type
// Receiver MUST know the structure sent....
// Implements a very simple packet compressione method, based on values degrading
// uint32_t => uint16_t => uint8_t where value is zero 
/*

   a: array key\0
   o: obiect key\0
   e: object or array end
   s: string key\0value\0
   b: unsigned byte key\0value
   w: unsigned word key\0value
   l: unsigned long key\0value
   B: signed byte key\0value
   W: signed word key\0value
   L: signed long key\0value 
   f: float
   d: double
   z: zero-value (no trailing data)
   n: zero string (no trailing data)  

   Finally, buffer is xor-encryptesd with special key

   example: "{u0x0000b0x00i0x0000{u0x000}}"

*/

// Encryption Xor key
// decoder must use it
#define XKEY 0xf1

// Resets all buffers
void jsonClear() {

  // intialize buffer pointer and levels
  rp = r;
  level = 0;

  // clears all buffers
  memset(s, 0, sizeof(s));
  memset(r, 0, sizeof(s));
  memset(comma, 0, sizeof(comma));
  memset(jclose, 0, sizeof(jclose));

}

void jsonInit() {

  jsonClear();
  
  // opens Json
  level=1;
  // compressed json doesn't send first and last '{}'
  //*rp++ = '{'; 
  strcpy(s, "{");
}

// print comma for level (autoicrement)
static const char *jsonComma() {
  return comma[level]++ ? "," : "";
}

// gets jsonBuffer
const char *jsonGetBuffer() {
  return s;
}
// gets jsonBuffer
uint16_t jsonGetBufferSize() {
  return strlen(s);
}

const char *jsonGetBase64() {
  size_t olen;
  static unsigned char b64buffer[BUFSIZE];
  // encrypts before send
  for(int i=0;i<rp-r;i++) r[i]=r[i] ^ XKEY;
  mbedtls_base64_encode(b64buffer, sizeof(b64buffer), &olen, rp, rp - r);
  ESP_LOGI(TAG,"Encrypted Size: %d -- Base64 size %d",rp-r,olen);
  return (char *)b64buffer;
}

// binary buffer add function
static void bpAddValue(const char vtype, const void *vvalue, const uint8_t vsize) {

  *rp++ = vtype;
  // checks special types (z=zero value, n= empty string)
  // in this way we can save some bytes in xmission
  if (vsize && vtype!='z' && vtype!='n') {    
    memcpy(rp, vvalue, vsize);
    rp += vsize;
  }
}

static void jsonOpen(const char *oname, char bracket, char cbracket) {
  *rp++ = bracket;
  sprintf(s + strlen(s), "%s\"%s\":%c", jsonComma(), oname, bracket);
  jclose[++level] = cbracket;
  comma[level] = 0;
}

void jsonClose() {
  if (!level) return;
  *rp++ = jclose[level];
  sprintf(s + strlen(s), "%c", jclose[level--]);
}

void jsonCloseAll() {
  if(!level) return;
  while (level > 0) jsonClose();
  strcat(s, "}");
  *rp++ = '}';
}

/**************************************************************************************

   jsonAddValue/hsonAddObject function are Overloaded, in this way we have automatic 'type' recognition
   and a kind of object-oriented usabel function in data module.

*/

// adds a integer object
void jsonAddValue_int_8(int8_t value) {
  bpAddValue(value==0 ? 'z' : 'B', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%d", jsonComma(), value);
}
// adds an unsigned integer object
void jsonAddValue_int_16(int16_t value) {
  bpAddValue(value==0 ? 'z' : 'W', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%d", jsonComma(), value);
}// adds an unsigned integer object
void jsonAddValue_int_32(int32_t value) {
  bpAddValue(value==0 ? 'z' : 'L', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%ld", jsonComma(), value);
}
// adds a char (char)
void jsonAddValue_char(char value) {
  bpAddValue(value==0 ? 'z' : 'b', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%c", jsonComma(), value);
}
// adds a uiigned short
void jsonAddValue_uint8_t(uint8_t value) {
  bpAddValue(value==0 ? 'z' : 'b', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%u", jsonComma(), value);
}
// adds a unsigned
void jsonAddValue_uint16_t(uint16_t value) {
  //if(value<=USHRT_MAX) return jsonAddValue((uint8_t)value);
  bpAddValue(value==0 ? 'z' : 'w', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%u", jsonComma(), value);
}
// adds a unsigned long
void jsonAddValue_uint32_t(uint32_t value) {
  //if(value<=UINT_MAX) return jsonAddValue((uint16_t)value);
  bpAddValue(value==0 ? 'z' : 'L', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%lu", jsonComma(), value);
}
// adds a float
void jsonAddValue_float(float value) {
  bpAddValue(value==0 ? 'z' : 'f', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%f", jsonComma(), value);
}
// adds a double
void jsonAddValue_double  (double value) {
  bpAddValue(value==0 ? 'z' : 'f', &value, sizeof(value));
  sprintf(s + strlen(s), "%s%f", jsonComma(), value);
}
// adds a string
void jsonAddValue_string(const char *value) {
  uint8_t l=strlen(value);
  bpAddValue(l ? 's' : 'n', value, strlen(value) + 1);
  sprintf(s + strlen(s), "%s\"%s\"", jsonComma(), value);
}
// adds a string
void jsonAddValue_printf(const char *format, ...) {
 
  char out[BUFTINY];
  va_list args;
  va_start(args, format);
  vsprintf(out, format, args);
  va_end(args);
 
  sprintf(s + strlen(s),"%s\"%s\"", jsonComma(),out);
  bpAddValue('s', out, strlen(out) + 1);
}

// adds an array header
void jsonAddArray(const char *oname) {
  bpAddValue('a', oname, strlen(oname) + 1);
  jsonOpen(oname, '[', ']');
}

// adds an object header
void jsonAddObject(const char *oname) {
  bpAddValue('o', oname, strlen(oname) + 1);
  jsonOpen(oname, '{', '}');
}

// adds a string data object
void jsonAddObject_string(const char *oname, const char *value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('s', value, strlen(value) + 1);
  sprintf(s + strlen(s), "%s\"%s\":\"%s\"", jsonComma(), oname, value);
}

// adds a string data object
void jsonAddObject_printf(const char *oname, const char *format, ...) {

  char out[BUFTINY];
  va_list args;
  va_start(args, format);
  vsprintf(out, format, args);
  va_end(args);

  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('s', out, strlen(out) + 1);
  sprintf(s + strlen(s), "%s\"%s\":\"%s\"", jsonComma(), oname, out);

}

// adds an unsigned short object
void jsonAddObject_uint8_t(const char *oname, uint8_t value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('b', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%u", jsonComma(), oname, value);
}

// adds a bool object
void jsonAddObject_bool(const char *oname, bool value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('b', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%u", jsonComma(), oname, value);
}

// adds a unsigned object
void jsonAddObject_uint16_t(const char *oname, uint16_t value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('w', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%u", jsonComma(), oname, value);
}

// adds an unsigned long object
void jsonAddObject_uint32_t(const char *oname, uint32_t value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('l', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%lu", jsonComma(), oname, value);
}

// adds a float
void jsonAddObject_float(const char *oname, float value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('f', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%f", jsonComma(), oname, value);
}

// adds a double
void jsonAddObject_double(const char *oname, double value) {
  bpAddValue('o', oname, strlen(oname) + 1);
  bpAddValue('d', &value, sizeof(value));
  sprintf(s + strlen(s), "%s\"%s\":%f", jsonComma(), oname, value);
}