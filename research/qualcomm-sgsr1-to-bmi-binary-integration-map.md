# Qualcomm SGSR1 integration mapped to Bright Memory: Infinite v1.34

Date: 2026-11-27

## Pinned upstream implementation inspected

Repository: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine  
Commit: 761e8393ae44a6cef48e51f37e315ccce494b055

Files inspected:
- Plugins/SGSR/Source/SGSR/Private/SGSR.cpp
- Plugins/SGSR/Source/SGSRSpatialUpscaling/Public/SGSRViewExtension.h
- Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRViewExtension.cpp
- Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.h
- Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.cpp
- Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaling.cpp
- Plugins/SGSR/Source/SGSRSpatialUpscaling/SGSRSpatialUpscaling.Build.cs
- Plugins/SGSR/SGSR1.uplugin

## Exact upstream SGSR1 integration flow

### 1. Register the view extension at module startup

In FSGSRModule::StartupModule():

    SGSRViewExtension = FSceneViewExtensions::NewExtension<FSGSRViewExtension>();

The extension is retained by the module and participates in the engine's view-family lifecycle. The module releases its reference in ShutdownModule().

### 2. Select the upscaler at BeginRenderViewFamily

In FSGSRViewExtension::BeginRenderViewFamily(FSceneViewFamily&):
- Check r.Qualcomm.SGSR.Enabled.
- Iterate InViewFamily.Views and allocate one FSGSRData per view.
- Detect whether any view requests EPrimaryScreenPercentageMethod::TemporalUpscale.
- If temporal upscaling is not requested, install:
  InViewFamily.SetPrimarySpatialUpscalerInterface(new FSGSRSpatialUpscaler(ESGSRMode::UpscalingOnly, ViewData));
- If temporal upscaling is requested, do not replace the temporal path with SGSR1; the plugin installs its secondary/post-processing mode instead.

The SGSR1 path therefore uses the primary spatial interface only when the view-family configuration is spatial, avoiding an unconditional overwrite of a temporal upscaler.

### 3. Implement the renderer interface

FSGSRSpatialUpscaler derives from ISpatialUpscaler and implements:
- GetDebugName()
- Fork_GameThread(const FSceneViewFamily&) const
- AddPasses(FRDGBuilder&, const FViewInfo&, const FInputs&) const

Fork_GameThread returns a new FSGSRSpatialUpscaler with the same mode and per-view data references.

AddPasses:
- validates PassInputs.SceneColor
- handles SecondaryToOutput separately and falls back to Unreal's default upscale pass for that stage
- finds the correct per-view data object by matching the FViewInfo pointer against View.Family->Views
- lazily initializes SGSR data/resources
- executes ordered render-graph subpasses: First, Scaler, Last
- returns the final screen-pass texture

### 4. Register shader source directory

The SGSRSpatialUpscaling module's StartupModule() maps the plugin shader directory:

    AddShaderSourceDirectoryMapping(TEXT("/Plugin/SGSR"), PluginShaderDir);

The module Build.cs depends on Core, CoreUObject, Engine, Projects, RenderCore, Renderer, and RHI, and includes the Renderer private include directory. SGSR is not just a CVar or a replacement texture; it is a native renderer module with RDG passes and compiled shader permutations.

### 5. Plugin platform/version declarations

SGSR1.uplugin declares EngineVersion 4.27.0 and Android/Win64 module platforms. The source itself has a static assertion requiring UE4.27.1 or newer (or UE5). Therefore, the descriptor alone is not enough to establish binary compatibility with BMI.

## Direct correspondence with the BMI binary audit

The BMI binary contains the same dispatch seam:
- FSceneViewFamily +0xc0 is read by AddMobilePostProcessingPasses at 0x78870c0.
- A non-null receiver is called through vtable byte offset +0x20.
- A null receiver uses ISpatialUpscaler::AddDefaultUpscalePass at 0x78fae30.
- The call arguments are consistent with AddPasses(GraphBuilder, View, Inputs), including the indirect result location in x8.
- FSceneViewFamily is the owner of the primary spatial upscaler interface, and the official setter is the intended way to assign it.

This is a strong structural match between Qualcomm's source integration and the target binary dispatch. It does not by itself prove that the plugin binary ABI, class layout, vtable order, shader platform, or engine changelist matches BMI.

## Important lifetime detail

Qualcomm's code deliberately allocates the interface with new in BeginRenderViewFamily and returns another new interface object from Fork_GameThread. It does not hold the interface in a TSharedPtr and then pass a borrowed pointer to the family.

That pattern is consistent with the view-family / renderer fork lifecycle expected by this API. The exact target binary's family copy/destructor behavior is still being checked separately; reproduce the upstream ownership pattern only after the target-build lifecycle is confirmed.

## BMI-specific integration plan based on the real plugin

1. Do not blindly inject Qualcomm's prebuilt plugin binaries into the game.
2. Recreate/adapt the SGSR1 source integration against the exact headers/ABI corresponding to BMI's engine build.
3. Register a view extension through the supported engine registry.
4. Select SGSR1 only when the view-family requests spatial upscaling; never replace a temporal upscaler.
5. Allocate per-view state for each family view, implement the Fork_GameThread contract, and preserve correct lifetime.
6. Implement the SGSR1 RDG shader passes and shader mapping; verify shader compilation/packaging for the target RHI.
7. Route the existing game render-scale controls into the render resolution independently of the upscaler.
8. Keep the engine default upscale path as the fallback whenever SGSR1 is disabled or unsupported.
9. Validate Android Vulkan and OpenGL ES separately; plugin Android metadata does not prove both RHIs are supported in the packaged title.

## Current evidence status

- [x] Qualcomm SGSR1 source integration path inspected at pinned commit.
- [x] BMI binary dispatch point matches the same ISpatialUpscaler::AddPasses seam structurally.
- [x] Upstream allocation and fork pattern identified.
- [ ] Exact BMI engine source/changelist and ABI matched.
- [ ] BMI target-build family copy/destructor behavior confirmed.
- [ ] SGSR shader permutations compiled for the game's packaged RHI.
- [ ] Native integration compiled/linked or binary patch produced.
- [ ] Repacked APK installed and runtime-validated.

No binary bytes have been modified and no modified APK has been produced by this research pass.
