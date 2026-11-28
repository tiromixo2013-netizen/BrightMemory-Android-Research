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
        const int32 Choice = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 6);
        const float Scale[] = { 0.0f, 0.75f, 0.85f, 1.0f, 1.2f, 1.35f, 1.5f };
        if (!SetCVarFloatIfPresent(TEXT("r.MobileContentScaleFactor"), Scale[Choice]))
            UE_LOG(LogTemp, Error, TEXT("Game mobile content scale CVar is unavailable."));
    }

    static void SetGraphicsQuality(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine) return;

        // 0 Low, 1 Medium, 2 High, 3 Very High, 4 Ultra, 5 Extreme.
        // The shipped UE4.27 binary exposes individual UGameUserSettings setters
        // for all eight scalability groups. Extreme uses the highest stock
        // group plus bounded mobile overrides; it is not claimed to be console
        // quality until the actual game is visually A/B tested.
        const int32 Quality = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 5);
        UGameUserSettings* Settings = GEngine->GetGameUserSettings();
        if (!Settings) return;

        const int32 GroupQuality[] = { 0, 1, 2, 3, 4, 4 };
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

        const float ViewDistance[] = { 0.60f, 0.80f, 1.00f, 1.10f, 1.20f, 1.30f };
        const float MipBias[] = { 1.0f, 0.5f, 0.0f, 0.0f, -0.25f, -0.5f };
        // Keep the stock scalability CVar range bounded; Extreme is expressed
        // through the explicit per-category setters and supported mobile CVars.
        const int32 Effects[] = { 0, 1, 2, 3, 3, 3 };
        const int32 Aniso[] = { 2, 4, 8, 8, 16, 16 };

        int32 Applied = 0;
        Applied += SetCVarFloatIfPresent(TEXT("r.ViewDistanceScale"), ViewDistance[Quality]) ? 1 : 0;
        Applied += SetCVarFloatIfPresent(TEXT("r.MipMapLODBias"), MipBias[Quality]) ? 1 : 0;
        Applied += SetCVarIntIfPresent(TEXT("r.EffectsQuality"), Effects[Quality]) ? 1 : 0;
        Applied += SetCVarIntIfPresent(TEXT("r.PostProcessQuality"), Effects[Quality]) ? 1 : 0;
        Applied += SetCVarIntIfPresent(TEXT("r.MaxAnisotropy"), Aniso[Quality]) ? 1 : 0;
        Applied += SetCVarIntIfPresent(TEXT("r.Mobile.AmbientOcclusionQuality"), Quality >= 2 ? 1 : 0) ? 1 : 0;

        UE_LOG(LogTemp, Log, TEXT("BMI Graphics Quality %d: all 8 scalability groups requested at tier %d; %d/6 optional mobile CVars applied."),
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

    static bool SetCVarIntAndVerify(const TCHAR* Name, int32 Value)
    {
        IConsoleVariable* CVar = FindCVar(Name);
        if (!CVar) return false;
        CVar->Set(Value, ECVF_SetByGameSetting);
        return CVar->GetInt() == Value;
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
    static FAutoConsoleCommand CmdGraphicsQuality(TEXT("bmi.SetGraphicsQuality"), TEXT("Set UE4 scalability quality 0..5."),
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
