#include "main.h"
#include "mqtt_client.h"
#include "10-mqtt.h"
#include "10-encoder.h"
#include "10-watchdogs.h"
#include "20-rpc-manager.h"
 
#define TSIZE 128
#define THEAD "nowireos"
#define BOARDID "esp32"
#define MAGIC_SUFFIX_KEY 0x1b2c

static esp_mqtt_client_handle_t client;
static const char *TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {

    char topic_buf[TSIZE];

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // topics to subscribe: nowireos/esp32/serial_str/rpc for RPC requests
            snprintf(topic_buf, TSIZE, "%s/%s/%s/rpc", THEAD, BOARDID, serial_str);
            esp_mqtt_client_subscribe(client,topic_buf, 0);
            // topics to subscribe: nowireos/esp32/serial_str/down for downlink messages
            snprintf(topic_buf, TSIZE, "%s/%s/%s/down", THEAD, BOARDID, serial_str);
            esp_mqtt_client_subscribe(client,topic_buf, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
            break;
        case MQTT_EVENT_PUBLISHED:
            // if we mqtt_send_logmay lead to race condition in log buffer, so we avoid logging here, but you can enable it for debugging
            //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            break;
        case MQTT_EVENT_DATA:
            char *topic = strndup(event->topic, event->topic_len);
            char *data = strndup(event->data, event->data_len);     
            ESP_LOGI(TAG, "MQTT_EVENT_DATA topic: %.*s, data: %.*s", event->topic_len, event->topic, event->data_len, event->data);
            mqtt_handle_received(topic,data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

void mqtt_send_up_data(const char *payload) {

    // TOPIC FORMAT: nowireos/esp32/serial_str/up/randseed+magic
    char topic_buf[TSIZE];

    // calculates magic suffix
    uint16_t randseed=rand();
    uint16_t magic=randseed ^ MAGIC_SUFFIX_KEY;
    snprintf(topic_buf, TSIZE, "%s/%s/%s/up/%04x%04x", THEAD, BOARDID, serial_str, randseed, magic);
    //ESP_LOGI(TAG, "Sending UP Json Data: %s :: %s", topic_buf, payload);
    esp_mqtt_client_publish(client, topic_buf, payload, 0, 1, 0);
}

void mqtt_send_rpc_response(const char *respid) {

    char topic_buf[TSIZE];

    // TOPIC FORMAT: nowireos/esp32/serial_str/rpc/respid
    snprintf(topic_buf, TSIZE, "%s/%s/%s/rpc/%s", THEAD, BOARDID, serial_str, respid);
    ESP_LOGI(TAG, "Sending RPC Json Response: %s :: %s", topic_buf, jsonGetBuffer());
    //ESP_LOGI(TAG, "Sending RPC Base64 Response: %s :: %s", topic_buf, jsonGetBase64());
    esp_mqtt_client_publish(client, topic_buf, jsonGetBase64(), 0, 1, 0);
}       

void mqtt_send_log(const char *data) {

    char topic_buf[TSIZE];

    // calculates magic suffix    
    uint16_t randseed=rand();
    uint16_t magic=randseed ^ MAGIC_SUFFIX_KEY;

    // TOPIC FORMAT: nowireos/esp32/serial_str/log/randseed+magic
    snprintf(topic_buf, TSIZE, "%s/%s/%s/log/%04x%04x", THEAD, BOARDID, serial_str, randseed, magic);
    esp_mqtt_client_publish(client, topic_buf,data , 0, 1, 0);
}       

void mqtt_handle_received(const char *topic, const char *data) {

    char topic_buf[TSIZE];

    // TOPIC FORMAT: nowireos/esp32/serial_str/rpc for RPC requests
    snprintf(topic_buf, TSIZE, "%s/%s/%s/rpc", THEAD, BOARDID, serial_str);

    if (strncmp(topic, topic_buf, strlen(topic_buf)) == 0) {
        ESP_LOGI(TAG, "Received RPC message: %s :: %s", topic_buf,data);
        // Handle RPC request
        rpcManage(data, true);
        return;
        }

    // TOPIC FORMAT: nowireos/esp32/serial_str/down for downlink messages
    snprintf(topic_buf, TSIZE, "%s/%s/%s/down", THEAD, BOARDID, serial_str);
    if (strncmp(topic, topic_buf, strlen(topic_buf)) == 0) {
        // Handle downlink message
        ESP_LOGI(TAG, "Received DOWN message: %s :: %s",topic_buf, data);
        return;
    }
 
    //esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    //esp_mqtt_client_publish(client, topic_up, request, 0, 1, 0);
}       

void mqtt_init(void) {

    static const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://rpc.somotech.it:2983",
        .broker.verification.certificate = NULL,

    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    watchdog_init(client);

}