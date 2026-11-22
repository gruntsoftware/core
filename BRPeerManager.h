//
//  BRPeerManager.h

#ifndef BRPeerManager_h
#define BRPeerManager_h

#include "BRPeer.h"
#include "BRMerkleBlock.h"
#include "BRTransaction.h"
#include "BRWallet.h"
#include "BRChainParams.h"
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PEER_MAX_CONNECTIONS 8

typedef struct BRPeerManagerStruct BRPeerManager;

// returns a newly allocated BRPeerManager struct that must be freed by calling BRPeerManagerFree()
BRPeerManager *BRPeerManagerNew(const BRChainParams *params, BRWallet *wallet, uint32_t earliestKeyTime,
                                BRMerkleBlock *blocks[], size_t blocksCount, const BRPeer peers[], size_t peersCount,
                                double fpRate);

// not thread-safe, set callbacks once before calling BRPeerManagerConnect()
void BRPeerManagerSetCallbacks(BRPeerManager *manager, void *info,
                               void (*syncStarted)(void *info),
                               void (*syncStopped)(void *info, int error),
                               void (*txStatusUpdate)(void *info),
                               void (*saveBlocks)(void *info, int replace, BRMerkleBlock *blocks[], size_t blocksCount),
                               void (*savePeers)(void *info, int replace, const BRPeer peers[], size_t peersCount),
                               int (*networkIsReachable)(void *info),
                               void (*threadCleanup)(void *info));

// optional: called when the manager detects and recovers from unexpected internal state (e.g. a missing
// checkpoint block) instead of crashing, so the app can report it to its own crash/analytics tooling.
// info is the same info pointer passed to BRPeerManagerSetCallbacks(); warning is a short, static,
// human-readable string with no dynamic/sensitive content, safe to log as-is.
void BRPeerManagerSetIntegrityWarningCallback(BRPeerManager *manager, void (*integrityWarning)(void *info, const char *warning));

void BRPeerManagerSetFixedPeer(BRPeerManager *manager, UInt128 address, uint16_t port);

// sets the preferred peers, replacing any set earlier. core doesn't make http requests, so the app fetches these from
// https://api.grunt.ltd/v1/litecoin/nodes and passes them in, with each node's "flags" value as BRPeer.services
// - peers that don't advertise SERVICES_NODE_NETWORK and SERVICES_NODE_BLOOM are ignored
// - a port of 0 means the standard port, timestamp and flags are ignored
// - they're tried before any other peer, with the hardcoded peers as backup if there are too few or they don't work
// - ignored while a fixed peer is set (see BRPeerManagerSetFixedPeer)
// thread-safe, call before BRPeerManagerConnect(), or while connected to apply to the next connection made
// returns the number of peers accepted
size_t BRPeerManagerSetApiPeers(BRPeerManager *manager, const BRPeer peers[], size_t peersCount);

BRPeerStatus BRPeerManagerConnectStatus(BRPeerManager *manager);
uint16_t BRPeerManagerStandardPort(BRPeerManager *manager);
void BRPeerManagerConnect(BRPeerManager *manager);
void BRPeerManagerDisconnect(BRPeerManager *manager);
void BRPeerManagerRescan(BRPeerManager *manager);
uint32_t BRPeerManagerEstimatedBlockHeight(BRPeerManager *manager);
uint32_t BRPeerManagerLastBlockHeight(BRPeerManager *manager);
uint32_t BRPeerManagerLastBlockTimestamp(BRPeerManager *manager);
double BRPeerManagerSyncProgress(BRPeerManager *manager, uint32_t startHeight);
size_t BRPeerManagerPeerCount(BRPeerManager *manager);
const char *BRPeerManagerDownloadPeerName(BRPeerManager *manager);
void BRPeerManagerPublishTx(BRPeerManager *manager, BRTransaction *tx, void *info,
                            void (*callback)(void *info, int error));
size_t BRPeerManagerRelayCount(BRPeerManager *manager, UInt256 txHash);

// enable or disable dual-peer privacy shield (disabled by default)
// when enabled, a verification peer receives a differently-tweaked bloom filter and downloads
// the same blocks as the primary download peer. the manager cross-compares transaction sets
// from both peers to:
//   1) detect lying-by-omission (peer hiding wallet transactions)
//   2) prevent either peer from learning which transactions are real vs false positives
//   3) allow a tighter fpRate without sacrificing privacy (cover comes from filter divergence)
void BRPeerManagerSetPrivacyShieldEnabled(BRPeerManager *manager, int enabled);

// frees memory allocated for manager (call BRPeerManagerDisconnect() first if connected)
void BRPeerManagerFree(BRPeerManager *manager);

#ifdef __cplusplus
}
#endif

#endif // BRPeerManager_h
