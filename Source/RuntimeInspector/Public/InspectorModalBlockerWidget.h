#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorModalBlockerWidget.generated.h"

class UButton;
class UCanvasPanel;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorModalBlockerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorModalBlockerWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleBlockerClicked();

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BlockerButton = nullptr;
};
