import { EventEmitter } from 'node:events'
import { createInterface } from 'node:readline'
import mineflayer from 'mineflayer'
import { AccountError, readAccounts } from './accounts.mjs'
import { runtimeSession } from './runtime-auth.mjs'
import { attachWorldLifecycle } from './world-lifecycle.mjs'
import { customMatches } from './custom-items.mjs'
import { playerState } from './monitor-state.mjs'
import { supportsGameVersion } from './game-data.mjs'
import { createTaskSender } from './task-sender.mjs'
import { TaskRunner, validateTasks } from './task-runner.mjs'
import { createRequire } from 'node:module'

const require = createRequire(import.meta.url)
const chatType = require('prismarine-chat')

console.log = console.info = console.warn = console.error = console.debug = () => {}
delete process.env.DEBUG
const input = createInterface({ input: process.stdin, crlfDelay: Infinity })
let bot, key, configuration, started = false, stopping = false, online = false
let batch = [], serial = 0
let detachWorld = () => {}
let ended = false, timer
let tasks, sendInput, sampleTimer, resourceTimer
let worldGeneration = 0
let manualPending = false
let pingKnown = false
const clientSettings = { locale: 'en_us' }
let translations
let translationLocale
let englishLanguage
let localizedLanguage

function normalizeLanguage (strings) {
  const result = Object.create(null)
  for (const [key, value] of Object.entries(strings)) {
    // Older Minecraft language files also use numeric placeholders.
    if (typeof value === 'string') result[key] = value.replace(/%(\d+\$)?[\d.]*[df]/g, '%$1s')
  }
  return result
}

/**
 * @param {{ toString(language?: Record<string, string>): string, toMotd(language?: Record<string, string>): string }} message
 * @param {boolean} [styled]
 */
function gameText (message, styled = false) {
  if (!englishLanguage && bot?.registry?.language) englishLanguage = normalizeLanguage(bot.registry.language)
  const useTranslations = translationLocale === clientSettings.locale
  if (useTranslations && englishLanguage && !localizedLanguage) localizedLanguage = { ...englishLanguage, ...translations }
  const language = (useTranslations ? localizedLanguage : undefined) ?? englishLanguage
  const render = lang => (styled ? message.toMotd(lang) : message.toString(lang)).slice(0, 4096)
  try {
    return render(language)
  } catch {
    try { return language ? render(undefined) : '' } catch { return '' }
  }
}

function setLocale (value) {
  const locale = typeof value === 'string' && /^[a-z]{2}_[a-z]{2}$/.test(value) ? value : 'en_us'
  if (clientSettings.locale === locale) return
  clientSettings.locale = locale
  if (!bot?.settings || ended || stopping) return
  if (bot._client.state === 'play' || bot._client.state === 'configuration') {
    /** @type {Partial<import('mineflayer').GameSettings> & { locale: string }} */
    const settings = { locale }
    bot.setSettings(settings)
  } else {
    bot.settings.locale = locale
  }
}

async function submitInput (request) {
  const { id, mode, text } = request
  if (!Number.isSafeInteger(id)) return
  const generation = worldGeneration
  const current = () => online && !ended && !stopping && generation === worldGeneration
  if (!current() || manualPending) {
    output({ event: 'inputResult', id, code: 'task.error.offline' })
    return
  }
  manualPending = true
  let code = ''
  try {
    await sendInput({ mode, text, delaySeconds: 0 }, current, () => {})
  } catch (error) {
    code = ['task.error.offline', 'task.error.chatSlash', 'task.error.chatLength', 'task.error.commandVersion', 'task.error.step'].includes(error?.code)
      ? error.code : 'monitor.inputFailed'
  } finally {
    manualPending = false
  }
  if (!ended) output({ event: 'inputResult', id, code })
}

function samplePlayer () {
  if (online) output({ event: 'playerState', data: playerState(bot, pingKnown) })
}

function startSampling () {
  let lastTime = process.hrtime.bigint()
  let lastCpu = process.cpuUsage()
  resourceTimer = setInterval(() => {
    const now = process.hrtime.bigint()
    const cpu = process.cpuUsage()
    const elapsed = Number(now - lastTime) / 1000
    const percent = 100 * (cpu.user - lastCpu.user + cpu.system - lastCpu.system) / elapsed
    output({ event: 'resources', cpu: percent, memory: process.memoryUsage.rss() })
    lastTime = now
    lastCpu = cpu
  }, 5000)
  sampleTimer = setInterval(samplePlayer, 1000)
}
const identities = new WeakMap()

