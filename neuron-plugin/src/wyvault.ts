/**
 * WyVault Lite — Neuron Plugin
 * wyvault.ts — Hardware class implementing Neuron's signing interface
 *
 * Neuron calls:
 *   - findDevices()       → list connected WyVault devices
 *   - connect()           → open serial port
 *   - getExtendedPublicKey() → get xpub for wallet import
 *   - signTransaction()   → sign a CKB transaction
 *   - disconnect()        → close serial port
 */

import { WyVaultTransport } from './transport'
import { DeviceInfo, ExtendedPublicKey, Manufacturer, TxDisplay } from './common'

// Shannon = 1e-8 CKB
const SHANNON_PER_CKB = 100_000_000n

function shannonToCkb(shannon: bigint | string): string {
  const s = BigInt(shannon)
  const whole = s / SHANNON_PER_CKB
  const frac = s % SHANNON_PER_CKB
  if (frac === 0n) return whole.toString()
  return `${whole}.${frac.toString().padStart(8, '0').replace(/0+$/, '')}`
}

export class WyVault {
  public deviceInfo: DeviceInfo
  public isConnected: boolean = false
  private transport: WyVaultTransport

  // CKB account path — same as Neuron's default
  protected defaultPath = "m/44'/309'/0'"

  constructor(device: DeviceInfo) {
    this.deviceInfo = device
    this.transport = new WyVaultTransport()
  }

  // ─── Static discovery ─────────────────────────────────────────────────────
  static async findDevices(): Promise<DeviceInfo[]> {
    return WyVaultTransport.findDevices()
  }

  // ─── Lifecycle ────────────────────────────────────────────────────────────
  async connect(): Promise<void> {
    await this.transport.connect(this.deviceInfo.descriptor)
    this.isConnected = true

    // Verify device responds
    const status = await this.transport.send({ action: 'ping' })
    if (!status.ok) throw new Error('WyVault ping failed')
    this.deviceInfo.firmwareVersion = status.firmware_version
    console.info(`[WyVault] Connected: ${this.deviceInfo.descriptor} fw=${status.firmware_version}`)
  }

  async disconnect(): Promise<void> {
    await this.transport.disconnect()
    this.isConnected = false
  }

  // ─── Key export (for wallet import into Neuron) ───────────────────────────
  async getExtendedPublicKey(): Promise<ExtendedPublicKey> {
    const resp = await this.transport.send({
      action: 'get_pubkey',
      path: this.defaultPath,
    })
    if (!resp.public_key || !resp.chain_code) {
      throw new Error('WyVault: no public key in response')
    }
    return {
      publicKey: resp.public_key,
      chainCode: resp.chain_code,
    }
  }

  // ─── Transaction signing ─────────────────────────────────────────────────
  /**
   * Sign a CKB transaction.
   * Called by Neuron's Hardware base class signTx() orchestration.
   *
   * @param walletId  Neuron wallet ID (for address lookup)
   * @param tx        The transaction object
   * @param witnesses Serialized witnesses (with empty lock placeholder)
   * @param path      BIP32 signing path
   * @returns 65-byte signature as 0x-prefixed hex
   */
  async signTransaction(
    walletId: string,
    tx: { hash: string; outputs?: Array<{ capacity: string }> },
    witnesses: string[],
    path: string,
  ): Promise<string> {
    // Build human-readable display for the WyVault screen
    const txDisplay = this.buildTxDisplay(tx)

    console.info('[WyVault] Requesting signature for tx:', tx.hash)
    console.info('[WyVault] Display:', txDisplay)

    const resp = await this.transport.send({
      action: 'sign_tx',
      path,
      tx_hash: tx.hash,
      tx_display: txDisplay,
      witnesses,
    })

    if (!resp.signature) {
      throw new Error('WyVault: no signature in response')
    }

    console.info('[WyVault] Signature received:', resp.signature.slice(0, 20) + '...')
    return resp.signature
  }

  // ─── Build display info for on-device tx review ───────────────────────────
  private buildTxDisplay(tx: {
    hash: string
    outputs?: Array<{ capacity: string; lock?: { args: string } }>
  }): TxDisplay {
    // Best-effort: extract first output for display
    // Full tx parsing would require Molecule deserialization
    const firstOutput = tx.outputs?.[0]
    const totalCapacity = tx.outputs?.reduce(
      (sum, o) => sum + BigInt(o.capacity ?? 0), 0n
    ) ?? 0n

    return {
      to: firstOutput?.lock?.args
        ? `...${firstOutput.lock.args.slice(-8)}`
        : 'unknown',
      amount_ckb: shannonToCkb(totalCapacity),
      fee_ckb: '(shown on device)',  // fee requires input sum — Neuron has it
    }
  }
}
