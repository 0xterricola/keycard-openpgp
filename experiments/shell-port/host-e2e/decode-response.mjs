#!/usr/bin/env node

import { readFileSync, writeFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { URDecoder } from "@ngraveio/bc-ur";

const OUT = "/tmp/openpgp-created-identity.pgp";

function parsePackets(data) {
  const packets = [];
  let off = 0;

  while (off < data.length) {
    const start = off;
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

        length =
          data.readUInt32BE(off);
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

    const bodyStart = off;
    const end = bodyStart + length;

    if (end > data.length) {
      throw new Error("truncated OpenPGP packet body");
    }

    packets.push({
      tag,
      raw: data.subarray(start, end),
      body: data.subarray(bodyStart, end),
    });

    off = end;
  }

  return packets;
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
    "usage: node decode-response.mjs <ur-text | ur-file | qr-image> [...]"
  );
  process.exit(1);
}

const decoder = new URDecoder();
let received = 0;

for (const arg of process.argv.slice(2)) {
  for (const part of readUrParts(arg)) {
    if (!part.toLowerCase().startsWith("ur:bytes/")) {
      throw new Error(`expected UR:BYTES response, got: ${part.slice(0, 32)}`);
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

/*
 * Shell emits BYTES whose CBOR value is the complete OpenPGP certificate.
 * decodeCBOR() removes that outer CBOR byte-string layer.
 */
const decoded = ur.decodeCBOR();
const cert = Buffer.from(decoded);

const packets = parsePackets(cert);
const tags = packets.map((p) => p.tag);

if (
  tags.length !== 3 ||
  tags[0] !== 6 ||
  tags[1] !== 13 ||
  tags[2] !== 2
) {
  throw new Error(
    `expected packet sequence [6,13,2], got [${tags.join(",")}]`
  );
}

writeFileSync(OUT, cert);

console.log(`Recovered certificate: ${cert.length} bytes`);
console.log(`Packet sequence:       ${tags.join(" -> ")}`);
console.log(`UID:                   ${packets[1].body.toString("utf8")}`);
console.log(`Wrote:                 ${OUT}`);
console.log("RESPONSE STRUCTURE CHECK: PASS");
