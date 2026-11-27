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
        const int32 Quality = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 4);
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetOverallScalabilityLevel(Quality);
            Settings->ApplySettings(false);
        }
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
        const float RenderPercent[] = { 50.0f, 62.5f, 75.0f, 87.5f, 100.0f };
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetResolutionScaleValueEx(RenderPercent[Preset]);
            Settings->ApplySettings(false);
        }
    }

    // Method IDs: 0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Mobile,
    // 4 Nearest, 5 Bilinear. External algorithms are enabled only if
    // their actual plugin CVar is registered in this engine build.
    static void SetUpscaler(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const int32 Method = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 5);

        if (Method == 4 || Method == 5)
        {
            SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR.Enabled"), 0);
            SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR2.Enabled"), 0);
            SetCVarIntIfPresent(TEXT("bmi.NativeUpscaleMethod"), Method == 4 ? 0 : 1);
            SetCVarIntIfPresent(TEXT("bmi.UseNativeSpatialUpscaler"), 1);
            return;
        }

        SetCVarIntIfPresent(TEXT("bmi.UseNativeSpatialUpscaler"), 0);
        if (Method == 0)
        {
            if (!SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR.Enabled"), 1))
                UE_LOG(LogTemp, Error, TEXT("SGSR1 selected but Qualcomm SGSR1 is not linked into this build."));
            SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR2.Enabled"), 0);
            return;
        }
        if (Method == 1)
        {
            if (!SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR2.Enabled"), 1))
                UE_LOG(LogTemp, Error, TEXT("SGSR2 selected but Qualcomm SGSR2 is not linked into this build."));
            SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR.Enabled"), 0);
            return;
        }
        UE_LOG(LogTemp, Error, TEXT("MWSR / FSR Mobile are not implemented in this engine build; selection rejected."));
    }

    static void SetUpscaleSharpness(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const float Sharpness = 1.0f + FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f);
        // Map the user slider 0..1 to SGSR1's documented 1..2 edge-strength range.
        if (!SetCVarFloatIfPresent(TEXT("r.Qualcomm.SGSR.Sharpness"), Sharpness))
            UE_LOG(LogTemp, Warning, TEXT("Selected upscaler exposes no runtime sharpness CVar in this build."));
    }

    static void SetCAS(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const bool bEnable = FCString::Atoi(*Args[0]) != 0;
        // A toggle is accepted only when the renderer CAS pass has registered.
        if (!SetCVarIntIfPresent(TEXT("bmi.CAS.Enabled"), bEnable ? 1 : 0))
            UE_LOG(LogTemp, Error, TEXT("AMD CAS pass is not registered in this build; refusing a fake toggle."));
    }

    static void SetCASSharpness(const TArray<FString>& Args)
    {
        if (Args.Num() != 1) return;
        const float Sharpness = FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f);
        if (!SetCVarFloatIfPresent(TEXT("bmi.CAS.Sharpness"), Sharpness))
            UE_LOG(LogTemp, Error, TEXT("AMD CAS pass is not registered; sharpness was not applied."));
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
        UE_LOG(LogTemp, Error, TEXT("Ultra (No Queue) is unavailable: no validated Vulkan queue-depth control exists for this build."));
    }

    static FAutoConsoleCommand CmdResolutionController(TEXT("bmi.SetResolutionController"), TEXT("Set native/540P..1080P mobile content scale option 0..6."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetResolutionController));
    static FAutoConsoleCommand CmdFrameLimit(TEXT("bmi.SetFrameLimit"), TEXT("Set UE4 frame limit 30,45,60,90,120."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameLimit));
    static FAutoConsoleCommand CmdRenderScale(TEXT("bmi.SetRenderScale"), TEXT("Set render scale percent."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetRenderScale));
    static FAutoConsoleCommand CmdGraphicsQuality(TEXT("bmi.SetGraphicsQuality"), TEXT("Set UE4 scalability quality 0..4."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetGraphicsQuality));
    static FAutoConsoleCommand CmdUpscalePreset(TEXT("bmi.SetUpscalePreset"), TEXT("Set unified upscaler preset 0..4."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscalePreset));
    static FAutoConsoleCommand CmdUpscaler(TEXT("bmi.SetUpscaler"), TEXT("Select upscaler: 0 SGSR1, 1 SGSR2, 2 MWSR, 3 FSR Lite, 4 Nearest, 5 Bilinear."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscaler));
    static FAutoConsoleCommand CmdUpscaleSharpness(TEXT("bmi.SetUpscaleSharpness"), TEXT("Set algorithm sharpness 0..1 when supported."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetUpscaleSharpness));
    static FAutoConsoleCommand CmdCAS(TEXT("bmi.SetCAS"), TEXT("Enable registered AMD CAS renderer pass."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetCAS));
    static FAutoConsoleCommand CmdCASSharpness(TEXT("bmi.SetCASSharpness"), TEXT("Set registered AMD CAS sharpness 0..1."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetCASSharpness));
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
