#!/usr/bin/env node

import { writeFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { encode, decode } from "cbor2";
import { UR, UREncoder, URDecoder } from "@ngraveio/bc-ur";

const verbose = process.env.DEMO_VERBOSE === "1";

const VERSION = 1;
const SIGN_MESSAGE = 2;
const MESSAGE_MAX_LEN = 104;

const messageText = process.argv[2];
const keyCreationTimeText = process.argv[3];
const signatureCreationTimeText = process.argv[4];

if (!messageText || !keyCreationTimeText || !signatureCreationTimeText) {
  console.error(
    'usage: node generate-sign-message-request.mjs "<message>" <key_creation_time> <signature_creation_time>'
  );
  process.exit(1);
}

function parseTimestamp(name, value) {
  const timestamp = Number(value);

  if (
    !Number.isInteger(timestamp) ||
    timestamp <= 0 ||
    timestamp > 0xffffffff
  ) {
    throw new Error(`${name} must be an integer from 1 through 4294967295`);
  }

  return timestamp;
}

const keyCreationTime = parseTimestamp(
  "key_creation_time",
  keyCreationTimeText
);

const signatureCreationTime = parseTimestamp(
  "signature_creation_time",
  signatureCreationTimeText
);

if (signatureCreationTime < keyCreationTime) {
  throw new Error(
    "signature_creation_time must not precede key_creation_time"
  );
}

const message = new TextEncoder().encode(messageText);

if (message.length === 0 || message.length > MESSAGE_MAX_LEN) {
  throw new Error(
    `message must encode to 1..${MESSAGE_MAX_LEN} bytes`
  );
}

for (let i = 0; i < message.length; i++) {
  const b = message[i];

  if (b === 0x0d) {
    if (i + 1 >= message.length || message[i + 1] !== 0x0a) {
      throw new Error("message contains a bare CR");
    }

    i++;
    continue;
  }

  if (b === 0x0a) {
    continue;
  }

  if (b < 0x20 || b > 0x7e) {
    throw new Error(
      "message must contain printable ASCII with LF or CRLF line endings"
    );
  }
}

/*
 * SIGN_MESSAGE:
 *
 * {
 *   1: version,
 *   2: operation,
 *   3: message bytes,
 *   4: key_creation_time,
 *   5: signature_creation_time
 * }
 */
const request = new Map([
  [1, VERSION],
  [2, SIGN_MESSAGE],
  [3, message],
  [4, keyCreationTime],
  [5, signatureCreationTime],
]);

const innerCbor = encode(request);

const ur = UR.fromBuffer(Buffer.from(innerCbor));
const encoder = new UREncoder(ur, 400, 0);
const urText = encoder.nextPart();

if (!urText.startsWith("ur:bytes/")) {
  throw new Error(`unexpected UR type: ${urText}`);
}

const decoder = new URDecoder();
decoder.receivePart(urText);

if (!decoder.isComplete() || !decoder.isSuccess()) {
  throw new Error(`UR self-check failed: ${decoder.resultError()}`);
}

const recoveredInner = decoder.resultUR().decodeCBOR();

if (!Buffer.from(recoveredInner).equals(Buffer.from(innerCbor))) {
  throw new Error("UR round trip changed the inner CBOR");
}

const parsed = decode(innerCbor);

if (
  !(parsed instanceof Map) ||
  parsed.get(1) !== VERSION ||
  parsed.get(2) !== SIGN_MESSAGE ||
  parsed.get(4) !== keyCreationTime ||
  parsed.get(5) !== signatureCreationTime ||
  !Buffer.from(parsed.get(3)).equals(Buffer.from(message))
) {
  throw new Error("CBOR request self-check failed");
}

const textOut = "/tmp/openpgp-sign-message.ur";
const qrOut = "/tmp/openpgp-sign-message.png";
const messageOut = "/tmp/openpgp-sign-message.txt";

writeFileSync(textOut, urText + "\n");
writeFileSync(messageOut, Buffer.from(message));

execFileSync(
  "qrencode",
  ["-l", "M", "-s", "6", "-m", "4", "-o", qrOut],
  { input: urText }
);

console.log(`message:                 ${JSON.stringify(messageText)}`);
console.log(`message bytes:           ${message.length}`);
console.log(`key_creation_time:       ${keyCreationTime}`);
console.log(`signature_creation_time: ${signatureCreationTime}`);
if (verbose) {
  console.log(
    `inner CBOR:              ${Buffer.from(innerCbor).toString("hex")}`
  );
  console.log(`UR:                      ${urText}`);
} else {
  console.log("inner CBOR:              [generated]");
  console.log("UR:                      [generated]");
}
console.log(`Wrote:                   ${textOut}`);
console.log(`Wrote:                   ${qrOut}`);
console.log(`Wrote:                   ${messageOut}`);
console.log("REQUEST SELF-CHECK: PASS");
