#!/usr/bin/env node

import { writeFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { encode, decode } from "cbor2";
import { UR, UREncoder, URDecoder } from "@ngraveio/bc-ur";

const VERSION = 1;
const CREATE_IDENTITY = 1;

const uidText = process.argv[2];
const creationTimeText = process.argv[3];

if (!uidText || !creationTimeText) {
  console.error(
    'usage: node generate-request.mjs "Name <email@example.com>" <unix_creation_time>'
  );
  process.exit(1);
}

const creationTime = Number(creationTimeText);

if (
  !Number.isInteger(creationTime) ||
  creationTime <= 0 ||
  creationTime > 0xffffffff
) {
  throw new Error("creation_time must be an integer from 1 through 4294967295");
}

const uid = new TextEncoder().encode(uidText);

if (uid.length === 0 || uid.length > 255) {
  throw new Error("UID must encode to 1..255 bytes");
}

for (const b of uid) {
  if (b < 0x20 || b > 0x7e) {
    throw new Error("UID must contain printable ASCII only");
  }
}

/*
 * Frozen OpenPGP request:
 *
 * {
 *   1: version,
 *   2: operation,
 *   3: UID bytes,
 *   4: creation_time
 * }
 */
const request = new Map([
  [1, VERSION],
  [2, CREATE_IDENTITY],
  [3, uid],
  [4, creationTime],
]);

const innerCbor = encode(request);

/*
 * UR.fromBuffer() creates a UR:BYTES object whose CBOR payload is a
 * byte string containing innerCbor. Shell's ui_qrscan(BYTES, ...)
 * removes that outer byte-string layer before parsing this request.
 */
const ur = UR.fromBuffer(Buffer.from(innerCbor));
const encoder = new UREncoder(ur, 400, 0);
const urText = encoder.nextPart();

if (!urText.startsWith("ur:bytes/")) {
  throw new Error(`unexpected UR type: ${urText}`);
}

/* Self-check the UR round trip. */
const decoder = new URDecoder();
decoder.receivePart(urText);

if (!decoder.isComplete() || !decoder.isSuccess()) {
  throw new Error(`UR self-check failed: ${decoder.resultError()}`);
}

const recoveredInner = decoder.resultUR().decodeCBOR();

if (!Buffer.from(recoveredInner).equals(Buffer.from(innerCbor))) {
  throw new Error("UR round trip changed the inner CBOR");
}

/* Self-check the frozen request fields. */
const parsed = decode(innerCbor);

if (
  !(parsed instanceof Map) ||
  parsed.get(1) !== VERSION ||
  parsed.get(2) !== CREATE_IDENTITY ||
  parsed.get(4) !== creationTime ||
  Buffer.from(parsed.get(3)).toString("utf8") !== uidText
) {
  throw new Error("CBOR request self-check failed");
}

const textOut = "/tmp/openpgp-create-identity.ur";
const qrOut = "/tmp/openpgp-create-identity.png";

writeFileSync(textOut, urText + "\n");

execFileSync(
  "qrencode",
  ["-l", "M", "-s", "6", "-m", "4", "-o", qrOut],
  { input: urText }
);

console.log(`UID:           ${uidText}`);
console.log(`creation_time: ${creationTime}`);
console.log(`inner CBOR:    ${Buffer.from(innerCbor).toString("hex")}`);
console.log(`UR:            ${urText}`);
console.log(`Wrote:         ${textOut}`);
console.log(`Wrote:         ${qrOut}`);
console.log("REQUEST SELF-CHECK: PASS");
