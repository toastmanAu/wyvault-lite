#include "fingerprint.h"
#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "fingerprint";
static bool s_available = false;

// TODO: configure to free IOs on JC3248W535
#define FP_UART_NUM   UART_NUM_1
#define FP_TX_PIN     17   // placeholder — wire to SFM-V1.7 RX
#define FP_RX_PIN     18   // placeholder — wire to SFM-V1.7 TX
#define FP_BAUD_RATE  57600

int fingerprint_init(void) {
    uart_config_t cfg = {
        .baud_rate = FP_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    esp_err_t r = uart_driver_install(FP_UART_NUM, 256, 0, 0, NULL, 0);
    if (r != ESP_OK) { ESP_LOGW(TAG, "UART install failed — sensor not present"); return -1; }
    uart_param_config(FP_UART_NUM, &cfg);
    uart_set_pin(FP_UART_NUM, FP_TX_PIN, FP_RX_PIN, -1, -1);
    // TODO: send handshake packet to sensor, check response
    ESP_LOGW(TAG, "Fingerprint sensor: STUB — real driver not yet implemented");
    s_available = false;  // Set true when real driver confirmed working
    return 0;
}

bool fingerprint_available(void) { return s_available; }

int fingerprint_enroll(void) {
    if (!s_available) { ESP_LOGW(TAG, "Sensor not available"); return -1; }
    // TODO: implement SFM-V1.7 enroll protocol (2-scan verification)
    return -1;
}

int fingerprint_verify(void) {
    if (!s_available) {
        ESP_LOGW(TAG, "Sensor not available — skipping fingerprint");
        return 0; // Graceful degradation: pass if sensor not connected
    }
    // TODO: implement SFM-V1.7 verify protocol
    return -1;
}

int fingerprint_clear_all(void) {
    if (!s_available) return -1;
    return -1; // TODO
}

int fingerprint_delete(uint16_t template_id) {
    if (!s_available) return -1;
    return -1; // TODO
}

int fingerprint_count(void) {
    if (!s_available) return 0;
    return -1; // TODO
}
