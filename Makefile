# Makefile for gruntsoftware/core
# Builds the SPV Litecoin wallet C library and runs tests
#
# Usage:
#   make            - build the test binary
#   make test       - build and run tests
#   make clean      - remove build artifacts
#   make testnet    - build and run tests against Litecoin testnet

CC       ?= gcc
CFLAGS   = -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
           -Wno-missing-field-initializers -O2 \
           -I. -Isecp256k1 \
           -DBITCOIN_TEST_NO_MAIN=0
LDFLAGS  = -lpthread -lm

# secp256k1 needs these defines (BRKey.c includes it inline via basic-config.h)
CFLAGS  += -DHAVE_CONFIG_H=0

# Source files (everything except test.c for the library)
LIB_SRCS = BRAddress.c \
           BRBIP32Sequence.c \
           BRBIP38Key.c \
           BRBIP39Mnemonic.c \
           BRBase58.c \
           BRBech32.c \
           BRBloomFilter.c \
           BRCrypto.c \
           BRKey.c \
           BRMerkleBlock.c \
           BRPaymentProtocol.c \
           BRPeer.c \
           BRPeerManager.c \
           BRSet.c \
           BRTransaction.c \
           BRWallet.c

TEST_SRC = test.c

# Object files
LIB_OBJS = $(LIB_SRCS:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)
ALL_OBJS = $(LIB_OBJS) $(TEST_OBJ)

# Output binary
TEST_BIN = test_runner

.PHONY: all test testnet clean

all: $(TEST_BIN)

$(TEST_BIN): $(ALL_OBJS)
	$(CC) $(ALL_OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	@echo "============================="
	@echo "Running gruntsoftware/core tests"
	@echo "============================="
	./$(TEST_BIN)

testnet: CFLAGS += -DLITECOIN_TESTNET=1
testnet: clean $(TEST_BIN)
	@echo "============================="
	@echo "Running tests (TESTNET)"
	@echo "============================="
	./$(TEST_BIN)

clean:
	rm -f $(ALL_OBJS) $(TEST_BIN)

# Dependencies (headers that trigger recompilation)
BRAddress.o:        BRAddress.h BRCrypto.h BRBase58.h BRBech32.h BRInt.h
BRBIP32Sequence.o:  BRBIP32Sequence.h BRKey.h BRCrypto.h BRInt.h
BRBIP38Key.o:       BRBIP38Key.h BRKey.h BRCrypto.h BRAddress.h BRInt.h
BRBIP39Mnemonic.o:  BRBIP39Mnemonic.h BRCrypto.h BRInt.h
BRBase58.o:         BRBase58.h BRCrypto.h BRInt.h
BRBech32.o:         BRBech32.h BRCrypto.h BRInt.h BRAddress.h
BRBloomFilter.o:    BRBloomFilter.h BRCrypto.h BRInt.h
BRCrypto.o:         BRCrypto.h BRInt.h
BRKey.o:            BRKey.h BRAddress.h BRBase58.h BRInt.h
BRMerkleBlock.o:    BRMerkleBlock.h BRCrypto.h BRInt.h
BRPaymentProtocol.o: BRPaymentProtocol.h BRTransaction.h BRAddress.h BRKey.h BRInt.h
BRPeer.o:           BRPeer.h BRMerkleBlock.h BRTransaction.h BRAddress.h BRSet.h BRArray.h BRCrypto.h BRInt.h
BRPeerManager.o:    BRPeerManager.h BRPeer.h BRMerkleBlock.h BRWallet.h BRBloomFilter.h BRSet.h BRArray.h BRChainParams.h BRInt.h
BRSet.o:            BRSet.h
BRTransaction.o:    BRTransaction.h BRKey.h BRAddress.h BRInt.h
BRWallet.o:         BRWallet.h BRTransaction.h BRAddress.h BRBIP32Sequence.h BRInt.h
test.o:             BRCrypto.h BRBloomFilter.h BRMerkleBlock.h BRWallet.h BRKey.h BRBIP38Key.h BRAddress.h \
                    BRBase58.h BRBech32.h BRBIP39Mnemonic.h BRPeer.h BRPeerManager.h BRChainParams.h \
                    BRPaymentProtocol.h BRInt.h BRArray.h BRSet.h BRTransaction.h
