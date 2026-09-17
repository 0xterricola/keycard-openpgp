import fs from "node:fs";
import * as openpgp from "openpgp";

async function main() {
  const armored = fs.readFileSync(
    "../../artifacts/terricola-testtt.asc",
    "utf8"
  );

  const key = await openpgp.readKey({ armoredKey: armored });

  console.log("fingerprint:", key.getFingerprint());
  console.log("userIDs:", key.getUserIDs());
}

main().catch(console.error);
