import fs from "node:fs";
import * as openpgp from "openpgp";

async function main() {
  const armored = fs.readFileSync(
    "../../artifacts/terricola-testtt.asc",
    "utf8"
  );

  const key: any = await openpgp.readKey({ armoredKey: armored });

  console.log("getUserIDs:", key.getUserIDs());

  for (const user of key.users) {
    console.log("UID:", user.userID?.userID);

    for (const cert of user.selfCertifications ?? []) {
      try {
        await cert.verify(
          key.keyPacket,
          cert.signatureType,
          { userID: user.userID, key: key.keyPacket }
        );

        console.log("self-cert: VERIFIED");
      } catch (err) {
        console.error("self-cert FAILED:", err);
      }
    }
  }
}

main().catch(console.error);
