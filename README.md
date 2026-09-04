# Mie Survival

**Current development version: v0.10.0 (content development)**

Mie Survival is the survival-focused evolution of the original ourCraft codebase. It keeps the voxel sandbox, multiplayer and rendering foundation while adding survival progression, world persistence, combat, base defence and native gameplay systems.

The repository started from the original ourCraft experiment, which was built around implementing Minecraft-like voxel technology from scratch, including transparent blocks, lighting, shadows and multiplayer.

Go check out the original development videos on [YouTube](https://www.youtube.com/watch?v=StNAG_tLEoU&list=PLKUl_fMWLdH-0H-tz0S144g5xXliHOIxC&index=4).

![image](https://github.com/user-attachments/assets/9f97b795-8f7e-4de0-abca-2945338721ca)

![image](https://github.com/user-attachments/assets/08b148c9-4c80-4cbc-83f1-c1ace1e61e0a)

![image](https://github.com/user-attachments/assets/d02a6717-8b47-4923-880d-1bc8e2574943)

![image](https://github.com/meemknight/ourCraft/assets/36445656/7e57cdc4-6f6c-4cc9-bce5-c8ff9131ab55)

![image](https://github.com/meemknight/ourCraft/assets/36445656/fd5ad17e-1bee-441d-8747-d4df4fdb850c)

![image](https://github.com/meemknight/ourCraft/assets/36445656/3f6c8976-8f63-4259-a1de-3305c4c52467)

## v0.10.0 subsistence expansion

v0.10.0 resumes content development on top of the published v0.9.7 stability
baseline. Its first playable slice expands food, exploration rewards and the
existing farming/cooking foundations without renumbering shipped content.

- Barns can contain carrots and igloos can contain potatoes.
- Carrots and potatoes are edible and supported by the persistent farm-plot model.
- The cooking pot prepares baked potatoes, vegetable stew and berry porridge.
- New item and recipe IDs are append-only; the recipe-discovery payload migrates from v0.9 automatically.
- Multiplayer protocol 4 rejects older clients before incompatible item and recipe registries are exchanged.

See [`docs/V0.10.0_CONTENT.md`](docs/V0.10.0_CONTENT.md).

## v0.9.4 gameplay and runtime stabilization

v0.9.4 adds no content. It concentrates on predictable movement, bounded server
work, hostile-network safety, durable saves and stable frame pacing on the
reference Intel HD 620 system.

- Jump input has a short buffer and coyote-time window, while collision flags are now idempotent.
- Block interaction uses voxel traversal instead of hundredths-of-a-block brute-force sampling.
- The server uses a bounded 20 Hz fixed timestep and recovers invalid transforms without terminating the session.
- AI navigation fields are reused and refreshed at a controlled rate instead of rebuilt for every player every tick.
- Entity and player updates are interest-filtered; generic entity updates are capped at 10 Hz.
- Every server-to-client payload has a size contract before parsing or decompression.
- World-difficulty and Field Guide messages now have distinct protocol identifiers.
- Entity sidecars are flushed durably before atomic promotion and chunks already queued for unload are not written twice.
- Low-end packages default to a stable 30 FPS at 1280x720; `MIE_FRAME_LIMIT=60` or `0` opt into 60 FPS or uncapped rendering.

See [`docs/V0.9.4_STABILIZATION.md`](docs/V0.9.4_STABILIZATION.md).

## v0.9.3.1 data-integrity hotfix

v0.9.3.1 is a corrective release for persistence and authoritative-world integrity. It adds no new content.

- Persisted entity sidecars are restored only for chunks whose terrain was actually restored from disk.
- Entity and block-data sidecars use transactional temp/backup writes; failed writes stay dirty and block chunk unload until persistence succeeds.
- Non-player entities are retained at the last loaded chunk boundary instead of disappearing when simulation outruns streaming.
- Player item drops are validated against the authoritative player position and consume inventory only after the server-side dropped entity is created.
- Entity IDs reserve persistent high-water ranges before reuse, with backup recovery and a high migration floor for legacy worlds.
- Placed-block collision checks now keep the target block position separate from the entity position.
- Save IDs, item/block IDs and multiplayer packet layouts remain unchanged from v0.9.3.

See [`docs/V0.9.3.1_DATA_INTEGRITY.md`](docs/V0.9.3.1_DATA_INTEGRITY.md).

## v0.9.3 authoritative gameplay stabilization

v0.9.3 validates block actions and positional item use against the authoritative
server state, rejects malformed packet payloads before reading them and restores
predicted client state after rejected mutations. New worlds can start in Survival
or Creative mode while legacy worlds continue as Survival and existing player saves
keep their persisted mode.

See [`docs/V0.9.3_STABILIZATION.md`](docs/V0.9.3_STABILIZATION.md).

## v0.9.2 gameplay & performance

v0.9.2 deliberately adds no new content. It focuses on making the existing game more robust, cheaper to simulate and more consistent to play.

- Chunk-coordinate and block-position hashes were replaced with properly distributed packed hashes instead of collapsing most coordinates into the same buckets.
- Region IDs now preserve signed X/Z coordinates correctly, including negative world quadrants.
- Server tick workers sleep while idle instead of continuously busy-waiting for work.
- The owner-side worker wait no longer burns a CPU core while a batch finishes and remains compatible with the existing chunk baker.
- Equipped item stats are aggregated through the authoritative player-stat path.
- Knockback resistance now affects server-authoritative melee knockback.
- Survival contract tests cover signed region IDs, hash distribution and knockback-resistance behavior.
- Save IDs, crafting recipe indexes and the multiplayer packet layout remain compatible with v0.9.1.

See [`docs/V0.9.2_GAMEPLAY_PERFORMANCE.md`](docs/V0.9.2_GAMEPLAY_PERFORMANCE.md).

## Reference low-end target

Official Windows and Flatpak builds use the low-end defaults and do not require
AVX2. The reference target is an Intel Core i5-7200U, Intel HD Graphics 620,
8 GB RAM and HDD at 1280x720. The Low profile prioritizes a stable 30 FPS while
keeping all gameplay systems enabled; visual effects can still be re-enabled.

See [`docs/LOW_END_PERFORMANCE.md`](docs/LOW_END_PERFORMANCE.md).

## v0.8.0 development

v0.8.0 starts from the validated v0.7.2 survival and packaging baseline. New gameplay work for this cycle remains on `agent/v0.8-development` until it is independently tested and approved for integration.

- New worlds offer Peaceful, Easy, Normal and Hard difficulty during creation.
- Hardcore locks the world to Hard and permanently disables character respawning.
- Difficulty is persisted per world, synchronized by the authoritative server and shown in the world list and pause/death menus.
- Incoming damage, starvation limits and siege wave size scale with difficulty.
- Peaceful worlds keep manual administrator sieges available but disable scheduled natural sieges.
- The persistent day/night cycle now drives a bounded natural-zombie budget at night, scaled by difficulty, living Survival players and world age.
- Natural night pressure pauses during siege events, while passive ecology becomes less frequent after sunset.
- Survival tools and melee weapons now have material-based durability, server-authoritative wear and breakage.
- Existing or newly crafted equipment without durability metadata starts at full durability; inventory cells and tooltips show the remaining condition.
- Furnaces now have dedicated input, fuel and output areas, persistent timed processing and server-authoritative multiplayer synchronization.
- All eight shipped furnace recipes remain compatible; charcoal, logs, planks and sticks provide bounded burn times, and blocked outputs do not waste fuel.
- Crafting packets now require the correct server-side station, and furnace output slots reject client-side insertion.
- The Field Guide now contains a paginated recipe book that permanently reveals recipes after all ingredient types have been found.
- Recipe discovery is synchronized with the inventory, persists in existing worlds through a backward-compatible extension and preserves all 103 crafting indexes.
- Existing worlds without difficulty metadata remain compatible and load as Normal, non-Hardcore worlds.

See [`docs/V0.8_DIFFICULTY_AND_HARDCORE.md`](docs/V0.8_DIFFICULTY_AND_HARDCORE.md)
[`docs/V0.8_DAY_NIGHT_PRESSURE.md`](docs/V0.8_DAY_NIGHT_PRESSURE.md) and
[`docs/V0.8_TOOL_DURABILITY.md`](docs/V0.8_TOOL_DURABILITY.md) and
[`docs/V0.8_FURNACE_PROCESSING.md`](docs/V0.8_FURNACE_PROCESSING.md) and
[`docs/V0.8_RECIPE_DISCOVERY.md`](docs/V0.8_RECIPE_DISCOVERY.md).

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
