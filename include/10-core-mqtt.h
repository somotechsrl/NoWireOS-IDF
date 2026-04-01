#ifndef __PROTO_10_CORE_MQTT_C__
#define __PROTO_10_CORE_MQTT_C__
//Extracted Prototyes
// ****************************
// src/10-core-mqtt.c prototypes
// ****************************
void mqtt_send_up_data(const char *payload);
void mqtt_send_rpc_response(const char *respid);
void mqtt_handle_received(const char *topic, const char *data);
void mqtt_init(void);
#endif
