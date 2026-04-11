#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPropertiesSectionWidget.generated.h"

class UInspectorPropertyRowWidget;
class UInspectorMaterialParamItem;
class UInspectorMaterialParamRowWidget;
class UInspectorPropertyItem;
class UInspectorWorldSubsystem;
class UBorder;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertiesSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorPropertiesSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetOnlyModified(bool bInOnlyModified);
    void RefreshFromSubsystem();
    void RefreshItemDisplay(UObject* ItemObject);
    int32 GetEntryWidgetCountForAutomation() const;
    bool HasSummaryHeader() const { return HeaderBorder != nullptr; }
    bool HasPropertyScrollRoot() const { return ScrollBox != nullptr; }
    UInspectorMaterialParamRowWidget* FindMaterialRowForAutomation(const UInspectorMaterialParamItem* Item) const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    void RefreshHeaderFromSubsystem();
    UWidget* CreatePropertyRow(UObject* ItemObject);

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> HeaderBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderActorText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderFocusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderSourceText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderStatusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> EntriesBox = nullptr;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    bool bOnlyModified = false;
};
