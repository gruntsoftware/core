//
//  BWPaymentProtocol.h
//
//  Created by Aaron Voisine on 9/7/15.
//  Copyright (c) 2015 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BWPaymentProtocol_h
#define BWPaymentProtocol_h

#include "BWTransaction.h"
#include "BWAddress.h"
#include "BWKey.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// BIP70 payment protocol: https://github.com/bitcoin/bips/blob/master/bip-0070.mediawiki
// BIP75 payment protocol encryption: https://github.com/bitcoin/bips/blob/master/bip-0075.mediawiki

typedef struct {
    char *network; // "main" or "test", default is "main"
    BWTxOutput *outputs; // where to send payments, outputs[n].amount defaults to 0
    size_t outCount;
    uint64_t time; // request creation time, seconds since unix epoch, optional
    uint64_t expires; // when this request should be considered invalid, optional
    char *memo; // human-readable description of request for the customer, optional
    char *paymentURL; // url to send payment and get payment ack, optional
    uint8_t *merchantData; // arbitrary data to include in the payment message, optional
    size_t merchDataLen;
} BWPaymentProtocolDetails;

// returns a newly allocated details struct that must be freed by calling BWPaymentProtocolDetailsFree()
BWPaymentProtocolDetails *BWPaymentProtocolDetailsNew(const char *network, const BWTxOutput outputs[], size_t outCount,
                                                      uint64_t time, uint64_t expires, const char *memo,
                                                      const char *paymentURL, const uint8_t *merchantData,
                                                      size_t merchDataLen);

// buf must contain a serialized details struct
// returns a details struct that must be freed by calling BWPaymentProtocolDetailsFree()
BWPaymentProtocolDetails *BWPaymentProtocolDetailsParse(const uint8_t *buf, size_t bufLen);

// writes serialized details struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolDetailsSerialize(const BWPaymentProtocolDetails *details, uint8_t *buf, size_t bufLen);

// frees memory allocated for details struct
void BWPaymentProtocolDetailsFree(BWPaymentProtocolDetails *details);

typedef struct {
    uint32_t version; // default is 1
    char *pkiType; // none / x509+sha256 / x509+sha1, default is "none"
    uint8_t *pkiData; // depends on pkiType, optional
    size_t pkiDataLen;
    BWPaymentProtocolDetails *details; // required
    uint8_t *signature; // pki-dependent signature, optional
    size_t sigLen;
} BWPaymentProtocolRequest;

// returns a newly allocated request struct that must be freed by calling BWPaymentProtocolRequestFree()
BWPaymentProtocolRequest *BWPaymentProtocolRequestNew(uint32_t version, const char *pkiType, const uint8_t *pkiData,
                                                      size_t pkiDataLen, BWPaymentProtocolDetails *details,
                                                      const uint8_t *signature, size_t sigLen);

// buf must contain a serialized request struct
// returns a request struct that must be freed by calling BWPaymentProtocolRequestFree()
BWPaymentProtocolRequest *BWPaymentProtocolRequestParse(const uint8_t *buf, size_t bufLen);

// writes serialized request struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolRequestSerialize(const BWPaymentProtocolRequest *req, uint8_t *buf, size_t bufLen);

// writes the DER encoded certificate corresponding to index to cert
// returns the number of bytes written to cert, or the total certLen needed if cert is NULL
// returns 0 if index is out-of-bounds
size_t BWPaymentProtocolRequestCert(const BWPaymentProtocolRequest *req, uint8_t *cert, size_t certLen, size_t idx);

// writes the hash of the request to md needed to sign or verify the request
// returns the number of bytes written, or the total mdLen needed if md is NULL
size_t BWPaymentProtocolRequestDigest(BWPaymentProtocolRequest *req, uint8_t *md, size_t mdLen);

// frees memory allocated for request struct
void BWPaymentProtocolRequestFree(BWPaymentProtocolRequest *req);

typedef struct {
    uint8_t *merchantData; // from request->details->merchantData, optional
    size_t merchDataLen;
    BWTransaction **transactions; // array of signed BWTransaction struct references to satisfy outputs from details
    size_t txCount;
    BWTxOutput *refundTo; // where to send refunds, if a refund is necessary, refundTo[n].amount defaults to 0
    size_t refundToCount;
    char *memo; // human-readable message for the merchant, optional
} BWPaymentProtocolPayment;

