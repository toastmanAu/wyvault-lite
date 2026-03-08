#include <stdio.h>
#include <string.h>
#include <string.h>
/*
 * WyVault Lite — crypto.c
 * STUB implementations — replace with trezor-crypto + BLAKE2b when integrating
 */
#include "crypto.h"
#include <string.h>
#include "esp_random.h"
#include "esp_log.h"
#include "mbedtls/sha512.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"

static const char *TAG = "crypto";

// ─── BIP39 stubs ─────────────────────────────────────────────────────────────
// TODO: integrate trezor-crypto bip39.c + wordlist
int bip39_generate(const char *words_out[BIP39_WORD_COUNT]) {
    ESP_LOGW(TAG, "bip39_generate: STUB");
    // Generate real entropy from hardware RNG
    uint8_t entropy[BIP39_ENTROPY_BYTES];
    esp_fill_random(entropy, sizeof(entropy));
    // TODO: convert entropy to mnemonic using BIP39 wordlist
    for (int i = 0; i < BIP39_WORD_COUNT; i++) words_out[i] = "abandon";
    return 0;
}

bool bip39_validate(const char *words[BIP39_WORD_COUNT]) {
    ESP_LOGW(TAG, "bip39_validate: STUB");
    return true;
}

int bip39_to_seed(const char *words[BIP39_WORD_COUNT],
                  const char *passphrase, uint8_t seed_out[64]) {
    ESP_LOGW(TAG, "bip39_to_seed: STUB");
    // TODO: PBKDF2-HMAC-SHA512("mnemonic" + passphrase, words, 2048 iter)
    esp_fill_random(seed_out, 64);
    return 0;
}

// ─── BIP32 stubs ─────────────────────────────────────────────────────────────
// TODO: integrate trezor-crypto bip32.c hdnode
int bip32_root_from_seed(const uint8_t seed[64], hd_node_t *node_out) {
    ESP_LOGW(TAG, "bip32_root_from_seed: STUB");
    memset(node_out, 0, sizeof(*node_out));
    memcpy(node_out->key, seed, 32);
    memcpy(node_out->chain_code, seed + 32, 32);
    return 0;
}

int bip32_derive_child(const hd_node_t *parent, uint32_t index, hd_node_t *child_out) {
    ESP_LOGW(TAG, "bip32_derive_child: STUB");
    *child_out = *parent;
    child_out->depth++;
    child_out->index = index;
    return 0;
}

int bip32_derive_path(const hd_node_t *root, const char *path, hd_node_t *node_out) {
    ESP_LOGW(TAG, "bip32_derive_path(%s): STUB", path);
    *node_out = *root;
    return 0;
}

int bip32_get_xpub(const hd_node_t *node, char *xpub_out, size_t xpub_len) {
    snprintf(xpub_out, xpub_len, "xpub_STUB");
    return 0;
}

// ─── CKB address ─────────────────────────────────────────────────────────────
int ckb_address_from_pubkey(const uint8_t pub_key[33],
                             char *address_out, size_t addr_len, bool mainnet) {
    // TODO: BLAKE2b-160(pubkey) → bech32m encode with "ckb"/"ckt" HRP
    snprintf(address_out, addr_len, "%s1qSTUB",
             mainnet ? "ckb" : "ckt");
    return 0;
}

// ─── BLAKE2b ─────────────────────────────────────────────────────────────────
// TODO: integrate BLAKE2 reference implementation (blake2b.c)
int blake2b_256(const uint8_t *data, size_t len, uint8_t hash_out[32]) {
    ESP_LOGW(TAG, "blake2b_256: STUB using SHA256");
    // Temporary: use SHA256 until BLAKE2b integrated
    // mbedtls_sha256() — swap for real BLAKE2b-256
    return -1; // STUB — do not use
}

int blake2b_160(const uint8_t *data, size_t len, uint8_t hash_out[20]) {
    uint8_t full[32];
    int r = blake2b_256(data, len, full);
    if (r == 0) memcpy(hash_out, full, 20);
    return r;
}

