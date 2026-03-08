/*
 * WyVault Lite — main.c
 * Hardware wallet firmware for JC3248W535 (ESP32-S3 N16R8)
 * 
 * Architecture:
 *   - Boot → check flash encryption → PIN screen
 *   - Unlocked → Home (accounts) | Sign | Settings
 *   - Signing: receive unsigned tx via USB serial → display details → approve → output signed QR
 *
 * Security model:
 *   - Flash encryption (AES-XTS) enabled at first boot
 *   - Secure boot v2 (enable manually after firmware stable)
 *   - Seed encrypted with AES-256-GCM, key = HKDF(PIN || device_secret)
 *   - Optional: fingerprint (SFM-V1.7) as second factor
 *   - WiFi disabled during signing operations
 *
 * Board: Guition JC3248W535 (ESP32-S3-WROOM-1U-N16R8)
 *   Display: AXS15231B, 320x480, QSPI
 *   Touch: CST820 (I2C)
 *   Flash: 16MB, PSRAM: 8MB
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_flash_encrypt.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_random.h"

#include "ui.h"
#include "crypto.h"
#include "storage.h"
#include "fingerprint.h"
#include "main.h"

static const char *TAG = "wyvault";

// ─── App state machine ───────────────────────────────────────────────────────

static app_state_t s_state = STATE_BOOT;
static int s_pin_attempts = 0;
#define MAX_PIN_ATTEMPTS 5

// ─── Security checks ─────────────────────────────────────────────────────────
static void check_flash_encryption(void) {
    if (!esp_flash_encryption_enabled()) {
        ESP_LOGW(TAG, "Flash encryption NOT enabled — device is not secure");
        ESP_LOGW(TAG, "Run 'idf.py efuse-burn-key' to enable for production");
        // In dev builds: continue anyway
        // In production builds: halt
        #ifdef CONFIG_WYVAULT_REQUIRE_FLASH_ENC
        ESP_LOGE(TAG, "Production mode requires flash encryption. Halting.");
        esp_restart();
        #endif
    } else {
        ESP_LOGI(TAG, "Flash encryption: ENABLED");
    }
}

// ─── Main task ───────────────────────────────────────────────────────────────
void app_main(void) {
    ESP_LOGI(TAG, "WyVault Lite v0.1 booting...");

    // Security checks
    check_flash_encryption();

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init storage
    storage_init();

    // Init display + touch
    ui_init();

    // Init fingerprint sensor (optional — stub if not connected)
    fingerprint_init();

    // Determine boot state
    if (storage_has_wallet()) {
        s_state = STATE_PIN_ENTRY;
        ESP_LOGI(TAG, "Wallet found — requesting PIN");
    } else {
        s_state = STATE_FIRST_RUN;
        ESP_LOGI(TAG, "No wallet — starting setup wizard");
    }

    // Hand off to UI state machine
    while (1) {
        switch (s_state) {
            case STATE_FIRST_RUN:
                ui_show_setup_wizard();
                // ui_show_setup_wizard blocks until complete
                s_state = STATE_PIN_ENTRY;
                break;

            case STATE_PIN_ENTRY: {
                char pin[9] = {0};
                ui_show_pin_entry(pin, sizeof(pin));
                // Verify PIN
                if (storage_verify_pin(pin)) {
                    s_pin_attempts = 0;
                    // Optional: fingerprint second factor
                    if (storage_fingerprint_enabled()) {
                        if (fingerprint_verify()) {
                            s_state = STATE_UNLOCKED;
                        } else {
                            ui_show_error("Fingerprint failed");
                            s_state = STATE_PIN_ENTRY;
                        }
                    } else {
                        s_state = STATE_UNLOCKED;
                    }
                } else {
                    s_pin_attempts++;
                    ESP_LOGW(TAG, "Wrong PIN (%d/%d)", s_pin_attempts, MAX_PIN_ATTEMPTS);
                    if (s_pin_attempts >= MAX_PIN_ATTEMPTS) {
                        ESP_LOGE(TAG, "Max PIN attempts — wiping device");
                        storage_wipe();
                        s_state = STATE_WIPED;
                    } else {
                        char msg[48];
                        snprintf(msg, sizeof(msg), "Wrong PIN (%d attempts left)",
                                 MAX_PIN_ATTEMPTS - s_pin_attempts);
                        ui_show_error(msg);
                    }
                }
                memset(pin, 0, sizeof(pin));  // Clear PIN from stack
                break;
            }

            case STATE_UNLOCKED:
                ui_show_home(&s_state);
                // ui_show_home updates s_state based on user nav
                break;

            case STATE_SIGNING:
                ui_show_sign_screen(&s_state);
                break;

            case STATE_SETTINGS:
                ui_show_settings(&s_state);
                break;

            case STATE_LOCKED:
                s_state = STATE_PIN_ENTRY;
                break;

            case STATE_WIPED:
                ui_show_wiped();
                while(1) vTaskDelay(pdMS_TO_TICKS(1000));  // Halt
                break;

            default:
                vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
