import fs from "node:fs";
import crypto from "node:crypto";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

const verbose = process.env.DEMO_VERBOSE === "1";

const {
  EthSignRequest,
  DataType,
  extend,
} = require("@keystonehq/bc-ur-registry-eth");

const {
  UR,
  UREncoder,
} = require("@ngraveio/bc-ur");

const [
  signoutFile,
  derivationPath,
  sourceFingerprint,
  owner,
  partsFile,
  requestIdFile,
] = process.argv.slice(2);

if (
  !signoutFile ||
  !derivationPath ||
  !sourceFingerprint ||
  !owner ||
  !partsFile ||
  !requestIdFile
) {
  throw new Error(
    "usage: node generate-thurin-eth-request.mjs " +
    "<signout.json> <path> <xfp> <owner> <parts-out> <request-id-out>"
  );
}

const payload = JSON.parse(
  fs.readFileSync(signoutFile, "utf8")
);

const sourceTypedData = payload.typedData;

if (!sourceTypedData) {
  throw new Error("sign-out JSON has no typedData");
}

const typedData = structuredClone(sourceTypedData);

if (!typedData.types.EIP712Domain) {
  typedData.types = {
    EIP712Domain: [
      { name: "name", type: "string" },
      { name: "version", type: "string" },
      { name: "chainId", type: "uint256" },
      { name: "verifyingContract", type: "address" },
    ],
    ...typedData.types,
  };

  console.log("EIP712Domain:     injected");
}

const chainId = Number(typedData.domain?.chainId);

if (!Number.isSafeInteger(chainId)) {
  throw new Error("typedData domain has invalid chainId");
}

const typedDataBytes = Buffer.from(
  JSON.stringify(typedData),
  "utf8"
);

const requestId = crypto.randomUUID();

const request = EthSignRequest.constructETHRequest(
  typedDataBytes,
  DataType.typedData,
  derivationPath,
  sourceFingerprint,
  requestId,
  chainId,
  owner,
  "thurin.id"
);

const cbor = extend.encodeDataItem(
  request.toDataItem()
);

const ur = new UR(
  Buffer.from(cbor),
  "eth-sign-request"
);

const encoder = new UREncoder(
  ur,
  180,
  0
);

const first = encoder.nextPart();
const parts = [first];

const multipart = first.match(
  /^ur:eth-sign-request\/1-(\d+)\//i
);

if (multipart) {
  const seqLen = Number(multipart[1]);

  for (let i = 1; i < seqLen; i++) {
    parts.push(encoder.nextPart());
  }
}

fs.writeFileSync(
  partsFile,
  parts.join("\n") + "\n"
);

const idBytes = request.getRequestId();

fs.writeFileSync(
  requestIdFile,
  Buffer.from(idBytes).toString("hex") + "\n"
);

console.log(`typedData bytes: ${typedDataBytes.length}`);
console.log(`chainId:         ${chainId}`);
console.log(`derivation:      ${derivationPath}`);
console.log(
  `source xfp:      ${
    verbose ? sourceFingerprint : "[captured from Keycard]"
  }`
);
console.log(
  `request id:      ${verbose ? requestId : "[generated]"}`
);
console.log(`UR frames:       ${parts.length}`);
console.log(`Wrote:           ${partsFile}`);
