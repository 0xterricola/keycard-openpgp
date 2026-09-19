# End-to-End QR Signing Prototype

## SAMA5D3 + NeoPGP + Trusted Display + Physical Approval + QR Transport

This document records the first successful end-to-end signing flow of the hardware prototype.

The prototype can now receive a signing request optically through a QR code, decode and validate that request locally, display the exact message on a trusted on-device display, require an explicit physical APPROVE or REJECT decision, invoke a NeoPGP hardware-backed signing key only after approval, and return the resulting signature as another QR code.

The signing message is no longer hardcoded in the application.

## End-to-End Flow

~~~text
Host
  │
  │ QR signing request
  ▼
USB camera
  │
  ▼
SAMA5D3
  │
  ├── decode QR
  ├── validate request
  ├── extract exact message
  │
  ▼
Trusted ST7789 display
  │
  │ physical decision
  ├─────────────────┐
  │                 │
REJECT            APPROVE
  │                 │
  │                 ▼
  │            NeoPGP PIN
  │                 │
  │                 ▼
  │          NeoPGP hardware key
  │                 │
  │                 ▼
  │          secp256k1 signature
  │                 │
  │                 ▼
  │            response QR
  │                 │
  └─────────────────┘
                    │
                    ▼
              ST7789 display
~~~

---

# End-to-End Test Result

Before breaking down the implementation, this is the complete terminal output from the first successful v4 reject/approve test.

The same externally generated QR signing request was presented twice.

The first request was physically rejected.

The second was physically approved.

## Raw Test Transcript

~~~text
# /root/sign_prompt_v4
Waiting for KC1 signing request on /dev/video0...
Valid QR signing request received
Exact scanned message: KEYCARD OPENPGP TEST
NeoPGP signing fingerprint: 31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
Signature counter before: 7
Signature counter after:  7
REJECTED - no signing APDU executed

# /root/sign_prompt_v4
Waiting for KC1 signing request on /dev/video0...
Valid QR signing request received
Exact scanned message: KEYCARD OPENPGP TEST
NeoPGP signing fingerprint: 31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
Signature counter before: 7
APPROVED
Exact message: KEYCARD OPENPGP TEST
Requesting NeoPGP hardware signature...
OpenPGP signing PIN:
QR version: 8
QR width: 49 modules
QR scale: 4 display pixels/module
QR payload:
KC1|FP=31CE69D66A5E9DE0F977B59C79BB391497E8E6D4|SIG=5067733470DE7FF3998A23CA82B14B02C99B9E718FC5E6A0BA5C375BAE571077CCD196D373B23B0332F4405C96197FA7CF03602E7CE5FEE74A63FCA81AD20890
Hardware signature (64 bytes):
5067733470DE7FF3998A23CA82B14B02C99B9E718FC5E6A0BA5C375BAE571077CCD196D373B23B0332F4405C96197FA7CF03602E7CE5FEE74A63FCA81AD20890
Signature counter after:  8
~~~

The central result is already visible directly in the transcript:

~~~text
REJECT:
Signature counter 7 → 7

APPROVE:
Signature counter 7 → 8
~~~

The remainder of this document explains what happened at each stage and why those two results matter.

---

# Full Test Breakdown

## 1. Start the signer

~~~text
# /root/sign_prompt_v4
~~~

This launches the v4 signing application on the SAMA5D3.

Unlike earlier versions of the prototype, v4 does not begin with a hardcoded signing message.

It waits for an external QR signing request.

---

## 2. Wait for an optical request

~~~text
Waiting for KC1 signing request on /dev/video0...
~~~

The SAMA5D3 opens the NexiGo USB webcam through the Linux V4L2 stack.

QR decoding happens locally using `libzbar`.

The expected prototype request format is:

~~~text
KC1|OP=PGP_SIGN|MSG=<message>
~~~

At this point, no signing operation has occurred.

---

## 3. Decode and validate the request

~~~text
Valid QR signing request received
Exact scanned message: KEYCARD OPENPGP TEST
~~~

The camera captured a QR code and the local parser confirmed that it was a valid `KC1` signing request.

The exact message extracted from the optical request was:

~~~text
KEYCARD OPENPGP TEST
~~~

This value is not compiled into v4 as the signing message.

It came from the external QR request.

The same parsed message is subsequently used for both:

~~~text
trusted display
      +
signing input
~~~

---

## 4. Identify the hardware signing key

~~~text
NeoPGP signing fingerprint: 31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
~~~

The SAMA5D3 communicates with the NeoPGP smart card through the OMNIKEY reader and retrieves the fingerprint of the physical signing key.

