// Copyright (c) Bright Memory Android Research contributors.
// Renderer-private bridge for UE4.27's built-in spatial upscaler.
// This is intentionally limited to the mobile-safe Nearest/Bilinear permutations.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "PostProcess/PostProcessUpscale.h"

class FBMISpatialUpscaler final : public ISpatialUpscaler
{
public:
    const TCHAR* GetDebugName() const override;
    ISpatialUpscaler* Fork_GameThread(const FSceneViewFamily& ViewFamily) const override;
    FScreenPassTexture AddPasses(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FInputs& PassInputs) const override;
};

class FBMISpatialUpscalerViewExtension final : public FSceneViewExtensionBase
{
public:
    FBMISpatialUpscalerViewExtension(const FAutoRegister& AutoRegister);

    void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
    void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
    void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
    void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
    void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
};

// Call once from FRendererModule::StartupModule(). Keep the returned extension
// registered for the lifetime of Renderer.
void RegisterBMISpatialUpscaler();
