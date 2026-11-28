#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"

namespace BMIGraphicsRuntime
{
    static IConsoleVariable* FindCVar(const TCHAR* Name)
    {
        return IConsoleManager::Get().FindConsoleVariable(Name);
    }

    static bool SetCVarIntIfPresent(const TCHAR* Name, int32 Value)
    {
        if (IConsoleVariable* CVar = FindCVar(Name))
        {
            CVar->Set(Value, ECVF_SetByGameSetting);
            return true;
        }
        return false;
    }

    static bool SetCVarFloatIfPresent(const TCHAR* Name, float Value)
    {
        if (IConsoleVariable* CVar = FindCVar(Name))
        {
            CVar->Set(Value, ECVF_SetByGameSetting);
            return true;
        }
        return false;
    }

    static bool SetCVarIntAndVerify(const TCHAR* Name, int32 Value)
    {
        IConsoleVariable* CVar = FindCVar(Name);
        if (!CVar) return false;
        CVar->Set(Value, ECVF_SetByGameSetting);
        return CVar->GetInt() == Value;
    }

    static bool SetCVarFloatAndVerify(const TCHAR* Name, float Value)
    {
        IConsoleVariable* CVar = FindCVar(Name);
        if (!CVar) return false;
        CVar->Set(Value, ECVF_SetByGameSetting);
        return FMath::IsNearlyEqual(CVar->GetFloat(), Value, 0.001f);
    }

