#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "InspectorTypes.h"
#include "Components/TextBlock.h"
#include "InspectorTestPageWidget.generated.h"

class UButton;
class UComboBoxString;
class UInspectorWorldSubsystem;
class UScrollBox;
class UTextBlock;
class UEditableTextBox;
class UVerticalBox;

namespace RICompactUI
{
    enum class ERISectionVisualStyle : uint8;
}

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorTestPageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorTestPageWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    int32 GetRenderedWorkflowRowCount() const;
    bool HasWorkflowSelectionRow() const;
    bool HasRenderedWorkflowRow(FName WorkflowId) const;
    FString GetSelectedWorkflowLabel() const { return SelectedWorkflowValueText ? SelectedWorkflowValueText->GetText().ToString() : FString(); }
    FString GetSelectedRemoteSessionLabel() const { return SelectedRemoteSessionValueText ? SelectedRemoteSessionValueText->GetText().ToString() : FString(); }
    FString GetRemoteSessionWorkflowValue() const { return RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString() : FString(); }
    FString GetRemoteSessionTargetQueryValue() const { return RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString() : FString(); }
    FString GetRoleCompareSummaryText() const { return DiagnosticsRoleCompareSummaryText ? DiagnosticsRoleCompareSummaryText->GetText().ToString() : FString(); }
    FString GetRoleCompareAvailableRoleText() const { return DiagnosticsRoleCompareAvailableRoleText ? DiagnosticsRoleCompareAvailableRoleText->GetText().ToString() : FString(); }
    FString GetRoleCompareStatsText() const { return DiagnosticsRoleCompareStatsText ? DiagnosticsRoleCompareStatsText->GetText().ToString() : FString(); }
    FString GetRoleComparePreviewText() const { return DiagnosticsRoleComparePreviewText ? DiagnosticsRoleComparePreviewText->GetText().ToString() : FString(); }
    FString GetSessionCompareSummaryText() const { return DiagnosticsSessionCompareSummaryText ? DiagnosticsSessionCompareSummaryText->GetText().ToString() : FString(); }
    FString GetSessionComparePairText() const { return DiagnosticsSessionComparePairText ? DiagnosticsSessionComparePairText->GetText().ToString() : FString(); }
    FString GetSessionCompareStatsText() const { return DiagnosticsSessionCompareStatsText ? DiagnosticsSessionCompareStatsText->GetText().ToString() : FString(); }
    FString GetSessionComparePreviewText() const { return DiagnosticsSessionComparePreviewText ? DiagnosticsSessionComparePreviewText->GetText().ToString() : FString(); }
    UButton* GetNamedButton(const FName& Name) const;
    bool HasPageScrollRoot() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_TestPageScroll")) != nullptr; }
    bool HasRemoteSessionSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_RemoteSessionSelectionRow")) != nullptr; }
    bool HasAvailableWorkflowSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_WorkflowDefinitionsScroll")) != nullptr; }
    bool HasAvailableTestsSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_TestDefinitionsScroll")) != nullptr; }
    bool HasReportSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_TestReportScroll")) != nullptr; }
    bool HasDiagnosticsSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_ToolsDiagnosticsBorder")) != nullptr; }
    bool HasActivityLogSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_ToolsActivityLogBorder")) != nullptr; }
    bool HasRemoteOverrideSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_RemoteSessionOverrideBox")) != nullptr; }
    bool IsTestsSectionExpanded() const { return AvailableTestsSectionWidget && AvailableTestsSectionWidget->GetVisibility() == ESlateVisibility::Visible; }
    bool IsRemoteSessionSectionExpanded() const { return RemoteSessionSectionWidget && RemoteSessionSectionWidget->GetVisibility() == ESlateVisibility::Visible; }
    bool IsDiagnosticsSectionExpanded() const { return DiagnosticsSectionWidget && DiagnosticsSectionWidget->GetVisibility() == ESlateVisibility::Visible; }
    bool IsActivityLogSectionExpanded() const { return ActivityLogSectionWidget && ActivityLogSectionWidget->GetVisibility() == ESlateVisibility::Visible; }
    bool IsRemoteOverrideSectionExpanded() const { return RemoteOverrideSectionWidget && RemoteOverrideSectionWidget->GetVisibility() == ESlateVisibility::Visible; }
    void ApplyPresentationCollapsedState(bool bCollapseTests, bool bCollapseRemote, bool bCollapseDiagnostics, bool bCollapseOverride, bool bCollapseActivityLog = true);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|SelfTest")
    void RefreshFromSubsystem();

    void HandleWorkflowSelected(FName WorkflowId);
    void HandleWorkflowRunRequested(FName WorkflowId);
    void HandleSelfTestSelected(FName TestId);
    void HandleSelfTestRunRequested(FName TestId);
    void HandleSelfTestResultSelected(FName TestId);

    UFUNCTION()
    void HandleRemoteSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    UWidget* CreateRemoteSessionSection();
    UWidget* CreateDiagnosticsSection();
    UWidget* CreateActivityLogSection();
    UWidget* CreateSectionTitle(const FString& InTitle, bool bEmphasis = false);
    UWidget* CreateCollapsibleSectionHeader(const FString& InTitle, UTextBlock*& OutToggleText, const FName& ButtonName);
    UWidget* CreateInfoRow(const FString& Label, UTextBlock*& OutValueText, bool bWrapValue = false, const FName& WidgetName = NAME_None);
    UWidget* CreateAvailableWorkflowRow(const FRIWorkflowDefinition& Definition);
    UWidget* CreateAvailableTestRow(const FRISelfTestDefinition& Definition);
    UWidget* CreateResultRow(const FRISelfTestResult& Result);
    void RebuildAvailableWorkflows();
    void RebuildAvailableTests();
    void RebuildResults();
    void RebuildActivityLog();
    void UpdateUIFromState();
    void SetSectionExpanded(UWidget* ContentWidget, UTextBlock* ToggleText, bool bExpanded, const FString& ExpandedLabel, const FString& CollapsedLabel);
    void SetStatusMessage(const FString& InMessage, bool bIsError);
    void ClearStatusMessage();
    void PullRemoteSessionContextFromSubsystem();
    void PushRemoteSessionContextToSubsystem();
    bool RunSingleWorkflow(FName WorkflowId);
    bool RunSingleTest(FName TestId);
    void BindSelectWorkflowButton(UButton* Button, FName WorkflowId);
    void BindRunWorkflowButton(UButton* Button, FName WorkflowId);
    void BindSelectTestButton(UButton* Button, FName TestId);
    void BindRunTestButton(UButton* Button, FName TestId);
    void BindResultViewButton(UButton* Button, FName TestId);
    void RefreshRemoteSessionSelection();
    bool EnsureRemoteSessionConnected();
    FText GetSelectedWorkflowDisplayText() const;
    FText GetSelectedTestDisplayText() const;
    const FRIWorkflowDefinition* FindWorkflowById(FName WorkflowId) const;
    const FRISelfTestResult* FindResultById(FName TestId) const;

    UFUNCTION()
    void HandleRunSelectedWorkflowClicked();

    UFUNCTION()
    void HandleSelectSafePatchWorkflowClicked();

    UFUNCTION()
    void HandleSelectPromoteWorkflowClicked();

    UFUNCTION()
    void HandleSelectActorPatchWorkflowClicked();

    UFUNCTION()
    void HandleSelectActorPromoteWorkflowClicked();

    UFUNCTION()
    void HandleSelectActorApplyWorkflowClicked();

    UFUNCTION()
    void HandleSelectActorEndToEndWorkflowClicked();

    UFUNCTION()
    void HandleSelectFullClosureWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemotePackagedFoundationWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemotePackagedPatchPullWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemotePackagedToSourceClosureWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemotePackagedMatrixWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteActorEndToEndWorkflowClicked();

    UFUNCTION()
    void HandleSelectRoleCompareFoundationWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteRuntimeFoundationWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionCompareWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionTargetSetCompareWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionCompareUIWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionCompareScopedUIWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionCompareMatrixWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteSessionContextUIWorkflowClicked();

    UFUNCTION()
    void HandleSelectRemoteWorkflowMatrixWorkflowClicked();

    UFUNCTION()
    void HandleRunSafePatchWorkflowClicked();

    UFUNCTION()
    void HandleRunPromoteWorkflowClicked();

    UFUNCTION()
    void HandleRunActorPatchWorkflowClicked();

    UFUNCTION()
    void HandleRunActorPromoteWorkflowClicked();

    UFUNCTION()
    void HandleRunActorApplyWorkflowClicked();

    UFUNCTION()
    void HandleRunActorEndToEndWorkflowClicked();

    UFUNCTION()
    void HandleRunFullClosureWorkflowClicked();

    UFUNCTION()
    void HandleRunRemotePackagedFoundationWorkflowClicked();

    UFUNCTION()
    void HandleRunRemotePackagedPatchPullWorkflowClicked();

    UFUNCTION()
    void HandleRunRemotePackagedToSourceClosureWorkflowClicked();

    UFUNCTION()
    void HandleRunRemotePackagedMatrixWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteActorEndToEndWorkflowClicked();

    UFUNCTION()
    void HandleRunRoleCompareFoundationWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteRuntimeFoundationWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionCompareWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionTargetSetCompareWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionCompareUIWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionCompareScopedUIWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionCompareMatrixWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteSessionContextUIWorkflowClicked();

    UFUNCTION()
    void HandleRunRemoteWorkflowMatrixWorkflowClicked();

    UFUNCTION()
    void HandleRunSelectedClicked();

    UFUNCTION()
    void HandleRunAllClicked();

    UFUNCTION()
    void HandleRefreshClicked();

    UFUNCTION()
    void HandleToggleTestsSectionClicked();

    UFUNCTION()
    void HandleToggleRemoteSectionClicked();

    UFUNCTION()
    void HandleToggleDiagnosticsSectionClicked();

    UFUNCTION()
    void HandleToggleActivityLogSectionClicked();

    UFUNCTION()
    void HandleToggleRemoteOverrideSectionClicked();

    UFUNCTION()
    void HandleSelectConfirmDialogClicked();

    UFUNCTION()
    void HandleSelectSettingsPreviewClicked();

    UFUNCTION()
    void HandleSelectSettingsHotkeyClicked();

    UFUNCTION()
    void HandleRunConfirmDialogClicked();

    UFUNCTION()
    void HandleRunSettingsPreviewClicked();

    UFUNCTION()
    void HandleRunSettingsHotkeyClicked();

    UFUNCTION()
    void HandleViewConfirmDialogResultClicked();

    UFUNCTION()
    void HandleViewSettingsPreviewResultClicked();

    UFUNCTION()
    void HandleViewSettingsHotkeyResultClicked();

    UFUNCTION()
    void HandleRemoteSessionRefreshClicked();

    UFUNCTION()
    void HandleRemoteSessionConnectClicked();

    UFUNCTION()
    void HandleRemoteSessionRunWorkflowClicked();

    UFUNCTION()
    void HandleRemoteSessionWorkflowCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRemoteSessionTargetQueryCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleBuildDiagnosticsRoleCompareClicked();

    UFUNCTION()
    void HandleBuildDiagnosticsSessionCompareClicked();

    UFUNCTION()
    void HandleClearActivityLogClicked();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    TArray<FRIWorkflowDefinition> AvailableWorkflows;
    TArray<FRISelfTestDefinition> AvailableTests;
    TArray<FRISelfTestResult> Results;
    TArray<FRIRuntimeSessionInfo> AvailableRemoteSessions;
    FRIWorkflowRunResult LastWorkflowRunResult;
    FName SelectedWorkflowId = NAME_None;
    FName SelectedTestId = NAME_None;
    FName SelectedResultId = NAME_None;
    FString SelectedRemoteSessionId;
    bool bViewingWorkflowReport = false;

    bool bRunning = false;
    bool bStatusIsError = false;
    FString StatusMessage;

    UPROPERTY(Transient)
    UVerticalBox* AvailableWorkflowsBox = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* AvailableTestsBox = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* ResultsBox = nullptr;

    UPROPERTY(Transient)
    UScrollBox* AvailableWorkflowsScroll = nullptr;

    UPROPERTY(Transient)
    UScrollBox* AvailableTestsScroll = nullptr;

    UPROPERTY(Transient)
    UScrollBox* ResultsScroll = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedWorkflowValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedTestValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedRemoteSessionValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* StatusMessageText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedResultStateText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedResultReportText = nullptr;

    UPROPERTY(Transient)
    UButton* RunSelectedWorkflowButton = nullptr;

    UPROPERTY(Transient)
    UButton* RunSelectedButton = nullptr;

    UPROPERTY(Transient)
    UButton* RunAllButton = nullptr;

    UPROPERTY(Transient)
    UButton* RefreshButton = nullptr;

    UPROPERTY(Transient)
    UWidget* RemoteSessionSectionWidget = nullptr;

    UPROPERTY(Transient)
    UWidget* AvailableTestsSectionWidget = nullptr;

    UPROPERTY(Transient)
    UWidget* DiagnosticsSectionWidget = nullptr;

    UPROPERTY(Transient)
    UWidget* ActivityLogSectionWidget = nullptr;

    UPROPERTY(Transient)
    UWidget* RemoteOverrideSectionWidget = nullptr;

    UPROPERTY(Transient)
    UTextBlock* TestsSectionToggleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* RemoteSectionToggleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsSectionToggleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* ActivityLogSectionToggleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* RemoteOverrideToggleText = nullptr;

    UPROPERTY(Transient)
    UComboBoxString* RemoteSessionComboBox = nullptr;

    UPROPERTY(Transient)
    UEditableTextBox* RemoteSessionWorkflowBox = nullptr;

    UPROPERTY(Transient)
    UEditableTextBox* RemoteSessionTargetQueryBox = nullptr;

    UPROPERTY(Transient)
    UButton* RemoteSessionRefreshButton = nullptr;

    UPROPERTY(Transient)
    UButton* RemoteSessionConnectButton = nullptr;

    UPROPERTY(Transient)
    UButton* RemoteSessionRunWorkflowButton = nullptr;

    UPROPERTY(Transient)
    UButton* DiagnosticsRoleCompareButton = nullptr;

    UPROPERTY(Transient)
    UButton* DiagnosticsSessionCompareButton = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsRoleCompareSummaryText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsRoleCompareAvailableRoleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsRoleCompareStatsText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsRoleComparePreviewText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsSessionCompareSummaryText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsSessionComparePairText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsSessionCompareStatsText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DiagnosticsSessionComparePreviewText = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* ActivityLogEntriesBox = nullptr;

    UPROPERTY(Transient)
    UButton* ActivityLogClearButton = nullptr;

    bool bTestsSectionExpanded = false;
    bool bRemoteSectionExpanded = false;
    bool bDiagnosticsSectionExpanded = false;
    bool bActivityLogSectionExpanded = false;
    bool bRemoteOverrideSectionExpanded = false;
};
