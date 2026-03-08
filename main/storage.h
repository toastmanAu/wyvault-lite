/*
 * WyVault Lite — storage.h
 * Encrypted NVS storage for wallet seed + settings
 *
 * Layout in NVS namespace "wyvault":
 *   "has_wallet"   u8      — 1 if wallet initialised
 *   "seed_enc"     blob    — AES-256-GCM encrypted 64-byte seed
 *   "seed_iv"      blob    — 12-byte IV for seed encryption
 *   "seed_tag"     blob    — 16-byte GCM auth tag
 *   "key_salt"     blob    — 32-byte HKDF salt (random, stored plaintext)
 *   "pin_hash"     blob    — bcrypt/PBKDF2 of PIN (for verification)
 *   "fp_enabled"   u8      — 1 if fingerprint enrolled
 *   "canary_addr"  str     — watch address for tamper detection
 *   "sign_count"   u32     — total signatures issued
 *   "settings"     blob    — packed settings struct
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "crypto.h"

typedef struct {
    bool  wifi_location_binding;  // Require known WiFi SSIDs to unlock
    bool  fingerprint_enabled;
    bool  duress_wallet_enabled;
    uint8_t auto_lock_minutes;    // 0 = never
    uint8_t signing_delay_secs;   // Mandatory delay before signing (0-30s)
    uint32_t max_signs_per_hour;  // 0 = unlimited
} wyvault_settings_t;

// Init storage subsystem
void storage_init(void);

// Check if wallet has been set up
bool storage_has_wallet(void);

// Save a new wallet seed (encrypted with PIN-derived key)
// Clears any existing wallet first
int storage_save_wallet(const uint8_t seed[64], const char *pin);

// Load and decrypt wallet seed
// Returns 0 on success, -1 on wrong PIN or tampered data
int storage_load_seed(const char *pin, uint8_t seed_out[64]);

// Verify PIN without loading seed (fast check)
bool storage_verify_pin(const char *pin);

// Change PIN (re-encrypts seed with new PIN)
int storage_change_pin(const char *old_pin, const char *new_pin);

// Fingerprint
bool storage_fingerprint_enabled(void);
void storage_set_fingerprint_enabled(bool enabled);

// Settings
void storage_get_settings(wyvault_settings_t *s);
void storage_save_settings(const wyvault_settings_t *s);

// Signing counter (tamper/anomaly detection)
uint32_t storage_get_sign_count(void);
void storage_increment_sign_count(void);

// Wipe everything — called after max PIN attempts
void storage_wipe(void);

// Canary address
void storage_set_canary_address(const char *addr);
bool storage_get_canary_address(char *addr_out, size_t len);
