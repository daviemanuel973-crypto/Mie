# ourCraft very_pre_alpha2 — non-destructive improvement audit

Baseline reviewed: upstream tag `very_pre_alpha2` (`568fa30bee158c9309cf5dbf8b4690cc5a844bce`).

The `main` branch is intentionally kept as the imported baseline. Changes should be developed in branches and merged only after validation so existing features and assets are never silently removed.

## Current strengths

- C++17 / CMake project with bundled dependencies.
- Chunk system and procedural world generation.
- Multiplayer networking via ENet with server-side validation and undo/rubber-banding logic.
- Entity/gameplay systems including AI, enemies, crafting, loot, food, inventory, block data and combat.
- Rendering stack already includes PBR, HDR/ACES, bloom, SSAO/HBAO, SSR, lighting, shadows, fog and underwater rendering.
- Audio system with per-material block sounds and music.
- Existing profiling, serialization and save-related infrastructure.

## Verified issue: B001 — footsteps while blocked by a wall

`Known Bugs I've Seen` documents:

> B001 = Stepping Sounds keep playing when attempting to move through a wall

In `src/gameLayer/gamePlayLogic.cpp`, the footstep code checks `isPlayerMovingSpeed`, but the existing `player.entity.forces.colidesBottom()` condition is commented out. The collision API already exposes `colidesBottom()`.

Low-risk correction: require both grounded collision and movement before starting/updating the footstep timer. This prevents footsteps while airborne and avoids the documented wall case where input/motion state can remain non-zero without valid walking contact.

## Build / portability findings

A clean Linux CMake configure reaches the bundled GLFW configuration, then stops because the environment is missing the RandR development headers (`libxrandr-dev`). This is an environment dependency, not evidence of lost source code.

Recommended next build step is a CI workflow on Ubuntu that installs the GLFW/X11 development prerequisites before running CMake. Windows should also get a separate MSVC job because the project has Windows-specific paths and APIs.

## High-priority improvements that preserve existing behavior

1. **Establish CI before refactors**
   - Windows/MSVC configure + build.
   - Linux/GCC configure + build with required X11/GL packages.
   - Keep the current executable behavior unchanged; CI only detects regressions.

2. **Fix documented bugs one at a time**
   - B001 footsteps / grounded state.
   - Investigate B002 glass break sound with a reproducible input sequence before changing audio routing.

3. **Make build options explicit instead of deleting features**
   - Add CMake options for AVX2, internal assertions, developer tools and release resource paths.
   - Preserve current defaults initially.
   - AVX2 is currently forced globally, which unnecessarily excludes older CPUs.

4. **Improve platform isolation**
   - Move Windows-only file-dialog code out of gameplay logic into the platform layer.
   - Keep the current Windows implementation and add a Linux implementation/fallback rather than removing the feature.

5. **Harden networking incrementally**
   - Bound packet decompression/allocation sizes before allocating from network-provided lengths.
   - Save server state/entity IDs and player data as already noted by project TODOs.
   - Consolidate validation messages only after compatibility tests.

6. **Resource/runtime safety**
   - Replace raw temporary arrays in shader/model/packet loading with RAII containers where behavior is equivalent.
   - Audit OpenGL resident texture handles and cleanup paths at shutdown.
   - Keep file formats and resource paths backward compatible.

7. **Performance without content loss**
   - Avoid rebaking transparent geometry when a chunk contains no transparent blocks (already suggested in project TODOs).
   - Change view distance incrementally instead of resetting all chunks.
   - Add measurements around chunk generation/baking before changing algorithms.

8. **Repository hygiene without removing original assets**
   - Keep the imported baseline intact.
   - Put generated saves/worlds/build output under ignored paths.
   - Add a third-party/license inventory before redistribution, especially for bundled libraries, music, textures and models.
   - Consider Git LFS for large authoring files (`.psd`, large binaries) only in a future migration; do not rewrite baseline history merely to enable it.

## Larger changes to postpone until CI is green

- Skybox/day-night/fog/reflection refactor.
- Cascaded shadow maps and God rays.
- Multiplayer buffering/message unification.
- Entity visibility changes across unloaded chunks.
- World-generation river/biome tuning.
- Broad renderer shader unification.

These can improve the project substantially, but they touch systems with high regression potential. They should follow reproducible builds and small automated/runtime checks.

## Preservation policy

- `main` remains the exact imported baseline plus repository bootstrap metadata until a reviewed change is deliberately merged.
- One functional change per PR where practical.
- Never delete an existing system merely because a replacement is introduced; keep backward-compatible paths until the replacement is validated.
- Preserve worlds, save formats, resource identifiers and network packet compatibility unless a migration path is included.
