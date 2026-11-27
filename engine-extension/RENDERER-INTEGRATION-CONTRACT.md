# UE4.27 renderer integration contract (source-verified)

Reference source: `jsuk10/UE4.27plus_Custom`, branch `main`.

Verified source files:

- `Engine/Source/Runtime/Engine/Public/SceneViewExtension.h`
- `Engine/Source/Runtime/Engine/Public/SceneView.h`
- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessUpscale.h`
- `Engine/Source/Runtime/Renderer/Private/ScreenPass.h`

## Confirmed extension lifecycle

`FSceneViewExtensionBase` is the supported base class. It is constructed through
`FSceneViewExtensions::NewExtension<T>(...)`, which supplies `FAutoRegister`
and registers the shared extension. The extension interface requires
`SetupViewFamily`, `SetupView`, `BeginRenderViewFamily`,
`PreRenderViewFamily_RenderThread`, and `PreRenderView_RenderThread`.
`PrePostProcessPass_RenderThread` and
`SubscribeToPostProcessingPass` are available hooks for RDG/post-process work.

## Confirmed spatial-upscaler contract

The reference renderer declares `ISpatialUpscaler` in the *private* header
`Renderer/Private/PostProcess/PostProcessUpscale.h`. Its contract is:

- `GetDebugName() const`
- `Fork_GameThread(const FSceneViewFamily&) const`
- `AddPasses(FRDGBuilder&, const FViewInfo&, const FInputs&) const`

`FInputs` carries `OverrideOutput`, required `SceneColor`, and `EUpscaleStage`.
The view family exposes `SetPrimarySpatialUpscalerInterface(ISpatialUpscaler*)`
and checks that the primary slot was not already assigned. The documented
integration point is `ISceneViewExtension::BeginRenderViewFamily`.

The private header is a material integration constraint: a normal third-party
plugin depending only on public Engine headers cannot safely include the renderer
implementation contract as-is. The production implementation must either live
inside the matching Renderer module, or the engine fork must add a deliberate
public/exported renderer interface and build dependency. Do not paper over this
with guessed include paths, copied private declarations, or ABI/vtable hooks.

## RDG screen-pass contract

The renderer's `FScreenPassTexture` is a texture plus view rectangle.
`AddDrawScreenPass` records a raster pass through `FRDGBuilder::AddPass`.
A custom spatial algorithm must return a valid output texture/view rect and
respect the input/output rectangles and render-graph lifetime rules.

## Consequences for this project

1. Keep the already verified `UGameUserSettings` controls separate from the renderer patch.
2. Do not claim SGSR1/2, MWSR, FSR Lite, CAS, or custom nearest/bilinear shaders are implemented merely because the extension seam exists.
3. Before coding the first shader pass, settle the module placement/public API decision above and confirm the exact Bright Memory engine snapshot matches this reference fork.
4. Compile the Renderer/engine target and test the Android Vulkan path before exposing any upscaler option.
5. No frame-generation or low-latency queue changes are implied by this spatial-upscaler contract.

## Verification boundary

This document verifies signatures and ownership from the cited public reference source.
It does **not** establish that this reference fork is byte-for-byte the engine used
to build Bright Memory v1.34, that the game has these headers available, or that
any plugin/renderer code compiles or runs in the shipped APK.
