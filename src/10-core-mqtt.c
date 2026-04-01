#include "main.h"
#include "mqtt_client.h"
 
#define TSIZE 128
#define THEAD "nowireos"
#define BOARDID "esp32"
static char topic_up[TSIZE];
static char topic_rpc[TSIZE];
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
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA topic: %.*s, data: %.*s", event->topic_len, event->topic, event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

void mqtt_send_up_data(const char *payload) {
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    esp_mqtt_client_publish(client, topic_up, payload, 0, 1, 0);
}       

void mqtt_send_rpc_response(const char $response,const char *payload,) {

    //esp_mqtt_client_handle_t client = esp_mqtt_client_init(NULL);
    esp_mqtt_client_publish(client, topic_up, payload, 0, 1, 0);
}       

void mqtt_init(void) {

    // sets topic values
    snprintf(topic_up, TSIZE, "%s/%s/%s/up",THEAD,BOARDID,mac_str);
    snprintf(topic_rpc, TSIZE, "%s/%s/%s/rpc", THEAD, BOARDID, mac_str);
    snprintf(topic_down, TSIZE, "%s/%s/%s/down", THEAD,BOARDID, mac_str);


    static const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://rpc.somotech.it:2983",
        .broker.verification.certificate = NULL,

    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}