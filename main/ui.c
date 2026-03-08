#include <string.h>
#include <string.h>
#include "ui.h"
#include "storage.h"
#include "crypto.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ui";

// TODO: init AXS15231B display via QSPI + CST820 touch via I2C
// Driver references:
//   https://github.com/Guition/JC3248W535 (board examples)
//   LVGL + esp_lcd_panel driver for AXS15231B

void ui_init(void) {
    ESP_LOGI(TAG, "UI init — display + touch (STUB)");
    // TODO:
    //   1. Init QSPI bus for AXS15231B
    //   2. esp_lcd_panel_new_io_spi() + esp_lcd_panel_draw_bitmap()
    //   3. Init LVGL with esp_lcd flush callback
    //   4. Init CST820 I2C touch driver, register LVGL indev
}

void ui_show_setup_wizard(void) {
    ESP_LOGI(TAG, "STUB: setup wizard");
    // TODO: LVGL wizard screens
    //   Screen 1: "Welcome to WyVault Lite" + START
    //   Screen 2: Choose PIN (numpad)
    //   Screen 3: Confirm PIN
    //   Screen 4: Generating wallet... (show progress)
    //   Screen 5: Write down your mnemonic (word by word)
    //   Screen 6: Confirm mnemonic (re-enter 4 random words)
    //   Screen 7: Done!

    // Generate wallet
    const char *words[BIP39_WORD_COUNT];
    bip39_generate(words);

    uint8_t seed[64];
    bip39_to_seed(words, "", seed);

    // STUB PIN
    const char *pin = "1234";
    storage_save_wallet(seed, pin);
    memset(seed, 0, sizeof(seed));

    ESP_LOGI(TAG, "Wallet created (STUB PIN: 1234)");
}

void ui_show_pin_entry(char *pin_out, size_t pin_len) {
    ESP_LOGI(TAG, "STUB: PIN entry — using '1234'");
    // TODO: LVGL numpad with shuffle option, backspace, confirm
    strncpy(pin_out, "1234", pin_len);
}

void ui_show_home(app_state_t *state) {
    ESP_LOGI(TAG, "STUB: home screen");
    // TODO: LVGL home
    //   - CKB address (first 12 chars... last 6 chars)
    //   - Balance (fetched via WiFi, optional)
    //   - Bottom nav: [Sign] [Settings] [Lock]
    vTaskDelay(pdMS_TO_TICKS(5000));
    *state = STATE_LOCKED;
}

void ui_show_tx_review(const ckb_tx_display_t *tx, bool *approved_out) {
    ESP_LOGI(TAG, "STUB: tx review — auto-approving");
    ESP_LOGI(TAG, "  To: %s", tx->to_address);
    ESP_LOGI(TAG, "  Amount: %llu shannon", tx->amount_shannon);
    ESP_LOGI(TAG, "  Fee: %llu shannon", tx->fee_shannon);
    // TODO: LVGL screen showing full tx details
    //   Large address display + amount + fee + CONFIRM / REJECT buttons
    //   Red REJECT, green CONFIRM — no accidental taps
    *approved_out = false; // STUB: auto-reject for safety
}

void ui_show_signed_qr(const uint8_t *signed_tx, size_t len) {
    ESP_LOGI(TAG, "STUB: show signed tx QR (%zu bytes)", len);
    // TODO: generate QR with nayuki qrcodegen, render via LVGL canvas
}

void ui_show_sign_screen(app_state_t *state) {
    ESP_LOGI(TAG, "STUB: sign screen");
    // TODO:
    //   1. Wait for unsigned tx via USB serial (JSON)
    //   2. Parse + display via ui_show_tx_review()
    //   3. If approved: load seed, derive key, sign, show QR
    //   4. Show signing delay countdown (security feature)
    vTaskDelay(pdMS_TO_TICKS(2000));
    *state = STATE_UNLOCKED;
}

void ui_show_settings(app_state_t *state) {
    ESP_LOGI(TAG, "STUB: settings screen");
    vTaskDelay(pdMS_TO_TICKS(2000));
    *state = STATE_UNLOCKED;
}

void ui_show_error(const char *msg) {
    ESP_LOGE(TAG, "ERROR: %s", msg);
    // TODO: LVGL modal — red background, message, dismiss button
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void ui_show_success(const char *msg) {
    ESP_LOGI(TAG, "SUCCESS: %s", msg);
    vTaskDelay(pdMS_TO_TICKS(1500));
}

void ui_show_wiped(void) {
    ESP_LOGE(TAG, "DEVICE WIPED");
    // TODO: Show red screen "Device wiped — restore from mnemonic"
}

void ui_show_mnemonic(const char *words[], int count) {
    for (int i = 0; i < count; i++) {
        ESP_LOGI(TAG, "Word %d: %s", i+1, words[i]);
    }
}

bool ui_confirm_mnemonic(const char *words[], int count) {
    return true; // STUB
}

void ui_show_countdown(const char *msg, int seconds) {
    ESP_LOGI(TAG, "%s (%ds)", msg, seconds);
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
}
