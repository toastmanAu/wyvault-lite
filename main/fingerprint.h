/*
 * WyVault Lite — fingerprint.h
 * SFM-V1.7 optical fingerprint sensor driver
 *
 * SFM-V1.7 protocol: UART at 57600 baud, 3.3V
 * Communicates via packet protocol (header + command + data + checksum)
 * Stores templates onboard (up to 1000 fingerprints)
 *
 * Wiring on JC3248W535:
 *   VCC → 3.3V
 *   GND → GND
 *   TX  → GPIO_NUM_? (free IO)
 *   RX  → GPIO_NUM_? (free IO)
 *   (Touch/Wakeup pin optional)
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

// Init UART to fingerprint sensor
// Returns 0 if sensor responds, -1 if not connected (graceful — sensor is optional)
int fingerprint_init(void);

// Check if sensor is available
bool fingerprint_available(void);

// Enroll a new fingerprint (takes 2 scans for verification)
// Returns template ID (0-999) on success, -1 on failure
int fingerprint_enroll(void);

// Verify fingerprint against stored templates
// Returns template ID on match, -1 on no match or timeout
int fingerprint_verify(void);

// Delete all stored templates
int fingerprint_clear_all(void);

// Delete specific template
int fingerprint_delete(uint16_t template_id);

// Get count of enrolled templates
int fingerprint_count(void);
