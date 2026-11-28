// Copyright (c) Bright Memory Android Research contributors.
#pragma once

#include "CoreMinimal.h"

// Renderer-owned capability gate for frame generation. This module deliberately
// does not claim a backend is available until the renderer adapter registers it.
namespace BMIFrameGeneration
{
    enum class EBackend : int32
    {
        Off = 0,
        QualcommAFME = 1,
        GLESReprojection = 2
    };

    enum EMultiplierMask : uint32
    {
        MultiplierX2 = 1u << 0,
        MultiplierX3 = 1u << 1,
        MultiplierX4 = 1u << 2
    };

    // Called by a real renderer adapter only after it has verified all required
    // inputs and presentation support. A shader file existing is not sufficient.
    void SetBackendCapabilities(EBackend Backend, uint32 SupportedMultipliers);
    bool IsBackendAvailable(EBackend Backend, int32 Multiplier);
    bool Select(EBackend Backend, int32 Multiplier);
}
