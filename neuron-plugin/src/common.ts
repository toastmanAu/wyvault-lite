/**
 * WyVault Lite — Neuron Plugin
 * common.ts — shared types mirroring Neuron's hardware/common.ts
 */

export enum Manufacturer {
  Ledger = 'Ledger',
  WyVault = 'WyVault',
}

export interface DeviceInfo {
  manufacturer: Manufacturer
  product: string
  descriptor: string    // serial port path e.g. /dev/ttyACM0 or COM3
  vendorId: string
  productId: string
  firmwareVersion?: string
}

export interface ExtendedPublicKey {
  publicKey: string    // compressed, hex
  chainCode: string    // hex
}

export interface PublicKey {
  publicKey: string
  path: string
}

// Protocol message types (JSON over USB CDC serial)
export interface WyVaultRequest {
  id: number
  action: 'get_pubkey' | 'sign_tx' | 'get_status' | 'ping'
  path?: string        // BIP32 path e.g. "m/44'/309'/0'/0/0"
  tx_hash?: string     // 0x-prefixed hex
  tx_display?: TxDisplay
  witnesses?: string[] // serialized witnesses for signing
}

export interface WyVaultResponse {
  id: number
  ok: boolean
  error?: string
  // get_pubkey
  public_key?: string   // compressed hex
  chain_code?: string   // hex
  // sign_tx
  signature?: string    // 0x-prefixed 65-byte hex (r+s+v)
  // get_status
  firmware_version?: string
  wallet_mode?: 'cold' | 'hot'
  has_wallet?: boolean
}

export interface TxDisplay {
  to: string
  amount_ckb: string   // human readable e.g. "100.5"
  fee_ckb: string
  data?: string
}
