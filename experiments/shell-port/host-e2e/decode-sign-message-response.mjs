#!/usr/bin/env node

import { readFileSync, writeFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { URDecoder } from "@ngraveio/bc-ur";

const OUT = "/tmp/openpgp-sign-message.sig";

function parsePacket(data) {
  let off = 0;

  if (data.length < 2) {
    throw new Error("truncated OpenPGP packet");
  }

  const ctb = data[off++];

  if ((ctb & 0x80) === 0) {
    throw new Error("invalid OpenPGP packet CTB");
  }

  let tag;
  let length;

  if (ctb & 0x40) {
    tag = ctb & 0x3f;

    if (off >= data.length) {
      throw new Error("truncated OpenPGP packet length");
    }

    const first = data[off++];

    if (first < 192) {
      length = first;
    } else if (first <= 223) {
      if (off >= data.length) {
        throw new Error("truncated OpenPGP packet length");
      }

      length = ((first - 192) << 8) + data[off++] + 192;
    } else if (first === 255) {
      if (off + 4 > data.length) {
        throw new Error("truncated OpenPGP packet length");
      }

      length = data.readUInt32BE(off);
      off += 4;
    } else {
      throw new Error("partial body lengths unsupported");
    }
  } else {
    tag = (ctb >> 2) & 0x0f;

    const lengthType = ctb & 0x03;

    if (lengthType === 0) {
      if (off >= data.length) {
        throw new Error("truncated OpenPGP packet length");
      }

      length = data[off++];
    } else if (lengthType === 1) {
      if (off + 2 > data.length) {
        throw new Error("truncated OpenPGP packet length");
      }

      length = data.readUInt16BE(off);
      off += 2;
    } else if (lengthType === 2) {
      if (off + 4 > data.length) {
        throw new Error("truncated OpenPGP packet length");
      }

      length = data.readUInt32BE(off);
      off += 4;
    } else {
      throw new Error("indeterminate packet lengths unsupported");
    }
  }

  const end = off + length;

  if (end !== data.length) {
    if (end > data.length) {
      throw new Error("truncated OpenPGP packet body");
    }

    throw new Error("unexpected trailing OpenPGP data");
  }

  return {
    tag,
    body: data.subarray(off, end),
  };
}

function readUrParts(arg) {
  let text;

  if (arg.startsWith("ur:")) {
    text = arg;
  } else if (/\.(png|jpg|jpeg)$/i.test(arg)) {
    text = execFileSync(
      "zbarimg",
      ["--raw", arg],
      { encoding: "utf8" }
    );
  } else {
    text = readFileSync(arg, "utf8");
  }

  return text
    .split(/\r?\n/)
    .map((part) => part.trim())
    .filter((part) => part.length > 0);
}

if (process.argv.length < 3) {
  console.error(
    "usage: node decode-sign-message-response.mjs <ur-text | ur-file | qr-image> [...]"
  );
  process.exit(1);
}

const decoder = new URDecoder();
let received = 0;

for (const arg of process.argv.slice(2)) {
  for (const part of readUrParts(arg)) {
    if (!part.toLowerCase().startsWith("ur:bytes/")) {
      throw new Error(
        `expected UR:BYTES response, got: ${part.slice(0, 32)}`
      );
    }

    decoder.receivePart(part);
    received++;

    if (decoder.isComplete()) {
      break;
    }
  }

  if (decoder.isComplete()) {
    break;
  }
}

if (!decoder.isComplete()) {
  throw new Error(
    `multipart UR response is incomplete after ${received} frame(s)`
  );
}

if (!decoder.isSuccess()) {
  throw new Error(`UR decode failed: ${decoder.resultError()}`);
}

const ur = decoder.resultUR();

if (ur.type !== "bytes") {
  throw new Error(`unexpected UR type: ${ur.type}`);
}

const decoded = ur.decodeCBOR();
const signature = Buffer.from(decoded);
const packet = parsePacket(signature);

if (packet.tag !== 2) {
  throw new Error(
    `expected OpenPGP Signature packet tag 2, got ${packet.tag}`
  );
}

if (packet.body.length < 6) {
  throw new Error("truncated OpenPGP Signature packet");
}

const version = packet.body[0];
const signatureType = packet.body[1];
const publicKeyAlgorithm = packet.body[2];
const hashAlgorithm = packet.body[3];

if (version !== 4) {
  throw new Error(`expected v4 signature, got v${version}`);
}

if (signatureType !== 0x01) {
  throw new Error(
    `expected canonical-text signature type 0x01, got 0x${signatureType.toString(16).padStart(2, "0")}`
  );
}

if (publicKeyAlgorithm !== 19) {
  throw new Error(
    `expected ECDSA algorithm 19, got ${publicKeyAlgorithm}`
  );
}

if (hashAlgorithm !== 8) {
  throw new Error(
    `expected SHA-256 algorithm 8, got ${hashAlgorithm}`
  );
}

writeFileSync(OUT, signature);

console.log(`Recovered signature:      ${signature.length} bytes`);
console.log(`Packet tag:               ${packet.tag}`);
console.log(`Version:                  ${version}`);
console.log(`Signature type:           0x01 canonical text`);
console.log(`Public-key algorithm:     19 ECDSA`);
console.log(`Hash algorithm:           8 SHA-256`);
console.log(`Wrote:                    ${OUT}`);
console.log("RESPONSE STRUCTURE CHECK: PASS");
