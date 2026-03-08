# WyVault Lite — Neuron Plugin

Neuron hardware wallet plugin for **WyVault Lite** (ESP32-S3 CKB hardware wallet).

## Status: SCAFFOLD — not production ready

## What it does

Lets Neuron (the official CKB desktop wallet) use WyVault Lite as a hardware signing device via USB.

**Flow:**
1. Plug WyVault into PC via USB
2. Open Neuron → Hardware Wallet → WyVault Lite
3. Import watch-only wallet (reads xpub from device)
4. When sending: Neuron requests signature → WyVault displays tx on-device screen → user approves → signed tx returned → Neuron broadcasts

## Protocol

JSON over USB CDC serial (115200 baud, newline-delimited).

**WyVault appears as:** `303a:1001` (Espressif ESP32-S3 CDC ACM)

### Request (host → device)
```json
{"id": 1, "action": "sign_tx", "path": "m/44'/309'/0'/0/0",
 "tx_hash": "0x...", "tx_display": {"to": "ckb1...", "amount_ckb": "100", "fee_ckb": "0.001"},
 "witnesses": ["0x..."]}
```

### Response (device → host)
```json
{"id": 1, "ok": true, "signature": "0x<65-byte-hex>"}
```

### Actions
| Action | Description |
|--------|-------------|
| `ping` | Health check, returns firmware version |
| `get_pubkey` | Get extended public key for BIP32 path |
| `sign_tx` | Sign transaction — shows on-device review screen |
| `get_status` | Get device status (wallet mode, firmware) |

## Neuron Integration

This plugin mirrors Neuron's `Hardware` abstract class interface. To integrate:

```typescript
// In neuron/packages/neuron-wallet/src/services/hardware/index.ts
import WyVault from './wyvault-plugin'

// Add to constructor:
this.supportedHardwares.set(Manufacturer.WyVault, WyVault)

// Add to findDevices():
WyVault.findDevices(),
```

## Building

```bash
npm install
npm run build
```

## Dual Wallet Architecture

WyVault exposes two logical wallets on a single device:

- **Cold** (`m/44'/309'/0'`) — air-gapped, signs via on-device approval
- **Hot** (`m/44'/309'/1'`) — connected mode, device can broadcast directly

The Neuron plugin handles the cold wallet. The hot wallet is managed directly on-device.
Transfers between cold and hot are handled on the device UI (no plugin needed).
