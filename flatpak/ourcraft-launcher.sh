#!/bin/sh
set -eu

# Flatpak maps these XDG directories to ~/.var/app/<app-id>/, so worlds,
# settings and other relative game files survive updates and reinstalls.
DATA_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/ourcraft"
mkdir -p "$DATA_ROOT"
cd "$DATA_ROOT"

exec /app/bin/ourcraft "$@"
