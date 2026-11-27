# Qualcomm Snapdragon Game Super Resolution 2 — UE4.27 source snapshot

Upstream repository: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine
Upstream branch: `engine/4.27`
Upstream directory: `Plugins/SGSR2/2pass_fs_taau`
Variant: two-pass fragment-shader temporal upscaler.
License: BSD-3-Clause; see `LICENSE`.

This snapshot preserves the upstream UE4.27 implementation for integration. SGSR2 is temporal, not spatial: it consumes scene color, depth, velocity/motion data, jitter and per-view temporal history. The upstream view extension installs an `ITemporalUpscaler` and requires the engine temporal-upscale path. It must not be routed through BMI's `ISpatialUpscaler` callback.

The source is vendored, but is not yet built against Bright Memory v1.34's exact engine snapshot. Keep the menu option gated until plugin compilation, shader compilation for the game's Android OpenGL ES target, temporal history lifecycle, motion-vector correctness and device image tests pass.
