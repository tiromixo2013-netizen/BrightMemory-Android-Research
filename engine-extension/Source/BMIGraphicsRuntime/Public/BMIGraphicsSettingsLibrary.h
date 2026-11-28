// Copyright (c) Bright Memory Android Research contributors.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BMIGraphicsSettingsLibrary.generated.h"

// Blueprint-callable entry points so the game's UMG settings screen can drive
// the runtime bridge without issuing console text from Blueprint graphs.
UCLASS()
class BMIGRAPHICSRUNTIME_API UBMIGraphicsSettingsLibrary final : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetResolutionController(int32 Choice);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetGraphicsQuality(int32 QualityLevel);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetRenderScale(float Percent);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetUpscaler(int32 Method);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetUpscaleSharpness(float Sharpness);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetUpscalePreset(int32 Preset);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetAMDCAS(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetCASSharpness(float Sharpness);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetFramePacing(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetLowLatency(int32 Mode);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetPerformanceHUD(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetFrameLimit(int32 FramesPerSecond);

    // 0=Off, 1=Qualcomm AFME x2. x3/x4 are intentionally unsupported.
    UFUNCTION(BlueprintCallable, Category="Bright Memory|Graphics")
    static bool SetFrameGeneration(int32 Mode);
};
