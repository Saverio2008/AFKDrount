import { setTimeout as delay } from 'node:timers/promises'
import prismarineAuth from 'prismarine-auth'
import { AccountError, readAccounts, saveAccount, withLock } from './accounts.mjs'
import { encryptedCacheFactory } from './credential-cache.mjs'

async function waitForLock (action) {
  const deadline = Date.now() + 90000
  for (;;) {
    try { return await action() } catch (error) {
      if (!(error instanceof AccountError) || error.code !== 'account.error.locked' || Date.now() >= deadline) throw error
      await delay(250)
    }
  }
}

export async function runtimeSession (directory, accountId, key) {
  let account
  const result = await waitForLock(() => withLock(directory, async () => {
    account = (await readAccounts(directory)).find(entry => entry.id === accountId && entry.type === 'microsoft')
    if (!account) throw new AccountError('account.error.notFound')
    const { Authflow, Titles } = prismarineAuth
    const flow = new Authflow(account.authId, encryptedCacheFactory(directory, account.authId, key),
      { flow: 'live', authTitle: Titles.MinecraftNintendoSwitch, deviceType: 'Nintendo' },
      () => { throw new AccountError('account.error.authentication') })
    return flow.getMinecraftJavaToken({ fetchProfile: true, fetchCertificates: true })
  }))
  if (!result.token || !/^[a-f0-9]{32}$/i.test(result.profile?.id ?? '') || !/^[A-Za-z0-9_]{1,16}$/.test(result.profile?.name ?? '')) {
    throw new AccountError('account.error.noProfile')
  }
  const uuid = result.profile.id.replace(/^(.{8})(.{4})(.{4})(.{4})(.{12})$/, '$1-$2-$3-$4-$5')
  await waitForLock(() => saveAccount(directory, { ...account, name: result.profile.name, uuid }, accountId))
  return result
}