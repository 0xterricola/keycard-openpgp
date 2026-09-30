# Keycard Shell OpenPGP Host E2E

This folder contains a small host-side test for the Keycard Shell OpenPGP identity flow.

You do **not** need to clone or fork the whole `keycard-openpgp` repository.

You only need four files:

    generate-request.mjs
    decode-response.mjs
    package.json
    package-lock.json

The test does this:

    computer creates request QR
              ↓
         Keycard Shell
              ↓
       Shell derives an
        OpenPGP key
              ↓
      creates certificate
              ↓
      animated response QR
              ↓
      computer decodes it
              ↓
        GnuPG verifies it
              ↓
             PASS

If the final GnuPG check reports a good self-signature, the OpenPGP identity flow worked end-to-end.

---

# Platform status

## macOS

Tested end-to-end on a physical Keycard Shell using:

- macOS
- MacBook built-in camera
- FFmpeg
- ZBar
- GnuPG
- Node.js

This is the currently known-good setup.

## Linux

The Node.js tools and command-line flow are expected to work on Linux.

Linux has **not yet been tested end-to-end by this project**.

A Linux webcam capture example is included below.

## Windows

Windows has **not yet been tested end-to-end by this project**.

The easiest current Windows route is WSL because the test scripts and examples use Unix-style temporary paths such as `/tmp`.

Native Windows support can be added later, but is not currently validated.

---

# 1. Download only the E2E tester

You do not need the rest of the repository.

## macOS or Linux

Create a folder:

    mkdir -p ~/keycard-openpgp-e2e
    cd ~/keycard-openpgp-e2e

Download the files:

    BASE="https://raw.githubusercontent.com/0xterricola/keycard-openpgp/feature/host-e2e-multipart-ur/experiments/shell-port/host-e2e"

    curl -LO "$BASE/generate-request.mjs"
    curl -LO "$BASE/decode-response.mjs"
    curl -LO "$BASE/package.json"
    curl -LO "$BASE/package-lock.json"

Check them:

    ls -lh

You should have:

    generate-request.mjs
    decode-response.mjs
    package.json
    package-lock.json

## Windows with WSL

Open your WSL terminal.

Then use the same commands:

    mkdir -p ~/keycard-openpgp-e2e
    cd ~/keycard-openpgp-e2e

    BASE="https://raw.githubusercontent.com/0xterricola/keycard-openpgp/feature/host-e2e-multipart-ur/experiments/shell-port/host-e2e"

    curl -LO "$BASE/generate-request.mjs"
    curl -LO "$BASE/decode-response.mjs"
    curl -LO "$BASE/package.json"
    curl -LO "$BASE/package-lock.json"

    ls -lh

---

# 2. Install the required tools

You need:

- Node.js 20 or newer
- npm
- qrencode
- GnuPG
- ZBar (`zbarimg`)
- FFmpeg if you want to extract QR frames from video

---

## macOS

With Homebrew:

    brew install node qrencode gnupg zbar ffmpeg

Check the tools:

    node --version
    npm --version
    qrencode --version
    gpg --version
    zbarimg --version
    ffmpeg -version | head -1

Your Node.js version should be 20 or newer.

---

## Ubuntu / Debian Linux

Install the command-line tools:

    sudo apt update
    sudo apt install -y nodejs npm qrencode gnupg zbar-tools ffmpeg

Check the versions:

    node --version
    npm --version
    qrencode --version
    gpg --version
    zbarimg --version
    ffmpeg -version | head -1

Node.js must be version 20 or newer.

Some Linux distributions ship an older Node.js version.

If `node --version` reports something older than 20, install a newer Node.js release using your preferred Node version manager or package source before continuing.

---

## Windows with WSL

Inside WSL:

    sudo apt update
    sudo apt install -y nodejs npm qrencode gnupg zbar-tools ffmpeg

Then check:

    node --version
    npm --version
    qrencode --version
    gpg --version
    zbarimg --version
    ffmpeg -version | head -1

Node.js must be version 20 or newer.

---

# 3. Install the JavaScript dependencies

From the folder containing the four downloaded files:

    npm ci

That creates `node_modules`.

You are now ready to generate a request.

---

# 4. Generate an OpenPGP identity request

Create a Unix timestamp:

    TS=$(date +%s)

Generate a test identity request:

    node generate-request.mjs \
      "Keycard Test <keycard@example.com>" \
      "$TS"

A successful run should end with:

    REQUEST SELF-CHECK: PASS

The generator creates:

    /tmp/openpgp-create-identity.ur
    /tmp/openpgp-create-identity.png

The PNG is the QR code that the Shell needs to scan.

---

# 5. Open the request QR

## macOS

    open /tmp/openpgp-create-identity.png

## Linux

    xdg-open /tmp/openpgp-create-identity.png

## Windows with WSL

