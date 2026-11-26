# Mobile Upscaler Dispatch — Static Analysis Notes

Target binary: Bright Memory: Infinite Android v1.34, ARM64
Library: `libUE4.so`
Build ID: `09ae6051d345ede937255d6bd76abefbf6d2b7e8`
SHA-256: `94ee451ed42c4aee4c885aac3e6b6ccef2d3ab109e01a86b10e527c32c82b35e`

## Confirmed code locations

- `AddMobilePostProcessingPasses`: `0x78870c0`
- `ISpatialUpscaler::AddDefaultUpscalePass`: `0x78fae30`
- `FSceneView::FSceneView(FSceneViewInitOptions const&)`: `0x8b35598`
- `FSceneRenderer::ApplyResolutionFraction`: `0x79e5ba0`
- `FVulkanSwapChain::RenderThreadPacing`: `0x626acb8`
- `FVulkanSwapChain::Present`: `0x626aea8`

## Dispatch observations

In `AddMobilePostProcessingPasses`, the relevant sequence is:

- `x19` holds the `FViewInfo const&` argument.
- `ldr x10, [x19]` loads the first pointer stored in the view object.
- `ldr x0, [x10, #0xc0]` loads a pointer from offset `0xc0` of that pointed-to object.
- If non-null, the code loads that returned object's vtable and calls the function at vtable offset `0x20`.

This means `+0xc0` is not itself proven to be a function pointer or a direct hook. It is a field read from the object referenced by the first pointer in `FViewInfo`; the returned object is then used as a polymorphic upscaler-like interface.

In `FSceneView::FSceneView`, the constructor copies `[FSceneViewInitOptions + 0xb0]` into the first pointer-sized field of the new `FSceneView`. This confirms the origin of the first pointer in the view object, but does not yet prove the concrete runtime type of that pointer or identify who writes the `+0xc0` field.

## Default fallback

When the loaded pointer is null, `AddMobilePostProcessingPasses` calls `ISpatialUpscaler::AddDefaultUpscalePass` at `0x78fae30`.

The default function records RDG passes and returns a screen-pass texture result. Any custom implementation must preserve RDG resource lifetime and the output structure; do not return a texture whose lifetime escapes the graph.

## Not yet proven

- The concrete type behind `FSceneViewInitOptions + 0xb0`.
- The writer/setter for the `+0xc0` field.
- The full virtual-call ABI and all parameter semantics.
- The exact `EUpscaleMethod` enum mapping.
- Whether a safe runtime replacement can be made without modifying other view-state behavior.

## Next investigation

Trace construction and assignment of the view-state interface pointer, then find the writer of its `+0xc0` field. Only after those are established should an implementation or binary patch be designed.