// returns a newly allocated payment struct that must be freed by calling BWPaymentProtocolPaymentFree()
BWPaymentProtocolPayment *BWPaymentProtocolPaymentNew(const uint8_t *merchantData, size_t merchDataLen,
                                                      BWTransaction *transactions[], size_t txCount,
                                                      const uint64_t refundToAmounts[],
                                                      const BWAddress refundToAddresses[], size_t refundToCount,
                                                      const char *memo);

// buf must contain a serialized payment struct
// returns a payment struct that must be freed by calling BWPaymentProtocolPaymentFree()
BWPaymentProtocolPayment *BWPaymentProtocolPaymentParse(const uint8_t *buf, size_t bufLen);

// writes serialized payment struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolPaymentSerialize(const BWPaymentProtocolPayment *payment, uint8_t *buf, size_t bufLen);

// frees memory allocated for payment struct (does not call BWTransactionFree() on transactions)
void BWPaymentProtocolPaymentFree(BWPaymentProtocolPayment *payment);

typedef struct {
    BWPaymentProtocolPayment *payment; // payment message that triggered this ack, required
    char *memo; // human-readable message for customer, optional
} BWPaymentProtocolACK;

// returns a newly allocated ACK struct that must be freed by calling BWPaymentProtocolACKFree()
BWPaymentProtocolACK *BWPaymentProtocolACKNew(BWPaymentProtocolPayment *payment, const char *memo);

// buf must contain a serialized ACK struct
// returns an ACK struct that must be freed by calling BWPaymentProtocolACKFree()
BWPaymentProtocolACK *BWPaymentProtocolACKParse(const uint8_t *buf, size_t bufLen);

// writes serialized ACK struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolACKSerialize(const BWPaymentProtocolACK *ack, uint8_t *buf, size_t bufLen);

// frees memory allocated for ACK struct
void BWPaymentProtocolACKFree(BWPaymentProtocolACK *ack);

typedef struct {
    BWKey senderPubKey; // sender's public key, required
    uint64_t amount; // amount is integer-number-of-satoshis, defaults to 0
    char *pkiType; // none / x509+sha256, default is "none"
    uint8_t *pkiData; // depends on pkiType, optional
    size_t pkiDataLen;
    char *memo; // human-readable description of invoice request for the receiver, optional
    char *notifyUrl; // URL to notify on encrypted payment request ready, optional
    uint8_t *signature; // pki-dependent signature, optional
    size_t sigLen;
} BWPaymentProtocolInvoiceRequest;

// returns a newly allocated invoice request struct that must be freed by calling BWPaymentProtocolInvoiceRequestFree()
BWPaymentProtocolInvoiceRequest *BWPaymentProtocolInvoiceRequestNew(BWKey *senderPubKey, uint64_t amount,
                                                                    const char *pkiType, uint8_t *pkiData,
                                                                    size_t pkiDataLen, const char *memo,
                                                                    const char *notifyUrl, const uint8_t *signature,
                                                                    size_t sigLen);
    
// buf must contain a serialized invoice request
// returns an invoice request struct that must be freed by calling BWPaymentProtocolInvoiceRequestFree()
BWPaymentProtocolInvoiceRequest *BWPaymentProtocolInvoiceRequestParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized invoice request to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolInvoiceRequestSerialize(BWPaymentProtocolInvoiceRequest *req, uint8_t *buf, size_t bufLen);
    
// writes the DER encoded certificate corresponding to index to cert
// returns the number of bytes written to cert, or the total certLen needed if cert is NULL
// returns 0 if index is out-of-bounds
size_t BWPaymentProtocolInvoiceRequestCert(const BWPaymentProtocolInvoiceRequest *req, uint8_t *cert, size_t certLen,
                                           size_t idx);
    
// writes the hash of the request to md needed to sign or verify the request
// returns the number of bytes written, or the total mdLen needed if md is NULL
size_t BWPaymentProtocolInvoiceRequestDigest(BWPaymentProtocolInvoiceRequest *req, uint8_t *md, size_t mdLen);

