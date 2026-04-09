#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "InspectorTypes.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

#include "InspectorFilePageWidget.generated.h"

class UButton;
class UComboBoxString;
class UEditableTextBox;
class UScrollBox;
class UVerticalBox;
class UInspectorWorldSubsystem;
struct FRIAuditLine;
struct FRIAuditReport;
struct FRIFileManagementSummary;
struct FRIRuntimeRoleCompareLine;
struct FRIRuntimeRoleCompareReport;
struct FRIRuntimeSessionTargetSetCompareLine;
struct FRIRuntimeSessionTargetSetCompareReport;

namespace RICompactUI
{
    enum class ERISectionVisualStyle : uint8;
}

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorFilePageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    explicit UInspectorFilePageWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    FString GetCompareDebugSummary() const;
    int32 GetRenderedCompareLineCount() const { return RenderedAuditLineCount; }
    int32 GetRenderedCompareDifferentCount() const { return RenderedAuditDifferentCount; }
    FString GetRenderedFirstAuditField() const { return RenderedFirstAuditField; }
    FString GetCompareModeLabel() const { return AuditModeText.Get() ? AuditModeText.Get()->GetText().ToString() : FString(); }
    FString GetComparePairLabel() const { return AuditPairText.Get() ? AuditPairText.Get()->GetText().ToString() : FString(); }
    FString GetComparePreviewText() const { return AuditPreviewText.Get() ? AuditPreviewText.Get()->GetText().ToString() : FString(); }
    FString GetCompareCacheLabel() const { return AuditCacheText.Get() ? AuditCacheText.Get()->GetText().ToString() : FString(); }
    FString GetRoleCompareSummaryText() const { return RoleCompareSummaryText.Get() ? RoleCompareSummaryText.Get()->GetText().ToString() : FString(); }
    FString GetRoleCompareStatsText() const { return RoleCompareStatsText.Get() ? RoleCompareStatsText.Get()->GetText().ToString() : FString(); }
    FString GetRoleComparePreviewText() const { return RoleComparePreviewText.Get() ? RoleComparePreviewText.Get()->GetText().ToString() : FString(); }
    FString GetRoleCompareAvailableRoleLabel() const { return RoleCompareAvailableRoleText.Get() ? RoleCompareAvailableRoleText.Get()->GetText().ToString() : FString(); }
    int32 GetRenderedRoleCompareLineCount() const { return RenderedRoleCompareLineCount; }
    int32 GetRenderedRoleCompareMissingCount() const { return RenderedRoleCompareMissingCount; }
    FString GetRemoteSessionCompareSummaryText() const { return RemoteSessionCompareSummaryText.Get() ? RemoteSessionCompareSummaryText.Get()->GetText().ToString() : FString(); }
    FString GetRemoteSessionCompareSessionsText() const { return RemoteSessionCompareSessionsText.Get() ? RemoteSessionCompareSessionsText.Get()->GetText().ToString() : FString(); }
    FString GetRemoteSessionCompareStatsText() const { return RemoteSessionCompareStatsText.Get() ? RemoteSessionCompareStatsText.Get()->GetText().ToString() : FString(); }
    FString GetRemoteSessionComparePreviewText() const { return RemoteSessionComparePreviewText.Get() ? RemoteSessionComparePreviewText.Get()->GetText().ToString() : FString(); }
    int32 GetRenderedRemoteSessionCompareLineCount() const { return RenderedRemoteSessionCompareLineCount; }
    int32 GetRenderedRemoteSessionCompareMismatchCount() const { return RenderedRemoteSessionCompareMismatchCount; }
    FString GetRuntimeSessionLabel() const { return RuntimeSessionText.Get() ? RuntimeSessionText.Get()->GetText().ToString() : FString(); }
    FString GetSelectedRoleLabel() const { return SelectedRoleText.Get() ? SelectedRoleText.Get()->GetText().ToString() : FString(); }
    FString GetSelectedActorSummaryLabel() const { return SelectedActorSummaryText.Get() ? SelectedActorSummaryText.Get()->GetText().ToString() : FString(); }
    FString GetSelectedActorClassLabel() const { return SelectedActorClassText.Get() ? SelectedActorClassText.Get()->GetText().ToString() : FString(); }
    FString GetSelectedActorSourceLabel() const { return SelectedActorSourcePathText.Get() ? SelectedActorSourcePathText.Get()->GetText().ToString() : FString(); }
    UButton* GetNamedButton(const FName& Name) const;
    FString GetNextStepLabel() const { return NextStepText.Get() ? NextStepText.Get()->GetText().ToString() : FString(); }
    FString GetActionGuideLabel() const { return ActionGuideText.Get() ? ActionGuideText.Get()->GetText().ToString() : FString(); }
    FString GetSelectedRemoteSessionLabel() const { return RemoteSessionSelectionText.Get() ? RemoteSessionSelectionText.Get()->GetText().ToString() : FString(); }
    FString GetRemoteSessionTargetQueryValue() const { return RemoteSessionTargetQueryBox.Get() ? RemoteSessionTargetQueryBox.Get()->GetText().ToString() : FString(); }
    FString GetRemoteSessionWorkflowValue() const { return RemoteSessionWorkflowBox.Get() ? RemoteSessionWorkflowBox.Get()->GetText().ToString() : FString(); }
    bool HasPageScrollRoot() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_FilePageScroll")) != nullptr; }
    bool HasDiagnosticsSection() const { return WidgetTree && WidgetTree->FindWidget(TEXT("RI_FileDiagnosticsSectionBody")) != nullptr; }
    bool HasRemoteSessionSection() const
    {
        return WidgetTree
            && (WidgetTree->FindWidget(TEXT("RI_RemoteSessionSelectionRow")) != nullptr
                || WidgetTree->FindWidget(TEXT("RI_RemoteSessionPickerRow")) != nullptr);
    }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    void RefreshFromSubsystem();

    void RefreshFastFromSubsystem();
    void ScheduleDeferredRefresh(bool bForceSessionRefresh = false, bool bForceTargetQueryRefresh = true);
    void CancelDeferredRefresh();
    void ApplyPresentationCollapsedState(bool bCollapseAudits, bool bCollapsePresets, bool bCollapseDiagnostics);
    bool IsAuditsSectionExpanded() const { return AuditsSectionBody && AuditsSectionBody->GetVisibility() == ESlateVisibility::Visible; }
    bool IsPresetsSectionExpanded() const { return PresetsSectionBody && PresetsSectionBody->GetVisibility() == ESlateVisibility::Visible; }
    bool IsDiagnosticsSectionExpanded() const { return DiagnosticsSectionBody && DiagnosticsSectionBody->GetVisibility() == ESlateVisibility::Visible; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BuildWidgetTree();
    UWidget* CreateSectionTitle(const FString& InTitle, bool bEmphasis = false);
    UWidget* CreateSelectionContextSection();
    UWidget* CreateMainActionsSection();
    UWidget* CreateStatusSection();
    UWidget* CreateAuditsSection();
    UWidget* CreatePresetsSection();
    UWidget* CreateDiagnosticsSection();
    UWidget* CreateCollapsibleSectionHeader(const FString& InTitle, TObjectPtr<UTextBlock>& OutToggleText, const FName& ButtonName);
    UWidget* CreateAuditLineCard(const FRIAuditLine& InLine, int32 DisplayIndex);
    UWidget* CreateRoleCompareLineCard(const FRIRuntimeRoleCompareLine& InLine, int32 DisplayIndex);
    UWidget* CreateRemoteSessionCompareLineCard(const FRIRuntimeSessionTargetSetCompareLine& InLine, int32 DisplayIndex);
    UWidget* CreateInfoRow(const FString& Label, TObjectPtr<UTextBlock>& OutValueText, bool bWrapValue = false, const FName& WidgetName = NAME_None);
    UWidget* CreateCountCell(const FString& Label, TObjectPtr<UTextBlock>& OutValueText);
    UButton* CreateActionButton(const FString& Label, const FName& Name, float Width = 78.f);
    void SetValueText(UTextBlock* TextWidget, const FString& InText, bool bIsError = false) const;
    void SetSectionExpanded(UVerticalBox* SectionBody, UTextBlock* ToggleText, bool bExpanded, const FString& ExpandedLabel, const FString& CollapsedLabel);
    void RebuildAuditLineCards(const FRIAuditReport& InReport);
    void RebuildRoleCompareLineCards(const FRIRuntimeRoleCompareReport& InReport);
    void RebuildRemoteSessionCompareLineCards(const FRIRuntimeSessionTargetSetCompareReport& InReport);
    void RefreshFromSubsystemInternal(bool bForceSessionRefresh, bool bForceTargetQueryRefresh);
    void ApplyFileSummaryToWidgets(const FRIFileManagementSummary* Summary);
    void ApplyPrimaryActionGuidance(const AActor* SelectedActor, const FRIFileManagementSummary* Summary);
    void ApplyPrimaryActionButtonStates(const AActor* SelectedActor, const FRIFileManagementSummary* Summary);
    void PullRemoteSessionContextFromSubsystem();
    void PushRemoteSessionContextToSubsystem();
    void UpdateRemoteSessionSelectionState();

    UFUNCTION()
    void HandleDeferredRefreshTimerElapsed();

    UFUNCTION()
    void HandleRefreshClicked();

    UFUNCTION()
    void HandleToggleAuditsSectionClicked();

    UFUNCTION()
    void HandleTogglePresetsSectionClicked();

    UFUNCTION()
    void HandleToggleDiagnosticsSectionClicked();

    UFUNCTION()
    void HandleStagePatchClicked();

    UFUNCTION()
    void HandleExportPatchClicked();

    UFUNCTION()
    void HandleSavePresetClicked();

    UFUNCTION()
    void HandleApplyLatestPresetClicked();

    UFUNCTION()
    void HandleBuildBaselineAuditClicked();

    UFUNCTION()
    void HandleBuildAuditClicked();

    UFUNCTION()
    void HandleBuildPatchVsSourceAuditClicked();

    UFUNCTION()
    void HandleBuildAppliedAuditClicked();

    UFUNCTION()
    void HandleBuildRuntimeRoleCompareClicked();

    UFUNCTION()
    void HandleBuildRemoteSessionCompareClicked();

    UFUNCTION()
    void HandleRemoteSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleRemoteSessionRefreshClicked();

    UFUNCTION()
    void HandleRemoteSessionConnectClicked();

    UFUNCTION()
    void HandleRemoteSessionPullPatchClicked();

    UFUNCTION()
    void HandleRemoteSessionRunWorkflowClicked();

    UFUNCTION()
    void HandleRemoteSessionTargetQueryCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRemoteSessionWorkflowCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleViewBaselineClicked();

    UFUNCTION()
    void HandleViewCurrentPatchClicked();

    UFUNCTION()
    void HandleViewPatchSourceClicked();

    UFUNCTION()
    void HandleViewAppliedClicked();

    UFUNCTION()
    void HandlePreviewPromoteClicked();

    UFUNCTION()
    void HandlePromoteApplyClicked();

    UFUNCTION()
    void HandleClearStagedClicked();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SnapshotCountText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PatchCountText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PresetCountText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditCountText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StagedPatchText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NextStepText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActionGuideText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RuntimeSessionText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectedRoleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectedActorSummaryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectedActorClassText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectedActorSourcePathText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PatchApplyText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PromotePreviewText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PromoteResultText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditSummaryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionCompareStatusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionSummaryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionSelectionText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionTargetQueryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionWorkflowText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditCacheText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditModeText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditPairText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditStatsText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditFirstFieldText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditPreviewText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> AuditLinesBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RoleCompareSummaryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RoleCompareAvailableRoleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RoleCompareStatsText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RoleComparePreviewText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RoleCompareLinesBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionCompareSummaryText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionCompareSessionsText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionCompareStatsText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RemoteSessionComparePreviewText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RemoteSessionCompareLinesBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LatestSnapshotText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LatestPatchFileText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LatestPresetText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LatestAuditFileText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> PageScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> AuditsSectionBody = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> PresetsSectionBody = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> DiagnosticsSectionBody = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AuditsSectionToggleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PresetsSectionToggleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DiagnosticsSectionToggleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RefreshButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> RemoteSessionComboBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> RemoteSessionTargetQueryBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> RemoteSessionWorkflowBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RemoteSessionRefreshButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RemoteSessionConnectButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RemoteSessionPullPatchButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RemoteSessionRunWorkflowButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> StagePatchButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ExportPatchButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> SavePresetButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ApplyLatestPresetButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildBaselineAuditButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildAuditButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildPatchVsSourceAuditButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildAppliedAuditButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildRuntimeRoleCompareButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BuildRemoteSessionCompareButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ViewBaselineButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ViewCurrentPatchButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ViewPatchSourceButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ViewAppliedButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PreviewPromoteButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PromoteApplyButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ClearStagedButton = nullptr;

    int32 RenderedAuditLineCount = 0;
    int32 RenderedAuditDifferentCount = 0;
    FString RenderedFirstAuditField;
    int32 RenderedRoleCompareLineCount = 0;
    int32 RenderedRoleCompareMissingCount = 0;
    int32 RenderedRemoteSessionCompareLineCount = 0;
    int32 RenderedRemoteSessionCompareMismatchCount = 0;

    TArray<FRIRuntimeSessionInfo> AvailableRemoteSessions;
    FString SelectedRemoteSessionId;
    bool bUpdatingRemoteSessionWidgets = false;
    bool bDeferredHeavyRefreshScheduled = false;
    bool bDeferredHeavyRefreshForceSessionRefresh = false;
    bool bDeferredHeavyRefreshForceTargetQueryRefresh = true;
    bool bAuditsSectionExpanded = false;
    bool bPresetsSectionExpanded = false;
    bool bDiagnosticsSectionExpanded = false;
    FTimerHandle DeferredHeavyRefreshTimerHandle;
};
