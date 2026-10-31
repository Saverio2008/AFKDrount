import { createHash, randomUUID } from 'node:crypto'
import { mkdir, readFile, rename, writeFile, open, rm } from 'node:fs/promises'
import { join } from 'node:path'

export class AccountError extends Error {
  constructor (code) { super(code); this.code = code }
}

const uuidPattern = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i
export const validId = value => typeof value === 'string' && uuidPattern.test(value)
const validName = value => typeof value === 'string' && /^[A-Za-z0-9_]{1,16}$/.test(value)

export function offlineUuid (name) {
  const bytes = createHash('md5').update(`OfflinePlayer:${name}`, 'utf8').digest()
  bytes[6] = (bytes[6] & 15) | 48
  bytes[8] = (bytes[8] & 63) | 128
  const hex = bytes.toString('hex')
  return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`
}

export async function atomicWrite (file, value) {
  const temporary = `${file}.${randomUUID()}.tmp`
  try {
    await writeFile(temporary, value, { mode: 0o600, flag: 'wx' })
    await rename(temporary, file)
  } finally {
    await rm(temporary, { force: true })
  }
}

export async function readAccounts (directory) {
  let data
  try { data = JSON.parse((await readFile(join(directory, 'accounts.json'))).toString('utf8')) } catch (error) {
    if (error.code === 'ENOENT') return []
    throw new AccountError('account.error.storage')
  }
  if (data.version !== 1 || !Array.isArray(data.accounts)) throw new AccountError('account.error.storage')
  const ids = new Set()
  for (const account of data.accounts) {
    if (!validId(account.id) || ids.has(account.id) || !validName(account.name) ||
        !validId(account.uuid) || !['offline', 'microsoft'].includes(account.type) ||
        (account.type === 'microsoft' && !validId(account.authId))) {
      throw new AccountError('account.error.storage')
    }
    ids.add(account.id)
  }
  return data.accounts.map(({ id, name, uuid, type, authId }) => ({ id, name, uuid, type, ...(type === 'microsoft' ? { authId } : {}) }))
}

export async function withLock (directory, action) {
  await mkdir(directory, { recursive: true, mode: 0o700 })
  const file = join(directory, 'accounts.lock')
  let lock
  try { lock = await open(file, 'wx', 0o600); await lock.writeFile(String(process.pid)) } catch (error) {
    if (error.code !== 'EEXIST') throw new AccountError('account.error.storage')
    let pid
    try { pid = Number((await readFile(file)).toString('utf8')) } catch { throw new AccountError('account.error.locked') }
    if (Number.isSafeInteger(pid) && pid > 0) {
      try { process.kill(pid, 0) } catch (probe) {
        if (probe.code === 'ESRCH') {
          await rm(file, { force: true })
          return withLock(directory, action)
        }
      }
    }
    throw new AccountError('account.error.locked')
  }
  try { return await action() } finally { await lock.close(); await rm(file, { force: true }) }
}

export async function saveAccount (directory, candidate, targetId = '') {
  return withLock(directory, async () => {
    const accounts = await readAccounts(directory)
    let index = targetId ? accounts.findIndex(account => account.id === targetId) : -1
    if (targetId && (index < 0 || accounts[index].type !== 'microsoft')) throw new AccountError('account.error.notFound')
    const duplicate = accounts.findIndex(account => account.type === candidate.type && account.uuid === candidate.uuid)
    if (index >= 0 && duplicate >= 0 && duplicate !== index) throw new AccountError('account.error.duplicate')
    if (index < 0) index = duplicate
    const account = { ...candidate, id: index >= 0 ? accounts[index].id : candidate.id }
    if (index < 0) accounts.push(account)
    else accounts[index] = account
    await atomicWrite(join(directory, 'accounts.json'), JSON.stringify({ version: 1, accounts }, null, 2))
    return { accounts, selectedId: account.id }
  })
}

export async function addOffline (directory, name) {
  if (!validName(name)) throw new AccountError('account.error.invalidName')
  return saveAccount(directory, { id: randomUUID(), name, uuid: offlineUuid(name), type: 'offline' })
}

export async function moveAccount (directory, accountId, targetId, after) {
  if (!validId(accountId) || !validId(targetId)) throw new AccountError('account.error.notFound')
  if (typeof after !== 'boolean') throw new AccountError('account.error.protocol')
  return withLock(directory, async () => {
    const accounts = await readAccounts(directory)
    const source = accounts.findIndex(account => account.id === accountId)
    const target = accounts.findIndex(account => account.id === targetId)
    if (source < 0 || target < 0) throw new AccountError('account.error.notFound')
    if (source === target) return { accounts }

    const destination = target + (after ? 1 : 0) - (source < target ? 1 : 0)
    if (destination !== source) {
      const [account] = accounts.splice(source, 1)
      accounts.splice(destination, 0, account)
      await atomicWrite(join(directory, 'accounts.json'), JSON.stringify({ version: 1, accounts }, null, 2))
    }
    return { accounts }
  })
}

export async function removeAccount (directory, accountId) {
  if (!validId(accountId)) throw new AccountError('account.error.notFound')
  return withLock(directory, async () => {
    const accounts = await readAccounts(directory)
    const target = accounts.find(account => account.id === accountId)
    if (!target) throw new AccountError('account.error.notFound')
    const remaining = accounts.filter(account => account.id !== accountId)
    if (target.type === 'microsoft' && !remaining.some(account => account.authId === target.authId)) {
      try { await rm(join(directory, 'credentials', target.authId), { recursive: true, force: true }) } catch {
        throw new AccountError('account.error.removeCredentials')
      }
    }
    try {
      await atomicWrite(join(directory, 'accounts.json'), JSON.stringify({ version: 1, accounts: remaining }, null, 2))
    } catch {
      throw new AccountError(target.type === 'microsoft' ? 'account.error.removeSave' : 'account.error.storage')
    }
    return { accounts: remaining }
  })
}
