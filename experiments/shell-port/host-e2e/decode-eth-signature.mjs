import fs from "node:fs";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

const {
  URDecoder,
} = require("@ngraveio/bc-ur");

const {
  ETHSignature,
} = require("@keystonehq/bc-ur-registry-eth");

const [input, expectedRequestIdFile] =
  process.argv.slice(2);

if (!input || !expectedRequestIdFile) {
  throw new Error(
    "usage: node decode-eth-signature.mjs " +
    "<ur-fragments.txt> <expected-request-id-file>"
  );
}

const parts = fs
  .readFileSync(input, "utf8")
  .split(/\r?\n/)
  .map((x) => x.trim())
  .filter(Boolean);

const decoder = new URDecoder();

for (const part of parts) {
  decoder.receivePart(part);

  if (decoder.isComplete()) {
    break;
  }
}

if (!decoder.isComplete()) {
  throw new Error("eth-signature UR is incomplete");
}

if (!decoder.isSuccess()) {
  throw new Error(decoder.resultError());
}

const ur = decoder.resultUR();

if (ur.type.toLowerCase() !== "eth-signature") {
  throw new Error(
    `expected eth-signature, got ${ur.type}`
  );
}

const result = ETHSignature.fromCBOR(ur.cbor);

const signature = Buffer.from(
  result.getSignature()
);

if (signature.length !== 65) {
  throw new Error(
    `expected 65-byte Ethereum signature, got ${signature.length}`
  );
}

const expectedId = fs
  .readFileSync(expectedRequestIdFile, "utf8")
  .trim()
  .toLowerCase();

const responseId = result.getRequestId();

if (responseId) {
  const actualId = Buffer.from(responseId)
    .toString("hex")
    .toLowerCase();

  if (actualId !== expectedId) {
    throw new Error(
      `request-id mismatch: ${actualId} != ${expectedId}`
    );
  }
}

console.error(
  `Ethereum signature: ${signature.length} bytes`
);

console.error(
  `v: 0x${signature
    .subarray(64)
    .toString("hex")}`
);

process.stdout.write(
  `0x${signature.toString("hex")}\n`
);
