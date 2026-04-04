#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_1
#define TXD_PIN 17
#define RXD_PIN 16
#define RTS_PIN 18
#define CTS_PIN 19
#define BUF_SIZE 1024

static const char *TAG = "MODBUS_RTU";

// Modbus CRC16 calculation
static uint16_t modbus_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// Send Modbus RTU request
static esp_err_t modbus_send_request(uint8_t slave_id, uint8_t function_code, uint16_t start_addr, uint16_t quantity) {
    uint8_t request[8];
    request[0] = slave_id;
    request[1] = function_code;
    request[2] = (start_addr >> 8) & 0xFF;
    request[3] = start_addr & 0xFF;
    request[4] = (quantity >> 8) & 0xFF;
    request[5] = quantity & 0xFF;
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;
    
    return uart_write_bytes(UART_NUM, (const char *)request, sizeof(request));
}

// Receive Modbus RTU response (basic example for read holding registers)
esp_err_t modbus_receive_response(uint8_t *response, size_t *len) {
    return uart_read_bytes(UART_NUM, response, BUF_SIZE, pdMS_TO_TICKS(1000));
}

static void modbus_rtu_client_task(void *pvParameters) {
    // UART configuration
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, RTS_PIN, CTS_PIN);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    while (1) {
        // Example: Read holding registers (function 3)
        modbus_send_request(1, 3, 0, 10); // Slave ID 1, start addr 0, quantity 10
        
        uint8_t response[BUF_SIZE];
        size_t len;
        if (modbus_receive_response(response, &len) == ESP_OK) {
            ESP_LOGI(TAG, "Received response: %d bytes", len);
            // Process response here
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main() {
    xTaskCreate(modbus_rtu_client_task, "modbus_task", 4096, NULL, 5, NULL);
}