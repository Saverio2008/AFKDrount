import { validateStep } from './task-sender.mjs'

export function validateTasks (tasks) {
  if (!Array.isArray(tasks) || tasks.length > 64) throw new Error('task.error.format')
  const ids = new Set()
  let steps = 0
  for (const task of tasks) {
    if (!task || typeof task.id !== 'string' || !task.id || ids.has(task.id) || typeof task.name !== 'string' ||
        !task.name.trim() || task.name.length > 80 || typeof task.enabled !== 'boolean' ||
        !['startup', 'connection', 'world'].includes(task.trigger) || !Array.isArray(task.steps) ||
        !task.steps.length || task.steps.length > 64) throw new Error('task.error.format')
    ids.add(task.id)
    steps += task.steps.length
    if (steps > 256) throw new Error('task.error.format')
    for (const step of task.steps) validateStep(step)
  }
}

export class TaskRunner {
  constructor ({ tasks = [], startup = false, send, report, setTimer = setTimeout, clearTimer = clearTimeout }) {
    validateTasks(tasks)
    this.tasks = structuredClone(tasks)
    this.startup = startup
    this.send = send
    this.report = report
    this.setTimer = setTimer
    this.clearTimer = clearTimer
    this.queue = []
    this.active = null
    this.index = 0
    this.epoch = 0
    this.online = false
    this.entered = false
    this.stopped = false
    this.timer = null
  }

  notify (kind, task = null, step = 0, code = '') {
    // Never include step text, command arguments or an exception's raw message.
    this.report({ event: 'task', kind, taskId: task?.id ?? '', name: task?.name ?? '', step, code })
  }

  ready () {
    if (this.stopped || this.online) return
    this.online = true
    const first = !this.entered
    this.entered = true
    for (const task of this.tasks) {
      if (task.enabled && (task.trigger === 'world' || first && (task.trigger === 'connection' || this.startup && task.trigger === 'startup')) &&
          this.active?.id !== task.id && !this.queue.some(entry => entry.id === task.id)) this.queue.push(task)
    }
    this.schedule()
  }

  pause () {
    this.online = false
    ++this.epoch
    this.clearTimer(this.timer)
    this.timer = null
  }

  stop () {
    this.pause()
    this.queue = []
    this.active = null
    this.stopped = true
  }

  configure (tasks = []) {
    validateTasks(tasks)
    if (JSON.stringify(tasks) === JSON.stringify(this.tasks)) return
    ++this.epoch
    this.clearTimer(this.timer)
    this.timer = null
    if (this.active || this.queue.length) this.notify('tasksCancelled')
    this.active = null
    this.queue = []
    this.tasks = structuredClone(tasks)
  }

  schedule () {
    if (!this.online || this.stopped || this.timer !== null) return
    if (this.active && this.index >= this.active.steps.length) {
      this.notify('taskCompleted', this.active)
      this.active = null
    }
    if (!this.active) {
      this.active = this.queue.shift() ?? null
      this.index = 0
      if (this.active) this.notify('taskStarted', this.active)
    }
    if (!this.active) return
    const task = this.active
    const index = this.index
    const epoch = this.epoch
    const current = () => !this.stopped && this.online && this.epoch === epoch && this.active === task
    this.timer = this.setTimer(async () => {
      this.timer = null
      if (!current()) return
      try {
        await this.send(task.steps[index], current, () => {
          if (this.active === task && this.index === index) {
            ++this.index
            this.notify('taskStepSent', task, index + 1)
          }
        })
      } catch (error) {
        if (current()) {
          const code = ['task.error.send', 'task.error.step', 'task.error.chatSlash', 'task.error.chatLength', 'task.error.commandVersion', 'task.error.offline'].includes(error?.code)
            ? error.code : 'task.error.send'
          this.notify('taskFailed', task, index + 1, code)
          this.active = null
        }
      }
      if (current() || this.online && !this.stopped && this.epoch === epoch && this.active === null) this.schedule()
    }, task.steps[index].delaySeconds * 1000)
  }
}