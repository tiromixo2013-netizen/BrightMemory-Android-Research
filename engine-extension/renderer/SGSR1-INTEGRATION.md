# Qualcomm SGSR1 integration notes (UE4.27)

## Confirmed upstream implementation

Reference repository: `SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine`, branch `engine/4.27`.

The official plugin's SGSR1 implementation is already an `ISpatialUpscaler` implementation, not a generic post-process effect:

- `Plugins/SGSR/Source/SGSR/Private/SGSR.cpp` creates `FSGSRViewExtension`.
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRViewExtension.cpp` checks `r.Qualcomm.SGSR.Enabled` and installs `FSGSRSpatialUpscaler` as the primary spatial upscaler when temporal upscaling is not requested.
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.cpp` builds the official SGSR subpasses and calls the SGSR upscale pass.
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/Subpasses/SGSRSubpassScaler.cpp` dispatches the official pixel shader through RDG.
- `Plugins/SGSR/Shaders/Private/PostProcessSGSR.usf` includes the SGSR shader implementation.

This means Bright Memory should use the official plugin's primary spatial-upscaler path. The BMI experimental spatial dispatcher must remain disabled when SGSR1 is selected; trying to install a second primary upscaler would conflict with the official view extension.

## Runtime sharpness finding and patch

The official UE4.27 mobile shader hardcodes `SGSR_EDGE_SHARPNESS` to `2.0`. The upstream `FSGSRPassParameters` does not expose a runtime sharpness parameter. A CVar-only implementation in the game runtime would therefore do nothing.

`patches/0002-sgsr1-runtime-sharpness.patch` changes the official plugin source in three places:

1. Adds `EdgeSharpness` to the RDG shader parameter struct.
2. Replaces the fixed mobile shader macro with a shader parameter.
3. Adds `r.Qualcomm.SGSR.Sharpness` and binds its clamped value to the SGSR1 pixel shader pass.

The game runtime maps the visible 0..1 sharpness slider to the official shader's 1..2 range. The shared five-level preset applies one Render Scale percentage and a recommended SGSR1 edge-sharpness value; it does not add another upscaler selector.

## Apply and validate

From the root of the official Qualcomm plugin checkout at branch `engine/4.27`:

```sh
git apply --check /path/to/0002-sgsr1-runtime-sharpness.patch
git apply /path/to/0002-sgsr1-runtime-sharpness.patch
```

The repository workflow `.github/workflows/reference-renderer-patch-check.yml` runs the patch applicability check against that upstream branch.

A clean `git apply --check` proves only that the patch matches the public upstream source. It does not prove that Bright Memory v1.34 ships the same plugin revision, that its UE4.27.2 source is ABI-compatible, that Android GLES shaders compile, or that the pass runs on the M14. Those require the exact target engine source, a UE Android build, and an in-game A/B test.

## Current limitations

- The exact engine/plugin snapshot used to build Bright Memory v1.34 is not established.
- The upstream plugin's `SGSR_TARGET` default is Mobile (0); UE4's half-precision path is explicitly unsupported and must remain off.
- The game's original UI widget is still cooked and has not been rebound to the runtime Blueprint functions.
- AMD CAS remains unavailable until a separate real RDG pass is registered after SGSR1.
- Frame-generation backends remain fail-closed.
