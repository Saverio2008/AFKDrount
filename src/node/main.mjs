import { createInterface } from 'node:readline'
import { AccountError, addOffline, moveAccount, readAccounts, removeAccount } from './accounts.mjs'
import { loginMicrosoft } from './authentication.mjs'

const output = message => process.stdout.write(`${JSON.stringify(message)}\n`)
// Authentication libraries must never write credentials or diagnostic text into IPC.
console.log = console.info = console.warn = console.error = console.debug = () => {}
delete process.env.DEBUG
const input = createInterface({ input: process.stdin, crlfDelay: Infinity })
const timeout = setTimeout(() => { output({ event: 'error', code: 'account.error.timeout' }); process.exit(1) }, 15 * 60 * 1000)
async function run () {
  const iterator = input[Symbol.asyncIterator]()
  const line = await iterator.next()
  input.close()
  if (line.done || typeof line.value !== 'string' || line.value.length > 65536) throw new AccountError('account.error.protocol')
  const request = JSON.parse(line.value)
  const directory = process.argv[2]
  if (!directory) throw new AccountError('account.error.storage')
  let result
  switch (request.command) {
    case 'list': result = { accounts: await readAccounts(directory) }; break
    case 'addOffline': result = await addOffline(directory, request.name); break
    case 'removeAccount': result = await removeAccount(directory, request.accountId); break
    case 'moveAccount': result = await moveAccount(directory, request.accountId, request.targetId, request.after); break
    case 'loginMicrosoft':
      result = await loginMicrosoft(directory, request, response => {
        if (typeof response.user_code !== 'string' || response.user_code.trim().length === 0) throw new AccountError('account.error.protocol')
        const url = new URL('https://www.microsoft.com/link')
        url.searchParams.set('otc', response.user_code)
        output({ event: 'deviceCode', code: response.user_code, url: url.href })
      })
      break
    default: throw new AccountError('account.error.protocol')
  }
  output({ event: 'result', ...result })
}

try {
  await run()
} catch (error) {
  output({ event: 'error', code: error instanceof AccountError ? error.code : 'account.error.backend' })
  process.exitCode = 1
} finally { clearTimeout(timeout); input.close() }