import { createRequire } from 'node:module'
import { createHash } from 'node:crypto'

const require = createRequire(import.meta.url)
const minecraftData = require('minecraft-data')
const { oldestSupportedVersion, latestSupportedVersion } = require('mineflayer')
const latestProtocol = minecraftData(latestSupportedVersion).version.version

const legacyLanguageHints = {
  sapling: ['tile.sapling.'],
  brown_mushroom: ['tile.mushroom.name'], red_mushroom: ['tile.mushroom.name'],
  brown_mushroom_block: ['tile.mushroom.name'], red_mushroom_block: ['tile.mushroom.name'],
  stone_button: ['tile.button.name'], wooden_button: ['tile.button.name'],
  monster_egg: ['tile.monsterStoneEgg.'], end_portal_frame: ['tile.endPortalFrame.name'],
  daylight_detector: ['tile.daylightDetector.name'], double_plant: ['tile.doublePlant.'],
  silver_glazed_terracotta: ['tile.glazedTerracottaSilver.name'],
  concrete: ['tile.concrete.'], concrete_powder: ['tile.concretePowder.'],
  boat: ['item.boat.'], bed: ['item.bed.', 'tile.bed.'],
  spawn_egg: ['item.monsterPlacer.'], banner: ['item.banner.', 'tile.banner.']
}

export function supportsGameVersion (version, protocol = minecraftData.versionsByMinecraftVersion.pc[version]?.version) {
  if (typeof version !== 'string' || !Number.isInteger(protocol)) return false
  try {
    const data = minecraftData(version)
    return Boolean(data?.itemsArray?.length > 0 && data.protocol &&
      data.version.version === protocol && !data.version['<'](oldestSupportedVersion) &&
      (!data.version['>'](latestSupportedVersion) || data.version.version === latestProtocol))
  } catch { return false }
}

export function supportedGameVersions () {
  const seen = new Set()
  return minecraftData.versions.pc.filter(entry => {
    if (seen.has(entry.minecraftVersion)) return false
    seen.add(entry.minecraftVersion)
    return supportsGameVersion(entry.minecraftVersion, entry.version)
  }).map(entry => ({ version: entry.minecraftVersion, release: /^\d+(?:\.\d+)+$/.test(entry.minecraftVersion) }))
}

export function itemCatalog (version) {
  const data = minecraftData(version)
  if (!data?.itemsArray) throw new Error('Unsupported game version')
  const translations = new Map()
  for (const [key, value] of Object.entries(data.language ?? {})) {
    if (!key.startsWith('item.') && !key.startsWith('tile.') && !key.startsWith('block.')) continue
    const name = String(value).toLowerCase()
    if (!translations.has(name)) translations.set(name, [])
    translations.get(name).push(key)
  }
  const legacy = data.isOlderThan('1.13')
  const languageKeys = Object.keys(data.language ?? {})
  return data.itemsArray.map(item => {
    const hints = legacy ? item.name.startsWith('record_') ? ['item.record.'] : legacyLanguageHints[item.name] ?? [] : []
    const aliases = hints.flatMap(hint => languageKeys.filter(key => hint.endsWith('.') ? key.startsWith(hint) : key === hint))
    const generic = aliases.filter(key => hints.includes(key) || key.endsWith('.name') && !hints.some(hint => hint.endsWith('.')))
    return {
    name: item.name,
    displayName: item.displayName,
    keys: [...new Set([`item.minecraft.${item.name}`, `block.minecraft.${item.name}`,
      ...generic, ...(translations.get(item.displayName.toLowerCase()) ?? [])])],
    aliases: [...new Set([...aliases, ...(item.variations ?? []).flatMap(variation => translations.get(variation.displayName.toLowerCase()) ?? [])])]
    }
  })
}

export function buildGameCatalog () {
  const sets = {}, versions = []
  for (const entry of supportedGameVersions()) {
    const items = itemCatalog(entry.version)
    const data = minecraftData(entry.version)
    const key = createHash('sha256').update(JSON.stringify(items)).digest('hex')
    sets[key] ??= items
    versions.push({ ...entry, catalog: key, legacy: data.isOlderThan('1.13'),
      commands: data.supportFeature('signedChat'), chatLength: data.supportFeature('lessCharsInChat') ? 100 : 256 })
  }
  return { versions, sets }
}
