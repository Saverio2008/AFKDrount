# Third-party notices

## Qt

AFK Drount uses the Qt Base and Qt Declarative shared libraries under the
GNU Lesser General Public License, version 3 (LGPLv3). Copyright (C) The Qt
Company Ltd. and other contributors. The release includes the LGPLv3 and
GPLv3 texts, Qt third-party attributions, and SDK build metadata.

- [LGPLv3](licenses/qt/LGPL-3.0-only.txt)
- [GPLv3](licenses/qt/GPL-3.0-only.txt)
- [Qt third-party notices](licenses/qt/THIRD-PARTY.txt)
- [Qt release manifest](licenses/qt/release.json)
- [Using a replacement Qt build](licenses/qt/REPLACING-QT.md)

The corresponding source links are generated from the release manifest and
included as `QT-SOURCES.md` at the top of each release archive.

## Node.js

AFK Drount distributes the Node.js runtime used by the bot backend. The
release includes the complete upstream Node.js license, including the
notices for components distributed with Node.js, under
`licenses/nodejs/<version>/LICENSE`. The directory name matches the
bundled runtime version. The [Node.js source repository](https://github.com/nodejs/node)
publishes the corresponding license with each version.

## Mineflayer and npm dependencies

AFK Drount uses [Mineflayer](https://github.com/PrismarineJS/mineflayer)
and the other npm packages listed below. The inventory is based on
`src/node/package-lock.json`
and records the license declared for each locked package version.

License and notice filenames in the table are relative to each package's
own directory under `backend/node_modules`. On macOS, `backend` is under
`Contents/Resources`. On Windows and Linux it is under the installed
executable directory. The packages retain the notices included in their
npm distributions. Supplemental texts are under `licenses/npm`.

For entries marked **Upstream review pending**, the npm distribution and
the corresponding upstream source tree did not provide an identifiable
complete notice. Confirm the applicable copyright and license terms
before public redistribution. The two `vec3` versions declare only
`BSD`, without identifying a variant.

The CC0-1.0 text for `railroad-diagrams` is the [SPDX license text](https://spdx.org/licenses/CC0-1.0.html).

| Package | Version | Declared license | License or notice |
| --- | --- | --- | --- |
| [@azure/msal-common](https://www.npmjs.com/package/@azure/msal-common/v/14.16.1) | 14.16.1 | MIT | `LICENSE` |
| [@azure/msal-node](https://www.npmjs.com/package/@azure/msal-node/v/2.16.3) | 2.16.3 | MIT | `LICENSE` |
| [@types/node](https://www.npmjs.com/package/@types/node/v/22.15.1) | 22.15.1 | MIT | `LICENSE` |
| [@types/node-rsa](https://www.npmjs.com/package/@types/node-rsa/v/1.1.4) | 1.1.4 | MIT | `LICENSE` |
| [@types/readable-stream](https://www.npmjs.com/package/@types/readable-stream/v/4.0.25) | 4.0.25 | MIT | `LICENSE` |
| [@xboxreplay/xboxlive-auth](https://www.npmjs.com/package/@xboxreplay/xboxlive-auth/v/5.1.0) | 5.1.0 | Apache-2.0 | `LICENSE` |
| [abort-controller](https://www.npmjs.com/package/abort-controller/v/3.0.0) | 3.0.0 | MIT | `LICENSE` |
| [aes-js](https://www.npmjs.com/package/aes-js/v/3.1.2) | 3.1.2 | MIT | `LICENSE.txt` |
| [ajv](https://www.npmjs.com/package/ajv/v/6.15.0) | 6.15.0 | MIT | `LICENSE` |
| [asn1](https://www.npmjs.com/package/asn1/v/0.2.3) | 0.2.3 | MIT | `LICENSE` |
| [base64-js](https://www.npmjs.com/package/base64-js/v/1.5.1) | 1.5.1 | MIT | `LICENSE` |
| [buffer](https://www.npmjs.com/package/buffer/v/6.0.3) | 6.0.3 | MIT | `LICENSE` |
| [buffer-equal](https://www.npmjs.com/package/buffer-equal/v/1.0.1) | 1.0.1 | MIT | `LICENSE` |
| [buffer-equal-constant-time](https://www.npmjs.com/package/buffer-equal-constant-time/v/1.0.1) | 1.0.1 | BSD-3-Clause | `LICENSE.txt` |
| [commander](https://www.npmjs.com/package/commander/v/2.20.3) | 2.20.3 | MIT | `LICENSE` |
| [debug](https://www.npmjs.com/package/debug/v/4.4.3) | 4.4.3 | MIT | `LICENSE` |
| [discontinuous-range](https://www.npmjs.com/package/discontinuous-range/v/1.0.0) | 1.0.0 | MIT | `LICENSE` |
| [ecdsa-sig-formatter](https://www.npmjs.com/package/ecdsa-sig-formatter/v/1.0.11) | 1.0.11 | Apache-2.0 | `LICENSE` |
| [endian-toggle](https://www.npmjs.com/package/endian-toggle/v/0.0.0) | 0.0.0 | MIT | `LICENSE` |
| [event-target-shim](https://www.npmjs.com/package/event-target-shim/v/5.0.1) | 5.0.1 | MIT | `LICENSE` |
| [events](https://www.npmjs.com/package/events/v/3.3.0) | 3.3.0 | MIT | `LICENSE` |
| [fast-deep-equal](https://www.npmjs.com/package/fast-deep-equal/v/3.1.3) | 3.1.3 | MIT | `LICENSE` |
| [fast-json-stable-stringify](https://www.npmjs.com/package/fast-json-stable-stringify/v/2.1.0) | 2.1.0 | MIT | `LICENSE` |
| [ieee754](https://www.npmjs.com/package/ieee754/v/1.2.1) | 1.2.1 | BSD-3-Clause | `LICENSE` |
| [json-schema-traverse](https://www.npmjs.com/package/json-schema-traverse/v/0.4.1) | 0.4.1 | MIT | `LICENSE` |
| [jsonwebtoken](https://www.npmjs.com/package/jsonwebtoken/v/9.0.3) | 9.0.3 | MIT | `LICENSE` |
| [jwa](https://www.npmjs.com/package/jwa/v/2.0.1) | 2.0.1 | MIT | `LICENSE` |
| [jws](https://www.npmjs.com/package/jws/v/4.0.1) | 4.0.1 | MIT | `LICENSE` |
| [lodash.includes](https://www.npmjs.com/package/lodash.includes/v/4.3.0) | 4.3.0 | MIT | `LICENSE` |
| [lodash.isboolean](https://www.npmjs.com/package/lodash.isboolean/v/3.0.3) | 3.0.3 | MIT | `LICENSE` |
| [lodash.isinteger](https://www.npmjs.com/package/lodash.isinteger/v/4.0.4) | 4.0.4 | MIT | `LICENSE` |
| [lodash.isnumber](https://www.npmjs.com/package/lodash.isnumber/v/3.0.3) | 3.0.3 | MIT | `LICENSE` |
| [lodash.isplainobject](https://www.npmjs.com/package/lodash.isplainobject/v/4.0.6) | 4.0.6 | MIT | `LICENSE` |
| [lodash.isstring](https://www.npmjs.com/package/lodash.isstring/v/4.0.1) | 4.0.1 | MIT | `LICENSE` |
| [lodash.merge](https://www.npmjs.com/package/lodash.merge/v/4.6.2) | 4.6.2 | MIT | `LICENSE` |
| [lodash.once](https://www.npmjs.com/package/lodash.once/v/4.1.1) | 4.1.1 | MIT | `LICENSE` |
| [lodash.reduce](https://www.npmjs.com/package/lodash.reduce/v/4.6.0) | 4.6.0 | MIT | `LICENSE` |
| [macaddress](https://www.npmjs.com/package/macaddress/v/0.5.4) | 0.5.4 | MIT | `LICENSE` |
| [minecraft-data](https://www.npmjs.com/package/minecraft-data/v/3.119.0) | 3.119.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/node-minecraft-data/tree/1cdd2d17afbb3f03d97767611a22edb2c1ee6ff6) |
| [minecraft-folder-path](https://www.npmjs.com/package/minecraft-folder-path/v/1.2.0) | 1.2.0 | MIT | `LICENSE` |
| [minecraft-protocol](https://www.npmjs.com/package/minecraft-protocol/v/1.68.0) | 1.68.0 | BSD-3-Clause | `LICENSE` |
| [mineflayer](https://www.npmjs.com/package/mineflayer/v/4.39.0) | 4.39.0 | MIT | `LICENSE` |
| [mojangson](https://www.npmjs.com/package/mojangson/v/2.1.0) | 2.1.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/node-mojangson/tree/626a05fc0b28d10a6d607fc82e59dbc41c352d7b) |
| [moo](https://www.npmjs.com/package/moo/v/0.5.3) | 0.5.3 | BSD-3-Clause | `LICENSE` |
| [ms](https://www.npmjs.com/package/ms/v/2.1.3) | 2.1.3 | MIT | `license.md` |
| [nearley](https://www.npmjs.com/package/nearley/v/2.20.1) | 2.20.1 | MIT | `LICENSE.txt` |
| [node-fetch](https://www.npmjs.com/package/node-fetch/v/2.7.0) | 2.7.0 | MIT | `LICENSE.md` |
| [node-rsa](https://www.npmjs.com/package/node-rsa/v/0.4.2) | 0.4.2 | MIT | `README.md (License and bundled RSA/JSBN notices)` |
| [prismarine-auth](https://www.npmjs.com/package/prismarine-auth/v/3.1.1) | 3.1.1 | MIT | `LICENSE` |
| [prismarine-biome](https://www.npmjs.com/package/prismarine-biome/v/1.4.0) | 1.4.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-biome/tree/57051697f91356c840f961e1bc650b4ff976576e) |
| [prismarine-block](https://www.npmjs.com/package/prismarine-block/v/1.23.0) | 1.23.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-block/tree/c52fba46b794429d89806bd63bb0e9de1be8f981) |
| [prismarine-chat](https://www.npmjs.com/package/prismarine-chat/v/1.13.0) | 1.13.0 | MIT | `LICENSE` |
| [prismarine-chunk](https://www.npmjs.com/package/prismarine-chunk/v/1.41.0) | 1.41.0 | MIT | `LICENSE` |
| [prismarine-entity](https://www.npmjs.com/package/prismarine-entity/v/2.6.0) | 2.6.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-entity/tree/4f2678f704d3aaa46c1ee8045a1e92d9b48dd937) |
| [prismarine-item](https://www.npmjs.com/package/prismarine-item/v/1.18.0) | 1.18.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-item/tree/e734157ec9153b1f4ca5d921c9ddaa2bc8ca880b) |
| [prismarine-nbt](https://www.npmjs.com/package/prismarine-nbt/v/2.8.0) | 2.8.0 | MIT | [Upstream review pending](https://github.com/prismarinejs/prismarine-nbt/tree/38e111a950f32eead3df481feadeece0cf097297) |
| [prismarine-physics](https://www.npmjs.com/package/prismarine-physics/v/1.11.1) | 1.11.1 | MIT | `LICENSE` |
| [prismarine-realms](https://www.npmjs.com/package/prismarine-realms/v/1.6.0) | 1.6.0 | MIT | `LICENSE` |
| [prismarine-recipe](https://www.npmjs.com/package/prismarine-recipe/v/1.5.0) | 1.5.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-recipe/tree/3ef96a62b21dcf899a0c5ac28ccd430db1b494d3) |
| [prismarine-registry](https://www.npmjs.com/package/prismarine-registry/v/1.12.0) | 1.12.0 | MIT | `LICENSE` |
| [prismarine-windows](https://www.npmjs.com/package/prismarine-windows/v/2.10.0) | 2.10.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-windows/tree/173c4dbdc39622b3fbfab768277d173b81f704e6) |
| [prismarine-world](https://www.npmjs.com/package/prismarine-world/v/3.7.0) | 3.7.0 | MIT | [Upstream review pending](https://github.com/PrismarineJS/prismarine-world/tree/66d4a32515a767bfeb2867d77ae348776467d904) |
| [process](https://www.npmjs.com/package/process/v/0.11.10) | 0.11.10 | MIT | `LICENSE` |
| [protodef](https://www.npmjs.com/package/protodef/v/1.19.0) | 1.19.0 | MIT | `LICENSE` |
| [protodef-validator](https://www.npmjs.com/package/protodef-validator/v/1.5.0) | 1.5.0 | MIT | [Upstream review pending](https://github.com/ProtoDef-io/node-protodef-validator/tree/57d83e7230969a50a01dcba8ed72e3f9a9e3d39e) |
| [punycode](https://www.npmjs.com/package/punycode/v/2.3.1) | 2.3.1 | MIT | `LICENSE-MIT.txt` |
| [railroad-diagrams](https://www.npmjs.com/package/railroad-diagrams/v/1.0.0) | 1.0.0 | CC0-1.0 | [CC0-1.0 text](licenses/npm/railroad-diagrams-1.0.0-CC0-1.0.txt) |
| [randexp](https://www.npmjs.com/package/randexp/v/0.4.6) | 0.4.6 | MIT | `LICENSE` |
| [readable-stream](https://www.npmjs.com/package/readable-stream/v/4.8.0) | 4.8.0 | MIT | `LICENSE` |
| [ret](https://www.npmjs.com/package/ret/v/0.1.15) | 0.1.15 | MIT | `LICENSE` |
| [rxjs](https://www.npmjs.com/package/rxjs/v/7.8.2) | 7.8.2 | Apache-2.0 | `LICENSE.txt` |
| [safe-buffer](https://www.npmjs.com/package/safe-buffer/v/5.2.1) | 5.2.1 | MIT | `LICENSE` |
| [semver](https://www.npmjs.com/package/semver/v/7.8.5) | 7.8.5 | ISC | `LICENSE` |
| [smart-buffer](https://www.npmjs.com/package/smart-buffer/v/4.2.0) | 4.2.0 | MIT | `LICENSE` |
| [string_decoder](https://www.npmjs.com/package/string_decoder/v/1.3.0) | 1.3.0 | MIT | `LICENSE` |
| [tr46](https://www.npmjs.com/package/tr46/v/0.0.3) | 0.0.3 | MIT | [Bundled license](licenses/npm/tr46-0.0.3-LICENSE.md) · [Upstream license](https://github.com/jsdom/tr46/blob/3a6f29721e7063b9ffd421e461a54beae6170001/LICENSE.md) |
| [tslib](https://www.npmjs.com/package/tslib/v/2.9.1) | 2.9.1 | 0BSD | `LICENSE.txt` |
| [typed-emitter](https://www.npmjs.com/package/typed-emitter/v/1.4.0) | 1.4.0 | MIT | `LICENSE` |
| [typed-emitter](https://www.npmjs.com/package/typed-emitter/v/2.1.0) | 2.1.0 | MIT | `LICENSE` |
| [uint4](https://www.npmjs.com/package/uint4/v/0.1.2) | 0.1.2 | MIT | [Upstream review pending](https://github.com/wtfaremyinitials/uint4/tree/da875014f02b854bbea27163c3c08a26724dd8b0) |
| [undici-types](https://www.npmjs.com/package/undici-types/v/6.21.0) | 6.21.0 | MIT | `LICENSE` |
| [uri-js](https://www.npmjs.com/package/uri-js/v/4.4.1) | 4.4.1 | BSD-2-Clause | `LICENSE` |
| [uuid](https://www.npmjs.com/package/uuid/v/11.1.1) | 11.1.1 | MIT | `LICENSE.md` |
| [uuid-1345](https://www.npmjs.com/package/uuid-1345/v/1.0.2) | 1.0.2 | MIT | `LICENSE` |
| [vec3](https://www.npmjs.com/package/vec3/v/0.1.10) | 0.1.10 | BSD | [Upstream review pending](https://github.com/PrismarineJS/node-vec3/tree/0e571deb0d892d0ddc9f4612ec937ae113c343be) |
| [vec3](https://www.npmjs.com/package/vec3/v/0.2.0) | 0.2.0 | BSD | [Upstream review pending](https://github.com/PrismarineJS/node-vec3/tree/3d6b2dc99b78b7c6280ff13f8067bb2ccda7c4e2) |
| [webidl-conversions](https://www.npmjs.com/package/webidl-conversions/v/3.0.1) | 3.0.1 | BSD-2-Clause | `LICENSE.md` |
| [whatwg-url](https://www.npmjs.com/package/whatwg-url/v/5.0.0) | 5.0.0 | MIT | `LICENSE.txt` |
| [xxhash-wasm](https://www.npmjs.com/package/xxhash-wasm/v/0.4.2) | 0.4.2 | MIT | `LICENSE.md` |
| [yggdrasil](https://www.npmjs.com/package/yggdrasil/v/1.8.0) | 1.8.0 | MIT | `LICENSE` |
