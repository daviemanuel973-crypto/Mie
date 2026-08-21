# Low-end performance profile

Target reference hardware:

- Intel Core i5-7200U (2C/4T)
- Intel HD Graphics 620
- 8 GB RAM
- HDD
- Windows x64 or Linux x86_64 / Flatpak

The profile targets stable 30 FPS first and up to 60 FPS at 1280x720 when GPU/CPU headroom permits. FPS is an engineering target until benchmarked on the reference machine.

Official Windows and Flatpak release builds enable `OURCRAFT_LOW_END_BUILD` and
disable the global AVX2 requirement. Faster computers can re-enable expensive
effects from the rendering settings without requiring a separate package.

## Changes

- Default view distance reduced from 15 to 5 (chunk matrix from 30x30 to 10x10).
- Aggressive terrain LOD (strength 5).
- One extra chunk-baking worker to leave CPU time for the game/server thread.
- Cheap water, shadows off, PBR off, bloom off, SSR off, HBAO/SSAO off.
- Point lights limited from 40 to 8.
- Lower-cost FXAA.
- Bloom buffers are not allocated while bloom is disabled.
- HBAO buffers are not resized/allocated while SSAO is disabled.
- Automatic exposure GPU mipmap/readback is skipped in the low-end build.
- Adaptive 60/30 FPS frame pacing with a 15 FPS background cap.
- Linux automatically retries via Mesa Zink when native OpenGL lacks ARB_bindless_texture (important for Intel Mesa).
- One-click Performance preset remains available in Rendering settings.

All gameplay/survival systems remain enabled. Expensive effects can still be re-enabled manually.