// frees memory allocated for invoice request struct
void BWPaymentProtocolInvoiceRequestFree(BWPaymentProtocolInvoiceRequest *req);

typedef enum {
    BWPaymentProtocolMessageTypeUnknown = 0,
    BWPaymentProtocolMessageTypeInvoiceRequest = 1,
    BWPaymentProtocolMessageTypeRequest = 2,
    BWPaymentProtocolMessageTypePayment = 3,
    BWPaymentProtocolMessageTypeACK = 4
} BWPaymentProtocolMessageType;

typedef struct {
    BWPaymentProtocolMessageType msgType; // message type of message, required
    uint8_t *message; // serialized payment protocol message, required
    size_t msgLen;
    uint64_t statusCode; // payment protocol status code, optional
    char *statusMsg; // human-readable payment protocol status message, optional
    uint8_t *identifier; // unique key to identify entire exchange, optional (should use sha256 of invoice request)
    size_t identLen;
} BWPaymentProtocolMessage;

// returns a newly allocated message struct that must be freed by calling BWPaymentProtocolMessageFree()
BWPaymentProtocolMessage *BWPaymentProtocolMessageNew(BWPaymentProtocolMessageType msgType, const uint8_t *message,
                                                      size_t msgLen, uint64_t statusCode, const char *statusMsg,
                                                      const uint8_t *identifier, size_t identLen);
    
// buf must contain a serialized message
// returns an message struct that must be freed by calling BWPaymentProtocolMessageFree()
BWPaymentProtocolMessage *BWPaymentProtocolMessageParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized message struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolMessageSerialize(const BWPaymentProtocolMessage *msg, uint8_t *buf, size_t bufLen);
    
// frees memory allocated for message struct
void BWPaymentProtocolMessageFree(BWPaymentProtocolMessage *msg);
    
typedef struct {
    BWPaymentProtocolMessageType msgType; // message type of decrypted message, required
    uint8_t *message; // encrypted payment protocol message, required
    size_t msgLen;
    BWKey receiverPubKey; // receiver's public key, required
    BWKey senderPubKey; // sender's public key, required
    uint64_t nonce; // microseconds since epoch, required
    uint8_t *signature; // signature over the full encrypted message with sender/receiver ec key respectively, optional
    size_t sigLen;
    uint8_t *identifier; // unique key to identify entire exchange, optional (should use sha256 of invoice request)
    size_t identLen;
    uint64_t statusCode; // payment protocol status code, optional
    char *statusMsg; // human-readable payment protocol status message, optional
} BWPaymentProtocolEncryptedMessage;

// returns a newly allocated encrypted message struct that must be freed with BWPaymentProtocolEncryptedMessageFree()
// message is the un-encrypted serialized payment protocol message
// one of either receiverKey or senderKey must contain a private key, and the other must contain only a public key
BWPaymentProtocolEncryptedMessage *BWPaymentProtocolEncryptedMessageNew(BWPaymentProtocolMessageType msgType,
                                                                        const uint8_t *message, size_t msgLen,
                                                                        BWKey *receiverKey, BWKey *senderKey,
                                                                        uint64_t nonce,
                                                                        const uint8_t *identifier, size_t identLen,
                                                                        uint64_t statusCode, const char *statusMsg);
    
// buf must contain a serialized encrytped message
// returns an encrypted message struct that must be freed by calling BWPaymentProtocolEncryptedMessageFree()
BWPaymentProtocolEncryptedMessage *BWPaymentProtocolEncryptedMessageParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized encrypted message to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BWPaymentProtocolEncryptedMessageSerialize(BWPaymentProtocolEncryptedMessage *msg, uint8_t *buf, size_t bufLen);

int BWPaymentProtocolEncryptedMessageVerify(BWPaymentProtocolEncryptedMessage *msg, BWKey *pubKey);

size_t BWPaymentProtocolEncryptedMessageDecrypt(BWPaymentProtocolEncryptedMessage *msg, uint8_t *out, size_t outLen,
                                                BWKey *privKey);

// frees memory allocated for encrypted message struct
void BWPaymentProtocolEncryptedMessageFree(BWPaymentProtocolEncryptedMessage *msg);

#ifdef __cplusplus
}
#endif

#endif // BWPaymentProtocol_h
