#!/usr/bin/env node

import fs from "node:fs";
import { URDecoder } from "@ngraveio/bc-ur";

const [input, expectedType] = process.argv.slice(2);

if (!input || !expectedType) {
  console.error(
    "usage: node check-ur-complete.mjs <ur-fragments.txt> <expected-type>"
  );
  process.exit(1);
}

const parts = fs
  .readFileSync(input, "utf8")
  .split(/\r?\n/)
  .map((part) => part.trim())
  .filter(Boolean);

if (parts.length === 0) {
  process.exit(2);
}

const decoder = new URDecoder();

for (const part of parts) {
  decoder.receivePart(part);

  if (decoder.isComplete()) {
    break;
  }
}

if (!decoder.isComplete()) {
  process.exit(2);
}

if (!decoder.isSuccess()) {
  process.exit(3);
}

const ur = decoder.resultUR();

if (ur.type.toLowerCase() !== expectedType.toLowerCase()) {
  process.exit(4);
}

process.exit(0);
