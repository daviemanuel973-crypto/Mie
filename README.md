# Mie Survival

**Current stable release: v0.9.4**  
**Current stabilization branch: v0.9.5 (in progress)**

Mie Survival is the survival-focused evolution of the original ourCraft codebase. It keeps the voxel sandbox, multiplayer and rendering foundation while adding survival progression, world persistence, combat, base defence and native gameplay systems.

## v0.9.5 stabilization in progress

v0.9.5 continues the 0.9.x quality pass without adding gameplay content. The current branch focuses on deterministic input, lower input latency, camera correctness, focus/Alt-Tab recovery, controller state, collision correctness, crash prevention and low-end runtime consistency.

The first v0.9.5 batches already:

- make button release events edge-triggered;
- clear pending input state on reset/focus loss;
- clear disconnected controller state and correct left/right trigger mapping;
- accumulate wheel events within a frame;
- suppress gameplay input while the main window is unfocused;
- consume pending GLFW input on the first game/UI query of the frame instead of one frame later;
- harden camera yaw/pitch against non-finite or incorrectly wrapped values;
- add a permanent Input and Camera Regression gate.

See [`docs/V0.9.5_STABILITY.md`](docs/V0.9.5_STABILITY.md) for the current release gate and remaining passes.

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

For the full project history, build instructions, controls and older release notes, use the repository history and the documentation under [`docs/`](docs/).
