// Copyright (c) Bright Memory Android Research contributors.
#include "BMIGraphicsSettingsLibrary.h"
#include "Engine/Engine.h"

namespace
{
    bool RunBMICommand(const TCHAR* Name, const FString& Value)
    {
        if (!GEngine || !Name || Value.IsEmpty())
        {
            return false;
        }
        const FString Command = FString::Printf(TEXT("%s %s"), Name, *Value);
        return GEngine->Exec(nullptr, *Command);
    }

    FString IntArg(int32 Value) { return FString::FromInt(Value); }
    FString FloatArg(float Value) { return FString::SanitizeFloat(Value); }
}

bool UBMIGraphicsSettingsLibrary::SetResolutionController(int32 Choice)
{
    if (Choice < 0 || Choice > 6) return false;
    return RunBMICommand(TEXT("bmi.SetResolutionController"), IntArg(Choice));
}

bool UBMIGraphicsSettingsLibrary::SetGraphicsQuality(int32 QualityLevel)
{
    if (QualityLevel < 0 || QualityLevel > 5) return false;
    return RunBMICommand(TEXT("bmi.SetGraphicsQuality"), IntArg(QualityLevel));
}

bool UBMIGraphicsSettingsLibrary::SetRenderScale(float Percent)
{
    if (!FMath::IsFinite(Percent) || Percent < 50.0f || Percent > 150.0f) return false;
    return RunBMICommand(TEXT("bmi.SetRenderScale"), FloatArg(Percent));
}

bool UBMIGraphicsSettingsLibrary::SetUpscaler(int32 Method)
{
    if (Method < 0 || Method > 5) return false;
    return RunBMICommand(TEXT("bmi.SetUpscaler"), IntArg(Method));
}

bool UBMIGraphicsSettingsLibrary::SetUpscaleSharpness(float Sharpness)
{
    if (!FMath::IsFinite(Sharpness) || Sharpness < 0.0f || Sharpness > 1.0f) return false;
    return RunBMICommand(TEXT("bmi.SetUpscaleSharpness"), FloatArg(Sharpness));
}

bool UBMIGraphicsSettingsLibrary::SetUpscalePreset(int32 Preset)
{
    if (Preset < 0 || Preset > 4) return false;
    return RunBMICommand(TEXT("bmi.SetUpscalePreset"), IntArg(Preset));
}

bool UBMIGraphicsSettingsLibrary::SetAMDCAS(bool bEnabled)
{
    return RunBMICommand(TEXT("bmi.SetCAS"), bEnabled ? TEXT("1") : TEXT("0"));
}

bool UBMIGraphicsSettingsLibrary::SetCASSharpness(float Sharpness)
{
    if (!FMath::IsFinite(Sharpness) || Sharpness < 0.0f || Sharpness > 1.0f) return false;
    return RunBMICommand(TEXT("bmi.SetCASSharpness"), FloatArg(Sharpness));
}

bool UBMIGraphicsSettingsLibrary::SetFramePacing(bool bEnabled)
{
    return RunBMICommand(TEXT("bmi.SetFramePacing"), bEnabled ? TEXT("1") : TEXT("0"));
}

bool UBMIGraphicsSettingsLibrary::SetLowLatency(int32 Mode)
{
    if (Mode < 0 || Mode > 2) return false;
    return RunBMICommand(TEXT("bmi.SetLowLatency"), IntArg(Mode));
}

bool UBMIGraphicsSettingsLibrary::SetFrameLimit(int32 FramesPerSecond)
{
    if (FramesPerSecond != 30 && FramesPerSecond != 45 && FramesPerSecond != 60 &&
        FramesPerSecond != 90 && FramesPerSecond != 120) return false;
    return RunBMICommand(TEXT("bmi.SetFrameLimit"), IntArg(FramesPerSecond));
}
