#include "InspectorBPLibrary.h"
#include "InspectorWorldSubsystem.h"
#include "Engine/World.h"

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