The key being presented to the user for approval is therefore tied to the actual card connected to the device.

---

# Reject Test

## 5. Read the card signing counter before the decision

~~~text
Signature counter before: 7
~~~

Before any decision is made, the NeoPGP card reports a signature counter of `7`.

This gives us a hardware-side reference point.

---

## 6. Physically reject the request

The RED button was pressed.

The application then reported:

~~~text
Signature counter after:  7
REJECTED - no signing APDU executed
~~~

The counter remained:

~~~text
7 → 7
~~~

The application returned through the rejection path before issuing the smart-card signing command.

Two observations therefore agree:

~~~text
Software:
No signing APDU was executed.

Hardware:
The card signature counter did not change.
~~~

The complete rejection path was:

~~~text
QR request
    ↓
camera
    ↓
request validation
    ↓
exact message
    ↓
trusted display
    ↓
physical RED
    ↓
REJECT
    ↓
no signing APDU
    ↓
counter remains 7
~~~

---

# Approve Test

The same signing request was then presented a second time.

## 7. Receive the same request again

~~~text
# /root/sign_prompt_v4
Waiting for KC1 signing request on /dev/video0...
Valid QR signing request received
Exact scanned message: KEYCARD OPENPGP TEST
NeoPGP signing fingerprint: 31CE69D66A5E9DE0F977B59C79BB391497E8E6D4
Signature counter before: 7
~~~

Notice that the counter is still `7`.

The rejected request did not consume a signature.

---

## 8. Physically approve the request

The GREEN button was pressed.

The application reported:

~~~text
APPROVED
Exact message: KEYCARD OPENPGP TEST
~~~

The important relationship is:

~~~text
QR-decoded message:
KEYCARD OPENPGP TEST

Trusted-display message:
KEYCARD OPENPGP TEST

Signing input:
KEYCARD OPENPGP TEST
~~~

---

## 9. Begin hardware signing

~~~text
Requesting NeoPGP hardware signature...
OpenPGP signing PIN:
~~~

Only after physical approval does the private-key path begin.

The current prototype performs approximately:

~~~text
exact message bytes
       ↓
     SHA-256
       ↓
NeoPGP PIN verification
       ↓
PSO: COMPUTE DIGITAL SIGNATURE
       ↓
secp256k1 ECDSA signature
~~~

The PIN itself is not printed in the transcript.

---

## 10. Generate the optical response

~~~text
QR version: 8
QR width: 49 modules
QR scale: 4 display pixels/module
~~~

After the hardware signature is returned, `libqrencode` creates the response QR.

The generated QR uses:

~~~text
Version: 8
Matrix: 49 × 49 modules
Scale: 4 display pixels per module
~~~

The QR is rendered directly on the 240×240 ST7789 display.

---

## 11. Construct the response payload

~~~text
QR payload:
KC1|FP=31CE69D66A5E9DE0F977B59C79BB391497E8E6D4|SIG=5067733470DE7FF3998A23CA82B14B02C99B9E718FC5E6A0BA5C375BAE571077CCD196D373B23B0332F4405C96197FA7CF03602E7CE5FEE74A63FCA81AD20890
~~~

The prototype response contains:

- `KC1` — prototype protocol identifier
- `FP=` — hardware signing-key fingerprint
- `SIG=` — hardware-generated signature

The result can therefore leave the device as machine-readable optical data.

---

## 12. Hardware signature returned by NeoPGP

~~~text
Hardware signature (64 bytes):
5067733470DE7FF3998A23CA82B14B02C99B9E718FC5E6A0BA5C375BAE571077CCD196D373B23B0332F4405C96197FA7CF03602E7CE5FEE74A63FCA81AD20890
~~~

The card returned a 64-byte secp256k1 ECDSA signature represented as:

~~~text
32-byte r || 32-byte s
~~~

For this test:

~~~text
r =
5067733470DE7FF3998A23CA82B14B02C99B9E718FC5E6A0BA5C375BAE571077

s =
CCD196D373B23B0332F4405C96197FA7CF03602E7CE5FEE74A63FCA81AD20890
~~~

This is currently the raw cryptographic signature produced by the NeoPGP signing primitive.

It is not yet a complete RFC OpenPGP detached-signature packet.

---

## 13. Confirm that the hardware key was used

~~~text
Signature counter after:  8
~~~

Before approval:

~~~text
7
~~~

After the signing operation:

~~~text
8
~~~

Therefore:

~~~text
7 → 8
~~~

This is consistent with one signing operation having occurred.

---

# Reject vs Approve

