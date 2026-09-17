import fs from "node:fs";
import * as openpgp from "openpgp";

async function main() {
  const armoredKey = fs.readFileSync(
    "../../artifacts/terricola-testtt.asc",
    "utf8"
  );

  const cleartextMessage = fs.readFileSync(
    "./attestation.txt.asc",
    "utf8"
  );

  const publicKey = await openpgp.readKey({ armoredKey });

  console.log("fingerprint:", publicKey.getFingerprint());
  console.log("message:\n", cleartextMessage);

  try {
    const message = await openpgp.readCleartextMessage({ cleartextMessage });

    const result = await openpgp.verify({
      message,
      verificationKeys: publicKey,
      config: {
        rejectCurves: new Set(),
      },
    });

    console.log("signed text:", message.getText());
    console.log("signatures:", result.signatures.length);

    for (const sig of result.signatures) {
      try {
        await sig.verified;
        console.log("signature: VERIFIED");
      } catch (err) {
        console.error("signature FAILED:", err);
      }
    }
  } catch (err) {
    console.error("verify setup FAILED:", err);
  }
}

main().catch(console.error);
