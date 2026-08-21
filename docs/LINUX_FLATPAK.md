# Mie Survival on Linux (Flatpak)

This branch packages Mie Survival v0.9.3.1 as a persistent Linux desktop application.

## App identity

- Flatpak ID: `io.github.daviemanuel973.Mie`
- Runtime: `org.freedesktop.Platform//25.08`
- SDK: `org.freedesktop.Sdk//25.08`
- Architecture currently validated by CI: `x86_64`

The Flatpak build disables the project's global AVX2 requirement to improve compatibility with older x86_64 Linux CPUs. Windows/developer builds keep the previous default unless `OURCRAFT_ENABLE_AVX2=OFF` is supplied.

The bundled GLFW 3.3.7 build currently uses its X11 backend (`GLFW_USE_WAYLAND=OFF`), so the Flatpak exposes X11/XWayland rather than claiming native Wayland support. This works directly on X11 desktops such as Linux Mint XFCE and through XWayland on most Wayland desktops.

## Persistent game data

The Flatpak launcher changes the working directory to the application's persistent XDG data directory before starting the game. On a normal desktop this maps to approximately:

`~/.var/app/io.github.daviemanuel973.Mie/data/ourcraft/`

Worlds, player settings and other relative files therefore survive normal Flatpak updates.

## Build and install locally

Install Flatpak and flatpak-builder using your distribution package manager, then configure Flathub and install the runtime/SDK:

```bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08
```

From the repository root:

```bash
flatpak-builder --user --force-clean --install --install-deps-from=flathub \
  build-flatpak flatpak/io.github.daviemanuel973.Mie.yml
flatpak run io.github.daviemanuel973.Mie
```

## CI bundle

`.github/workflows/linux-flatpak-ci.yml` builds the manifest on Ubuntu, exports a Flatpak repository, creates `Mie-Survival-Linux-x86_64.flatpak`, installs it back into the runner and checks its Flatpak metadata. The resulting bundle is uploaded as a GitHub Actions artifact.

A standalone `.flatpak` bundle can be installed with:

```bash
flatpak install --user ./Mie-Survival-Linux-x86_64.flatpak
```

A standalone bundle is useful for testing, but a hosted Flatpak repository is preferred for automatic/update-manager updates.

## Updateable repository

`.github/workflows/publish-flatpak-pages.yml` builds a signed OSTree Flatpak repository and publishes it through GitHub Pages. It also creates:

- `Mie-Survival.flatpakref` for one-click/install-by-URL setup;
- `Mie-Survival.flatpakrepo` for explicitly adding the update repository;
- `Mie-Survival-Linux-x86_64.flatpak` as a direct-download fallback.

The intended public repository URL is:

`https://daviemanuel973-crypto.github.io/Mie/repo/`

Once published, the install URL is:

`https://daviemanuel973-crypto.github.io/Mie/Mie-Survival.flatpakref`

Users installed from that `.flatpakref` receive new repository builds through their normal Flatpak update flow. Command-line users can update explicitly with:

```bash
flatpak update io.github.daviemanuel973.Mie
```

## One-time publisher setup

GitHub Pages must use **GitHub Actions** as the publishing source for this repository.

The update repository is signed. Generate a dedicated key on a trusted Linux machine:

```bash
chmod +x flatpak/generate-signing-key.sh
./flatpak/generate-signing-key.sh
```

The helper writes the private material outside the repository by default under `~/mie-flatpak-signing/`.

Create these repository Actions secrets:

- `FLATPAK_GPG_PRIVATE_KEY_B64`: contents of `flatpak-private-key.b64`
- `FLATPAK_GPG_KEY_ID`: contents of `key-id.txt`

Never commit the private key.

After Pages and the two secrets are configured, run **Publish Flatpak Repository** manually or push a tag matching `linux-v*` (for example `linux-v0.9.3.1`).

## Sandbox permissions

The manifest grants only the game-facing permissions currently required:

- GPU/DRI for OpenGL rendering;
- X11/XWayland for the game window;
- PulseAudio for sound;
- network access for multiplayer;
- IPC for graphics/window-system interoperability.

The game does not receive general host filesystem access. Saves remain inside the Flatpak application data directory.
