#include "InspectorBPLibrary.h"
#include "InspectorWorldSubsystem.h"
#include "Engine/World.h"

UInspectorWorldSubsystem* UInspectorBPLibrary::GetInspectorSubsystem(UObject* WorldContextObject)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UInspectorWorldSubsystem>();
#else
    return nullptr;
#endif
}

void UInspectorBPLibrary::ToggleInspector(UObject* WorldContextObject)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!WorldContextObject) return;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    if (UInspectorWorldSubsystem* Sub = World->GetSubsystem<UInspectorWorldSubsystem>())
    {
        Sub->Toggle();
    }
#endif
}