// ─── CKB signing ─────────────────────────────────────────────────────────────
int ckb_parse_unsigned_tx(const char *tx_json, ckb_tx_display_t *display_out) {
    // TODO: parse Molecule-serialized tx JSON
    memset(display_out, 0, sizeof(*display_out));
    snprintf(display_out->to_address, sizeof(display_out->to_address),
             "ckb1STUB...");
    display_out->amount_shannon = 10000000000ULL; // 100 CKB
    display_out->fee_shannon    = 1000;
    return 0;
}

int ckb_sign_tx(const uint8_t privkey[32],
                const uint8_t tx_hash[32],
                uint8_t sig_out[65]) {
    ESP_LOGW(TAG, "ckb_sign_tx: STUB — no real signing");
    // TODO: secp256k1_ecdsa_sign(privkey, tx_hash) via esp-idf mbedtls or trezor-crypto
    memset(sig_out, 0xAB, 65);
    return 0;
}

// ─── AES-256-GCM ─────────────────────────────────────────────────────────────
int aes_gcm_encrypt(const uint8_t key[AES_KEY_LEN],
                    const uint8_t *plaintext, size_t plaintext_len,
                    uint8_t *ciphertext_out,
                    uint8_t iv_out[AES_IV_LEN],
                    uint8_t tag_out[AES_TAG_LEN]) {
    // Generate random IV
    esp_fill_random(iv_out, AES_IV_LEN);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES,
                                  key, AES_KEY_LEN * 8);
    if (ret) goto done;
    ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                     plaintext_len, iv_out, AES_IV_LEN,
                                     NULL, 0,
                                     plaintext, ciphertext_out,
                                     AES_TAG_LEN, tag_out);
done:
    mbedtls_gcm_free(&gcm);
    return ret;
}

int aes_gcm_decrypt(const uint8_t key[AES_KEY_LEN],
                    const uint8_t *ciphertext, size_t ciphertext_len,
                    const uint8_t iv[AES_IV_LEN],
                    const uint8_t tag[AES_TAG_LEN],
                    uint8_t *plaintext_out) {
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES,
                                  key, AES_KEY_LEN * 8);
    if (ret) goto done;
    ret = mbedtls_gcm_auth_decrypt(&gcm, ciphertext_len,
                                    iv, AES_IV_LEN,
                                    NULL, 0,
                                    tag, AES_TAG_LEN,
                                    ciphertext, plaintext_out);
done:
    mbedtls_gcm_free(&gcm);
    return ret;
}

// ─── Key derivation ──────────────────────────────────────────────────────────
int derive_storage_key(const char *pin, const uint8_t *salt, size_t salt_len,
                        uint8_t key_out[AES_KEY_LEN]) {
    // Get device secret
    uint8_t device_secret[32];
    get_device_secret(device_secret);

    // IKM = PIN bytes + device secret
    uint8_t ikm[256];
    size_t pin_len = strlen(pin);
    memcpy(ikm, pin, pin_len);
    memcpy(ikm + pin_len, device_secret, 32);
    size_t ikm_len = pin_len + 32;

    // HKDF-SHA256
    const uint8_t info[] = "WyVaultLite_v1";
    int ret = mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                            salt, salt_len,
                            ikm, ikm_len,
                            info, sizeof(info) - 1,
                            key_out, AES_KEY_LEN);
    // Clear sensitive data
    memset(ikm, 0, sizeof(ikm));
    memset(device_secret, 0, sizeof(device_secret));
    return ret;
}

int get_device_secret(uint8_t secret_out[32]) {
    // TODO: read from eFuse BLOCK3 (user data, 256 bits)
    // On first run: generate + burn. On subsequent runs: read.
    // For now: use a fixed stub (INSECURE — replace before production)
    static const uint8_t STUB_SECRET[32] = {
        0x57,0x79,0x56,0x61,0x75,0x6c,0x74,0x4c,  // "WyVaultL"
        0x69,0x74,0x65,0x44,0x65,0x76,0x53,0x65,  // "iteDevSe"
        0x63,0x72,0x65,0x74,0x30,0x30,0x30,0x30,  // "cret0000"
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,  // "00000000"
    };
    memcpy(secret_out, STUB_SECRET, 32);
    return 0;
}
