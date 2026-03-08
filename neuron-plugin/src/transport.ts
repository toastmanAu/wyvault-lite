/**
 * WyVault Lite — Neuron Plugin
 * transport.ts — USB CDC serial transport layer
 *
 * WyVault appears as a CDC ACM serial device (same as WyTerminal).
 * VID: 303a (Espressif), PID: 1001 (ESP32-S3 JTAG/CDC)
 * Protocol: newline-delimited JSON, 115200 baud
 *
 * Request:  {"id":1,"action":"sign_tx","tx_hash":"0x...","tx_display":{...}}\n
 * Response: {"id":1,"ok":true,"signature":"0x..."}\n
 */

import { SerialPort } from 'serialport'
import { DeviceInfo, Manufacturer, WyVaultRequest, WyVaultResponse } from './common'

const WYVAULT_VID = '303a'
const WYVAULT_PID = '1001'
const BAUD_RATE = 115200
const RESPONSE_TIMEOUT_MS = 60_000  // 60s — user needs time to approve on device

export class WyVaultTransport {
  private port: SerialPort | null = null
  private buffer = ''
  private pendingRequests = new Map<number, {
    resolve: (r: WyVaultResponse) => void
    reject: (e: Error) => void
    timer: NodeJS.Timeout
  }>()
  private nextId = 1

  // ─── Device discovery ──────────────────────────────────────────────────────
  static async findDevices(): Promise<DeviceInfo[]> {
    const ports = await SerialPort.list()
    return ports
      .filter(p =>
        p.vendorId?.toLowerCase() === WYVAULT_VID &&
        p.productId?.toLowerCase() === WYVAULT_PID
      )
      .map(p => ({
        manufacturer: Manufacturer.WyVault,
        product: 'WyVault Lite',
        descriptor: p.path,
        vendorId: p.vendorId ?? WYVAULT_VID,
        productId: p.productId ?? WYVAULT_PID,
      }))
  }

  // ─── Connect ───────────────────────────────────────────────────────────────
  async connect(portPath: string): Promise<void> {
    if (this.port?.isOpen) return

    this.port = new SerialPort({
      path: portPath,
      baudRate: BAUD_RATE,
      autoOpen: false,
    })

    await new Promise<void>((resolve, reject) => {
      this.port!.open(err => err ? reject(err) : resolve())
    })

    // Accumulate data until newline, then dispatch response
    this.port.on('data', (chunk: Buffer) => {
      this.buffer += chunk.toString('utf8')
      const lines = this.buffer.split('\n')
      this.buffer = lines.pop() ?? ''
      for (const line of lines) {
        const trimmed = line.trim()
        if (!trimmed) continue
        try {
          const resp: WyVaultResponse = JSON.parse(trimmed)
          this.dispatch(resp)
        } catch {
          console.warn('[WyVault] Unparseable line:', trimmed)
        }
      }
    })

    this.port.on('error', err => {
      console.error('[WyVault] Serial error:', err)
    })
  }

  async disconnect(): Promise<void> {
    if (this.port?.isOpen) {
      await new Promise<void>(resolve => this.port!.close(() => resolve()))
    }
    this.port = null
  }

  // ─── Send request, await response ─────────────────────────────────────────
  async send(req: Omit<WyVaultRequest, 'id'>): Promise<WyVaultResponse> {
    if (!this.port?.isOpen) throw new Error('WyVault not connected')

    const id = this.nextId++
    const message = JSON.stringify({ ...req, id }) + '\n'

    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pendingRequests.delete(id)
        reject(new Error(`WyVault request timeout (${req.action})`))
      }, RESPONSE_TIMEOUT_MS)

      this.pendingRequests.set(id, { resolve, reject, timer })
      this.port!.write(message, err => {
        if (err) {
          clearTimeout(timer)
          this.pendingRequests.delete(id)
          reject(err)
        }
      })
    })
  }

  private dispatch(resp: WyVaultResponse) {
    const pending = this.pendingRequests.get(resp.id)
    if (!pending) return
    clearTimeout(pending.timer)
    this.pendingRequests.delete(resp.id)
    if (resp.ok) {
      pending.resolve(resp)
    } else {
      pending.reject(new Error(resp.error ?? 'WyVault error'))
    }
  }

  get isConnected() {
    return this.port?.isOpen ?? false
  }
}
