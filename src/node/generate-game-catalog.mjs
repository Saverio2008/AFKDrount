import { writeFile } from 'node:fs/promises'
import { buildGameCatalog } from './game-data.mjs'

if (!process.argv[2]) throw new Error('A catalog output path is required')
await writeFile(process.argv[2], JSON.stringify(buildGameCatalog()))