#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InspectorBPLibrary.generated.h"

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector", meta = (WorldContext = "WorldContextObject"))
    static class UInspectorWorldSubsystem* GetInspectorSubsystem(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    static void ToggleInspector(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Editor")
    static bool GenerateSettingsPageBlueprintLayout();
};
