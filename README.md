# Mie Survival

**Current development version: v0.8.0**

Mie Survival is the survival-focused evolution of the original ourCraft codebase. It keeps the voxel sandbox, multiplayer and rendering foundation while adding survival progression, world persistence, combat, base defence and native gameplay systems.

The repository started from the original ourCraft experiment, which was built around implementing Minecraft-like voxel technology from scratch, including transparent blocks, lighting, shadows and multiplayer.

Go check out the original development videos on [YouTube](https://www.youtube.com/watch?v=StNAG_tLEoU&list=PLKUl_fMWLdH-0H-tz0S144g5xXliHOIxC&index=4).

![image](https://github.com/user-attachments/assets/9f97b795-8f7e-4de0-abca-2945338721ca)

![image](https://github.com/user-attachments/assets/08b148c9-4c80-4cbc-83f1-c1ace1e61e0a)

![image](https://github.com/user-attachments/assets/d02a6717-8b47-4923-880d-1bc8e2574943)

![image](https://github.com/meemknight/ourCraft/assets/36445656/7e57cdc4-6f6c-4cc9-bce5-c8ff9131ab55)

![image](https://github.com/meemknight/ourCraft/assets/36445656/fd5ad17e-1bee-441d-8747-d4df4fdb850c)

![image](https://github.com/meemknight/ourCraft/assets/36445656/3f6c8976-8f63-4259-a1de-3305c4c52467)

## v0.8 development

v0.8 starts from the validated v0.7.2 survival and packaging baseline. New gameplay work for this cycle belongs on `agent/v0.8-development` until it is independently tested and approved for integration.

## v0.7.2 stabilization

v0.7.2 preserves the v0.7 survival progression and concentrates on correcting the systems recovered in v0.7.1.

- Peace-time ecology spawns pigs, cats and neutral common goblins instead of surprise zombies.
- Scheduled sieges begin on every seventh night, contain three waves and remove only surviving wave enemies at the following sunrise.
- Manual spawn eggs and the administrator siege command remain independent from scheduled cleanup.
- Respawn is server-authoritative and searches for solid ground with two clear blocks.
- The Field Guide is reachable from the inventory and opens directly when used.
- Cobwebs slow players, creatures and dropped items; fragile containers break quickly.
- Entity persistence reserves restored IDs, and release packages explicitly exclude saved worlds and player settings.

## v0.7.1 recovery and survival progression

v0.7.1 is the recovery baseline. This release reconciled the source tree with systems that were present in the shipped v0.7 Windows executable and hardened the game after the reported runtime crash.

- Package, installer and runtime version are `0.7.1`.
- v0.7 item IDs are preserved for existing saves: Field Guide, charcoal, cassiterite concentrate, tin, bronze and bronze tools/weapons.
- Player persistence carries the Field Guide progression state while retaining legacy-save compatibility.
- The server grants the starter Field Guide, tracks one-time objectives/rewards and synchronizes the 8-byte guide progress state to clients using the v0.7 protocol contract.
- Native villager-society state is persistent and server-authoritative; visible/interactable villager NPC work remains a separate gameplay milestone.
- Windows builds install crash diagnostics that write a readable crash report and minidump for future native crashes.
- Windows Release CI and Linux Flatpak CI validate the recovery branch before it reaches `main`.

## Survival + Linux package

The Mie branch adds the Survival v0.1 gameplay loop (hunger, food, crafting progression, armour and survival-first spawn) and includes an installable Linux Flatpak target.

- Flatpak app ID: `io.github.daviemanuel973.Mie`
- Runtime: `org.freedesktop.Platform//25.08`
- CI creates an installable `Mie-Survival-Linux-x86_64.flatpak` bundle.
- A signed Flatpak repository workflow is included for normal `flatpak update` delivery.
- Linux saves live in the Flatpak persistent application-data area and survive package updates.

See [`docs/SURVIVAL_MODE.md`](docs/SURVIVAL_MODE.md) and [`docs/LINUX_FLATPAK.md`](docs/LINUX_FLATPAK.md).

## v0.4 world/runtime update

The v0.4 Windows work added a reliable saved-world selector, water swimming/buoyancy, the original natural hostile spawning, cleaner small-rock terrain generation, console-free release startup and a checksum-verified background updater. v0.7.2 supersedes that peace-time spawn policy with pigs, cats and neutral common goblins. Existing v0.3 worlds remain compatible, and terrain changes apply only to newly generated chunks.

See [`docs/V0.4_WORLD_AND_RUNTIME.md`](docs/V0.4_WORLD_AND_RUNTIME.md) and [`docs/WINDOWS_UPDATES.md`](docs/WINDOWS_UPDATES.md).

## v0.5 base defence

The v0.5 work added a server-authoritative siege loop with a preparation warning, three scaling waves, reinforced barricades, wooden spike traps and a compact HUD. Raid enemies deliberately lock onto survival players, damage nearby wooden defences and remain capped for Low-preset hardware. Use `/siege start` as an administrator to begin the warning immediately during a playtest.

See [`docs/V0.5_BASE_DEFENCE.md`](docs/V0.5_BASE_DEFENCE.md).

## v0.6 native systems foundation

The v0.6 foundation froze v0.5 content/save contracts and added stable content IDs, atomic world-schema migrations, budgeted gameplay scheduling, dirty-state replication, multiplayer interest, headless metrics and a persistent server-authoritative prototype machine. Player traversal added optional automatic jumping/running, faster ladder and vine climbing, and `F5` cycling among first person, third person back and third person front.

Builds from that milestone identify as `0.6.0`. See [`docs/V0.6_NATIVE_FOUNDATION.md`](docs/V0.6_NATIVE_FOUNDATION.md) and the [`v0.6 execution plan`](docs/ideias/sistemas-nativos/07-plano-v0.6.md).

## Features and todos

- [ ] Rendering system
  - [ ] Shaders:
    - [x] Animated water
    - [x] No visual artifacts on textures
    - [x] PBR pipeline
    - [x] Lights
    - [ ] Lights stored in cube maps
    - [ ] Sky Box reflection
    - [x] SSR
    - [x] HBAO / SSAO
    - [x] HDR, ACES tone mapping
    - [x] Bloom
    - [x] Automatic exposure
    - [x] Lens flare
    - [x] Color grading
    - [x] Fog (todo improve)
    - [x] Underwater fog (todo improve)
    - [ ] God rays
    - [x] Fake Shadows for all light types (todo improve)
  - [x] Shadows (todo optimize)
    - [ ] Cascaded shadow maps
    - [ ] Depth of field (maybe blur far stuff)
  - [ ] Use the same shader for all things in game
- [x] Chunk system
- [ ] Multi player
  - [x] Connection to server and handshake
  - [x] Server can validate moves
  - [x] Server knows player position to optimize chunk logic stuff
  - [x] Undo Stuff On client
  - [ ] Buffering
  - [x] Rubber banding
  - [x] Entities
