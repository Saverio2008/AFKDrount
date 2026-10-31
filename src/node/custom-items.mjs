import nbt from 'prismarine-nbt'

function plain (value) {
  return value && typeof value === 'object' && typeof value.type === 'string' && Object.hasOwn(value, 'value')
    ? nbt.simplify(value) : value
}

export function fieldValue (root, path) {
  if (typeof path !== 'string' || !/^(?:\/(?:[^~/]|~[01])*)+$/.test(path)) return undefined
  let value = root
  for (const part of path.slice(1).split('/')) {
    const key = part.replace(/~1/g, '/').replace(/~0/g, '~')
    if (!value || typeof value !== 'object' || !Object.hasOwn(value, key)) return undefined
    value = value[key]
  }
  return value
}

export function customMatches (item, rules) {
  if (!item) return []
  let tag
  const components = Object.create(null)
  try { tag = plain(item.nbt) } catch {}
  for (const component of item.components ?? []) {
    const name = String(component.type).replace(/^minecraft:/, '')
    try { components[name] = plain(component.data) } catch {}
  }
  const matches = []
  const seen = new Set()
  for (const rule of rules) {
    if (!rule.enabled) continue
    for (const matcher of rule.customItems ?? []) {
      const key = JSON.stringify([matcher.source, matcher.path, matcher.id])
      if (!seen.has(key) && fieldValue(matcher.source === 'nbt' ? tag : components, matcher.path) === matcher.id) {
        seen.add(key)
        matches.push({ id: matcher.id, source: matcher.source, path: matcher.path })
      }
    }
  }
  return matches
}