# Windows launcher and verified updates

Windows packages include two GUI-subsystem executables:

- `MieLauncher.exe` is the shortcut target. It starts the game immediately and then checks for an update without opening a terminal or updater window.
- `Mie.exe` is the game runtime. Installable builds disable the internal console and profiler overlay by default.

The launcher checks the repository's latest GitHub Release at most once every six hours. A newer installer is accepted only when the release also contains
`Mie-Survival-Windows-x64-Setup.exe.sha256` and the downloaded installer's SHA-256 matches it.

Verified updates are staged under the installation's `updates` directory. On the next launch, a temporary updater applies the installer silently and starts the new launcher. The stable Inno Setup `AppId` upgrades the existing installation in place. Files in `resources/worlds` and `playerSettings` are not removed by update installs.

The first package containing this launcher must still be installed once over an older package that did not contain updater code. After that bootstrap update, later tagged releases are discovered automatically.

## Publishing

Push a supported release tag such as `v0.9.3.1`. Three- and four-component
versions are understood by both the launcher and release workflows. The
`Publish Windows Release` workflow builds and tests the game, creates the
portable ZIP and installer, calculates SHA-256 checksums for both packages and
publishes the assets to a GitHub Release. The updater ignores releases that do
not contain the expected installer/checksum pair.
