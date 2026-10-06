import fs from "node:fs";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

const { URDecoder } = require("@ngraveio/bc-ur");
const { CryptoHDKey } = require("@keystonehq/bc-ur-registry");

const verbose = process.env.DEMO_VERBOSE === "1";

const input = process.argv[2];
const addressIndexText = process.argv[3] ?? "0";
const addressIndex = Number(addressIndexText);

if (
  !Number.isSafeInteger(addressIndex) ||
  addressIndex < 0
) {
  throw new Error(
    "address index must be a non-negative integer"
  );
}

if (!input) {
  throw new Error(
    "usage: node decode-eip4527-account.mjs <ur-fragments.txt>"
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
  throw new Error("crypto-hdkey UR is incomplete");
}

if (!decoder.isSuccess()) {
  throw new Error(decoder.resultError());
}

const ur = decoder.resultUR();

if (ur.type.toLowerCase() !== "crypto-hdkey") {
  throw new Error(`expected crypto-hdkey, got ${ur.type}`);
}

const hd = CryptoHDKey.fromCBOR(ur.cbor);
const origin = hd.getOrigin();

if (!origin) {
  throw new Error("crypto-hdkey has no origin");
}

function componentText(component) {
  const wildcard =
    typeof component.isWildcard === "function" &&
    component.isWildcard();

  const hardened =
    typeof component.isHardened === "function" &&
    component.isHardened();

  if (wildcard) {
    return `*${hardened ? "'" : ""}`;
  }

  const index = component.getIndex();

  return `${index}${hardened ? "'" : ""}`;
}

function keypathText(keypath, includeM) {
  if (!keypath) {
    return "";
  }

  if (typeof keypath.getComponents === "function") {
    const components = keypath.getComponents();

    if (Array.isArray(components) && components.length > 0) {
      const body = components.map(componentText).join("/");
      return includeM ? `m/${body}` : body;
    }
  }

  if (typeof keypath.getPath === "function") {
    let path = String(keypath.getPath());

    path = path.replace(/^[mM]\//, "");

    return includeM ? `m/${path}` : path;
  }

  throw new Error("unable to decode keypath");
}

function fingerprintHex(value) {
  if (typeof value === "number") {
    return value.toString(16).padStart(8, "0").toUpperCase();
  }

  const buf = Buffer.from(value);

  if (buf.length !== 4) {
    throw new Error(
      `expected 4-byte source fingerprint, got ${buf.length}`
    );
  }

  return buf.toString("hex").toUpperCase();
}

const originPath = keypathText(origin, true);

const sourceFingerprint = fingerprintHex(
  origin.getSourceFingerprint()
);

const children =
  typeof hd.getChildren === "function"
    ? hd.getChildren()
    : null;

let childPath;
let childDescription;

if (children) {
  childPath = keypathText(children, false);

  if (!childPath) {
    throw new Error("empty EIP4527 child derivation path");
  }

  if (childPath.includes("*")) {
    childPath = childPath.replace("*", "0");
  }

  childDescription = keypathText(children, false);
} else {
  if (originPath !== "m/44'/60'/0'") {
    throw new Error(
      `unexpected standard Ethereum origin: ${originPath}`
    );
  }

  // Keycard Shell account.standard exports the account-level key only.
  // The normal Ethereum address screen derives change=0, then address index.
  // Our runtime address capture explicitly uses index 0.
  childPath = `0/${addressIndex}`;
  childDescription =
    `(implicit Shell standard: 0/${addressIndex})`;
}

const signingPath = `${originPath}/${childPath}`;

console.error(`EIP4527 origin:     ${originPath}`);
console.error(`EIP4527 children:   ${childDescription}`);
console.error(`EIP4527 sign path:  ${signingPath}`);
console.error(
  `source fingerprint: ${
    verbose ? sourceFingerprint : "[captured from Keycard]"
  }`
);

process.stdout.write(
  `${signingPath}\t${sourceFingerprint}\n`
);
