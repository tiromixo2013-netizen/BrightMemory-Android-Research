#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"

namespace BMIGraphicsRuntime
{
    static void SetFloatCVar(const TCHAR* Name, float Value)
    {
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            CVar->Set(Value, ECVF_SetByGameSetting);
        }
    }

    static void SetRenderScale(const TArray<FString>& Args)
    {
        if (Args.Num() != 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: bmi.SetRenderScale <0.50..1.50>"));
            return;
        }
        const float Scale = FMath::Clamp(FCString::Atof(*Args[0]), 0.50f, 1.50f);
        SetFloatCVar(TEXT("r.MobileContentScaleFactor"), Scale);
        UE_LOG(LogTemp, Log, TEXT("BMI render scale requested: %.2f"), Scale);
    }

    static void SetGraphicsQuality(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: bmi.SetGraphicsQuality <0..4>"));
            return;
        }
        const int32 Quality = FMath::Clamp(FCString::Atoi(*Args[0]), 0, 4);
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetOverallScalabilityLevel(Quality);
            Settings->ApplySettings(false);
            UE_LOG(LogTemp, Log, TEXT("BMI overall scalability level requested: %d"), Quality);
        }
    }

    static void SetFrameLimit(const TArray<FString>& Args)
    {
        if (Args.Num() != 1 || !GEngine)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: bmi.SetFrameLimit <30|45|60|90|120>"));
            return;
        }
        const int32 Requested = FCString::Atoi(*Args[0]);
        const int32 Allowed[] = { 30, 45, 60, 90, 120 };
        bool bSupportedValue = false;
        for (int32 Value : Allowed)
        {
            bSupportedValue |= (Requested == Value);
        }
        if (!bSupportedValue)
        {
            UE_LOG(LogTemp, Warning, TEXT("Unsupported BMI frame limit: %d"), Requested);
            return;
        }
        if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
        {
            Settings->SetFrameRateLimit(static_cast<float>(Requested));
            Settings->ApplySettings(false);
            UE_LOG(LogTemp, Log, TEXT("BMI frame limit requested: %d"), Requested);
        }
    }

    static void SetShadowQuality(const TArray<FString>& Args)
    {
        if (Args.Num() != 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: bmi.SetShadowQuality <0|1>"));
            return;
        }
        SetFloatCVar(TEXT("r.ShadowQuality"), FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f));
    }

    static void SetMobileAO(const TArray<FString>& Args)
    {
        if (Args.Num() != 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: bmi.SetMobileAO <0|1>"));
            return;
        }
        SetFloatCVar(TEXT("r.Mobile.AmbientOcclusionQuality"), FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 1.0f));
    }

    static FAutoConsoleCommand CmdRenderScale(
        TEXT("bmi.SetRenderScale"),
        TEXT("Set r.MobileContentScaleFactor through the existing UE4 mobile scale path."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetRenderScale));
    static FAutoConsoleCommand CmdGraphicsQuality(
        TEXT("bmi.SetGraphicsQuality"),
        TEXT("Set the UE4 overall scalability level (0..4)."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetGraphicsQuality));
    static FAutoConsoleCommand CmdFrameLimit(
        TEXT("bmi.SetFrameLimit"),
        TEXT("Set the UE4 GameUserSettings frame-rate limit."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetFrameLimit));
    static FAutoConsoleCommand CmdShadowQuality(
        TEXT("bmi.SetShadowQuality"),
        TEXT("Set the existing r.ShadowQuality CVar (0..1)."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetShadowQuality));
    static FAutoConsoleCommand CmdMobileAO(
        TEXT("bmi.SetMobileAO"),
        TEXT("Set the existing r.Mobile.AmbientOcclusionQuality CVar (0..1)."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&SetMobileAO));
}

class FBMIGraphicsRuntimeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogTemp, Log, TEXT("BMIGraphicsRuntime loaded. Experimental runtime controls registered."));
    }
    virtual void ShutdownModule() override
    {
        UE_LOG(LogTemp, Log, TEXT("BMIGraphicsRuntime unloaded."));
    }
};

IMPLEMENT_MODULE(FBMIGraphicsRuntimeModule, BMIGraphicsRuntime)
