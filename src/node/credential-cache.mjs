import { createCipheriv, createDecipheriv, randomBytes } from 'node:crypto'
import { mkdir, readFile } from 'node:fs/promises'
import { join } from 'node:path'
import { AccountError, atomicWrite, validId } from './accounts.mjs'

function decryptCache (bytes, key, aad) {
  if (bytes.length < 29 || bytes[0] !== 1) throw new AccountError('account.error.credentials')
  const cipher = createDecipheriv('aes-256-gcm', key, bytes.subarray(1, 13))
  cipher.setAAD(aad)
  cipher.setAuthTag(bytes.subarray(13, 29))
  return JSON.parse(Buffer.concat([cipher.update(bytes.subarray(29)), cipher.final()]).toString('utf8'))
}

export function encryptedCacheFactory (directory, accountId, key) {
  if (!validId(accountId) || !Buffer.isBuffer(key) || key.length !== 32) throw new AccountError('account.error.credentials')
  return ({ cacheName }) => {
    if (!/^[a-z0-9]+$/.test(cacheName)) throw new AccountError('account.error.credentials')
    const folder = join(directory, 'credentials', accountId)
    const file = join(folder, `${cacheName}.enc`)
    const aad = Buffer.from(`${accountId}:${cacheName}`)
    let cache
    const getCached = async () => {
      if (cache !== undefined) return cache
      try {
        const bytes = await readFile(file)
        cache = decryptCache(bytes, key, aad)
        return cache
      } catch (error) {
        if (error.code === 'ENOENT') { cache = {}; return cache }
        throw new AccountError('account.error.credentials')
      }
    }
    const setCached = async value => {
      await mkdir(folder, { recursive: true, mode: 0o700 })
      const iv = randomBytes(12)
      const cipher = createCipheriv('aes-256-gcm', key, iv)
      cipher.setAAD(aad)
      const bytes = Buffer.concat([cipher.update(JSON.stringify(value), 'utf8'), cipher.final()])
      await atomicWrite(file, Buffer.concat([Buffer.from([1]), iv, cipher.getAuthTag(), bytes]))
      cache = value
    }
    // setCachedPartial is called by the prismarine-auth token managers.
    //noinspection JSUnusedGlobalSymbols
    return {
      getCached,
      setCached,
      setCachedPartial: async value => setCached({ ...await getCached(), ...value }),
      reset: async () => { await setCached({}); return {} }
    }
  }
}
