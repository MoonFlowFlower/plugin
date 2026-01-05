#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InspectorBPLibrary.generated.h"

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    static void ToggleInspector(UObject* WorldContextObject);
};
