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
        const int32 Quality = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 5);
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            // UE4 exposes five standard scalability tiers (0..4). Extreme uses
            // Epic scalability plus explicit mobile CVar overrides below.
            Settings->SetOverallScalabilityLevel(FMath::Min(Quality, 4));
            Settings->ApplySettings(false);
        }
        const float ViewDistance[] = { 0.60f, 0.80f, 1.00f, 1.10f, 1.20f, 1.35f };
        const float MipBias[] = { 1.0f, 0.5f, 0.0f, 0.0f, -0.25f, -0.5f };
        const int32 Effects[] = { 0, 1, 2, 3, 3, 4 };
        const int32 Aniso[] = { 2, 4, 8, 8, 8, 16 };
        SetCVarFloatIfPresent(TEXT("r.ViewDistanceScale"), ViewDistance[Quality]);
        SetCVarFloatIfPresent(TEXT("r.MipMapLODBias"), MipBias[Quality]);
        SetCVarIntIfPresent(TEXT("r.EffectsQuality"), Effects[Quality]);
        SetCVarIntIfPresent(TEXT("r.PostProcessQuality"), Effects[Quality]);
        SetCVarIntIfPresent(TEXT("r.MaxAnisotropy"), Aniso[Quality]);
        SetCVarIntIfPresent(TEXT("r.ShadowQuality"), Quality >= 2 ? 1 : 0);
        SetCVarIntIfPresent(TEXT("r.Mobile.AmbientOcclusionQuality"), Quality >= 2 ? 1 : 0);
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

    // Unified preset maps to render percentage; it does not silently rewrite
    // the independent Graphics Quality setting.
    static void SetUpscalePreset(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine) return;
        const int32 Preset = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 4);
        int32 Method = 5;
        if (IConsoleVariable* Selected = FindCVar(TEXT("bmi.Pipeline.Upscaler")))
            Method = FMath::Clamp(Selected->GetInt(), 0, 5);

        // Method-specific render fractions, one shared preset selector.
        // Rows: SGSR1, SGSR2, MWSR, FSR Mobile, Nearest, Bilinear.
        const float Scale[6][5] = {
            { 50.0f, 60.0f, 72.0f, 86.0f, 100.0f },
            { 50.0f, 62.5f, 75.0f, 87.5f, 100.0f },
            { 50.0f, 60.0f, 75.0f, 90.0f, 100.0f },
            { 50.0f, 62.5f, 75.0f, 87.5f, 100.0f },
            { 50.0f, 65.0f, 80.0f, 90.0f, 100.0f },
            { 50.0f, 65.0f, 80.0f, 90.0f, 100.0f }
        };
        const float Sharpness[5] = { 0.90f, 0.72f, 0.52f, 0.30f, 0.12f };
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetResolutionScaleValueEx(Scale[Method][Preset]);
            Settings->ApplySettings(false);
        }
        SetCVarFloatIfPresent(TEXT("bmi.Pipeline.UpscaleSharpness"), Sharpness[Preset]);
    }

    // Method IDs: 0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Mobile,
    // 4 Nearest, 5 Bilinear. External algorithms are enabled only if
    // their actual plugin CVar is registered in this engine build.
    static void SetUpscaler(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Method = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 5);
        // Only methods with a registered RDG implementation may be selected.
        IConsoleVariable* Mask = FindCVar(TEXT("bmi.Pipeline.RegisteredUpscalerMask"));
        if (!Mask || (Mask->GetInt() & (1 << Method)) == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Requested upscaler has no registered renderer pass; selection rejected."));
            return;
        }
        // Keep the renderer extension opt-in: never replace the game's
        // original spatial path until both renderer CVars are present and a
        // registered method has passed the availability-mask check.
        IConsoleVariable* SelectedMethod = FindCVar(TEXT("bmi.Pipeline.Upscaler"));
        IConsoleVariable* PipelineEnabled = FindCVar(TEXT("bmi.Pipeline.Enabled"));
        if (!SelectedMethod || !PipelineEnabled)
        {
            UE_LOG(LogTemp, Error, TEXT("BMI unified renderer pipeline is not installed; keeping the game's original upscaler."));
            return;
        }
        SelectedMethod->Set(Method, ECVF_SetByGameSetting);
        PipelineEnabled->Set(1, ECVF_SetByGameSetting);

        // One primary Renderer RDG pipeline owns all spatial methods.
        SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR.Enabled"), 0);
        SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR2.Enabled"), 0);
    }

    static void SetUpscaleSharpness(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const float Sharpness = FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f);
        SetCVarFloatIfPresent(TEXT("bmi.Pipeline.UpscaleSharpness"), Sharpness);
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
        const int32 Mode = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 2);
        if (Mode == 0)
        {
            SetCVarIntIfPresent(TEXT("r.OneFrameThreadLag"), 1);
            return;
        }
        if (Mode == 1)
        {
            // UE4's supported low-latency thread-sync path. This trades some
            // throughput for reduced game/render thread overlap.
            SetCVarIntIfPresent(TEXT("r.OneFrameThreadLag"), 0);
            return;
        }
        UE_LOG(LogTemp, Error, TEXT("Ultra (No Queue) is unavailable: no validated active-RHI queue-depth control exists for this build."));
    }

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
