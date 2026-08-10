#!/bin/sh
set -eu

# Flatpak maps XDG_DATA_HOME to ~/.var/app/<app-id>/data.
# Engine assets stay read-only under /app; only user-editable content lives here.
DATA_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/ourcraft"
CONTENT_ROOT="$DATA_ROOT/resources"
mkdir -p "$CONTENT_ROOT/worlds" "$DATA_ROOT/playerSettings"

for name in texturePacks skins; do
    src="/app/share/ourcraft/resources/$name"
    dst="$CONTENT_ROOT/$name"
    if [ -d "$src" ]; then
        mkdir -p "$dst"
        cp -a -n "$src/." "$dst/"
    fi
done

cd "$DATA_ROOT"

# CI can validate writable-data initialization without launching OpenGL.
if [ "${1:-}" = "--prepare-data-only" ]; then
    exit 0
fi

echo "Mie: Flatpak texture-array compatibility renderer enabled" >&2

exec /app/bin/ourcraft "$@"
