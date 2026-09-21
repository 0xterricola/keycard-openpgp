import fs from "node:fs";
import { verifyAttestation } from "@thurinlabs/identity-kit";

async function main() {
  const pgpPublicKey = fs.readFileSync(
    "../../artifacts/terricola-testtt.asc",
    "utf8"
  );

  const pgpSignature = fs.readFileSync(
    "../../artifacts/hardware-thurin-attestation.asc",
    "utf8"
  );

  const result = await verifyAttestation({
    pgpPublicKey,
    pgpSignature,
    fingerprint: "31CE69D66A5E9DE0F977B59C79BB391497E8E6D4",
    ethAddress: "0x9ce2e20fc392304fd1e50541ec67168913b5f3ff",
  });

  console.log(result);

  if (!result.verified) {
    throw new Error(
      `hardware attestation verification failed: ${result.reason ?? "unknown reason"}`
    );
  }

  console.log("HARDWARE OPENPGP ATTESTATION VERIFIED");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
