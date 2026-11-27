// Copyright (c) Bright Memory Android Research contributors.

#include "BMISpatialUpscaler.h"

#include "HAL/IConsoleManager.h"
#include "ScreenPass.h"

namespace
{
    // Disabled by default: preserve the game's original upscale path until
    // the setting is explicitly enabled by the game's graphics UI/config.
    TAutoConsoleVariable<int32> CVarBMIUseNativeSpatialUpscaler(
        TEXT("bmi.UseNativeSpatialUpscaler"),
        0,
        TEXT("Use BMI's renderer-private wrapper around UE4.27's built-in spatial upscale pass."),
        ECVF_RenderThreadSafe);

    // Android/mobile permutations in this engine reference compile Nearest
    // and Bilinear unconditionally. Other permutations are SM5-only here.
    TAutoConsoleVariable<int32> CVarBMIUpscaleMethod(
        TEXT("bmi.NativeUpscaleMethod"),
        1,
        TEXT("0=Nearest, 1=Bilinear (mobile-safe built-in methods only)."),
        ECVF_RenderThreadSafe);

    TSharedPtr<FBMISpatialUpscalerViewExtension, ESPMode::ThreadSafe> GBMIUpscalerExtension;
}

const TCHAR* FBMISpatialUpscaler::GetDebugName() const
{
    return TEXT("BMI Native Spatial Upscaler");
}

ISpatialUpscaler* FBMISpatialUpscaler::Fork_GameThread(const FSceneViewFamily& ViewFamily) const
{
    return new FBMISpatialUpscaler();
}

FScreenPassTexture FBMISpatialUpscaler::AddPasses(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FInputs& PassInputs) const
{
    const int32 MethodValue = FMath::Clamp(CVarBMIUpscaleMethod.GetValueOnRenderThread(), 0, 1);
    const EUpscaleMethod Method = MethodValue == 0
        ? EUpscaleMethod::Nearest
        : EUpscaleMethod::Bilinear;

    return ISpatialUpscaler::AddDefaultUpscalePass(
        GraphBuilder,
        View,
        PassInputs,
        Method,
        FPaniniProjectionConfig());
}

FBMISpatialUpscalerViewExtension::FBMISpatialUpscalerViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
}

void FBMISpatialUpscalerViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FBMISpatialUpscalerViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
}

void FBMISpatialUpscalerViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
    if (CVarBMIUseNativeSpatialUpscaler.GetValueOnGameThread() == 0)
    {
        return;
    }

    // Never overwrite an upscaler already supplied by the game or another extension.
    if (InViewFamily.GetPrimarySpatialUpscalerInterface() == nullptr)
    {
        // FSceneViewFamily owns this interface through ISceneViewFamilyExtention.
        InViewFamily.SetPrimarySpatialUpscalerInterface(new FBMISpatialUpscaler());
    }
}

void FBMISpatialUpscalerViewExtension::PreRenderViewFamily_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneViewFamily& InViewFamily)
{
}

void FBMISpatialUpscalerViewExtension::PreRenderView_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneView& InView)
{
}

void RegisterBMISpatialUpscaler()
{
    if (!GBMIUpscalerExtension.IsValid())
    {
        GBMIUpscalerExtension = FSceneViewExtensions::NewExtension<FBMISpatialUpscalerViewExtension>();
    }
}
