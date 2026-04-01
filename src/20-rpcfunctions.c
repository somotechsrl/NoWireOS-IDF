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
    //jsonAddObject("hw", String(BOARDID).c_str());
    jsonAddObject_string("sn",serial_str );
    jsonAddObject_string("fw", REVISION);
    jsonAddObject_string("mac", mac_str);
    //jsonAddObject_uint32_t("up", (uint32_t)(millis() / 1000));
    jsonAddObject_string("ip", ipaddr);
    jsonAddObject_float("te", tsens_out);
    //jsonAddObject("mf",ESP.getFreeHeap());
}
