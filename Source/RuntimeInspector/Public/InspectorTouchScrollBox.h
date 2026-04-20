#pragma once

#include "CoreMinimal.h"
#include "Components/ScrollBox.h"
#include "InspectorTouchScrollBox.generated.h"

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorTouchScrollBox : public UScrollBox
{
    GENERATED_BODY()

public:
    UInspectorTouchScrollBox(const FObjectInitializer& ObjectInitializer);

    virtual void SynchronizeProperties() override;

    bool IsRuntimeInspectorTouchAdapterEnabled() const { return bRuntimeInspectorTouchAdapterEnabled; }
    void ApplyRuntimeInspectorDefaults();

private:
    UPROPERTY(EditAnywhere, Category = "RuntimeInspector|Scroll")
    bool bRuntimeInspectorTouchAdapterEnabled = true;
};

namespace RIInspectorTouchScroll
{
    RUNTIMEINSPECTOR_API void Configure(UScrollBox* ScrollBox);
    RUNTIMEINSPECTOR_API bool HasTouchSupport(const UScrollBox* ScrollBox);
    RUNTIMEINSPECTOR_API FString Describe(const UScrollBox* ScrollBox);
}
