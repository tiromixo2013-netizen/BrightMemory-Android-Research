# Frame Generation Renderer Adapter Contract

This is the renderer-side capability gate for the two independent frame
extrapolation choices. It is intentionally separate from
`BMIUnifiedSpatialUpscaler`: frame generation is not an upscaling pass.

## Backend identifiers

- `0`: Off.
- `1`: Qualcomm AFME; only x2 may be registered.
- `2`: portable GLES motion-vector reprojection; x2/x3/x4 may be registered
  only when each output count has a complete implementation.

## Capability registration

A backend must call
`BMIFrameGeneration::SetBackendCapabilities(Backend, SupportedMultipliers)`
only after it has verified, for the active RHI and device:

1. Real previous/current scene-color resources and a valid motion-vector input.
2. Motion-vector direction, units, jitter convention and view rect.
3. A compiled renderer pass and valid RDG resource lifetime.
4. Disocclusion/invalid-vector handling and camera-cut history invalidation.
5. UI composition ordering and a presentation scheduler with the requested
   number of distinct generated output slots.

The gate is fail-closed. Merely compiling
`BMIGLESFrameReprojection.frag` must not register GLES capability. AFME is
restricted to x2. A backend must never advertise x3/x4 by repeating or blending
frames.

## Current implementation boundary

This file adds selection and capability gating only. No AFME adapter, GLES
RDG pass, motion-vector provider, frame queue or presentation scheduler is
registered by this commit. Both backends therefore remain unavailable by
default. A separate renderer integration against the exact UE4.27.2 game
source and an Android GLES build is still required.
