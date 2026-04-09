// Copyright Epic Games, Inc. All Rights Reserved.

#include "RuntimeInspector.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "FRuntimeInspectorModule"

static TAutoConsoleVariable<int32> CVarRIThemePreset(
	TEXT("ri.ThemePreset"),
	-1,
	TEXT("RuntimeInspector theme preset override. -1=use project settings, 0=StudioSlate, 1=SoftContrast"),
	ECVF_Default
);

int32 RI_GetThemePresetOverrideValue()
{
	return CVarRIThemePreset.GetValueOnGameThread();
}

void FRuntimeInspectorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRuntimeInspectorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRuntimeInspectorModule, RuntimeInspector)
