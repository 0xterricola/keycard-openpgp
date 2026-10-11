#!/usr/bin/env node

import { spawnSync } from "node:child_process";
import {
  mkdtempSync,
  readFileSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const HOST = resolve(ROOT, "experiments/shell-port/host-e2e");

function run(program, args, options = {}) {
  const result = spawnSync(program, args, {
    stdio: "inherit",
    ...options,
  });

  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(`${program} exited with status ${result.status}`);
  }
}

function capture(program, args, options = {}) {
  const result = spawnSync(program, args, {
    encoding: "utf8",
    ...options,
  });

  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(result.stderr || `${program} failed`);
  }
  return result.stdout;
}

function option(args, name) {
  const index = args.indexOf(name);
  if (index < 0 || !args[index + 1]) {
    throw new Error(`Missing required option: ${name}`);
  }
  return args[index + 1];
}

function certificateMetadata(path) {
  const output = capture("gpg", [
    "--batch",
    "--import-options", "show-only",
    "--with-colons",
    "--import",
    path,
  ]);

  const lines = output.split("\n");
  const pub = lines.find(line => line.startsWith("pub:"));
  const fpr = lines.find(line => line.startsWith("fpr:"));

  if (!pub || !fpr) {
    throw new Error("Cannot read OpenPGP certificate metadata");
  }

  const creationTime = Number(pub.split(":")[5]);
  const fingerprint = fpr.split(":")[9];

  if (!Number.isInteger(creationTime) || creationTime <= 0 ||
      !/^[0-9a-f]{40}$/i.test(fingerprint)) {
    throw new Error("Invalid certificate metadata");
  }

  return { creationTime, fingerprint };
}

const [command, ...args] = process.argv.slice(2);

try {
  switch (command) {
    case "create-identity": {
      const uid = option(args, "--uid");
      const timestamp = String(Math.floor(Date.now() / 1000));

      run("node", [
        resolve(HOST, "generate-request.mjs"),
        uid,
        timestamp,
      ]);

      break;
    }

    case "decode-identity": {
      const input = option(args, "--input");

      run("node", [
        resolve(HOST, "decode-response.mjs"),
        input,
      ]);

      break;
    }

    case "sign-message": {
      const cert = option(args, "--cert");
      const messageFile = option(args, "--file");

      const { creationTime, fingerprint } =
        certificateMetadata(cert);

      const message = readFileSync(messageFile, "utf8");

      const timestamp = Math.max(
        Math.floor(Date.now() / 1000),
        creationTime,
      );

      console.log(`Certificate fingerprint: ${fingerprint}`);

      run("node", [
        resolve(HOST, "generate-sign-message-request.mjs"),
        message,
        String(creationTime),
        String(timestamp),
      ]);

      break;
    }

    case "decode-signature": {
      const input = option(args, "--input");

      run("node", [
        resolve(HOST, "decode-sign-message-response.mjs"),
        input,
      ]);

      break;
    }

    case "verify": {
      const cert = option(args, "--cert");
      const message = option(args, "--file");
      const signature = option(args, "--signature");

      const home = mkdtempSync(
        resolve(tmpdir(), "keycard-openpgp-gnupg-"),
      );

      try {
        run("gpg", [
          "--homedir", home,
          "--batch",
          "--import",
          cert,
        ]);

        run("gpg", [
          "--homedir", home,
          "--batch",
          "--verify",
          signature,
          message,
        ]);

        console.log("SIGNATURE VERIFICATION: PASS");
      } finally {
        rmSync(home, { recursive: true, force: true });
      }

      break;
    }

    default:
      console.log(`
Keycard Shell OpenPGP Reference Client

Commands:

  create-identity --uid "Name <email@example.com>"

  decode-identity --input response.ur

  sign-message --cert certificate.pgp --file message.txt

  decode-signature --input response.ur

  verify --cert certificate.pgp --file message.txt --signature message.sig

This client uses the existing experimental Shell QR scripts.
      `.trim());
      if (command) process.exitCode = 1;
  }
} catch (error) {
  console.error(`ERROR: ${error.message}`);
  process.exitCode = 1;
}
