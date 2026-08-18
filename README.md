# Brainwallet: Core

**Brainwallet Core** is the free, open-source SPV wallet engine that powers [Brainwallet](https://brainwallet.co), a self-custodial [Litecoin](https://litecoin.org) wallet. It's a fork of [breadwallet-core](https://github.com/breadwallet/breadwallet-core) retargeted from Bitcoin to Litecoin, written in portable C99 so it can be embedded directly in both the [Android](https://github.com/gruntsoftware/android) and [iOS](https://github.com/gruntsoftware/ios) apps.

### CircleCI status
[![Release](https://img.shields.io/github/v/release/gruntsoftware/core?style=plastic)](https://github.com/gruntsoftware/core/releases)
[![CircleCI](https://dl.circleci.com/status-badge/img/gh/gruntsoftware/core/tree/main.svg?style=svg)](https://dl.circleci.com/status-badge/redirect/gh/gruntsoftware/core/tree/main)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Used By

This library ships embedded (as a git submodule / vendored source) in both Brainwallet apps — it is not distributed or installed on its own:
- **Android**: [gruntsoftware/android](https://github.com/gruntsoftware/android) (`app/src/main/jni/core`)
- **iOS**: [gruntsoftware/ios](https://github.com/gruntsoftware/ios) (`Modules/core`)

## Important Links
- **Android Repo**: [gruntsoftware/android](https://github.com/gruntsoftware/android)
- **iOS Repo**: [gruntsoftware/ios](https://github.com/gruntsoftware/ios)
- **Website**: [brainwallet.co](https://brainwallet.co)
- **Support**: [brainwallet.co/support](https://www.brainwallet.co/support)

## Why this exists

**Standalone, not a client of our servers.** This library implements [SPV](https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki) (simplified payment verification) — it connects directly to the Litecoin peer-to-peer network to sync headers/merkle blocks and broadcast transactions, with no Brainwallet-run backend in the loop for balances or broadcasting.

**Deterministic recovery.** [BIP32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki) hierarchical-deterministic wallet logic (`BRBIP32Sequence.c`) means one seed phrase recovers the full balance and transaction history on any device, forever.

**Keys never leave the device.** Key generation, signing, and storage handoff all happen locally (`BRKey.c`, wrapping [`secp256k1`](https://github.com/bitcoin-core/secp256k1)) — this library has no network path that could exfiltrate a private key, and no custody of user funds.

**Built on open standards**, not a proprietary protocol:
- [SPV](https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki) for fast sync without running a full node
- [BIP32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki) deterministic wallets
- [BIP38](https://github.com/bitcoin/bips/blob/master/bip-0038.mediawiki) import of password-protected paper wallets (`BRBIP38Key.c`)
- [BIP39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) mnemonic seed phrases (`BRBIP39Mnemonic.c`)
- [BIP70](https://github.com/bitcoin/bips/blob/master/bip-0070.mediawiki) payment protocol support (`BRPaymentProtocol.c`)

## Features

- Full SPV sync engine: peer discovery/selection, bloom filters, merkle block download and chain reorg handling (`BRPeerManager.c`)
- **Dual-peer "privacy shield"** — an addition on top of upstream breadwallet-core: an optional second, differently-filtered peer cross-validates block/transaction data to detect a peer lying by omission and to obscure which bloom-filter matches are real
- Adaptive bloom filter false-positive rate tuning based on observed tx volume
- BIP32 HD wallets, BIP38 encrypted keys, BIP39 mnemonics, BIP70 payment requests
- Hardcoded Litecoin mainnet/testnet chain parameters and checkpoints (`BRChainParams.h`)

## Auditing code

### Prerequisites
- A C99 compiler (`gcc` or `clang`) and `make`
- This repo uses a git submodule for `secp256k1` — clone with `git clone --recurse-submodules`, or run `git submodule update --init --recursive` after a normal clone
- The `secp256k1` submodule must stay pinned at commit `b408c6a` — later upstream commits removed `secp256k1/src/basic-config.h`, which `BRKey.c` depends on directly; CI verifies this pin

```
make            # build the test binary (mainnet)
make test       # build and run tests (mainnet)
make testnet    # build and run tests against Litecoin testnet
make clean      # remove build artifacts
```

## Architecture

- **Primitives**: `BRInt.h` (fixed-width int/hash types), `BRArray.h` (growable-array macros), `BRSet.h` (hash set), `BRCrypto.c` (SHA/RIPEMD/AES/ChaCha20/Poly1305)
- **Keys & encoding**: `BRKey.c` (EC keys via `secp256k1`), `BRBase58.c`, `BRBech32.c`, `BRBIP32Sequence.c`, `BRBIP38Key.c`, `BRBIP39Mnemonic.c`
- **Chain data model**: `BRAddress.c`, `BRTransaction.c`, `BRMerkleBlock.c`, `BRBloomFilter.c`
- **Chain parameters**: `BRChainParams.h` — mainnet/testnet DNS seeds, magic numbers, and checkpoints
- **Wallet**: `BRWallet.c` — UTXO/balance tracking, transaction creation and fee logic
- **Networking / SPV sync**: `BRPeer.c` (peer wire protocol) and `BRPeerManager.c` (sync engine, the largest file in the repo)
- **Payment protocol**: `BRPaymentProtocol.c`

Everything is plain C with opaque structs and `New`/`Free` pairs (e.g. `BRPeerManagerNew`/`BRPeerManagerFree`) — callbacks are wired in explicitly, with no hidden global state.

## Testing

`test.c` compiles into a single `test_runner` binary driven by `BRRunTests()`, covering every module. CI runs on CircleCI (`.circleci/config.yml`) with a mainnet build, a testnet build, and a separate AddressSanitizer build.

## Security

Found a security vulnerability? Please **do not** open a public issue — report it via [brainwallet.co/support](https://www.brainwallet.co/support), clearly marked as a security report. Since this library ships embedded in both apps, reports about it are handled under the same disclosure process described in the Android and iOS repos' `SECURITY.md`.

## License

Brainwallet Core is released under the [MIT License](LICENSE), inherited from the original [breadwallet-core](https://github.com/breadwallet/breadwallet-core) (Copyright (c) 2015 breadwallet LLC).

## About

Brainwallet Core is a fork of [breadwallet-core](https://github.com/breadwallet/breadwallet-core), originally written by [Aaron Voisine](https://github.com/voisine) ([@voisine](https://github.com/voisine)) — co-founder of [BRD](https://brd.com) (formerly Breadwallet) and creator of the open-source SPV wallet code that now powers millions of users across BRD, Litewallet, and Brainwallet. Aaron's original wallet-core design is the foundation this library builds on, retargeted here from Bitcoin to Litecoin and extended with features like the dual-peer privacy shield.

In June 2026, Aaron joined Brainwallet as an investor and advisor — see the [press release](<https://www.brainwallet.co/pressreleases/aaron-voisine-co-founder-of-brd-(formerly-breadwallet)-joins-brainwallet-as-investor-and-advisor>) for details.

---

## Release Notes

For the full, up-to-date changelog see [GitHub Releases](https://github.com/gruntsoftware/core/releases) and the [compare view](https://github.com/gruntsoftware/core/compare). Highlights from recent versions:

---

### **v10.4.0**  [PR [#21](https://github.com/gruntsoftware/core/pull/21)/[#22](https://github.com/gruntsoftware/core/pull/22)]
---
#### 🔧 Build Fix
- **`BRChainParams.h` failed to compile under GCC** (`initializer element is not constant`) — GCC only accepts a compound-literal static initializer when it's an object's *entire* initializer, not nested inside the checkpoint tables' larger aggregate initializer. Added `uint256_init()`, a bare brace-list variant with no such restriction.

#### 🇱🇹 Litecoin Correctness
- The three test failures carried as "known issues" from v10.3.0 (`BRKeyTests`, `BRBIP32SequenceTests`, `BRMerkleBlockTests`) turned out to be leftover Bitcoin-mainnet fixtures from this fork's breadwallet-core origin — Litecoin's code was correctly rejecting them, but the tests never checked the return value. Converted every fixture to genuine Litecoin-derived vectors and re-enabled `BRBIP38KeyTests`, which had been permanently disabled instead of fixed.
- Renamed `BITCOIN_PUBKEY_ADDRESS`/`BITCOIN_SCRIPT_ADDRESS`/`BITCOIN_PRIVKEY` (+ testnet variants) to `LITECOIN_*` — same values, but the old naming is exactly what made the fixture bugs above easy to introduce and hard to notice for 8 years.

#### 🐛 Real Bug Fixes
- **Strict-aliasing miscompilation under GCC** in `UIntNSetBE()`/`UIntNSetLE()` (`BRInt.h`) — used throughout wire-format serialization (transactions, blocks, peer protocol). Found by reproducing against real GCC locally; GCC's optimizer was silently dropping a write due to a pointer-cast aliasing violation. Fixed by writing byte-by-byte instead.
- Two more genuine UB bugs found along the way: a `memcpy()` with a `NULL` source in `BRHMACDRBG()`, and an unaligned/aliasing pointer cast in `BRMurmur3_32()`.
- Several real memory leaks, including one in `BRWalletCreateTransaction()`/`BRWalletCreateOpsTransaction()`/`BRWalletFeeForTxAmount()` affecting every successful call to the most commonly used function in the wallet API.

**Full Changelog**: https://github.com/gruntsoftware/core/compare/v10.3.0...v10.4.0

---

### **v10.3.0**  [PR [#17](https://github.com/gruntsoftware/core/pull/17)]
---
#### 🐛 Crash Fix
- **Native SIGSEGV crash** (`BRPeerManagerLastBlockHeight`, offset `0xb8`) — `BRPeerManagerRescan()` assigned `BRSetGet()`'s result directly to `manager->lastBlock` with no `NULL` check; when a checkpoint block wasn't found in the block set this crashed every subsequent accessor. Root-caused from a symbolicated Firebase Crashlytics tombstone (issue `a82fa4956025d87609f0f8ba2e87a11d`, regressed) that pointed at the exact function and byte offset. `BRPeerManagerRescan()` now keeps the previous `lastBlock` instead of nulling it out on a lookup miss, and all four public accessors that read `lastBlock` are now guarded as defense in depth.
- Added `BRPeerManagerSetIntegrityWarningCallback()` so embedding apps can route these recovered-from states into their own crash/analytics tooling.

#### 🧪 Test Fixes
- `BRTransactionTests()` had a duplicated `BRTransactionCopy` test block causing a compile error; removed.
- `test.c` guarded `main()` with `#ifndef BITCOIN_TEST_NO_MAIN`, which only checks definedness (not value) against the Makefile's `-DBITCOIN_TEST_NO_MAIN=0`, always excluding `main()`. Fixed to `#if !BITCOIN_TEST_NO_MAIN`. `make test` now builds, links, and runs end-to-end for the first time.

**Full Changelog**: https://github.com/gruntsoftware/core/compare/v10.2.0...v10.3.0

---

### **v10.2.0**  [PR [#12](https://github.com/gruntsoftware/core/pull/12)/[#13](https://github.com/gruntsoftware/core/pull/13)]
---
- Reset the default bloom filter false-positive rate (FPF)
- Removed non-SPV peers from the hardcoded peer list; rate/fee handling updates

---

### **v10.1.1 / v10.1.0**  [PR [#9](https://github.com/gruntsoftware/core/pull/9)]
---
- `fix: filter out rejected tx`
- `fix: set r = 1, update error label` — small `BRPeerManager` test fix

---

### **v10.0.1 – v10.0.0**  [PR [#6](https://github.com/gruntsoftware/core/pull/6)]
---
- Changes to meet iOS 18.4.1 linking requirements
- `techdebt: fix disconnect behavior revert`

---

### **v4.2.0**  [PR [#5](https://github.com/gruntsoftware/core/pull/5)]
---
#### Feat/new-peer-discovery
To improve peer discovery, added support for selected peers sourced from `api.blockchair.com/litecoin/nodes`:
- Increased `PEER_MAX_CONNECTIONS` to 8
- Added `isFeatureSelectedPeersOn` toggle and `selectedPeerIPs`
- Added `_BRPeerManagerFindPeersV2` to parse and populate `manager->peers`

---

### **v4.1.0 – v4.0.0**  [PRs [#1](https://github.com/gruntsoftware/core/pull/1)–[#4](https://github.com/gruntsoftware/core/pull/4)]
---
- Re-added the missing module map file
- Re-instated git submodules, fixed disconnect behavior
- Initial retargeting from `breadwallet-core`/`brainwallet-co` references to `gruntsoftware/core`
