// Copyright (c) Bright Memory Android Research contributors.
#include "BMIFrameGeneration.h"

#include "HAL/IConsoleManager.h"

namespace BMIFrameGeneration
{
    namespace
    {
        TAutoConsoleVariable<int32> CVarMethod(
            TEXT("bmi.FrameGeneration.Method"), 0,
            TEXT("0=Off, 1=Qualcomm AFME, 2=portable GLES reprojection."),
            ECVF_RenderThreadSafe);

        TAutoConsoleVariable<int32> CVarMultiplier(
            TEXT("bmi.FrameGeneration.Multiplier"), 2,
            TEXT("Generated output multiplier: x2, x3 or x4."),
            ECVF_RenderThreadSafe);

        TAutoConsoleVariable<int32> CVarAFMECapabilities(
            TEXT("bmi.FrameGeneration.AFMEAvailable"), 0,
            TEXT("Renderer-owned AFME capability flag; set only by a verified adapter."),
            ECVF_ReadOnly);

        TAutoConsoleVariable<int32> CVarGLESCapabilities(
            TEXT("bmi.FrameGeneration.GLESAvailable"), 0,
            TEXT("Renderer-owned GLES reprojection capability flag; set only by a verified adapter."),
            ECVF_ReadOnly);

        uint32 MaskForMultiplier(int32 Multiplier)
        {
            switch (Multiplier)
            {
                case 2: return MultiplierX2;
                case 3: return MultiplierX3;
                case 4: return MultiplierX4;
                default: return 0;
            }
        }

        uint32 GAFMEMask = 0;
        uint32 GGLESMask = 0;
        FCriticalSection GCapabilitiesLock;
    }

    void SetBackendCapabilities(EBackend Backend, uint32 SupportedMultipliers)
    {
        FScopeLock Lock(&GCapabilitiesLock);
        if (Backend == EBackend::QualcommAFME)
        {
            // Qualcomm AFME 1.0 is x2-only.
            GAFMEMask = SupportedMultipliers & MultiplierX2;
            CVarAFMECapabilities->Set(GAFMEMask != 0 ? 1 : 0, ECVF_SetByCode);
        }
        else if (Backend == EBackend::GLESReprojection)
        {
            GGLESMask = SupportedMultipliers &
                (MultiplierX2 | MultiplierX3 | MultiplierX4);
            CVarGLESCapabilities->Set(GGLESMask != 0 ? 1 : 0, ECVF_SetByCode);
        }
    }

    bool IsBackendAvailable(EBackend Backend, int32 Multiplier)
    {
        const uint32 Bit = MaskForMultiplier(Multiplier);
        if (Bit == 0) return false;
        FScopeLock Lock(&GCapabilitiesLock);
        if (Backend == EBackend::QualcommAFME)
            return (GAFMEMask & Bit) != 0;
        if (Backend == EBackend::GLESReprojection)
            return (GGLESMask & Bit) != 0;
        return Backend == EBackend::Off;
    }

    bool Select(EBackend Backend, int32 Multiplier)
    {
        if (Backend == EBackend::Off)
        {
            CVarMethod->Set(0, ECVF_SetByCode);
            return true;
        }
        if (!IsBackendAvailable(Backend, Multiplier))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Frame generation selection rejected: backend %d x%d is not registered."),
                static_cast<int32>(Backend), Multiplier);
            return false;
        }
        CVarMultiplier->Set(Multiplier, ECVF_SetByCode);
        CVarMethod->Set(static_cast<int32>(Backend), ECVF_SetByCode);
        return true;
    }
}
