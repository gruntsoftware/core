# core
SPV Litecoin wallet C library

We have taken the breadwallet-core library and made changes to make it better for SPV clients like [Brainwallet](https://www.brainwallet.co)
We reconize that there are other more modern libraries but the existing user base means we intend to fully optimize this library and use it to its full potential


## secp256k1
We leverage [secp256k1](https://github.com/bitcoin-core/secp256k1) @ b408c6a because this commit was the last one the Bitcoin devs used with a config file. We may update this at some point.

## Releases
Current: v10.0.0

## Contact
kerry@grunt.ltd
---
[Legacy getting started](https://github.com/breadwallet/breadwallet-core/wiki)
