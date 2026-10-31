export function attachWorldLifecycle (bot, callbacks, now = () => performance.now()) {
  let inWorld = false, synchronized = false, baseline = null, health = null, lastMovement = -Infinity
  const position = () => {
    const value = bot.entity?.position
    return value && [value.x, value.y, value.z].every(Number.isFinite) ? value : null
  }
  const begin = kind => {
    inWorld = false
    synchronized = false
    baseline = null
    health = null
    lastMovement = -Infinity
    callbacks.transition(kind)
  }
  const ready = () => {
    const value = position()
    if (inWorld || !synchronized || !value) return
    inWorld = true
    baseline = value.clone()
    callbacks.ready()
  }
  const movement = () => {
    const value = position()
    if (!inWorld || !value || !baseline) return
    if (value.distanceTo(baseline) > 0.01 && now() - lastMovement >= 500) {
      lastMovement = now()
      baseline = value.clone()
      callbacks.incident('unexpectedMovement')
    }
  }
  const onLogin = () => begin('login')
  const onRespawn = () => begin('respawn')
  const onConfigure = () => begin('configuration')
  const onForcedMove = () => {
    if (inWorld) movement()
    else { synchronized = true; ready() }
  }
  const onHealth = () => {
    if (inWorld && health !== null && Number.isFinite(bot.health) && bot.health < health) callbacks.incident('hurt')
    health = Number.isFinite(bot.health) ? bot.health : null
  }
  bot.on('login', onLogin)
  bot.on('respawn', onRespawn)
  bot.on('spawn', ready)
  bot.on('forcedMove', onForcedMove)
  bot.on('move', movement)
  bot.on('health', onHealth)
  bot._client.on('start_configuration', onConfigure)
  return () => {
    bot.removeListener('login', onLogin)
    bot.removeListener('respawn', onRespawn)
    bot.removeListener('spawn', ready)
    bot.removeListener('forcedMove', onForcedMove)
    bot.removeListener('move', movement)
    bot.removeListener('health', onHealth)
    bot._client.removeListener('start_configuration', onConfigure)
  }
}