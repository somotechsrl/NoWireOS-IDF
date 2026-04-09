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
static char topic_up[TSIZE];
static char topic_rpc[TSIZE];
static char topic_log[TSIZE];
static char topic_down[TSIZE];
static esp_mqtt_client_handle_t client;

static const char *TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client,topic_rpc, 0);
            esp_mqtt_client_subscribe(client,topic_down, 0);
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

    // calculates magic suffix
    char xtopic[TSIZE*2];
    uint16_t randseed=rand();
    uint16_t magic=randseed ^ MAGIC_SUFFIX_KEY;
    snprintf(xtopic,TSIZE*2,"%s/%04x%04x",topic_up,randseed,magic);
    
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    ESP_LOGI(TAG, "Sending UP Json Data: %s :: %s", xtopic, payload);
    esp_mqtt_client_publish(client, xtopic, payload, 0, 1, 0);
}

void mqtt_send_rpc_response(const char *respid) {
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    char topic_resp[TSIZE];
    snprintf(topic_resp, TSIZE, "%s/%s/%s/rpc/%s", THEAD, BOARDID, serial_str, respid);
    ESP_LOGI(TAG, "Sending RPC Json Response: %s :: %s", topic_resp, jsonGetBuffer());
    ESP_LOGI(TAG, "Sending RPC Base64 Response: %s :: %s", topic_resp, jsonGetBase64());
    esp_mqtt_client_publish(client, topic_resp, jsonGetBase64(), 0, 1, 0);
}       

void mqtt_send_log(const char *data) {

    // calculates magic suffix
    char xtopic[TSIZE];
    uint16_t randseed=rand();
    uint16_t magic=randseed ^ MAGIC_SUFFIX_KEY;
    //snprintf(xtopic,TSIZE,"%s/%04x%04x",topic_log,randseed,magic);
    strcpy(xtopic,topic_log); // for logs we don't use magic suffix to make it easier to subscribe to all logs with a single wildcard topic
    printf("Sending Log Data: %s :: %s", xtopic, data);
    esp_mqtt_client_publish(client, xtopic,data , 0, 1, 0);
}       

void mqtt_handle_received(const char *topic, const char *data) {
    if (strncmp(topic, topic_rpc, strlen(topic_rpc)) == 0) {
        ESP_LOGI(TAG, "Received RPC message: %s :: %s", topic,data);
        // Handle RPC request
        rpcManage(data, true);
    } else if (strncmp(topic, topic_down, strlen(topic_down)) == 0) {
        // Handle downlink message
        ESP_LOGI(TAG, "Received DOWN message: %s :: %s",topic, data);
    }
 
    //esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    //esp_mqtt_client_publish(client, topic_up, request, 0, 1, 0);
}       

void mqtt_init(void) {

    // sets topic values
    snprintf(topic_up, TSIZE, "%s/%s/%s/up",THEAD,BOARDID,serial_str);
    snprintf(topic_rpc, TSIZE, "%s/%s/%s/rpc", THEAD, BOARDID, serial_str);
    snprintf(topic_log, TSIZE, "%s/%s/%s/log", THEAD, BOARDID, serial_str);
    snprintf(topic_down, TSIZE, "%s/%s/%s/down", THEAD,BOARDID, serial_str);

    static const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://rpc.somotech.it:2983",
        .broker.verification.certificate = NULL,

    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    watchdog_init(client);

}