function output (value) {
  if (process.stdout.writableLength > 16 * 1024 * 1024) { process.exit(2) }
  process.stdout.write(`${JSON.stringify(value)}\n`)
}
function flush () {
  if (batch.length) { output({ event: 'entities', entries: batch }); batch = [] }
}
function finish (kind, text = '') {
  if (ended) return
  ended = true
  online = false
  tasks?.stop()
  ++worldGeneration
  clearInterval(timer)
  clearInterval(sampleTimer)
  clearInterval(resourceTimer)
  detachWorld()
  flush()
  output({ event: 'end', kind, text: String(text).slice(0, 4096) })
  input.close()
  process.exitCode = 0
  const shutdown = setTimeout(() => process.exit(0), 1000)
  shutdown.unref()
}
function stop () {
  stopping = true
  tasks?.stop()
  bot?.quit()
  finish('stopped')
}
function identity (entity) {
  if (!identities.has(entity)) identities.set(entity, String(++serial))
  return identities.get(entity)
}
function entityEvent (entity, gone = false) {
  if (!online || !['item', 'Item', 'item_stack'].includes(entity?.name)) return
  const id = identity(entity)
  if (gone) batch.push({ id, gone: true })
  else {
    const p = entity.position
    if (!p || ![p.x, p.y, p.z].every(Number.isFinite)) return
    let item
    try { item = entity.getDroppedItem?.() } catch {}
    // Unknown item metadata is retained so its earlier trajectory is not lost.
    const matches = customMatches(item, configuration.rules)
    if (item && !configuration.rules.some(rule => rule.enabled &&
      (rule.items.includes(item.name) || (rule.customItems?.length ?? 0) > 0))) return
    const stackSize = item?.components?.find(component => component.type === 'max_stack_size')?.data ??
      bot.registry.itemsByName[item?.name]?.stackSize
    batch.push({ id, position: [p.x, p.y, p.z], item: item?.name ?? '', count: item?.count ?? 0, stackSize: stackSize ?? 0, customMatches: matches })
  }
  if (batch.length >= 256) flush()
}
function incident (kind) { output({ event: 'incident', kind }) }
function validateItems () {
  for (const rule of configuration.rules) {
    if (rule.enabled && rule.items.some(name => !bot.registry.itemsByName[name])) return false
  }
  return true
}
async function start (request) {
  const account = (await readAccounts(process.argv[2])).find(entry => entry.id === request.accountId)
  if (!account) throw new AccountError('account.error.notFound')
  if (stopping) return
  configuration = request.instance.configuration
  validateTasks(configuration.tasks ?? [])
  if (!supportsGameVersion(configuration.gameVersion)) {
    output({ event: 'error', code: 'config.error.version' })
    finish('configuration')
    return
  }
  key = Buffer.from(request.key ?? '', 'base64')
  const options = {
    host: request.instance.server, port: request.instance.port, username: account.name,
    version: configuration.gameVersion,
    auth: 'offline', hideErrors: true, respawn: false,
    clientSettings,
    plugins: {
      // Seed the first play-state settings packet after Mineflayer loads its internal plugin.
      locale: bot => { bot.settings.locale = clientSettings.locale }
    }
  }
  if (account.type === 'microsoft') {
    options.auth = (client, settings) => {
      runtimeSession(process.argv[2], account.id, key).then(result => {
        if (stopping || ended) return
        const session = { accessToken: result.token, selectedProfile: result.profile, availableProfile: [result.profile] }
        Object.assign(client, result.certificates)
        client.session = session
        client.username = result.profile.name
        settings.accessToken = result.token
        settings.haveCredentials = true
        output({ event: 'profile', name: result.profile.name })
        client.emit('session', session)
        settings.connect(client)
      }).catch(error => {
        if (stopping || ended) return
        output({ event: 'error', code: error instanceof AccountError ? error.code : 'account.error.authentication' })
        finish('authentication')
        bot?.end()
      })
    }
  }
  bot = mineflayer.createBot(options)
  const ChatMessage = chatType(configuration.gameVersion)
  const sender = createTaskSender(bot, () => online && !ended && !stopping)
  let sendTail = Promise.resolve()
  sendInput = (step, current, submitted) => {
    const operation = sendTail.then(() => sender(step, current, submitted))
    sendTail = operation.catch(() => {})
    return operation
  }
  tasks = new TaskRunner({
    tasks: configuration.tasks ?? [], startup: request.startupTasks === true,
    send: sendInput, report: output
  })
  detachWorld = attachWorldLifecycle(bot, {
    transition: kind => {
      flush()
      online = false
      ++worldGeneration
      if (kind === 'login') pingKnown = false
      tasks.pause()
      output(kind === 'login' ? { event: 'connected' } : { event: 'worldTransition', kind })
    },
    ready: () => {
      if (ended || stopping) return
      if (!validateItems()) {
        output({ event: 'error', code: 'session.error.item' }); finish('configuration'); bot.end(); return
      }
      flush()
      output({ event: 'reset' })
      online = true
      output({ event: 'online', version: bot.version,
        commandReady: bot.supportFeature('signedChat') && typeof bot._client._signedChat === 'function' &&
          bot._client.chat === bot._client._signedChat })
      for (const entity of Object.values(bot.entities)) entityEvent(entity)
      samplePlayer()
      tasks.ready()
    },
    incident
  })
  bot.on('entitySpawn', entityEvent)
  bot.on('entityMoved', entityEvent)
  bot.on('entityUpdate', entityEvent)
  bot.on('entityGone', entity => entityEvent(entity, true))
  bot._client.on('player_info', packet => {
    if (packet.data.some(entry => entry.uuid === bot.player?.uuid && Number.isFinite(entry.latency ?? entry.ping))) pingKnown = true
  })
  bot.on('death', () => { samplePlayer(); incident('death'); finish('death'); bot.end() })
  const reportTitle = (kind, component) => {
    if (component == null) return
    let text
    try {
      text = gameText(ChatMessage.fromNotch(component))
    } catch {
      return
    }
    output({ event: 'serverEvent', kind, text })
  }
  // Mineflayer unwraps the NBT title before emitting it, losing its component type.
  if (bot.supportFeature('titleUsesNewPackets')) {
    bot._client.on('set_title_text', ({ text }) => reportTitle('title', text))
    bot._client.on('set_title_subtitle', ({ text }) => reportTitle('subtitle', text))
  } else if (bot.supportFeature('titleUsesLegacyPackets')) {
    bot._client.on('title', ({ action, text }) => {
      if (action === 0 || action === 1) reportTitle(action === 1 ? 'subtitle' : 'title', text)
    })
  }
  EventEmitter.prototype.on.call(bot, 'title_clear', () => output({ event: 'serverEvent', kind: 'title_clear', text: '' }))
  EventEmitter.prototype.on.call(bot, 'title_times', (fadeIn, stay, fadeOut) => output({ event: 'serverEvent', kind: 'title_times', text: [fadeIn, stay, fadeOut].join(', '), ticks: fadeIn + stay + fadeOut }))
  bot.on('actionBar', message => output({ event: 'serverEvent', kind: 'actionBar', text: gameText(message) }))
  bot._client.on('action_bar', ({ text }) => {
    if (text != null) bot.emit('actionBar', ChatMessage.fromNotch(text))
  })
  bot.on('message', (message, position) => {
    if (position === 'game_info') return
    output({ event: 'serverEvent', kind: 'chat', text: gameText(message), motd: gameText(message, true) })
  })
  bot.on('kicked', reason => {
    let text = ''
    if (reason != null) {
      try { text = gameText(ChatMessage.fromNotch(reason)) } catch { text = typeof reason === 'string' ? reason.slice(0, 4096) : '' }
    }
    output({ event: 'serverEvent', kind: 'kicked', text })
    finish('kicked')
  })
  bot.on('error', () => { finish(online ? 'disconnect' : 'connectionError'); bot.end() })
  bot.on('end', () => finish(stopping ? 'stopped' : online ? 'disconnect' : 'connectionError'))
  timer = setInterval(flush, 100)
  startSampling()
}

