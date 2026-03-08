#include "storage.h"
#include "crypto.h"
#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/sha256.h"

static const char *TAG = "storage";
#define NVS_NS "wyvault"

void storage_init(void) {
    ESP_LOGI(TAG, "Storage initialised");
}

bool storage_has_wallet(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    uint8_t val = 0;
    nvs_get_u8(h, "has_wallet", &val);
    nvs_close(h);
    return val == 1;
}

int storage_save_wallet(const uint8_t seed[64], const char *pin) {
    // Generate random salt
    uint8_t salt[32];
    esp_fill_random(salt, sizeof(salt));

    // Derive AES key from PIN + device secret
    uint8_t key[AES_KEY_LEN];
    if (derive_storage_key(pin, salt, sizeof(salt), key) != 0) return -1;

    // Encrypt seed
    uint8_t ciphertext[64], iv[AES_IV_LEN], tag[AES_TAG_LEN];
    if (aes_gcm_encrypt(key, seed, 64, ciphertext, iv, tag) != 0) {
        memset(key, 0, sizeof(key)); return -1;
    }
    memset(key, 0, sizeof(key));

    // PIN hash for fast verification (PBKDF2-SHA256, 10000 iter)
    // TODO: replace with proper PBKDF2 — using SHA256 stub for now
    uint8_t pin_hash[32];
    mbedtls_sha256((uint8_t*)pin, strlen(pin), pin_hash, 0);

    // Write to NVS
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));
    nvs_set_blob(h, "seed_enc", ciphertext, 64);
    nvs_set_blob(h, "seed_iv",  iv,         AES_IV_LEN);
    nvs_set_blob(h, "seed_tag", tag,         AES_TAG_LEN);
    nvs_set_blob(h, "key_salt", salt,        32);
    nvs_set_blob(h, "pin_hash", pin_hash,    32);
    nvs_set_u8(h, "has_wallet", 1);
    nvs_set_u8(h, "fp_enabled", 0);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Wallet saved");
    return 0;
}

int storage_load_seed(const char *pin, uint8_t seed_out[64]) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return -1;

    uint8_t salt[32], iv[AES_IV_LEN], tag[AES_TAG_LEN], ciphertext[64];
    size_t len;
    len = 32;    nvs_get_blob(h, "key_salt", salt,       &len);
    len = AES_IV_LEN; nvs_get_blob(h, "seed_iv", iv,    &len);
    len = AES_TAG_LEN; nvs_get_blob(h, "seed_tag", tag, &len);
    len = 64;    nvs_get_blob(h, "seed_enc", ciphertext, &len);
    nvs_close(h);

    uint8_t key[AES_KEY_LEN];
    if (derive_storage_key(pin, salt, 32, key) != 0) return -1;
    int ret = aes_gcm_decrypt(key, ciphertext, 64, iv, tag, seed_out);
    memset(key, 0, sizeof(key));
    if (ret != 0) ESP_LOGE(TAG, "Seed decrypt failed — wrong PIN or tampered data");
    return ret;
}

bool storage_verify_pin(const char *pin) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    uint8_t stored_hash[32]; size_t len = 32;
    nvs_get_blob(h, "pin_hash", stored_hash, &len);
    nvs_close(h);
    uint8_t pin_hash[32];
    mbedtls_sha256((uint8_t*)pin, strlen(pin), pin_hash, 0);
    return memcmp(pin_hash, stored_hash, 32) == 0;
}

int storage_change_pin(const char *old_pin, const char *new_pin) {
    uint8_t seed[64];
    if (storage_load_seed(old_pin, seed) != 0) return -1;
    int ret = storage_save_wallet(seed, new_pin);
    memset(seed, 0, sizeof(seed));
    return ret;
}

bool storage_fingerprint_enabled(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    uint8_t val = 0; nvs_get_u8(h, "fp_enabled", &val);
    nvs_close(h); return val == 1;
}

void storage_set_fingerprint_enabled(bool enabled) {
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    nvs_set_u8(h, "fp_enabled", enabled ? 1 : 0);
    nvs_commit(h); nvs_close(h);
}

void storage_get_settings(wyvault_settings_t *s) {
    memset(s, 0, sizeof(*s));
    s->auto_lock_minutes = 5;
    s->signing_delay_secs = 3;
}

void storage_save_settings(const wyvault_settings_t *s) {
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    nvs_set_blob(h, "settings", s, sizeof(*s));
    nvs_commit(h); nvs_close(h);
}

uint32_t storage_get_sign_count(void) {
    nvs_handle_t h; uint32_t count = 0;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u32(h, "sign_count", &count); nvs_close(h);
    }
    return count;
}

void storage_increment_sign_count(void) {
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    uint32_t count = 0; nvs_get_u32(h, "sign_count", &count);
    nvs_set_u32(h, "sign_count", count + 1);
    nvs_commit(h); nvs_close(h);
}

void storage_wipe(void) {
    ESP_LOGE(TAG, "WIPING DEVICE");
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    nvs_erase_all(h); nvs_commit(h); nvs_close(h);
    ESP_LOGE(TAG, "Device wiped");
}

void storage_set_canary_address(const char *addr) {
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    nvs_set_str(h, "canary_addr", addr);
    nvs_commit(h); nvs_close(h);
}

bool storage_get_canary_address(char *addr_out, size_t len) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    esp_err_t r = nvs_get_str(h, "canary_addr", addr_out, &len);
    nvs_close(h); return r == ESP_OK;
}
