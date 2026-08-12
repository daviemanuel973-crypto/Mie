# ourCraft

It is the third time I have tried to make Minecraft from scratch.
This time I want to add a lot of harder-to-implement features like transparent blocks, light shadows, and multi-player!

Go check out the videos on [YouTube about it](https://www.youtube.com/watch?v=StNAG_tLEoU&list=PLKUl_fMWLdH-0H-tz0S144g5xXliHOIxC&index=4)!


![image](https://github.com/user-attachments/assets/9f97b795-8f7e-4de0-abca-2945338721ca)

![image](https://github.com/user-attachments/assets/08b148c9-4c80-4cbc-83f1-c1ace1e61e0a)

![image](https://github.com/user-attachments/assets/d02a6717-8b47-4923-880d-1bc8e2574943)

![image](https://github.com/meemknight/ourCraft/assets/36445656/7e57cdc4-6f6c-4cc9-bce5-c8ff9131ab55)

![image](https://github.com/meemknight/ourCraft/assets/36445656/fd5ad17e-1bee-441d-8747-d4df4fdb850c)

![image](https://github.com/meemknight/ourCraft/assets/36445656/3f6c8976-8f63-4259-a1de-3305c4c52467)

## Survival + Linux package

The Mie branch adds the Survival v0.1 gameplay loop (hunger, food, crafting progression, armour and survival-first spawn) and now includes an installable Linux Flatpak target.

- Flatpak app ID: `io.github.daviemanuel973.Mie`
- Runtime: `org.freedesktop.Platform//25.08`
- CI creates an installable `Mie-Survival-Linux-x86_64.flatpak` bundle.
- A signed Flatpak repository workflow is included for normal `flatpak update` delivery.
- Linux saves live in the Flatpak persistent application-data area and survive package updates.

See [`docs/SURVIVAL_MODE.md`](docs/SURVIVAL_MODE.md) and [`docs/LINUX_FLATPAK.md`](docs/LINUX_FLATPAK.md).

## v0.4 world/runtime update

The current Windows work adds a reliable saved-world selector, water swimming/buoyancy, natural zombie and goblin spawning, cleaner small-rock terrain generation, console-free release startup and a checksum-verified background updater. Existing v0.3 worlds remain compatible, and terrain changes apply only to newly generated chunks.

See [`docs/V0.4_WORLD_AND_RUNTIME.md`](docs/V0.4_WORLD_AND_RUNTIME.md) and [`docs/WINDOWS_UPDATES.md`](docs/WINDOWS_UPDATES.md).

## v0.5 base defence

The v0.5 work adds a server-authoritative siege loop with a preparation warning,
three scaling waves, reinforced barricades, wooden spike traps and a compact HUD.
Raid enemies deliberately lock onto survival players, damage nearby wooden
defences and remain capped for Low-preset hardware. Use `/siege start` as an
administrator to begin the warning immediately during a playtest.

See [`docs/V0.5_BASE_DEFENCE.md`](docs/V0.5_BASE_DEFENCE.md).

Features and todos:

- [ ] Rendering system
  - [ ] Shaders:
  	- [x] Animated nice water 😻
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
  	- [x] Fog -(todo improve)
  	- [x] Underwater fog -(todo improve)
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
