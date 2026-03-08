/**
 * WyVault Lite — Neuron Plugin
 * index.ts — plugin entry point
 *
 * To integrate with Neuron:
 * 1. Copy this plugin into neuron/packages/neuron-wallet/src/services/hardware/
 * 2. In hardware/index.ts: add WyVault to supportedHardwares map
 * 3. In hardware/common.ts: add WyVault to Manufacturer enum
 *
 * Alternatively, once Neuron supports dynamic plugin loading,
 * this can be installed as a standalone npm package.
 */

export { WyVault } from './wyvault'
export { WyVaultTransport } from './transport'
export { Manufacturer, type DeviceInfo, type ExtendedPublicKey, type WyVaultRequest, type WyVaultResponse } from './common'

// Integration shim — shows how to register with Neuron's HardwareWalletService
export function registerWithNeuron(hardwareWalletService: {
  supportedHardwares: Map<string, unknown>
}) {
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const { WyVault } = require('./wyvault')
  hardwareWalletService.supportedHardwares.set('WyVault', WyVault)
  console.info('[WyVault Plugin] Registered with Neuron HardwareWalletService')
}
