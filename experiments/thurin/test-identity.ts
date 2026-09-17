import fs from "node:fs";
import { parsePgpKey } from "@thurinlabs/identity-kit";

async function main() {
  const armored = fs.readFileSync(
    "../../artifacts/terricola-testtt.asc",
    "utf8"
  );

  const info = await parsePgpKey(armored);

  console.dir(info, { depth: null });
}

main().catch(console.error);
