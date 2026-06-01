#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorFunctionsSectionWidget.generated.h"

class UBorder;
class UButton;
class UInspectorFunctionItem;
class UInspectorFunctionRowWidget;
class UInspectorWorldSubsystem;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorFunctionsSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorFunctionsSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetAutoRefreshOnConstruct(bool bInAutoRefreshOnConstruct) { bAutoRefreshOnConstruct = bInAutoRefreshOnConstruct; }
    void RefreshFromSubsystem();
    void RefreshFromSubsystemDeferred();
    void CancelDeferredRefresh();
    bool FlushDeferredRefreshForAutomation(int32 MaxIterations = 128);
    bool IsDeferredRefreshPendingForAutomation() const { return bDeferredRefreshPending; }
    int32 GetEntryWidgetCountForAutomation() const;
    UInspectorFunctionRowWidget* FindFunctionRowForAutomation(const UInspectorFunctionItem* Item) const;
    bool ScrollToItemForAutomation(UInspectorFunctionItem* Item);

    bool HasFunctionsSection() const { return FunctionsSectionBorder != nullptr; }
    bool HasFunctionScrollRoot() const { return FunctionsScrollBox != nullptr; }
    bool HasTouchScrollSupportForAutomation() const;
    bool HasFocusSummary() const { return FocusSummaryText != nullptr; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    UWidget* CreateSectionTitle(const FString& InTitle);
    UWidget* CreateFunctionRow(UInspectorFunctionItem* Item);
    void ScheduleDeferredRefreshTick();
    void ProcessDeferredRefresh(int32 Serial);
    bool BuildNextDeferredFunctionBatch();
    void FinishDeferredRefresh();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    UPROPERTY(Transient)
    UBorder* FunctionsSectionBorder = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* RootBox = nullptr;

    UPROPERTY(Transient)
    UTextBlock* FocusSummaryText = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* FunctionsEntriesBox = nullptr;

    UPROPERTY(Transient)
    UScrollBox* FunctionsScrollBox = nullptr;

    int32 DeferredRefreshSerial = 0;
    bool bAutoRefreshOnConstruct = true;
    bool bDeferredRefreshPending = false;
    bool bDeferredRefreshCollectPending = false;
    TArray<TWeakObjectPtr<UInspectorFunctionItem>> DeferredFunctionItems;
    int32 DeferredFunctionIndex = 0;
};
