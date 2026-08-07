#!/bin/sh
set -eu

OUT_DIR="${1:-$HOME/mie-flatpak-signing}"
UID_TEXT="Mie Survival Flatpak Repository <flatpak@mie.local>"
mkdir -p "$OUT_DIR"
chmod 700 "$OUT_DIR"

if gpg --batch --list-secret-keys "$UID_TEXT" >/dev/null 2>&1; then
  echo "A signing key for Mie Survival already exists; reusing it."
else
  gpg --batch --passphrase '' --quick-gen-key "$UID_TEXT" rsa3072 sign 0
fi

KEY_ID="$(gpg --batch --with-colons --list-secret-keys "$UID_TEXT" | awk -F: '/^sec:/ {print $5; exit}')"
test -n "$KEY_ID"

gpg --batch --export "$KEY_ID" > "$OUT_DIR/flatpak-public-key.gpg"
gpg --batch --armor --export-secret-key "$KEY_ID" | base64 --wrap=0 > "$OUT_DIR/flatpak-private-key.b64"
printf '%s\n' "$KEY_ID" > "$OUT_DIR/key-id.txt"
chmod 600 "$OUT_DIR/flatpak-private-key.b64"

echo "Generated/reused Flatpak signing key: $KEY_ID"
echo "Private CI value: $OUT_DIR/flatpak-private-key.b64"
echo "Public key:       $OUT_DIR/flatpak-public-key.gpg"
echo "Key ID:           $OUT_DIR/key-id.txt"
echo "Never commit flatpak-private-key.b64 to the repository."
