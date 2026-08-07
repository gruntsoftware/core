# Release Notes

All notable changes to `gruntsoftware/core` are documented in this file, newest first.

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