One simple option is to open the Linux temporary folder in Windows Explorer:

    explorer.exe /tmp

Then open:

    openpgp-create-identity.png

Leave the QR visible on the screen.

---

# 6. Scan the request with the Keycard Shell

On the Shell, go to:

    Extras
      → OpenPGP

Scan the QR displayed on the computer.

The Shell should show one paged review containing:

    UID
    Unix time
    fingerprint

For this example, the UID should be:

    Keycard Test <keycard@example.com>

Review the information.

Then approve it.

The Shell should generate the OpenPGP certificate and display an **animated QR code**.

Keep the animated QR open.

That QR is the certificate response coming back from the Shell.

---

# 7. Capture the animated response

You need a collection of clear images containing different frames of the animated QR.

There are several ways to do this.

---

## Method A: use any phone or camera

This is the most platform-independent method.

Record about 30 seconds of the Shell's animated QR using:

- a phone
- a webcam application
- another camera

Try to keep:

- the Shell steady
- the QR square to the camera
- glare off the display
- the QR reasonably large in the video

Copy the video to the computer.

For the commands below, rename it:

    shell-response.mp4

Create a frame directory:

    rm -rf /tmp/openpgp-frames
    mkdir -p /tmp/openpgp-frames

Extract 15 images per second:

    ffmpeg \
      -i shell-response.mp4 \
      -vf "fps=15" \
      /tmp/openpgp-frames/frame-%05d.png

Continue to the QR extraction section below.

---

## Method B: macOS built-in camera

This method has been tested.

First list the cameras:

    ffmpeg -f avfoundation -list_devices true -i "" 2>&1 | \
      sed -n '/AVFoundation video devices:/,/AVFoundation audio devices:/p'

Example:

    [0] MacBook Pro Camera
    [1] iPhone Camera

If the MacBook camera is device `0`, create the frame directory:

    rm -rf /tmp/openpgp-frames
    mkdir -p /tmp/openpgp-frames

Hold the Shell's animated QR in front of the MacBook camera.

Then run:

    ffmpeg \
      -f avfoundation \
      -framerate 30 \
      -i "0:none" \
      -t 30 \
      -vf "fps=15" \
      /tmp/openpgp-frames/frame-%05d.png

If your camera has a different device number, replace `0`.

This exact method has been used successfully on real hardware.

---

## Method C: Linux webcam

This method has not yet been validated by this project.

Linux cameras often appear as:

    /dev/video0

You can check:

    ls /dev/video*

If your camera is `/dev/video0`, create the frame directory:

    rm -rf /tmp/openpgp-frames
    mkdir -p /tmp/openpgp-frames

Then try:

    ffmpeg \
      -f v4l2 \
      -framerate 30 \
      -i /dev/video0 \
      -t 30 \
      -vf "fps=15" \
      /tmp/openpgp-frames/frame-%05d.png

Hold the Shell's animated QR in front of the camera while it records.

Camera devices and supported formats vary between Linux systems, so you may need to adjust the input device or capture settings.

---

## Windows

The live-camera method has not yet been tested on Windows.

For now, the simplest option is:

1. record the animated Shell QR with a phone or camera
2. copy the video into your WSL-accessible filesystem
3. use Method A above

This avoids depending on Windows-specific webcam capture configuration.

---

# 8. Extract UR fragments from the QR images

Create an empty output file:

    : > /tmp/openpgp-response-frames.txt

