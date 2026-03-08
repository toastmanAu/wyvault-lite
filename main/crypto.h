/*
 * WyVault Lite — crypto.h
 * CKB/Bitcoin signing primitives: BIP39, BIP32, secp256k1, blake2b, AES-GCM
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ─── BIP39 ───────────────────────────────────────────────────────────────────
#define BIP39_WORD_COUNT    24
#define BIP39_ENTROPY_BYTES 32   // 256-bit entropy → 24 words

// Generate 24-word mnemonic from hardware RNG
// words_out: array of BIP39_WORD_COUNT pointers into static wordlist
int bip39_generate(const char *words_out[BIP39_WORD_COUNT]);

// Validate a mnemonic (check words + checksum)
bool bip39_validate(const char *words[BIP39_WORD_COUNT]);

// Derive 512-bit seed from mnemonic + optional passphrase
// seed_out must be 64 bytes
int bip39_to_seed(const char *words[BIP39_WORD_COUNT],
                  const char *passphrase,
                  uint8_t seed_out[64]);

// ─── BIP32 HD derivation ─────────────────────────────────────────────────────
typedef struct {
    uint8_t key[32];        // private key
    uint8_t chain_code[32]; // chain code
    uint8_t pub_key[33];    // compressed public key
    uint32_t depth;
    uint32_t index;
} hd_node_t;

// Derive root node from 64-byte BIP39 seed
int bip32_root_from_seed(const uint8_t seed[64], hd_node_t *node_out);

// Derive child node (hardened if index >= 0x80000000)
int bip32_derive_child(const hd_node_t *parent, uint32_t index, hd_node_t *child_out);

// Convenience: derive by path string e.g. "m/44'/309'/0'/0/0"
int bip32_derive_path(const hd_node_t *root, const char *path, hd_node_t *node_out);

// Get xpub string for a node (for watch-only companion app)
int bip32_get_xpub(const hd_node_t *node, char *xpub_out, size_t xpub_len);

// ─── CKB address ─────────────────────────────────────────────────────────────
// Derive CKB mainnet address from compressed public key
// Uses secp256k1/blake160 lock script (m/44'/309'/0'/0/0)
int ckb_address_from_pubkey(const uint8_t pub_key[33],
                              char *address_out, size_t addr_len,
                              bool mainnet);

// ─── Transaction signing ─────────────────────────────────────────────────────
typedef struct {
    char     to_address[100];
    uint64_t amount_shannon;   // 1 CKB = 100,000,000 shannon
    uint64_t fee_shannon;
    char     data_hex[256];    // optional OP_RETURN / cell data
} ckb_tx_display_t;

// Parse unsigned tx JSON → display struct (for showing on screen before signing)
int ckb_parse_unsigned_tx(const char *tx_json, ckb_tx_display_t *display_out);

// Sign CKB transaction
// privkey: 32-byte private key
// tx_hash: 32-byte transaction hash (blake2b-256 of tx minus witnesses)
// sig_out: 65-byte signature (r[32] + s[32] + v[1])
int ckb_sign_tx(const uint8_t privkey[32],
                const uint8_t tx_hash[32],
                uint8_t sig_out[65]);

// ─── BLAKE2b ─────────────────────────────────────────────────────────────────
// CKB uses BLAKE2b-256 with no personalisation
int blake2b_256(const uint8_t *data, size_t len, uint8_t hash_out[32]);
int blake2b_160(const uint8_t *data, size_t len, uint8_t hash_out[20]);

// ─── AES-256-GCM (for seed encryption) ──────────────────────────────────────
#define AES_KEY_LEN   32
#define AES_IV_LEN    12
#define AES_TAG_LEN   16

// Encrypt plaintext with AES-256-GCM
// iv_out: random IV written here (12 bytes)
// tag_out: authentication tag (16 bytes)
int aes_gcm_encrypt(const uint8_t key[AES_KEY_LEN],
                    const uint8_t *plaintext, size_t plaintext_len,
                    uint8_t *ciphertext_out,
                    uint8_t iv_out[AES_IV_LEN],
                    uint8_t tag_out[AES_TAG_LEN]);

// Decrypt — returns -1 if tag verification fails (tampered data)
int aes_gcm_decrypt(const uint8_t key[AES_KEY_LEN],
                    const uint8_t *ciphertext, size_t ciphertext_len,
                    const uint8_t iv[AES_IV_LEN],
                    const uint8_t tag[AES_TAG_LEN],
                    uint8_t *plaintext_out);

// ─── Key derivation ──────────────────────────────────────────────────────────
// Derive AES key from PIN + device secret (eFuse-burned)
// HKDF-SHA256(IKM = PIN || device_secret, salt = random, info = "WyVaultLite")
int derive_storage_key(const char *pin,
                        const uint8_t *salt, size_t salt_len,
                        uint8_t key_out[AES_KEY_LEN]);

// Read device secret from eFuse (or generate + burn on first run)
int get_device_secret(uint8_t secret_out[32]);
