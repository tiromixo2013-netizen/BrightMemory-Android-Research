//============================================================================================================
//
//
//                  Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
//                              SPDX-License-Identifier: BSD-3-Clause
//
//============================================================================================================
#include "SGSRSpatialUpscaling.h"
#include "LogSGSR.h"

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"

IMPLEMENT_MODULE(FSGSRSpatialUpscalingModule, SGSRSpatialUpscaling)

DEFINE_LOG_CATEGORY(LogSGSR);

void FSGSRSpatialUpscalingModule::StartupModule()
{
	const TSharedPtr<IPlugin> SGSRPlugin = IPluginManager::Get().FindPlugin(TEXT("SGSR"));
	if (!SGSRPlugin.IsValid())
	{
		UE_LOG(LogSGSR, Error, TEXT("SGSR plugin descriptor was not found. Shader directory mapping was not registered."));
		return;
	}

	const FString PluginShaderDir = FPaths::Combine(SGSRPlugin->GetBaseDir(), TEXT("Shaders"));
	if (!FPaths::DirectoryExists(PluginShaderDir))
	{
		UE_LOG(LogSGSR, Error, TEXT("SGSR shader directory does not exist: %s"), *PluginShaderDir);
		return;
	}

	AddShaderSourceDirectoryMapping(TEXT("/Plugin/SGSR"), PluginShaderDir);
	UE_LOG(LogSGSR, Log, TEXT("SGSR Spatial Upscaling Module Started; shader root: %s"), *PluginShaderDir);
}

void FSGSRSpatialUpscalingModule::ShutdownModule()
{
	UE_LOG(LogSGSR, Log, TEXT("SGSR Spatial Upscaling Module Shutdown"));
}