Run ZBar over every captured image:

    for f in /tmp/openpgp-frames/*.png; do
      zbarimg \
        --set '*.disable' \
        --set 'qrcode.enable' \
        --raw \
        "$f" \
        2>/dev/null >> /tmp/openpgp-response-frames.txt || true
    done

Now keep only unique OpenPGP response fragments:

    awk '/^[Uu][Rr]:[Bb][Yy][Tt][Ee][Ss]\// && !seen[$0]++' \
      /tmp/openpgp-response-frames.txt \
      > /tmp/openpgp-response-frames-unique.txt

Count the unique fragments:

    wc -l /tmp/openpgp-response-frames-unique.txt

Look at the first few:

    head -5 /tmp/openpgp-response-frames-unique.txt

Valid fragments begin with:

    UR:BYTES/

You may see values such as:

    UR:BYTES/280-2/...
    UR:BYTES/281-2/...
    UR:BYTES/282-2/...

Those numbers may look strange.

That is normal.

The Shell uses BC-UR fountain encoding.

You do not need to capture specific numbered frames, and the frames do not need to arrive in order.

You only need enough valid fragments for the decoder to reconstruct the response.

---

# 9. Decode the response

Run:

    node decode-response.mjs \
      /tmp/openpgp-response-frames-unique.txt

A successful result looks similar to:

    Recovered certificate: 236 bytes
    Packet sequence:       6 -> 13 -> 2
    UID:                   Keycard Test <keycard@example.com>
    Wrote:                 /tmp/openpgp-created-identity.pgp
    RESPONSE STRUCTURE CHECK: PASS

The exact certificate size may differ.

The important line is:

    RESPONSE STRUCTURE CHECK: PASS

The resulting OpenPGP certificate is written to:

    /tmp/openpgp-created-identity.pgp

The expected packet sequence is:

    6  = Public Key
    13 = User ID
    2  = Signature

---

# 10. Inspect the certificate with GnuPG

You can inspect it without importing it into your normal keyring:

    gpg \
      --import-options show-only \
      --import \
      /tmp/openpgp-created-identity.pgp

You should see:

- a secp256k1 public key
- a fingerprint
- the UID you requested

You can also inspect the OpenPGP packets:

    gpg --list-packets /tmp/openpgp-created-identity.pgp

The packet order should be:

    public key
    user ID
    signature

---

# 11. Verify the self-signature

Use a temporary GnuPG directory so this test does not modify your normal GPG keyring.

Remove any old test directory:

    rm -rf /tmp/keycard-openpgp-gnupg

Create a clean one:

    mkdir -m 700 /tmp/keycard-openpgp-gnupg

Import the certificate:

    GNUPGHOME=/tmp/keycard-openpgp-gnupg \
      gpg --import /tmp/openpgp-created-identity.pgp

Show the fingerprint:

    GNUPGHOME=/tmp/keycard-openpgp-gnupg \
      gpg --list-keys --with-fingerprint

Verify the signatures:

    GNUPGHOME=/tmp/keycard-openpgp-gnupg \
      gpg --check-sigs

A successful result should include:

    [self-signature]

and:

    gpg: 1 good signature

That is the final end-to-end success condition.

---

# What just happened?

The computer created an identity request containing:

    UID
    +
    creation time

The request was encoded as `UR:BYTES` and displayed as a QR code.

The Shell scanned it.

The Shell then:

1. derived the OpenPGP key
2. created the public-key packet
3. created the requested User ID packet
4. created the identity certification signature
5. assembled the OpenPGP certificate
6. encoded the certificate as `UR:BYTES`
7. displayed the response as an animated QR

The computer then:

1. captured images of that animation
2. extracted BC-UR fragments from the QR images
3. reconstructed the OpenPGP certificate
4. checked the packet structure
5. gave the certificate to GnuPG
6. asked GnuPG to verify its self-signature

The complete path is:

    request generator
          ↓
      request QR
          ↓
     Keycard Shell
          ↓
    OpenPGP certificate
          ↓
    animated BC-UR QR
          ↓
     response decoder
          ↓
         GnuPG
          ↓
    good self-signature

---

# Troubleshooting

## No `UR:BYTES` fragments were captured

Check:

    wc -l /tmp/openpgp-response-frames-unique.txt

If it reports `0`, the QR was probably not clear enough in the images.

Try:

- holding the Shell more steadily
- reducing glare
- moving the Shell closer
- moving it slightly farther away
- keeping the display square to the camera
- making the QR occupy more of the image
- recording for longer

---

## ZBar reports something strange like `I2/5:172066`

That is a false barcode detection.

It is not part of the OpenPGP response.

Valid fragments begin with:

    UR:BYTES/

The extraction command in this README disables other barcode types and keeps only UR response fragments.

---

## `multipart UR response is incomplete`

This means you captured valid QR fragments, but not enough of them.

Capture the animation for longer.

For example, record 60 seconds instead of 30 seconds.

Then repeat:

1. frame extraction
2. QR extraction
3. response decoding

Because BC-UR uses fountain encoding, collecting additional valid frames gives the decoder more information with which to reconstruct the response.

---

## `RESPONSE STRUCTURE CHECK: PASS`, but GnuPG reports a bad signature

Save:

    /tmp/openpgp-created-identity.pgp

Also save:

- the decoder output
- the GnuPG output

A response QR appearing on the Shell does not by itself prove that the OpenPGP certificate is cryptographically valid.

The final expected result is:

    gpg: 1 good signature

---

# Known-good hardware result

The complete procedure has been tested on a physical Keycard Shell running beta3 using macOS and a MacBook camera.

The captured Shell response decoded as:

    Recovered certificate: 236 bytes
    Packet sequence:       6 -> 13 -> 2
    UID:                   Keycard Test <keycard@example.com>
    RESPONSE STRUCTURE CHECK: PASS

The certificate imported successfully into an isolated GnuPG keyring.

GnuPG independently reported:

    [self-signature]

and:

    gpg: 1 good signature

That is a complete OpenPGP identity end-to-end pass.

Linux and Windows instructions in this document are provided to make the test easier to reproduce on other platforms, but those platform-specific paths have not yet been validated by this project.
