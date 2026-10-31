export class TaskSendError extends Error {
  constructor (code = 'task.error.send') { super(code); this.code = code }
}

export function validateStep (step) {
  if (!step || !['chat', 'command'].includes(step.mode) || typeof step.text !== 'string' ||
      !step.text.trim() || step.text.length > 256 || /[\u0000-\u001f\u007f-\u009f\u2028\u2029]/u.test(step.text) ||
      !Number.isFinite(step.delaySeconds) || step.delaySeconds < 0 || step.delaySeconds > 86400) {
    throw new TaskSendError('task.error.step')
  }
  if (step.mode === 'chat' && step.text.trimStart().startsWith('/')) throw new TaskSendError('task.error.chatSlash')
  if (step.mode === 'command' && (!step.text.replace(/^\//, '').trim() || /^\s|^\/\s|^\/\//u.test(step.text))) {
    throw new TaskSendError('task.error.step')
  }
}

export function createTaskSender (bot, ready) {
  const client = bot._client
  const originalWrite = client.write
  const messages = new Set(['chat', 'chat_message', 'chat_preview', 'chat_command', 'chat_command_signed'])
  let ticket = null
  let commandPending = false
  client.write = function (name, parameters) {
    if (!messages.has(name)) return originalWrite.call(this, name, parameters)
    const allowed = ticket && !ticket.sent && ready() && ticket.current() && client.state === 'play' && client.serializer?.writable
    const command = name === 'chat_command' || name === 'chat_command_signed'
    if (!allowed || (ticket.mode === 'command'
      ? !command || parameters?.command !== ticket.text
      : command || name === 'chat_preview' || parameters?.message !== ticket.text)) {
      throw new TaskSendError()
    }
    originalWrite.call(this, name, parameters)
    ticket.sent = true
    ticket.submitted()
  }
  return async (step, current, submitted) => {
    validateStep(step)
    if (step.mode === 'chat' && step.text.length > (bot.supportFeature('lessCharsInChat') ? 100 : 256)) {
      throw new TaskSendError('task.error.chatLength')
    }
    if (step.mode === 'command' && (!bot.supportFeature('signedChat') ||
        typeof client._signedChat !== 'function' || client.chat !== client._signedChat)) {
      throw new TaskSendError('task.error.commandVersion')
    }
    if (!ready() || !current() || client.state !== 'play' || !client.serializer?.writable || typeof client.chat !== 'function') {
      throw new TaskSendError('task.error.offline')
    }
    // 1.19.0 requires a server barrier before ordinary chat can follow a command.
    if (step.mode === 'chat' && commandPending && bot.supportFeature('chatCommandsQueuedToMainThread')) {
      try { await bot.tabComplete('/', false, false, 5000) } catch { throw new TaskSendError() }
      if (!ready() || !current()) throw new TaskSendError('task.error.offline')
    }
    const text = step.mode === 'command' ? step.text.replace(/^\//, '') : step.text
    const permit = { mode: step.mode, text, current, submitted, sent: false }
    ticket = permit
    try {
      const options = { timestamp: BigInt(Date.now()), skipPreview: true }
      client.chat(step.mode === 'command' ? '/' + text : text, options)
    } catch (error) {
      throw error instanceof TaskSendError ? error : new TaskSendError()
    } finally { ticket = null }
    // Deferred writes have no ticket and are rejected by the permanent packet guard.
    if (!permit.sent) throw new TaskSendError()
    commandPending = step.mode === 'command'
  }
}