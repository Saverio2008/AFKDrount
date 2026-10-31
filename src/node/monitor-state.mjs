const finite = value => Number.isFinite(value) ? value : null

function attributeValue (attribute) {
  if (!attribute || !Number.isFinite(attribute.value)) return null
  const modifiers = attribute.modifiers ?? []
  const sum = operation => modifiers.reduce((total, modifier) => total + (modifier.operation === operation ? modifier.amount : 0), 0)
  const base = attribute.value + sum(0)
  const total = base * (1 + sum(1))
  return finite(modifiers.reduce((value, modifier) => Number(modifier.operation) === 2 ? value * (1 + modifier.amount) : value, total))
}

export function playerState (bot, pingKnown) {
  const position = bot.entity?.position
  const attributes = bot.entity?.attributes ?? {}
  const maximum = attributes['minecraft:max_health'] ?? attributes['minecraft:generic.max_health'] ??
    attributes['generic.max_health'] ?? attributes['generic.maxHealth'] ?? attributes.max_health
  return {
    health: finite(bot.health),
    maxHealth: attributeValue(maximum),
    food: finite(bot.food),
    saturation: finite(bot.foodSaturation),
    position: position && [position.x, position.y, position.z].every(Number.isFinite) ? [position.x, position.y, position.z] : null,
    dimension: bot.game?.dimension ?? null,
    gameMode: bot.game?.gameMode ?? null,
    level: finite(bot.experience?.level),
    experience: finite(bot.experience?.progress),
    ping: pingKnown ? finite(bot.player?.ping) : null
  }
}