input.on('line', line => {
  if (line.length > 32 * 1024 * 1024) { stop(); return }
  try {
    const request = JSON.parse(line)
    if (line.length > 1024 * 1024 && request.command !== 'setTranslations') { stop(); return }
    if (!started && request.command === 'start') {
      started = true
      setLocale(request.locale)
      start(request).catch(error => {
        output({ event: 'error', code: error instanceof AccountError ? error.code : 'session.error.backend' })
        finish('configuration')
      })
    } else if (request.command === 'stop') stop()
    else if (request.command === 'setLocale') setLocale(request.locale)
    else if (request.command === 'setTranslations') {
      translations = normalizeLanguage(request.translations ?? {})
      translationLocale = request.locale
      localizedLanguage = undefined
    }
    else if (request.command === 'manualInput') void submitInput(request)
    else if (request.command === 'apply' && online) {
      flush()
      const previous = configuration
      configuration = request.configuration
      validateTasks(configuration.tasks ?? [])
      if (!validateItems()) {
        configuration = previous
        output({ event: 'applyRejected', code: 'session.error.item', revision: request.revision }); return
      }
      tasks.configure(configuration.tasks ?? [])
      output({ event: 'applied', configuration, revision: request.revision })
      // Resend current positions after the ordered configuration boundary.
      for (const entity of Object.values(bot.entities)) entityEvent(entity)
      flush()
    }
  } catch { output({ event: 'error', code: 'session.error.protocol' }); stop() }
})
input.on('close', () => { if (!ended) stop() })
process.on('exit', () => key?.fill(0))
process.on('SIGTERM', stop)
process.on('SIGINT', stop)
process.on('uncaughtException', () => { output({ event: 'error', code: 'session.error.backend' }); stop() })
process.on('unhandledRejection', () => { output({ event: 'error', code: 'session.error.backend' }); stop() })
