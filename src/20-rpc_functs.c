#include "main.h"
#include "10-json-encoder.h"
#include "driver/temperature_sensor.h"

// Compiles data for GetInfo and Status
// Don't intialize object!!!!
void sysGetInfo(void) {
    
    float tsens_out=0;
 /*
    temperature_sensor_handle_t temp_handle = NULL;
    // Enable temperature sensor
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
    // Get converted sensor data
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tsens_out));
    printf("Temperature in %f °C\n", tsens_out);
    // Disable the temperature sensor if it is not needed and save the power
    ESP_ERROR_CHECK(temperature_sensor_disable(temp_handle));
 */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    jsonAddObject_string("chip_model", chip_info.model == CHIP_ESP32 ? "ESP32" : "Other");   // total currently free in all non-continues blocks
    jsonAddObject_uint32_t("cores", chip_info.cores); // minimum free ever
    jsonAddObject_uint32_t("revision", chip_info.revision); // largest continues block
 
    char features[BUFTINY];
    snprintf(features, sizeof(features), "%s%s%s",
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT " : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : ""
        );
    jsonAddObject_string("features", features);

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    jsonAddObject_uint32_t("mem_tf", info.total_free_bytes);   // total currently free in all non-continues blocks
    jsonAddObject_uint32_t("mem_mf", info.minimum_free_bytes); // minimum free ever
    jsonAddObject_uint32_t("mem_lb", info.largest_free_block); // largest continues block to allocate big array

    //jsonAddObject("hw", String(BOARDID).c_str());
    jsonAddObject_string("sn",serial_str );
    jsonAddObject_string("fw", REVISION);
    jsonAddObject_string("mac", mac_str);
    //jsonAddObject_uint32_t("up", (uint32_t)(millis() / 1000));

    jsonAddObject_string("ip", ipaddr);
    jsonAddObject_float("te", tsens_out);
    //jsonAddObject("mf",ESP.getFreeHeap());
}
