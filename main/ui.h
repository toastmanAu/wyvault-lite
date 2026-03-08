/*
 * WyVault Lite — ui.h
 * LVGL screen management for JC3248W535 (AXS15231B 320x480 touch)
 *
 * Screen flow:
 *   setup_wizard → pin_entry → home → [sign | settings]
 *
 * All blocking screens return when user action completes.
 * State transitions handled in main.c
 */
#pragma once
#include <stdint.h>
#include "main.h"   // for app_state_t
#include "crypto.h" // for ckb_tx_display_t

// Init display driver + LVGL + touch
void ui_init(void);

// ─── Setup wizard ─────────────────────────────────────────────────────────────
// First-run wizard: choose PIN → generate mnemonic → confirm mnemonic → done
// Blocks until complete. Writes wallet to storage internally.
void ui_show_setup_wizard(void);

// ─── PIN entry ────────────────────────────────────────────────────────────────
// Show PIN pad, write entered PIN into pin_out (null-terminated)
// Blocks until user taps CONFIRM
void ui_show_pin_entry(char *pin_out, size_t pin_len);

// ─── Home screen ──────────────────────────────────────────────────────────────
// Shows: CKB address (truncated), balance (read-only, fetched via WiFi if enabled)
// Bottom nav: Sign | Settings | Lock
// Updates *state on user action
void ui_show_home(app_state_t *state);

// ─── Signing screen ───────────────────────────────────────────────────────────
// Receive unsigned tx via USB serial, display details, CONFIRM or REJECT
// On CONFIRM: signs internally, displays signed tx as QR code
// Updates *state when done
void ui_show_sign_screen(app_state_t *state);

// Display tx details for user review before signing
// Shows: to address, amount, fee, data (if any)
void ui_show_tx_review(const ckb_tx_display_t *tx, bool *approved_out);

// Show signed tx as QR code (for companion app to scan)
void ui_show_signed_qr(const uint8_t *signed_tx, size_t len);

// ─── Settings screen ─────────────────────────────────────────────────────────
void ui_show_settings(app_state_t *state);

// ─── Utility screens ──────────────────────────────────────────────────────────
void ui_show_error(const char *msg);
void ui_show_success(const char *msg);
void ui_show_wiped(void);  // "Device wiped" — shown after max PIN attempts

// Show mnemonic words (for backup) — one word at a time with prev/next
void ui_show_mnemonic(const char *words[], int count);

// Ask user to confirm mnemonic words (anti-typo check)
bool ui_confirm_mnemonic(const char *words[], int count);

// Countdown timer on screen (for signing delay security feature)
void ui_show_countdown(const char *msg, int seconds);
