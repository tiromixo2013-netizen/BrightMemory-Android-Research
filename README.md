# Bright Memory: Infinite — Android Graphics Research

This repository is the research scaffold for investigating graphics settings and Android rendering compatibility.

## Target settings (research/specification only)

The items below are desired controls, **not claims that the game currently supports them**. Each control must be verified against the actual APK, engine version, renderer, and device before implementation.

1. Render Scale
2. Upscaling Method: SGSR1, SGSR2, MWSR, FSR Lite, Nearest, Bilinear
3. Upscale Sharpness
4. Unified Upscale Preset: Ultra Performance, Performance, Balanced, Quality, Ultra Quality (only expose presets supported by the selected method)
5. AMD CAS: On/Off + CAS Sharpness
6. Low Latency: On/Off + queue mode (only if the render path exposes a safe control)
7. Frame Pacing
8. FPS Limit: 30 / 60 / 90 / 120 / 144 / 180 (device/game-supported values only)
9. Frame Generation: x2 / x3 / x4 (only if a real compatible implementation can be integrated; never a visual placeholder)
10. Performance HUD: FPS, frame time, 1% low, CPU/GPU when measurable

## Device under test

Samsung Galaxy M14 4G, Android 15, ARM64, Adreno 610.

## Next steps

- Add a legally obtained APK/XAPK to a private local test workspace; do not commit copyrighted game packages.
- Record package ID, version, ABI, engine, graphics API (Vulkan/OpenGL ES), and APK hashes.
- Inspect existing settings and rendering hooks before choosing an implementation.
- Keep unsupported controls hidden rather than presenting fake switches.

## Safety and distribution

This repository contains research notes and configuration scaffolding only. It does not distribute the game, modified game packages, or copyrighted assets.
