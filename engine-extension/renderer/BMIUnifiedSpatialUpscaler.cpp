// Copyright (c) Bright Memory Android Research contributors.
#include "BMIUnifiedSpatialUpscaler.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeRWLock.h"

namespace {
 FRWLock GRegistryLock;
 TMap<int32, FBMIUpscalePass> GUpscalers;
 FBMICASPass GCASPass;
 FBMIHudPass GHudPass;
 TAutoConsoleVariable<int32> CVarBMIMethod(TEXT("bmi.Pipeline.Upscaler"),5,TEXT("0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Mobile, 4 Nearest, 5 Bilinear."),ECVF_RenderThreadSafe);
 TAutoConsoleVariable<int32> CVarBMICASEnabled(TEXT("bmi.Pipeline.CAS"),0,TEXT("Append registered CAS pass after spatial reconstruction."),ECVF_RenderThreadSafe);
 TAutoConsoleVariable<float> CVarBMICASSSharpness(TEXT("bmi.Pipeline.CASSharpness"),0.5f,TEXT("CAS sharpness 0..1."),ECVF_RenderThreadSafe);
 TAutoConsoleVariable<int32> CVarBMIHudEnabled(TEXT("bmi.Pipeline.HUD"),0,TEXT("Append registered HUD pass after reconstruction."),ECVF_RenderThreadSafe);
}
const TCHAR* FBMIUnifiedSpatialUpscaler::GetDebugName() const { return TEXT("BMI Unified Spatial Reconstruction Pipeline"); }
ISpatialUpscaler* FBMIUnifiedSpatialUpscaler::Fork_GameThread(const FSceneViewFamily& ViewFamily) const {
 auto* Copy=new FBMIUnifiedSpatialUpscaler();
 Copy->Method=FMath::Clamp(CVarBMIMethod.GetValueOnGameThread(),0,5);
 Copy->bCAS=CVarBMICASEnabled.GetValueOnGameThread()!=0;
 Copy->CASSharpness=FMath::Clamp(CVarBMICASSSharpness.GetValueOnGameThread(),0.0f,1.0f);
 Copy->bHud=CVarBMIHudEnabled.GetValueOnGameThread()!=0;
 return Copy;
}
FScreenPassTexture FBMIUnifiedSpatialUpscaler::AddPasses(FRDGBuilder& GraphBuilder,const FViewInfo& View,const FInputs& PassInputs) const {
 check(PassInputs.SceneColor.IsValid());
 FScreenPassTexture Output;
 FBMIUpscalePass Upscale=FindUpscaler(Method);
 if (Upscale) Output=Upscale(GraphBuilder,View,PassInputs);
 else if (Method==4||Method==5) {
  const EUpscaleMethod NativeMethod=Method==4?EUpscaleMethod::Nearest:EUpscaleMethod::Bilinear;
  Output=ISpatialUpscaler::AddDefaultUpscalePass(GraphBuilder,View,PassInputs,NativeMethod,FPaniniProjectionConfig());
 } else {
  UE_LOG(LogRenderer,Error,TEXT("BMI method %d is not registered; using safe Bilinear fallback."),Method);
  Output=ISpatialUpscaler::AddDefaultUpscalePass(GraphBuilder,View,PassInputs,EUpscaleMethod::Bilinear,FPaniniProjectionConfig());
 }
 if (bCAS) { FBMICASPass Pass=FindCAS(); if(Pass) Output=Pass(GraphBuilder,View,Output,CASSharpness); else UE_LOG(LogRenderer,Warning,TEXT("CAS requested but no CAS RDG pass is registered; skipped.")); }
 if (bHud) { FBMIHudPass Pass=FindHud(); if(Pass) Output=Pass(GraphBuilder,View,Output); }
 return Output;
}
void FBMIUnifiedSpatialUpscaler::RegisterUpscaler(int32 Method,FBMIUpscalePass Pass) { if(Method<0||Method>3||!Pass)return; FWriteScopeLock Lock(GRegistryLock); GUpscalers.Add(Method,MoveTemp(Pass)); }
void FBMIUnifiedSpatialUpscaler::RegisterCAS(FBMICASPass Pass) { FWriteScopeLock Lock(GRegistryLock); GCASPass=MoveTemp(Pass); }
void FBMIUnifiedSpatialUpscaler::RegisterHud(FBMIHudPass Pass) { FWriteScopeLock Lock(GRegistryLock); GHudPass=MoveTemp(Pass); }
FBMIUpscalePass FBMIUnifiedSpatialUpscaler::FindUpscaler(int32 Method) { FReadScopeLock Lock(GRegistryLock); const auto* P=GUpscalers.Find(Method); return P?*P:FBMIUpscalePass(); }
FBMICASPass FBMIUnifiedSpatialUpscaler::FindCAS() { FReadScopeLock Lock(GRegistryLock); return GCASPass; }
FBMIHudPass FBMIUnifiedSpatialUpscaler::FindHud() { FReadScopeLock Lock(GRegistryLock); return GHudPass; }

FBMIUnifiedUpscalerViewExtension::FBMIUnifiedUpscalerViewExtension(const FAutoRegister& AutoRegister)
 : FSceneViewExtensionBase(AutoRegister) {}
void FBMIUnifiedUpscalerViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}
void FBMIUnifiedUpscalerViewExtension::SetupView(FSceneViewFamily& InViewFamily,FSceneView& InView) {}
void FBMIUnifiedUpscalerViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {
 if (InViewFamily.GetPrimarySpatialUpscalerInterface()==nullptr) {
  InViewFamily.SetPrimarySpatialUpscalerInterface(new FBMIUnifiedSpatialUpscaler());
 }
}
void FBMIUnifiedUpscalerViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList,FSceneViewFamily& InViewFamily) {}
void FBMIUnifiedUpscalerViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList,FSceneView& InView) {}
void RegisterBMIUnifiedSpatialUpscaler() {
 static TSharedPtr<FBMIUnifiedUpscalerViewExtension,ESPMode::ThreadSafe> Extension;
 if (!Extension.IsValid()) Extension=FSceneViewExtensions::NewExtension<FBMIUnifiedUpscalerViewExtension>();
}
