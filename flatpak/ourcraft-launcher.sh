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

# The renderer relies on GL_ARB_bindless_texture for block/entity texture handles.
# Mesa's native Intel OpenGL path can expose the required GL version without a
# reliable bindless implementation on older Intel hardware. Zink provides the
# OpenGL API over the Vulkan driver and is the preferred Flatpak compatibility
# path. Respect an explicit user override for debugging/other GPUs.
if [ -z "${MESA_LOADER_DRIVER_OVERRIDE:-}" ]; then
    export MESA_LOADER_DRIVER_OVERRIDE=zink
fi

# Keep Zink's descriptor implementation automatic so Mesa can choose the most
# appropriate backend for the installed Vulkan driver.
export ZINK_DESCRIPTORS="${ZINK_DESCRIPTORS:-auto}"

echo "Mie: OpenGL backend override=${MESA_LOADER_DRIVER_OVERRIDE}, ZINK_DESCRIPTORS=${ZINK_DESCRIPTORS}" >&2

exec /app/bin/ourcraft "$@"