| Stage | REJECT | APPROVE |
|---|---|---|
| External QR received | Yes | Yes |
| Request validated | Yes | Yes |
| Message extracted | `KEYCARD OPENPGP TEST` | `KEYCARD OPENPGP TEST` |
| NeoPGP key identified | Yes | Yes |
| Counter before | `7` | `7` |
| Physical decision | RED | GREEN |
| PIN requested | No | Yes |
| Signing APDU executed | No | Yes |
| Signature produced | No | Yes |
| Response QR generated | No | Yes |
| Counter after | `7` | `8` |

The central result is:

~~~text
                SAME EXTERNAL REQUEST
                         │
                         ▼
                  TRUSTED DISPLAY
                         │
                PHYSICAL DECISION
                    /          \
                   /            \
               REJECT          APPROVE
                  │                │
                  ▼                ▼
             no key use       hardware key
                  │                │
                  ▼                ▼
                7 → 7            7 → 8
                                   │
                                   ▼
                              response QR
~~~

---

# Hardware Used

The successful v4 test used:

- Microchip SAMA5D3 Xplained board
- NeoPGP-compatible OpenPGP smart card
- HID Global OMNIKEY 3x21 smart-card reader
- Adafruit 1.3" 240×240 ST7789 display
- Physical GREEN APPROVE button
- Physical RED REJECT button
- NexiGo N960E USB webcam
- Custom Buildroot Linux image

---

# Prototype Request Protocol

The current request format is:

~~~text
KC1|OP=PGP_SIGN|MSG=KEYCARD OPENPGP TEST
~~~

`KC1` is prototype framing and is not an OpenPGP standard.

The parser only accepts requests beginning with:

~~~text
KC1|OP=PGP_SIGN|MSG=
~~~

Response objects such as:

~~~text
KC1|FP=...|SIG=...
~~~

are rejected as signing requests.

---

# Trusted Display

For the demonstrated request, the device displayed:

~~~text
OPENPGP SIGN

NEOPGP CARD

31CE69D6...97E8E6D4

KEYCARD OPENPGP TEST

GREEN APPROVE

RED REJECT
~~~

The displayed message and the signing input originate from the same parsed request buffer.

The current v4 prototype also limits messages to 20 printable ASCII characters so that the complete message must fit on the trusted display.

A message that cannot be shown completely is rejected rather than partially displayed and signed.

---

# Security Properties Demonstrated

## Human approval gates key use

~~~text
REJECT:  7 → 7
APPROVE: 7 → 8
~~~

The same external request reached the approval UI in both cases.

Only physical approval caused a signing operation.

## The signing message originates from the optical request

The message is received through the camera and QR parser rather than compiled into v4.

## Reviewed and signed data share one source

The trusted display and hardware-signing call both use the parsed request message.

## Request and response objects are separated

Signing requests and signature responses use distinct prototype message forms.

## The private key remains hardware-backed

The private key resides on the NeoPGP card.

The SAMA5D3 receives the resulting signature, not the private key.

---

# Current Limitations

This remains a prototype and should not yet be described as a production fully air-gapped signer.

Most importantly, the NeoPGP signing PIN is currently entered through an SSH terminal during development.

QR request and QR response transport have been demonstrated, but trusted local PIN entry has not yet moved entirely onto the standalone device.

Ethernet and SSH also remain present during development.

The current `KC1` request and response formats are prototype formats rather than finalized protocols.

The current signing result is the raw secp256k1 cryptographic signature returned by the NeoPGP signing primitive, not yet a complete RFC OpenPGP detached-signature object.

---

# Next Steps

1. Add trusted local PIN entry.
2. Remove SSH from the normal signing path.
3. Physically disconnect Ethernet during normal operation.
4. Bind responses explicitly to requests with a request ID and/or digest.
5. Replace prototype `KC1` framing with a rigorously specified canonical encoding.
6. Support longer messages with secure pagination or structured signing intents.
7. Produce complete OpenPGP signature objects.
8. Add host-side request-generation and response-verification tooling.
9. Add replay protection where appropriate.
10. Add malformed-request and display-consistency tests.

---

# Milestone

The v4 test is the first end-to-end demonstration in this project where an externally generated QR request was captured by the SAMA5D3, validated locally, presented to the user on a trusted display, physically approved or rejected, conditionally signed by a NeoPGP hardware key, and returned as a machine-readable QR response.

The rejected request left the hardware signing counter unchanged:

~~~text
7 → 7
~~~

The approved request performed one hardware-backed signing operation:

~~~text
7 → 8
~~~

> **Receiving a signing request is not sufficient to use the key. A physical approval decision is required before the NeoPGP private key performs a signing operation.**