    static void SetRenderScale(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine) return;
        const float Percent = FMath::Clamp(FCString::Atof(*Args[0]), 50.0f, 150.0f);
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetResolutionScaleValueEx(Percent);
            Settings->ApplySettings(false);
        }
    }

    static void SetResolutionController(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;

        // 0 = Native; the remaining entries are target short-side pixel heights.
        // Derive the scale from the actual display dimensions so the aspect
        // ratio and Android panel/DPI resolution are never changed.
        const int32 Choice = FCString::Atoi(*Args[0]);
        if (Choice < 0 || Choice > 6)
        {
            UE_LOG(LogTemp, Warning, TEXT("Resolution Controller: invalid choice %d."), Choice);
            return;
        }

        IConsoleVariable* ContentScale = FindCVar(TEXT("r.MobileContentScaleFactor"));
        if (!ContentScale)
        {
            UE_LOG(LogTemp, Error, TEXT("Resolution Controller: r.MobileContentScaleFactor is unavailable."));
            return;
        }

        const FIntPoint Display = GEngine && GEngine->GetGameUserSettings()
            ? GEngine->GetGameUserSettings()->GetDesktopResolution()
            : FIntPoint::ZeroValue;
        const int32 ShortSide = FMath::Min(Display.X, Display.Y);
        const int32 LongSide = FMath::Max(Display.X, Display.Y);
        if (ShortSide <= 0 || LongSide <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Resolution Controller: could not read valid display pixel dimensions."));
            return;
        }

        const int32 TargetHeight[] = { ShortSide, 540, 640, 720, 900, 980, 1080 };
        // Never upscale beyond Native. Native always restores the panel's full
        // content scale, while lower presets preserve the device aspect ratio.
        const float Scale = Choice == 0
            ? 1.0f
            : FMath::Clamp(static_cast<float>(TargetHeight[Choice]) / static_cast<float>(ShortSide), 0.25f, 1.0f);

        ContentScale->Set(Scale, ECVF_SetByGameSetting);
        const float AppliedScale = ContentScale->GetFloat();
        if (!FMath::IsNearlyEqual(AppliedScale, Scale, 0.001f))
        {
            UE_LOG(LogTemp, Error, TEXT("Resolution Controller: requested scale %.3f but CVar readback is %.3f."),
                Scale, AppliedScale);
            return;
        }

        const int32 AppliedShort = FMath::RoundToInt(ShortSide * AppliedScale);
        const int32 AppliedLong = FMath::RoundToInt(LongSide * AppliedScale);
        UE_LOG(LogTemp, Log, TEXT("Resolution Controller: %s -> approximately %dx%d content (%0.1f%%); display/DPI unchanged."),
            Choice == 0 ? TEXT("Native") : *FString::Printf(TEXT("%dP"), TargetHeight[Choice]),
            AppliedLong, AppliedShort, AppliedScale * 100.0f);
    }

    static void SetGraphicsQuality(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine || !Args[0].IsNumeric())
        {
            UE_LOG(LogTemp, Warning, TEXT("Graphics Quality: expected one numeric tier from 0 to 6."));
            return;
        }

        // 0 Very Low, 1 Low, 2 Medium, 3 High, 4 Very High, 5 Ultra, 6 Extreme.
        // Native UE4 scalability tops out at group 4. Extreme therefore keeps
        // the maximum stock group and raises only supported mobile effect CVars
        // above Ultra; missing CVars are skipped, never represented as applied.
        const int32 Quality = FCString::Atoi(*Args[0]);
        if (Quality < 0 || Quality > 6)
        {
            UE_LOG(LogTemp, Warning, TEXT("Graphics Quality: invalid tier %d; valid range is 0..6."), Quality);
            return;
        }

        UGameUserSettings* Settings = GEngine->GetGameUserSettings();
        if (!Settings) return;

        const int32 GroupQuality[] = { 0, 0, 1, 2, 3, 4, 4 };
        const int32 Group = GroupQuality[Quality];
        Settings->SetOverallScalabilityLevel(Group);
        Settings->SetViewDistanceQuality(Group);
        Settings->SetShadowQuality(Group);
        Settings->SetAntiAliasingQuality(Group);
        Settings->SetTextureQuality(Group);
        Settings->SetVisualEffectQuality(Group);
        Settings->SetFoliageQuality(Group);
        Settings->SetShadingQuality(Group);
        Settings->SetPostProcessingQuality(Group);
        Settings->ApplySettings(false);

        const float ViewDistance[] = { 0.50f, 0.65f, 0.80f, 1.00f, 1.10f, 1.20f, 1.35f };
        const float MipBias[] = { 1.5f, 1.0f, 0.5f, 0.0f, -0.25f, -0.5f, -0.75f };
        const int32 Effects[] = { 0, 0, 1, 2, 3, 3, 3 };
        const int32 Aniso[] = { 1, 2, 4, 8, 8, 16, 16 };
        const int32 Shadows[] = { 0, 0, 1, 2, 3, 4, 5 };
        const int32 MobileAO[] = { 0, 0, 0, 1, 1, 2, 3 };

        int32 Applied = 0;
        Applied += SetCVarFloatAndVerify(TEXT("r.ViewDistanceScale"), ViewDistance[Quality]) ? 1 : 0;
        Applied += SetCVarFloatAndVerify(TEXT("r.MipMapLODBias"), MipBias[Quality]) ? 1 : 0;
        Applied += SetCVarIntAndVerify(TEXT("r.EffectsQuality"), Effects[Quality]) ? 1 : 0;
        Applied += SetCVarIntAndVerify(TEXT("r.PostProcessQuality"), Effects[Quality]) ? 1 : 0;
        Applied += SetCVarIntAndVerify(TEXT("r.MaxAnisotropy"), Aniso[Quality]) ? 1 : 0;
        Applied += SetCVarIntAndVerify(TEXT("r.ShadowQuality"), Shadows[Quality]) ? 1 : 0;
        Applied += SetCVarIntAndVerify(TEXT("r.Mobile.AmbientOcclusionQuality"), MobileAO[Quality]) ? 1 : 0;

        UE_LOG(LogTemp, Log, TEXT("BMI Graphics Quality %d: scalability tier %d requested; %d/7 optional CVars verified."),
            Quality, Group, Applied);
    }

    static void SetFrameLimit(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine) return;
        const int32 Requested = FCString::Atoi(*Args[0]);
        const int32 Allowed[] = { 30, 45, 60, 90, 120 };
        bool bAllowed = false;
        for (int32 Rate : Allowed) bAllowed |= Requested == Rate;
        if (!bAllowed) return;
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetFrameRateLimit(static_cast<float>(Requested));
            Settings->ApplySettings(false);
        }
    }

    // SGSR1 is the only upscaler in this profile. Preset values map to
    // render percentage and the official Qualcomm SGSR1 edge-sharpness input.
    static void SetUpscalePreset(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine) return;
        const int32 Preset = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 4);
        const float RenderScale[] = { 50.0f, 60.0f, 72.0f, 86.0f, 100.0f };
        const float EdgeSharpness[] = { 1.90f, 1.72f, 1.52f, 1.30f, 1.12f };

        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetResolutionScaleValueEx(RenderScale[Preset]);
            Settings->ApplySettings(false);
        }
        SetCVarFloatIfPresent(TEXT("r.Qualcomm.SGSR.Sharpness"), EdgeSharpness[Preset]);
    }

    // The menu exposes one fixed upscaler: Qualcomm SGSR1.
    static void SetUpscaler(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || FCString::Atoi(*Args[0]) != 0) return;

        IConsoleVariable* SGSR1 = FindCVar(TEXT("r.Qualcomm.SGSR.Enabled"));
        if (!SGSR1)
        {
            UE_LOG(LogTemp, Error, TEXT("Qualcomm SGSR1 plugin is not loaded; selection rejected."));
            return;
        }

        SetCVarIntIfPresent(TEXT("bmi.Pipeline.Enabled"), 0);
        SetCVarIntIfPresent(TEXT("r.TemporalAA.Upsampling"), 0);
        SGSR1->Set(1, ECVF_SetByGameSetting);
    }

    static void SetUpscaleSharpness(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        // Qualcomm's UE4.27 SGSR1 shader accepts edge sharpness [1.0, 2.0].
        // The user-facing 0..1 slider maps linearly to that real shader value.
        const float Sharpness = 1.0f + FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f);
        SetCVarFloatIfPresent(TEXT("r.Qualcomm.SGSR.Sharpness"), Sharpness);
    }

    static void SetCAS(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const bool bEnable = FCString::Atoi(*Args[0]) != 0;
        IConsoleVariable* Available = FindCVar(TEXT("bmi.Pipeline.CASAvailable"));
        if (!Available || Available->GetInt() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("No AMD CAS RDG pass is registered; refusing a non-functional toggle."));
            return;
        }
        SetCVarIntIfPresent(TEXT("bmi.Pipeline.CAS"), bEnable ? 1 : 0);
    }

    static void SetCASSharpness(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const float Sharpness = FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f);
        IConsoleVariable* Available = FindCVar(TEXT("bmi.Pipeline.CASAvailable"));
        if (!Available || Available->GetInt() == 0) return;
        SetCVarFloatIfPresent(TEXT("bmi.Pipeline.CASSharpness"), Sharpness);
    }

    static void SetFramePacing(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Enabled = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 1);
        // The inspected target profile disables Vulkan. Vulkan-only fallback
        // pacers must not be presented as working on this OpenGL ES build.
        if (Enabled == 0)
        {
            SetCVarIntIfPresent(TEXT("r.Vulkan.CPURenderthreadFramePacer"), 0);
            SetCVarIntIfPresent(TEXT("r.Vulkan.CPURHIThreadFramePacer"), 0);
            return;
        }
        UE_LOG(LogTemp, Error, TEXT("Frame Pacing is not enabled: the active Android OpenGL ES pacer/Swappy path has not yet been integrated."));
    }

    static void SetPerformanceHUD(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const bool bEnabled = FCString::Atoi(*Args[0]) != 0;
        IConsoleVariable* Available = FindCVar(TEXT("bmi.Pipeline.HUDAvailable"));
        if (bEnabled && (!Available || Available->GetInt() == 0))
        {
            UE_LOG(LogTemp, Warning, TEXT("No in-pipeline performance HUD compositor is registered yet."));
            return;
        }
        SetCVarIntIfPresent(TEXT("bmi.Pipeline.HUD"), bEnabled ? 1 : 0);
    }

    static void SetLowLatency(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Mode = FCString::Atoi(*Args[0]);
        if (Mode < 0 || Mode > 2) return;

        if (Mode == 2)
        {
            // UE4.27 has no verified Android GLES zero-queued-present control.
            // Never approximate it with glFinish, RHICmdBypass or Vulkan CVars.
            UE_LOG(LogTemp, Error, TEXT("Ultra (Zero Queue) is unavailable: Android GLES/Swappy queue-depth control is not registered and measured."));
            return;
        }

        // UE4.27's documented low-latency sync mode is r.GTSyncType=1:
        // Game Thread synchronizes to the RHI Thread. Keep OneFrameThreadLag
        // enabled as in Epic's documented low-latency example; forcing it to
        // zero is a different, more aggressive trade-off and can cost FPS.
        const int32 SyncType = Mode == 1 ? 1 : 0;
        const bool bSyncApplied = SetCVarIntAndVerify(TEXT("r.GTSyncType"), SyncType);
        const bool bLagApplied = SetCVarIntAndVerify(TEXT("r.OneFrameThreadLag"), 1);
        if (!bSyncApplied || !bLagApplied)
        {
            UE_LOG(LogTemp, Error, TEXT("Low Latency mode %d was not applied: required UE4 thread-sync CVars are missing or rejected (r.GTSyncType=%d, r.OneFrameThreadLag=1)."),
                Mode, SyncType);
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("Low Latency %s applied: r.GTSyncType=%d, r.OneFrameThreadLag=1."),
            Mode == 1 ? TEXT("On") : TEXT("Off"), SyncType);
    }

    static bool IsFrameGenerationAvailable(int32 Method, int32 Multiplier)
    {
        if (Method == 1)
        {
            IConsoleVariable* Available = FindCVar(TEXT("bmi.FrameGeneration.AFMEAvailable"));
            return Multiplier == 2 && Available && Available->GetInt() != 0;
        }
        if (Method == 2)
        {
            const TCHAR* Name = Multiplier == 2 ? TEXT("bmi.FrameGeneration.GLESX2Available")
                : Multiplier == 3 ? TEXT("bmi.FrameGeneration.GLESX3Available")
                : Multiplier == 4 ? TEXT("bmi.FrameGeneration.GLESX4Available") : nullptr;
            IConsoleVariable* Available = Name ? FindCVar(Name) : nullptr;
            return Available && Available->GetInt() != 0;
        }
        return false;
    }

    static void SetFrameGenerationMethod(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Method = FCString::Atoi(*Args[0]);
        if (Method < 0 || Method > 2) return;
        if (Method == 0)
        {
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Method"), 0);
            return;
        }
        IConsoleVariable* MultiplierVar = FindCVar(TEXT("bmi.FrameGeneration.Multiplier"));
        int32 Multiplier = MultiplierVar ? MultiplierVar->GetInt() : 2;

        // AFME is x2-only. Selecting it while x3/x4 was stored resets the
        // multiplier to x2; higher multipliers belong to the GLES backend.
        if (Method == 1 && Multiplier != 2)
        {
            Multiplier = 2;
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Multiplier"), 2);
        }
        if (!IsFrameGenerationAvailable(Method, Multiplier))
        {
            UE_LOG(LogTemp, Warning, TEXT("Frame Generation backend %d is unavailable for x%d."), Method, Multiplier);
            return;
        }
        SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Method"), Method);
    }

    static void SetFrameGenerationMultiplier(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Multiplier = FCString::Atoi(*Args[0]);
        if (Multiplier < 2 || Multiplier > 4) return;
        IConsoleVariable* MethodVar = FindCVar(TEXT("bmi.FrameGeneration.Method"));
        const int32 Method = MethodVar ? MethodVar->GetInt() : 0;
        if (Method == 0)
        {
            // Store the selection for when a backend is chosen, without enabling FG.
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Multiplier"), Multiplier);
            return;
        }
        if (Method == 1 && Multiplier > 2)
        {
            // AFME cannot generate x3/x4. Route the request to the portable
            // GLES backend only when that exact multiplier is registered.
            if (!IsFrameGenerationAvailable(2, Multiplier))
            {
                UE_LOG(LogTemp, Warning, TEXT("AFME is x2-only and GLES x%d is not registered."), Multiplier);
                return;
            }
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Method"), 2);
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Multiplier"), Multiplier);
            return;
        }
        if (!IsFrameGenerationAvailable(Method, Multiplier))
        {
            UE_LOG(LogTemp, Warning, TEXT("Frame Generation x%d is unavailable for selected backend %d."), Multiplier, Method);
            return;
        }
        SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Multiplier"), Multiplier);
    }

    static void SetFrameGeneration(const TArray<FString>& Args)
    {
        // Backward-compatible API: 0=Off, 1=x2, 2=x3, 3=x4.
        if (Args.Num() != 1) return;
        const int32 Mode = FCString::Atoi(*Args[0]);
        if (Mode < 0 || Mode > 3) return;
        if (Mode == 0)
        {
            SetCVarIntIfPresent(TEXT("bmi.FrameGeneration.Method"), 0);
            return;
        }
        TArray<FString> MultiplierArgs;
        MultiplierArgs.Add(FString::FromInt(Mode + 1));
        SetFrameGenerationMultiplier(MultiplierArgs);
    }

    static FAutoConsoleCommand CmdFrameGenerationMethod(TEXT("bmi.SetFrameGenerationMethod"), TEXT("Select 0 Off, 1 Qualcomm AFME, 2 GLES reprojection; requires a registered backend."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameGenerationMethod));
    static FAutoConsoleCommand CmdFrameGenerationMultiplier(TEXT("bmi.SetFrameGenerationMultiplier"), TEXT("Select generated output multiplier 2, 3 or 4; requires selected backend support."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameGenerationMultiplier));
    static FAutoConsoleCommand CmdFrameGeneration(TEXT("bmi.SetFrameGeneration"), TEXT("Legacy frame generation selection: 0 Off, 1 x2, 2 x3, 3 x4; backend and multiplier are validated separately."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameGeneration));
    static FAutoConsoleCommand CmdResolutionController(TEXT("bmi.SetResolutionController"), TEXT("Set native/540P..1080P mobile content scale option 0..6."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetResolutionController));
    static FAutoConsoleCommand CmdFrameLimit(TEXT("bmi.SetFrameLimit"), TEXT("Set UE4 frame limit 30,45,60,90,120."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameLimit));
    static FAutoConsoleCommand CmdRenderScale(TEXT("bmi.SetRenderScale"), TEXT("Set render scale percent."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetRenderScale));
    static FAutoConsoleCommand CmdGraphicsQuality(TEXT("bmi.SetGraphicsQuality"), TEXT("Set UE4 scalability quality 0..6 (Very Low..Extreme)."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetGraphicsQuality));
    static FAutoConsoleCommand CmdUpscalePreset(TEXT("bmi.SetUpscalePreset"), TEXT("Set unified upscaler preset 0..4."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscalePreset));
    static FAutoConsoleCommand CmdUpscaler(TEXT("bmi.SetUpscaler"), TEXT("Select upscaler: 0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Mobile, 4 Nearest, 5 Bilinear."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscaler));
    static FAutoConsoleCommand CmdUpscaleSharpness(TEXT("bmi.SetUpscaleSharpness"), TEXT("Set algorithm sharpness 0..1 when supported."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscaleSharpness));
    static FAutoConsoleCommand CmdCAS(TEXT("bmi.SetCAS"), TEXT("Enable registered AMD CAS renderer pass."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetCAS));
    static FAutoConsoleCommand CmdCASSharpness(TEXT("bmi.SetCASSharpness"), TEXT("Set registered AMD CAS sharpness 0..1."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetCASSharpness));
    static FAutoConsoleCommand CmdFramePacing(TEXT("bmi.SetFramePacing"), TEXT("Frame pacing; refuses Vulkan-only mode on the inspected GLES target."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFramePacing));
    static FAutoConsoleCommand CmdPerformanceHUD(TEXT("bmi.SetPerformanceHUD"), TEXT("Enable the registered post-upscale performance HUD pass."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetPerformanceHUD));
    static FAutoConsoleCommand CmdLowLatency(TEXT("bmi.SetLowLatency"), TEXT("Low latency: 0 off, 1 UE4 thread sync, 2 unsupported no-queue."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetLowLatency));
}

class FBMIGraphicsRuntimeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogTemp, Log, TEXT("BMIGraphicsRuntime settings bridge registered."));
    }
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FBMIGraphicsRuntimeModule, BMIGraphicsRuntime)
