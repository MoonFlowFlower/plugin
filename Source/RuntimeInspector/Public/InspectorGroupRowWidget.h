#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorGroupRowWidget.generated.h"

class UButton;
class UHorizontalBox;
class UInspectorGroupItem;
class UInspectorWorldSubsystem;
class UTextBlock;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorGroupRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorGroupRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetGroupItem(UInspectorGroupItem* InItem);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    bool CanExpandItem() const;

    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY(Transient)
    TObjectPtr<UButton> RootButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ExpanderText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText = nullptr;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorGroupItem> GroupItem;
};
