#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "10-encoder.h"

#define TAG "MODBUS_TCP"
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static uint16_t modbus_transaction_id = 1;

static bool modbus_receive_all(int sock, uint8_t *buffer, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        int received = recv(sock, buffer + offset, length - offset, 0);
        if (received <= 0) {
            ESP_LOGW(TAG, "receive failed: %s", strerror(errno));
            return false;
        }
        offset += received;
    }
    return true;
}

int modbus_tcp_connect(const char *host, uint16_t port) {

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &remote_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "invalid Modbus server address: %s", host);
        close(sock);
        return -1;
    }

    struct timeval timeout = {
        .tv_sec = MODBUS_TCP_REQUEST_TIMEOUT_SEC,
        .tv_usec = 0,
    };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        ESP_LOGW(TAG, "connect() failed: %s", strerror(errno));
        close(sock);
        return -1;
    }

    ESP_LOGI(TAG, "connected to Modbus server %s:%u", host, port);
    return sock;
}

int modbus_tcp_disconnect(int sock) {
    return close(sock);
}   

static uint8_t modbus_error;
static uint16_t *modbus_tcp_read(int sock, uint8_t unit_id, uint8_t func, uint16_t start_address, uint16_t quantity) {
    
    modbus_error = 0; // reset error before call
    static uint16_t dest[MODBUS_TCP_MAX_REGISTERS];

    if (quantity == 0 || quantity > MODBUS_TCP_MAX_REGISTERS) {
        ESP_LOGE(TAG, "invalid register count %u", quantity);
        return NULL;
    }

    uint8_t request[12];
    request[0] = (modbus_transaction_id >> 8) & 0xFF;
    request[1] = modbus_transaction_id & 0xFF;
    request[2] = 0;
    request[3] = 0;
    request[4] = 0;
    request[5] = 6;
    request[6] = unit_id;
    request[7] = func;
    request[8] = (start_address >> 8) & 0xFF;
    request[9] = start_address & 0xFF;
    request[10] = (quantity >> 8) & 0xFF;
    request[11] = quantity & 0xFF;

    if (send(sock, request, sizeof(request), 0) != sizeof(request)) {
        ESP_LOGW(TAG, "failed to send Modbus request: %s", strerror(errno));
        return NULL;
    }

    modbus_transaction_id++;
    if (modbus_transaction_id == 0) {
        modbus_transaction_id = 1;
    }

    uint8_t header[7];
    if (!modbus_receive_all(sock, header, sizeof(header))) {
        return NULL;
    }

    uint16_t recv_transaction = (header[0] << 8) | header[1];
    uint16_t protocol_id = (header[2] << 8) | header[3];
    uint16_t length = (header[4] << 8) | header[5];
    uint8_t recv_unit = header[6];

    if (recv_transaction != (modbus_transaction_id - 1)) {
        ESP_LOGW(TAG, "transaction id mismatch: expected %u got %u", modbus_transaction_id - 1, recv_transaction);
    }
    if (protocol_id != 0) {
        ESP_LOGW(TAG, "unexpected Modbus protocol id %u", protocol_id);
    }
    if (recv_unit != unit_id) {
        ESP_LOGW(TAG, "unexpected unit id %u", recv_unit);
    }
    if (length < 2) {
        ESP_LOGE(TAG, "invalid Modbus response length %u", length);
        return NULL;
    }

    size_t pdu_length = length - 1;
    if (pdu_length > 253) {
        ESP_LOGE(TAG, "too large Modbus PDU length %zu", pdu_length);
        return NULL;
    }

    uint8_t pdu[253];
    if (!modbus_receive_all(sock, pdu, pdu_length)) {
        return NULL;
    }

    if (pdu[0] & 0x80 || pdu[0] != func) {
        modbus_error = pdu[1];
        ESP_LOGE(TAG, "Modbus exception response function=0x%02X code=0x%02X rs=%04X rn=%d", pdu[0], pdu[1], start_address,quantity);
        return NULL;
    }

    uint8_t byte_count = pdu[1];
    if (byte_count != quantity * 2) {
        ESP_LOGE(TAG, "unexpected byte count %u", byte_count);
        return NULL;
    }

    for (uint16_t index = 0; index < quantity; ++index) {
        size_t offset = 2 + index * 2;
        dest[index] = ((uint16_t)pdu[offset] << 8) | pdu[offset + 1];
    }

    return dest;
}


// Not really used but can be used for future expansion to other function codes
uint16_t *modbus_tcp_read_coils(int sock, uint8_t unit_id,uint16_t start_address, uint16_t quantity) {
    return modbus_tcp_read(sock, unit_id, 0x01, start_address, quantity);
}
uint16_t *modbus_tcp_read_discrete_inputs(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity) {
    return modbus_tcp_read(sock, unit_id, 0x02, start_address, quantity);
}

uint16_t *modbus_tcp_read_holding_registers(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity) {
   return modbus_tcp_read(sock, unit_id, 0x03, start_address, quantity);
}
uint16_t *modbus_tcp_read_input_registers(int sock, uint8_t unit_id, uint16_t start_address, uint16_t quantity) {
    return modbus_tcp_read(sock, unit_id, 0x04, start_address, quantity);
}

// Default entry for modbus tcp client task, will be called by modbus client task loop for each call in config
uint16_t *modbus_tcp_read_json(int sock,uint8_t unit_id, uint8_t func, uint16_t start_address, uint16_t quantity) {

    uint16_t *response;
    char jobjectid[64];
    sprintf(jobjectid,"x%04x",start_address);

    // reads data from modbus tcp server, response is array of uint16_t, jsonAddValue will add as number, if want to add as string need to convert to string first
    response=modbus_tcp_read(sock, unit_id , func, start_address, quantity);

    jsonAddArray(jobjectid);
    jsonAddValue_uint8_t(func);
    jsonAddValue_uint16_t(modbus_error);
    jsonAddValue_uint16_t(start_address);

    // get values for non null response, if response is null, it means there was an error, modbus_error variable will have error code, if response is not null, modbus_error should be 0
    for (uint16_t i = 0; response && i < quantity; ++i) {
        jsonAddValue_uint16_t(response[i]);
        }

    jsonClose();

    return response;
}

