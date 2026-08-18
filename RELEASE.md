# Release Notes

All notable changes to `gruntsoftware/core` are documented in this file, newest first.

## v10.4.0

**Branch:** `release/v10.4.0` &middot; **Merged from:** `develop` (PRs [#21](https://github.com/gruntsoftware/core/pull/21)/[#22](https://github.com/gruntsoftware/core/pull/22))

Resolves both items carried as "Known issues" from v10.3.0, plus a broader correctness/memory-safety pass this
work surfaced along the way. What started as fixing a CircleCI compile failure grew into root-causing every
subsequent CI failure it exposed, rather than working around them.

### Fixed

- **`BRChainParams.h` failed to compile under GCC** (`initializer element is not constant`) — the exact
  GCC-vs-Clang gap flagged as a known issue in v10.3.0. The `uint256(...)` macro expands to a `UInt256`
  compound literal; GCC only accepts a compound literal as a static-storage-duration object's initializer when
  it's the object's *entire* initializer, not nested as a member's sub-initializer inside the checkpoint
  tables' larger aggregate initializer. Added `uint256_init()`, which expands to a bare `{ .u8 = {...} }`
  brace list instead — ordinary constant-expression syntax with no such restriction.
- **The three pre-existing test failures from v10.3.0** (`BRKeyTests`, `BRBIP32SequenceTests`,
  `BRMerkleBlockTests`) — all three turned out to be leftover Bitcoin-mainnet test fixtures and WIF strings
  from this fork's breadwallet-core origin that Litecoin's own code correctly rejects. `BRKeySetPrivKey()`'s
  return value was never checked, so a rejected Bitcoin-format WIF silently left the test reusing a stale key
  from the previous sub-test instead of failing loudly. Converted every fixture to genuine Litecoin-derived
  vectors; `BRMerkleBlockTests` in particular used a real Bitcoin block whose SHA256d difficulty target has no
  relation to Litecoin's Scrypt proof-of-work, so it never could have passed — replaced with a synthetic block
  whose nonce was mined for real against Litecoin's own proof-of-work using this library's own `BRScrypt()`.
- **`BRBIP38KeyTests` had been permanently disabled** rather than fixed, for the same reason: BIP38's
  addresshash is network-specific by construction, so Bitcoin's canonical vectors could never pass under
  Litecoin. Converted 4 of the 8 vectors to genuine Litecoin WIF/BIP38 pairs and re-enabled the suite; the
  4 EC-multiplied vectors are intentionally left out (documented in a comment) since this library only ever
  implemented BIP38 EC-multiply *decode*, not encode, and hand-rolling new encode-side crypto solely to
  manufacture test fixtures wasn't worth the risk.
- **A real strict-aliasing miscompilation under GCC** in `UIntNSetBE()`/`UIntNSetLE()` (`BRInt.h`), found by
  installing real GCC locally and reproducing `BRMerkleBlockTests`' failure directly (previously it looked
  Clang-only-diagnosable: every individual field checked out correct by hand, which turned out to mean the
  *data* was fine and the *compiled code* wasn't). These functions wrote via a whole-object assignment through
  a pointer cast to a locally-declared union type; GCC's optimizer used the resulting strict-aliasing violation
  to justify silently dropping the write. Fixed by writing byte-by-byte instead, matching the alias-safe
  pattern the `Get` counterparts already used. This is used throughout wire-format serialization (transactions,
  blocks, peer protocol), so this miscompilation was plausibly affecting more than the one test that surfaced
  it.
- **Two more genuine undefined-behavior bugs** found investigating the above, neither of which turned out to be
  the root cause but both real: `BRHMACDRBG()` called `memcpy()` with a `NULL` source at length 0 (UB in C even
  at length 0) on every "generate more output" call, which was causing all 8 `BRDrbgTests` NIST vectors to fail
  under GCC; and `BRMurmur3_32()` cast an arbitrary, non-4-byte-aligned buffer to `uint32_t*` and dereferenced
  it directly, a genuine unaligned-read + strict-aliasing violation reachable from `BRBloomFilterTests`.
- **Several real memory leaks**, found once `build-asan`'s LeakSanitizer could finally run to completion (it
  couldn't, until the above fixes landed): `BRPaymentProtocolPaymentFree()` freed its transactions array's
  container without freeing the transactions it held, and separately never freed the struct itself; and — the
  highest-impact one — `BRWalletCreateTransaction()`, `BRWalletCreateOpsTransaction()`, and
  `BRWalletFeeForTxAmount()` each leak a local output's script buffer on every call, affecting every successful
  call to the most commonly used function in the wallet API, not just this test suite.
- Renamed `BITCOIN_PUBKEY_ADDRESS`/`BITCOIN_SCRIPT_ADDRESS`/`BITCOIN_PRIVKEY` (+ testnet variants) to
  `LITECOIN_*` (`BRAddress.h`/`.c`, `BRKey.c`). Pure rename, same values — the misleading naming is exactly
  what made the test-vector bugs above easy to introduce and hard to notice for 8 years.

### Verification

- `make test` / `make testnet` pass locally under both Clang and real GCC (Homebrew `gcc-16` — this machine's
  `cc`/`gcc` is Apple Clang, which never reproduced the strict-aliasing bug above).
- CI green on both `build-and-test` and `build-asan`.
- The one change that touches computed wire-format output (`UIntNSetBE`/`UIntNSetLE`) was independently
  verified against real, on-chain Litecoin transactions pulled from an independent block explorer — parsed,
  re-serialized to byte-identical output, and hash-matched against the real network's txids. That verification
  is now a permanent part of the test suite: `BRRealChainDataTests()` embeds 25 real pre-SegWit-activation
  transactions (spanning genesis block 1 through block 806400) that are round-tripped end to end, plus 10 real
  post-activation SegWit transactions used to verify the marker/flag detection that documents this library's
  known SegWit-unsupported limitation, since `BRTransactionParse`/`BRTransactionSerialize` have no SegWit
  wire-format support.

## v10.3.0

**Branch:** `release/v10.3.0` &middot; **Merged from:** `develop` (PR [#17](https://github.com/gruntsoftware/core/pull/17))

### Fixed

- **Native SIGSEGV crash** (`BRPeerManagerLastBlockHeight`, offset `0xb8`) — `BRPeerManagerRescan()` assigned
  `BRSetGet()`'s result directly to `manager->lastBlock` with no `NULL` check. When a checkpoint block wasn't
  found in the block set, this left every subsequent read of `manager->lastBlock->height`/`->timestamp`
  dereferencing `NULL`. Root-caused from a symbolicated Firebase Crashlytics tombstone
  (issue `a82fa4956025d87609f0f8ba2e87a11d`, 3 crashes, regressed) that pointed at the exact function and byte
  offset. `BRPeerManagerRescan()` now keeps the previous `lastBlock` instead of nulling it out when the
  checkpoint lookup misses.
- **Defense in depth**: `BRPeerManagerEstimatedBlockHeight`, `BRPeerManagerLastBlockHeight`,
  `BRPeerManagerLastBlockTimestamp`, and `BRPeerManagerSyncProgress` — the full public accessor surface that
  reads `manager->lastBlock` — now guard against it being `NULL` so a future regression degrades gracefully
  instead of crashing the host process.
- **`make test` was silently unbuildable**: `BRTransactionTests()` had a whole `BRTransactionCopy` test block
  duplicated verbatim, causing a `redefinition of 'src'`/`'tgt'` compile error; and `test.c` guarded `main()`
  with `#ifndef BITCOIN_TEST_NO_MAIN`, which only checks definedness (not value) and so always excluded
  `main()` given the Makefile's `-DBITCOIN_TEST_NO_MAIN=0`. Both fixed — `make test` now builds, links, and
  runs end-to-end.

### Added

- **`BRPeerManagerSetIntegrityWarningCallback()`** — a new optional callback fired when the manager detects and
  recovers from unexpected internal state (e.g. the missing-checkpoint case above) instead of crashing, so
  embedding apps (Android, iOS) can route these recovered-from states into their own crash/analytics tooling
  rather than them going unmonitored.

### Known issues (not fixed in this release)

- Three pre-existing test failures unrelated to the above: `BRKeyTests`, `BRBIP32SequenceTests`
  (`BRBIP32BitIDKey`), `BRMerkleBlockTests` (`BRMerkleBlockParse`).
- A GCC-vs-Clang compiler portability gap in `BRChainParams.h`'s checkpoint tables: the `uint256(...)` macro
  used inside file-scope `static const` array initializers is accepted by Clang (used locally and on this
  release's CI setup) but rejected by strict GCC (`initializer element is not constant`), as hit on CircleCI's
  GCC-based build image. Tracked for a follow-up release.
