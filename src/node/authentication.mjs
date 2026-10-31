import { randomUUID } from 'node:crypto'
import { AccountError, readAccounts, saveAccount, validId } from './accounts.mjs'
import { encryptedCacheFactory } from './credential-cache.mjs'

export async function loginMicrosoft (directory, request, onDeviceCode, authenticate) {
  const accounts = await readAccounts(directory)
  const previous = request.accountId ? accounts.find(account => account.id === request.accountId && account.type === 'microsoft') : null
  if (request.accountId && !previous) throw new AccountError('account.error.notFound')
  const key = Buffer.from(request.key ?? '', 'base64')
  const authId = previous?.authId ?? randomUUID()
  const cache = encryptedCacheFactory(directory, authId, key)
  if (!authenticate) {
    const { default: prismarineAuth } = await import('prismarine-auth')
    const { Authflow, Titles } = prismarineAuth
    authenticate = async (cacheFactory, codeCallback) => {
      const flow = new Authflow(authId, cacheFactory, { flow: 'live', authTitle: Titles.MinecraftNintendoSwitch, deviceType: 'Nintendo' }, codeCallback)
      return flow.getMinecraftJavaToken({ fetchProfile: true })
    }
  }
  let result
  try { result = await authenticate(cache, onDeviceCode) } catch (error) {
    if (error instanceof AccountError) throw error
    throw new AccountError('account.error.authentication')
  } finally { key.fill(0) }
  const profile = result.profile
  const uuid = typeof profile?.id === 'string' ? profile.id.replace(/^([a-f0-9]{8})([a-f0-9]{4})([a-f0-9]{4})([a-f0-9]{4})([a-f0-9]{12})$/i, '$1-$2-$3-$4-$5') : ''
  if (!validId(uuid) || !/^[A-Za-z0-9_]{1,16}$/.test(profile?.name ?? '')) throw new AccountError('account.error.noProfile')
  return saveAccount(directory, { id: previous?.id ?? randomUUID(), type: 'microsoft', name: profile.name, uuid, authId }, previous?.id)
}