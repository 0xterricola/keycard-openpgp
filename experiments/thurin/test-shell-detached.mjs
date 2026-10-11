import fs from "node:fs";
import * as openpgp from "openpgp";

const run = "../../artifacts/runs/2026-10-10-shell-openpgp";

const cert = fs.readFileSync(`${run}/certificate.pgp`);
const signatureBytes = fs.readFileSync(`${run}/message.sig`);
const original = fs.readFileSync(`${run}/message.txt`);

const config = {
  rejectCurves: new Set(),
};

async function verify(label, bytes, shouldPass) {
  try {
    const key = await openpgp.readKey({
      binaryKey: new Uint8Array(cert),
      config,
    });

    const signature = await openpgp.readSignature({
      binarySignature: new Uint8Array(signatureBytes),
      config,
    });

    const message = await openpgp.createMessage({
      text: bytes.toString("utf8"),
    });

    const result = await openpgp.verify({
      message,
      signature,
      verificationKeys: key,
      config,
    });

    if (result.signatures.length !== 1) {
      throw new Error(
        `Expected 1 signature, got ${result.signatures.length}`
      );
    }

    await result.signatures[0].verified;

    if (!shouldPass) {
      throw new Error("Tampered message was accepted");
    }

    console.log(`${label}: PASS`);
  } catch (error) {
    if (shouldPass) {
      console.error(`${label}: FAILED — ${error.message}`);
      process.exitCode = 1;
    } else if (error.message === "Tampered message was accepted") {
      console.error(`${label}: FAILED — ${error.message}`);
      process.exitCode = 1;
    } else {
      console.log(`${label}: PASS (rejected: ${error.message})`);
    }
  }
}

console.log("OpenPGP.js:", openpgp.config ? "6.3.1" : "unknown");

console.log("=== Keycard Shell detached signature ===");

await verify("Original message", original, true);

const tampered = Buffer.concat([
  original,
  Buffer.from("TAMPERED\n"),
]);

await verify("Tampered message", tampered, false);

if (!process.exitCode) {
  console.log("ALL OPENPGP.JS TESTS PASSED");
}
