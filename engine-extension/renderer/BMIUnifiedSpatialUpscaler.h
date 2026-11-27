// Copyright (c) Bright Memory Android Research contributors.
// UE4.27 Renderer-private single spatial post-process path.
// Spatial reconstruction, optional CAS and optional HUD are chained here.
// Frame Generation is deliberately separate.
#pragma once
#include "CoreMinimal.h"
#include "PostProcess/PostProcessUpscale.h"
#include "ScreenPass.h"

using FBMIUpscalePass = TFunction<FScreenPassTexture(FRDGBuilder&, const FViewInfo&, const ISpatialUpscaler::FInputs&)>;
using FBMICASPass = TFunction<FScreenPassTexture(FRDGBuilder&, const FViewInfo&, const FScreenPassTexture&, float)>;
using FBMIHudPass = TFunction<FScreenPassTexture(FRDGBuilder&, const FViewInfo&, const FScreenPassTexture&)>;

class FBMIUnifiedSpatialUpscaler final : public ISpatialUpscaler
{
public:
 const TCHAR* GetDebugName() const override;
 ISpatialUpscaler* Fork_GameThread(const FSceneViewFamily& ViewFamily) const override;
 FScreenPassTexture AddPasses(FRDGBuilder& GraphBuilder,const FViewInfo& View,const FInputs& PassInputs) const override;
 // 0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Mobile, 4 Nearest, 5 Bilinear.
 static void RegisterUpscaler(int32 Method, FBMIUpscalePass Pass);
 static void RegisterCAS(FBMICASPass Pass);
 static void RegisterHud(FBMIHudPass Pass);
private:
 static FBMIUpscalePass FindUpscaler(int32 Method);
 static FBMICASPass FindCAS();
 static FBMIHudPass FindHud();
 int32 Method = 5;
 bool bCAS = false;
 float CASSharpness = 0.5f;
 bool bHud = false;
};
