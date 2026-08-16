#include "InspectorTestPageWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorTouchScrollBox.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    static const FName RI_WorkflowIdSafePatch(TEXT("mainline_safe_patch_core"));
    static const FName RI_WorkflowIdPromote(TEXT("mainline_promote_source_assets"));
    static const FName RI_WorkflowIdActorPatch(TEXT("mainline_actor_patch_roundtrip"));
    static const FName RI_WorkflowIdActorPromote(TEXT("mainline_actor_promote_file_closure"));
    static const FName RI_WorkflowIdActorApply(TEXT("mainline_actor_apply_file_closure"));
    static const FName RI_WorkflowIdActorEndToEnd(TEXT("mainline_actor_end_to_end_closure"));
    static const FName RI_WorkflowIdFullClosure(TEXT("mainline_full_closure"));
    static const FName RI_WorkflowIdRemotePackagedFoundation(TEXT("mainline_remote_packaged_foundation"));
    static const FName RI_WorkflowIdRemotePackagedPatchPull(TEXT("mainline_remote_packaged_patch_pull"));
    static const FName RI_WorkflowIdRemotePackagedToSourceClosure(TEXT("mainline_remote_packaged_to_source_closure"));
    static const FName RI_WorkflowIdRemotePackagedMatrix(TEXT("mainline_remote_packaged_matrix_default"));
    static const FName RI_WorkflowIdRemoteActorEndToEnd(TEXT("mainline_remote_actor_end_to_end_closure"));
    static const FName RI_WorkflowIdRoleCompareFoundation(TEXT("mainline_role_compare_foundation"));
    static const FName RI_WorkflowIdRemoteRuntimeFoundation(TEXT("mainline_remote_runtime_foundation"));
    static const FName RI_WorkflowIdRemoteSessionCompareFoundation(TEXT("mainline_remote_session_compare_foundation"));
    static const FName RI_WorkflowIdRemoteSessionTargetSetCompareFoundation(TEXT("mainline_remote_session_target_set_compare_foundation"));
    static const FName RI_WorkflowIdRemoteSessionCompareUIFoundation(TEXT("mainline_remote_session_compare_ui_foundation"));
    static const FName RI_WorkflowIdRemoteSessionCompareScopedUIFoundation(TEXT("mainline_remote_session_compare_scoped_ui_foundation"));
    static const FName RI_WorkflowIdRemoteSessionCompareMatrixFoundation(TEXT("mainline_remote_session_compare_matrix_foundation"));
    static const FName RI_WorkflowIdRemoteSessionContextUIFoundation(TEXT("mainline_remote_session_context_ui_foundation"));
    static const FName RI_WorkflowIdRemoteWorkflowMatrixFoundation(TEXT("mainline_remote_workflow_matrix_foundation"));
    static const FName RI_TestIdConfirmDialog(TEXT("confirm_dialog_color_input"));
    static const FName RI_TestIdSettingsPreview(TEXT("settings_preview"));
    static const FName RI_TestIdSettingsHotkey(TEXT("settings_hotkey_rebind"));

    static FLinearColor RI_TestSectionColor() { return RICompactUI::GetSectionSurfaceBackgroundColor(); }
    static FLinearColor RI_TestRowColor() { return RICompactUI::GetRowSurfaceBackgroundColor(); }
    static FLinearColor RI_TestSelectedRowColor() { return RICompactUI::GetSelectedRowSurfaceBackgroundColor(); }
    static FLinearColor RI_TestTextColor() { return RICompactUI::GetStrongTextColor(); }
    static FLinearColor RI_TestMutedTextColor() { return RICompactUI::GetMutedTextColor(); }
    static FLinearColor RI_TestWarningColor() { return RICompactUI::GetWarningTextColor(); }
    static FLinearColor RI_TestErrorColor() { return RICompactUI::GetErrorTextColor(); }
    static FLinearColor RI_TestSuccessColor() { return RICompactUI::GetSuccessTextColor(); }

    static UTextBlock* RI_MakeText(UWidgetTree* WidgetTree, const FString& InText, int32 Size, bool bBold, const FLinearColor& Color, bool bWrap = false)
    {
        return RICompactUI::MakeText(WidgetTree, InText, Size, bBold, Color, bWrap);
    }

    static bool RI_IsLocalRuntimeSessionId(const FString& SessionId)
    {
        return SessionId.StartsWith(TEXT("local_"));
    }

    static FString RI_BuildRemoteSessionLabel(const FRIRuntimeSessionInfo& SessionInfo)
    {
        FString Label = SessionInfo.DisplayName.IsEmpty() ? SessionInfo.SessionId : SessionInfo.DisplayName;
        if (!SessionInfo.SessionType.IsEmpty())
        {
            Label += FString::Printf(TEXT(" [%s]"), *SessionInfo.SessionType);
        }
        return Label;
    }

    static FString RI_BuildRemoteSessionSummaryText(const FRIRuntimeSessionInfo& SessionInfo)
    {
        FString Summary = SessionInfo.Summary;
        if (Summary.IsEmpty())
        {
            Summary = FString::Printf(
                TEXT("%s | %s | %s"),
                *RI_BuildRemoteSessionLabel(SessionInfo),
                *SessionInfo.WorldTypeLabel,
                *SessionInfo.NetModeLabel);
        }

        if (!SessionInfo.LastError.IsEmpty())
        {
            Summary += FString::Printf(TEXT(" | LastError=%s"), *SessionInfo.LastError);
        }

        return Summary;
    }

    static const FRIRuntimeSessionInfo* RI_FindRemoteSession(const TArray<FRIRuntimeSessionInfo>& Sessions, const FString& SessionId)
    {
        return Sessions.FindByPredicate([&SessionId](const FRIRuntimeSessionInfo& SessionInfo)
        {
            return SessionInfo.SessionId == SessionId;
        });
    }

    static void RI_ConfigureNamedScrollBoxTouchSupport(UWidgetTree* WidgetTree, const FName& ScrollBoxName)
    {
        if (!WidgetTree)
        {
            return;
        }

        RIInspectorTouchScroll::Configure(Cast<UScrollBox>(WidgetTree->FindWidget(ScrollBoxName)));
    }
}

void UInspectorTestPageButtonBinding::Initialize(
    UInspectorTestPageWidget* InOwner,
    FName InItemId,
    ERIInspectorTestPageButtonAction InAction)
{
    Owner = InOwner;
    ItemId = InItemId;
    Action = InAction;
}

void UInspectorTestPageButtonBinding::HandleClicked()
{
    UInspectorTestPageWidget* Page = Owner.Get();
    if (!Page || ItemId == NAME_None)
    {
        return;
    }

    switch (Action)
    {
    case ERIInspectorTestPageButtonAction::SelectWorkflow:
        Page->HandleWorkflowSelected(ItemId);
        break;
    case ERIInspectorTestPageButtonAction::RunWorkflow:
        Page->HandleWorkflowRunRequested(ItemId);
        break;
    case ERIInspectorTestPageButtonAction::SelectSelfTest:
        Page->HandleSelfTestSelected(ItemId);
        break;
    case ERIInspectorTestPageButtonAction::RunSelfTest:
        Page->HandleSelfTestRunRequested(ItemId);
        break;
    case ERIInspectorTestPageButtonAction::ViewSelfTestResult:
        Page->HandleSelfTestResultSelected(ItemId);
        break;
    default:
        break;
    }
}

UInspectorTestPageWidget::UInspectorTestPageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

bool UInspectorTestPageWidget::HasCompleteActionBindingsForAutomation() const
{
    if (ConfiguredActionBindings.Num() != GetExpectedActionBindingCountForAutomation())
    {
        return false;
    }

    return !ConfiguredActionBindings.ContainsByPredicate([](const UInspectorTestPageButtonBinding* Binding)
    {
        return !IsValid(Binding);
    });
}

bool UInspectorTestPageWidget::HasTouchScrollSupportForAutomation() const
{
    if (!WidgetTree)
    {
        return false;
    }

    const TArray<FName> ScrollNames = {
        TEXT("RI_TestPageScroll"),
        TEXT("RI_WorkflowDefinitionsScroll"),
        TEXT("RI_TestDefinitionsScroll"),
        TEXT("RI_TestResultsScroll"),
        TEXT("RI_TestReportScroll"),
        TEXT("RI_ToolsActivityLogScroll")
    };

    for (const FName& ScrollName : ScrollNames)
    {
        if (!RIInspectorTouchScroll::HasTouchSupport(Cast<UScrollBox>(WidgetTree->FindWidget(ScrollName))))
        {
            return false;
        }
    }

    return true;
}

void UInspectorTestPageWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
    RefreshFromSubsystem();
}

TSharedRef<SWidget> UInspectorTestPageWidget::RebuildWidget()
{
    const bool bHasUsableBlueprintLayout = AvailableWorkflowsScroll
        || (WidgetTree && WidgetTree->FindWidget(TEXT("RI_TestPageScroll")) != nullptr);

    if (WidgetTree && (!WidgetTree->RootWidget || !bHasUsableBlueprintLayout))
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorTestPageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree)
    {
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_TestPageScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_WorkflowDefinitionsScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_TestDefinitionsScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_TestResultsScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_TestReportScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_ToolsActivityLogScroll"));
    }
    RefreshFromSubsystem();
}

void UInspectorTestPageWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_TestRoot"));
    RootBorder->SetPadding(RICompactUI::GetPanelPadding());
    RootBorder->SetBrushColor(RICompactUI::GetPageBackgroundColor());

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestRootBox"));
    RootBorder->SetContent(RootBox);

    UInspectorTouchScrollBox* PageScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_TestPageScroll"));
    RIInspectorTouchScroll::Configure(PageScroll);
    if (UVerticalBoxSlot* VBoxSlot = RootBox->AddChildToVerticalBox(PageScroll))
    {
        VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestMainBox"));
    PageScroll->AddChild(MainBox);

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Tools Workspace"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(RI_MakeText(
        WidgetTree,
        TEXT("Keep high-frequency workflows in view and collapse deep diagnostics until they are actually needed."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_TestMutedTextColor(),
        true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Run Controls"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    UBorder* ControlsBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_TestControlsBorder"));
    ControlsBorder->SetPadding(FMargin(5.f, 4.f));
    ControlsBorder->SetBrushColor(RI_TestRowColor());
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(ControlsBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    UVerticalBox* ControlsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestControlsBox"));
    ControlsBorder->SetContent(ControlsBox);

    if (UVerticalBoxSlot* VBoxSlot = ControlsBox->AddChildToVerticalBox(CreateInfoRow(TEXT("Selected Workflow"), SelectedWorkflowValueText, true, TEXT("RI_WorkflowSelectionRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = ControlsBox->AddChildToVerticalBox(CreateInfoRow(TEXT("Selected Test"), SelectedTestValueText, true, TEXT("RI_TestSelectionRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    UVerticalBox* ActionRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestActionRow"));
    if (UVerticalBoxSlot* VBoxSlot = ControlsBox->AddChildToVerticalBox(ActionRow))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
    }

    auto MakeButton = [this](const TCHAR* Name, const TCHAR* Label, UButton*& OutButton) -> UButton*
    {
        OutButton = RICompactUI::MakeLabeledButton(
            WidgetTree,
            Name,
            Label,
            FCString::Strcmp(Label, TEXT("Run Workflow")) == 0 ? RICompactUI::ERIButtonVisualStyle::Primary : RICompactUI::ERIButtonVisualStyle::Secondary,
            0.f);
        return OutButton;
    };

    MakeButton(TEXT("BTN_RunSelectedWorkflow"), TEXT("Run Workflow"), RunSelectedWorkflowButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRunSelectedWorkflowClicked);
    if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(RunSelectedWorkflowButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_RunSelected"), TEXT("Run Test"), RunSelectedButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRunSelectedClicked);
    if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(RunSelectedButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_RunAll"), TEXT("Run All"), RunAllButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRunAllClicked);
    if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(RunAllButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_RefreshTests"), TEXT("Refresh"), RefreshButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRefreshClicked);
    if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(RefreshButton))
    {
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    StatusMessageText = RI_MakeText(WidgetTree, TEXT(""), RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true);
    if (UVerticalBoxSlot* VBoxSlot = ControlsBox->AddChildToVerticalBox(StatusMessageText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("High Frequency Workflows"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    AvailableWorkflowsScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_WorkflowDefinitionsScroll"));
    RIInspectorTouchScroll::Configure(AvailableWorkflowsScroll);
    USizeBox* WorkflowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_WorkflowDefinitionsSize"));
    WorkflowSize->SetMaxDesiredHeight(RICompactUI::GetCompactListHeight());
    AvailableWorkflowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_WorkflowDefinitionsBox"));
    AvailableWorkflowsScroll->AddChild(AvailableWorkflowsBox);
    WorkflowSize->SetContent(AvailableWorkflowsScroll);
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(WorkflowSize))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    UWidget* TestsHeader = CreateCollapsibleSectionHeader(TEXT("Tests"), TestsSectionToggleText, TEXT("BTN_ToggleTestsSection"));
    if (UButton* HeaderButton = Cast<UButton>(TestsHeader))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleToggleTestsSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(TestsHeader))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    AvailableTestsScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_TestDefinitionsScroll"));
    RIInspectorTouchScroll::Configure(AvailableTestsScroll);
    USizeBox* TestsSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_TestDefinitionsSize"));
    TestsSize->SetMaxDesiredHeight(RICompactUI::GetCompactListHeight());
    AvailableTestsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestDefinitionsBox"));
    AvailableTestsScroll->AddChild(AvailableTestsBox);
    TestsSize->SetContent(AvailableTestsScroll);
    AvailableTestsSectionWidget = TestsSize;
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(AvailableTestsSectionWidget))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    UWidget* RemoteHeader = CreateCollapsibleSectionHeader(TEXT("Remote Session"), RemoteSectionToggleText, TEXT("BTN_ToggleRemoteSection"));
    if (UButton* HeaderButton = Cast<UButton>(RemoteHeader))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleToggleRemoteSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(RemoteHeader))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    RemoteSessionSectionWidget = CreateRemoteSessionSection();
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(RemoteSessionSectionWidget))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    UWidget* DiagnosticsHeader = CreateCollapsibleSectionHeader(TEXT("Diagnostics"), DiagnosticsSectionToggleText, TEXT("BTN_ToggleDiagnosticsSection"));
    if (UButton* HeaderButton = Cast<UButton>(DiagnosticsHeader))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleToggleDiagnosticsSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(DiagnosticsHeader))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    DiagnosticsSectionWidget = CreateDiagnosticsSection();
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(DiagnosticsSectionWidget))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    UWidget* ActivityLogHeader = CreateCollapsibleSectionHeader(TEXT("Activity Log"), ActivityLogSectionToggleText, TEXT("BTN_ToggleActivityLogSection"));
    if (UButton* HeaderButton = Cast<UButton>(ActivityLogHeader))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleToggleActivityLogSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(ActivityLogHeader))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    ActivityLogSectionWidget = CreateActivityLogSection();
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(ActivityLogSectionWidget))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Recent Results"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    ResultsScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_TestResultsScroll"));
    RIInspectorTouchScroll::Configure(ResultsScroll);
    USizeBox* ResultsSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_TestResultsSize"));
    ResultsSize->SetMaxDesiredHeight(RICompactUI::GetCompactListHeight());
    AvailableTestsBox->SetVisibility(ESlateVisibility::Visible);
    ResultsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestResultsBox"));
    ResultsScroll->AddChild(ResultsBox);
    ResultsSize->SetContent(ResultsScroll);
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(ResultsSize))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Selected Report"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    UBorder* ReportBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_TestReportBorder"));
    ReportBorder->SetPadding(FMargin(6.f, 4.f));
    ReportBorder->SetBrushColor(RI_TestRowColor());
    USizeBox* ReportSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_TestReportSize"));
    ReportSize->SetMinDesiredHeight(84.f);
    ReportSize->SetMaxDesiredHeight(120.f);
    ReportSize->SetContent(ReportBorder);
    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(ReportSize))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    UVerticalBox* ReportBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_TestReportBox"));
    ReportBorder->SetContent(ReportBox);

    SelectedResultStateText = RI_MakeText(WidgetTree, TEXT("No result selected"), RICompactUI::GetLabelFontSize(), true, RI_TestMutedTextColor());
    if (UVerticalBoxSlot* VBoxSlot = ReportBox->AddChildToVerticalBox(SelectedResultStateText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UInspectorTouchScrollBox* ReportScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_TestReportScroll"));
    RIInspectorTouchScroll::Configure(ReportScroll);
    SelectedResultReportText = RI_MakeText(WidgetTree, TEXT("Run a test to see the detailed report."), RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true);
    ReportScroll->AddChild(SelectedResultReportText);
    ReportBox->AddChildToVerticalBox(ReportScroll);

    WidgetTree->RootWidget = RootBorder;
    SetSectionExpanded(AvailableTestsSectionWidget, TestsSectionToggleText, bTestsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(RemoteSessionSectionWidget, RemoteSectionToggleText, bRemoteSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(DiagnosticsSectionWidget, DiagnosticsSectionToggleText, bDiagnosticsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(ActivityLogSectionWidget, ActivityLogSectionToggleText, bActivityLogSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(RemoteOverrideSectionWidget, RemoteOverrideToggleText, bRemoteOverrideSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

UWidget* UInspectorTestPageWidget::CreateSectionTitle(const FString& InTitle, bool bEmphasis)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        bEmphasis ? RICompactUI::ERISectionVisualStyle::Emphasis : RICompactUI::ERISectionVisualStyle::Standard);
}

UButton* UInspectorTestPageWidget::GetNamedButton(const FName& Name) const
{
    if (Name == TEXT("BTN_RunSelectedWorkflow"))
    {
        return RunSelectedWorkflowButton;
    }
    if (Name == TEXT("BTN_RunSelected"))
    {
        return RunSelectedButton;
    }
    if (Name == TEXT("BTN_RunAll"))
    {
        return RunAllButton;
    }
    if (Name == TEXT("BTN_RefreshTests"))
    {
        return RefreshButton;
    }
    if (Name == TEXT("BTN_RemoteSessionRefresh"))
    {
        return RemoteSessionRefreshButton;
    }
    if (Name == TEXT("BTN_RemoteSessionConnect"))
    {
        return RemoteSessionConnectButton;
    }
    if (Name == TEXT("BTN_RemoteSessionRunWorkflow"))
    {
        return RemoteSessionRunWorkflowButton;
    }
    if (Name == TEXT("BTN_DiagnosticsRoleCompare"))
    {
        return DiagnosticsRoleCompareButton;
    }
    if (Name == TEXT("BTN_DiagnosticsSessionCompare"))
    {
        return DiagnosticsSessionCompareButton;
    }
    return nullptr;
}

UWidget* UInspectorTestPageWidget::CreateCollapsibleSectionHeader(const FString& InTitle, UTextBlock*& OutToggleText, const FName& ButtonName)
{
    UButton* HeaderButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
    RICompactUI::ConfigureButton(HeaderButton, RICompactUI::ERIButtonVisualStyle::Header, false);
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RICompactUI::GetButtonFillColor(RICompactUI::ERIButtonVisualStyle::Header));
    HeaderButton->AddChild(Border);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border->SetContent(Row);

    UTextBlock* TitleText = RI_MakeText(WidgetTree, InTitle, RICompactUI::GetSectionTitleFontSize(), true, RICompactUI::GetButtonTextColor(RICompactUI::ERIButtonVisualStyle::Header));
    if (UHorizontalBoxSlot* HBoxSlot = Row->AddChildToHorizontalBox(TitleText))
    {
        HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutToggleText = RI_MakeText(WidgetTree, TEXT("Show"), RICompactUI::GetMutedFontSize(), true, RICompactUI::GetButtonTextColor(RICompactUI::ERIButtonVisualStyle::Header));
    Row->AddChildToHorizontalBox(OutToggleText);
    return HeaderButton;
}

UWidget* UInspectorTestPageWidget::CreateInfoRow(const FString& Label, UTextBlock*& OutValueText, bool bWrapValue, const FName& WidgetName)
{
    return RICompactUI::MakeStackedValueRow(
        WidgetTree,
        Label,
        OutValueText,
        RI_TestRowColor(),
        RI_TestMutedTextColor(),
        RI_TestTextColor(),
        bWrapValue,
        NAME_None,
        WidgetName);
}

void UInspectorTestPageWidget::SetSectionExpanded(UWidget* ContentWidget, UTextBlock* ToggleText, bool bExpanded, const FString& ExpandedLabel, const FString& CollapsedLabel)
{
    if (ContentWidget)
    {
        ContentWidget->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (ToggleText)
    {
        ToggleText->SetText(FText::FromString(bExpanded ? ExpandedLabel : CollapsedLabel));
    }
}

UWidget* UInspectorTestPageWidget::CreateRemoteSessionSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    PanelBorder->SetPadding(FMargin(5.f, 4.f));
    PanelBorder->SetBrushColor(RI_TestRowColor());
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(PanelBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    PanelBorder->SetContent(PanelBox);

    if (UVerticalBoxSlot* VBoxSlot = PanelBox->AddChildToVerticalBox(CreateInfoRow(TEXT("Selected Session"), SelectedRemoteSessionValueText, true, TEXT("RI_RemoteSessionSelectionRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    RemoteSessionComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RI_RemoteSessionCombo"));
    RICompactUI::ConfigureComboBoxString(RemoteSessionComboBox, RI_TestTextColor(), 220.0f, RICompactUI::ERIInputVisualStyle::Strong);
    RemoteSessionComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UInspectorTestPageWidget::HandleGenerateRemoteSessionOptionWidget);
    if (UVerticalBoxSlot* VBoxSlot = PanelBox->AddChildToVerticalBox(RICompactUI::MakeStackedContentRow(
        WidgetTree,
        TEXT("Session Picker"),
        RICompactUI::WrapFixedHeight(WidgetTree, RemoteSessionComboBox, RICompactUI::GetInputHeight()),
        RI_TestRowColor(),
        RI_TestMutedTextColor(),
        TEXT("RI_RemoteSessionPickerRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }
    RemoteSessionComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionSelectionChanged);

    UWidget* OverrideHeader = CreateCollapsibleSectionHeader(TEXT("Advanced Override"), RemoteOverrideToggleText, TEXT("BTN_ToggleRemoteOverride"));
    if (UButton* HeaderButton = Cast<UButton>(OverrideHeader))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleToggleRemoteOverrideSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = PanelBox->AddChildToVerticalBox(OverrideHeader))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    UVerticalBox* OverrideBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_RemoteSessionOverrideBox"));
    RemoteOverrideSectionWidget = OverrideBox;
    PanelBox->AddChildToVerticalBox(OverrideBox);

    RemoteSessionWorkflowBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_RemoteSessionWorkflowBox"));
    RemoteSessionWorkflowBox->SetText(FText::FromString(SelectedWorkflowId != NAME_None ? SelectedWorkflowId.ToString() : TEXT("")));
    RemoteSessionWorkflowBox->OnTextCommitted.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionWorkflowCommitted);
    RICompactUI::ConfigureEditableTextBox(RemoteSessionWorkflowBox, RI_TestTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Strong);
    if (UVerticalBoxSlot* VBoxSlot = OverrideBox->AddChildToVerticalBox(RICompactUI::MakeStackedContentRow(
        WidgetTree,
        TEXT("Workflow Id"),
        RICompactUI::WrapFixedHeight(WidgetTree, RemoteSessionWorkflowBox, RICompactUI::GetInputHeight()),
        RI_TestRowColor(),
        RI_TestMutedTextColor(),
        TEXT("RI_RemoteSessionWorkflowRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    RemoteSessionTargetQueryBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_RemoteSessionTargetQueryBox"));
    RemoteSessionTargetQueryBox->SetText(FText::FromString(TEXT("")));
    RemoteSessionTargetQueryBox->OnTextCommitted.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionTargetQueryCommitted);
    RICompactUI::ConfigureEditableTextBox(RemoteSessionTargetQueryBox, RI_TestTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Strong);
    if (UVerticalBoxSlot* VBoxSlot = OverrideBox->AddChildToVerticalBox(RICompactUI::MakeStackedContentRow(
        WidgetTree,
        TEXT("Target Query"),
        RICompactUI::WrapFixedHeight(WidgetTree, RemoteSessionTargetQueryBox, RICompactUI::GetInputHeight()),
        RI_TestRowColor(),
        RI_TestMutedTextColor(),
        TEXT("RI_RemoteSessionTargetRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    UVerticalBox* ButtonRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_RemoteSessionButtonRow"));
    if (UVerticalBoxSlot* VBoxSlot = PanelBox->AddChildToVerticalBox(ButtonRow))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
    }

    auto MakeButton = [this](const TCHAR* Name, const TCHAR* Label, UButton*& OutButton) -> UButton*
    {
        const RICompactUI::ERIButtonVisualStyle Style = FCString::Strcmp(Label, TEXT("Run Remote Workflow")) == 0
            ? RICompactUI::ERIButtonVisualStyle::Primary
            : RICompactUI::ERIButtonVisualStyle::Secondary;
        OutButton = RICompactUI::MakeLabeledButton(WidgetTree, Name, Label, Style, 0.0f);
        return OutButton;
    };

    MakeButton(TEXT("BTN_RemoteSessionRefresh"), TEXT("Refresh"), RemoteSessionRefreshButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionRefreshClicked);
    if (UVerticalBoxSlot* VBoxSlot = ButtonRow->AddChildToVerticalBox(RemoteSessionRefreshButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_RemoteSessionConnect"), TEXT("Connect"), RemoteSessionConnectButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionConnectClicked);
    if (UVerticalBoxSlot* VBoxSlot = ButtonRow->AddChildToVerticalBox(RemoteSessionConnectButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_RemoteSessionRunWorkflow"), TEXT("Run Remote Workflow"), RemoteSessionRunWorkflowButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleRemoteSessionRunWorkflowClicked);
    if (UVerticalBoxSlot* VBoxSlot = ButtonRow->AddChildToVerticalBox(RemoteSessionRunWorkflowButton))
    {
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    UTextBlock* HelpText = RI_MakeText(
        WidgetTree,
        TEXT("Selected session is used when running workflows. Packaged workflows appear automatically if provided by the subsystem."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_TestMutedTextColor(),
        true);
    if (UVerticalBoxSlot* VBoxSlot = PanelBox->AddChildToVerticalBox(HelpText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
    }

    return Outer;
}

UWidget* UInspectorTestPageWidget::HandleGenerateRemoteSessionOptionWidget(FString InItemText)
{
    return WidgetTree
        ? RICompactUI::MakeComboBoxItemText(WidgetTree, InItemText, RI_TestTextColor())
        : nullptr;
}

UWidget* UInspectorTestPageWidget::CreateDiagnosticsSection()
{
    UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ToolsDiagnosticsBorder"));
    PanelBorder->SetPadding(FMargin(5.f, 4.f));
    PanelBorder->SetBrushColor(RI_TestRowColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsDiagnosticsBox"));
    PanelBorder->SetContent(Box);

    UTextBlock* HelpText = RI_MakeText(
        WidgetTree,
        TEXT("Advanced compare tools stay available here, but they are collapsed by default so daily users can focus on inspect, edit, and stage."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_TestMutedTextColor(),
        true);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(HelpText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsDiagnosticsButtonRow"));
    Box->AddChildToVerticalBox(Row);

    auto MakeButton = [this](const TCHAR* Name, const TCHAR* Label, UButton*& OutButton) -> UButton*
    {
        OutButton = RICompactUI::MakeLabeledButton(WidgetTree, Name, Label, RICompactUI::ERIButtonVisualStyle::Secondary, 0.0f);
        return OutButton;
    };

    MakeButton(TEXT("BTN_DiagnosticsRoleCompare"), TEXT("Build Role Compare"), DiagnosticsRoleCompareButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleBuildDiagnosticsRoleCompareClicked);
    if (UVerticalBoxSlot* VBoxSlot = Row->AddChildToVerticalBox(DiagnosticsRoleCompareButton))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    MakeButton(TEXT("BTN_DiagnosticsSessionCompare"), TEXT("Build Session Compare"), DiagnosticsSessionCompareButton)->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleBuildDiagnosticsSessionCompareClicked);
    if (UVerticalBoxSlot* VBoxSlot = Row->AddChildToVerticalBox(DiagnosticsSessionCompareButton))
    {
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateSectionTitle(TEXT("Role Compare"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Summary"), DiagnosticsRoleCompareSummaryText, true, TEXT("RI_ToolsRoleCompareSummaryRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Available Role"), DiagnosticsRoleCompareAvailableRoleText, false, TEXT("RI_ToolsRoleCompareRoleRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Stats"), DiagnosticsRoleCompareStatsText, true, TEXT("RI_ToolsRoleCompareStatsRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    UBorder* RolePreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ToolsRoleComparePreviewBorder"));
    RolePreviewBorder->SetPadding(FMargin(5.f, 4.f));
    RolePreviewBorder->SetBrushColor(RI_TestRowColor());
    DiagnosticsRoleComparePreviewText = RI_MakeText(WidgetTree, TEXT("No runtime role compare preview."), RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true);
    RolePreviewBorder->SetContent(DiagnosticsRoleComparePreviewText);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RolePreviewBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateSectionTitle(TEXT("Session Compare"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Summary"), DiagnosticsSessionCompareSummaryText, true, TEXT("RI_ToolsSessionCompareSummaryRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Session Pair"), DiagnosticsSessionComparePairText, false, TEXT("RI_ToolsSessionComparePairRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(CreateInfoRow(TEXT("Stats"), DiagnosticsSessionCompareStatsText, true, TEXT("RI_ToolsSessionCompareStatsRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    UBorder* SessionPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ToolsSessionComparePreviewBorder"));
    SessionPreviewBorder->SetPadding(FMargin(5.f, 4.f));
    SessionPreviewBorder->SetBrushColor(RI_TestRowColor());
    DiagnosticsSessionComparePreviewText = RI_MakeText(WidgetTree, TEXT("No session compare preview."), RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true);
    SessionPreviewBorder->SetContent(DiagnosticsSessionComparePreviewText);
    Box->AddChildToVerticalBox(SessionPreviewBorder);

    return PanelBorder;
}

UWidget* UInspectorTestPageWidget::CreateActivityLogSection()
{
    UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ToolsActivityLogBorder"));
    PanelBorder->SetPadding(FMargin(5.f, 4.f));
    PanelBorder->SetBrushColor(RI_TestRowColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsActivityLogBox"));
    PanelBorder->SetContent(Box);

    UTextBlock* HelpText = RI_MakeText(
        WidgetTree,
        TEXT("Recent RuntimeInspector actions and results. Routine page refreshes are intentionally excluded."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_TestMutedTextColor(),
        true);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(HelpText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    ActivityLogClearButton = RICompactUI::MakeLabeledButton(WidgetTree, TEXT("BTN_ClearActivityLog"), TEXT("Clear"), RICompactUI::ERIButtonVisualStyle::Secondary, 0.0f);
    ActivityLogClearButton->OnClicked.AddDynamic(this, &UInspectorTestPageWidget::HandleClearActivityLogClicked);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(ActivityLogClearButton))
    {
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    UInspectorTouchScrollBox* LogScroll = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_ToolsActivityLogScroll"));
    RIInspectorTouchScroll::Configure(LogScroll);
    USizeBox* LogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ToolsActivityLogSize"));
    LogSize->SetMinDesiredHeight(72.f);
    LogSize->SetMaxDesiredHeight(140.f);
    ActivityLogEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsActivityLogEntriesBox"));
    LogScroll->AddChild(ActivityLogEntriesBox);
    LogSize->SetContent(LogScroll);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(LogSize))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
    }

    return PanelBorder;
}

UWidget* UInspectorTestPageWidget::CreateAvailableWorkflowRow(const FRIWorkflowDefinition& Definition)
{
    const FString DisabledReason = bRunning ? TEXT("A workflow is already running.") : TEXT("Select a workflow to run it.");
    const FName RowName(*FString::Printf(TEXT("RI_Workflow_%s"), *Definition.WorkflowId.ToString()));
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), RowName);
    Border->SetPadding(FMargin(5.f, 3.f));
    Border->SetBrushColor(Definition.WorkflowId == SelectedWorkflowId ? RI_TestSelectedRowColor() : RI_TestRowColor());

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border->SetContent(Row);

    UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* HBoxSlot = Row->AddChildToHorizontalBox(TextBox))
    {
        HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HBoxSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    const FString Title = Definition.Category.IsEmpty()
        ? Definition.DisplayName
        : FString::Printf(TEXT("%s [%s]"), *Definition.DisplayName, *Definition.Category);
    TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, Title, RICompactUI::GetLabelFontSize(), true, RI_TestTextColor()));

    FString MetaLine = Definition.bRequiresPIE ? TEXT("PIE Required") : TEXT("No PIE requirement");
    if (Definition.bRequiresSelectedActor)
    {
        MetaLine += TEXT(" • Selected Actor Required");
    }
    if (Definition.bMutatesRuntime)
    {
        MetaLine += TEXT(" • Mutates Runtime");
    }
    if (Definition.bMutatesSource)
    {
        MetaLine += TEXT(" • Mutates Source");
    }
    MetaLine += FString::Printf(TEXT(" • Profiles=%d • Tests=%d"), Definition.VerificationProfileIds.Num(), Definition.SelfTestIds.Num());
    if (Definition.ChildWorkflowIds.Num() > 0)
    {
        MetaLine += FString::Printf(TEXT(" • Nested=%d"), Definition.ChildWorkflowIds.Num());
    }
    if (Definition.Tags.Num() > 0)
    {
        MetaLine += FString::Printf(TEXT(" • Tags=%s"), *FString::Join(Definition.Tags, TEXT(",")));
    }
    if (UVerticalBoxSlot* VBoxSlot = TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, MetaLine, RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    }

    if (!Definition.Description.IsEmpty())
    {
        if (UVerticalBoxSlot* VBoxSlot = TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, Definition.Description, RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
        }
    }

    UHorizontalBox* ButtonsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Row->AddChildToHorizontalBox(ButtonsBox);

    UButton* SelectButton = RICompactUI::MakeLabeledButton(WidgetTree, NAME_None, TEXT("Select"), RICompactUI::ERIButtonVisualStyle::Subtle, 64.f, RICompactUI::GetButtonHeight(), RICompactUI::GetLabelFontSize());
    BindSelectWorkflowButton(SelectButton, Definition.WorkflowId);
    RICompactUI::SetWidgetEnabledState(SelectButton, !bRunning, DisabledReason, TEXT("Select this workflow."));
    if (UHorizontalBoxSlot* HBoxSlot = ButtonsBox->AddChildToHorizontalBox(SelectButton))
    {
        HBoxSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    UButton* RunButton = RICompactUI::MakeLabeledButton(WidgetTree, NAME_None, TEXT("Run"), RICompactUI::ERIButtonVisualStyle::Primary, 64.f, RICompactUI::GetButtonHeight(), RICompactUI::GetLabelFontSize());
    BindRunWorkflowButton(RunButton, Definition.WorkflowId);
    RICompactUI::SetWidgetEnabledState(RunButton, !bRunning, DisabledReason, TEXT("Run this workflow."));
    if (UHorizontalBoxSlot* HBoxSlot = ButtonsBox->AddChildToHorizontalBox(RunButton))
    {
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    return Border;
}

int32 UInspectorTestPageWidget::GetRenderedWorkflowRowCount() const
{
    return AvailableWorkflowsBox ? AvailableWorkflowsBox->GetChildrenCount() : 0;
}

bool UInspectorTestPageWidget::HasWorkflowSelectionRow() const
{
    return WidgetTree && WidgetTree->FindWidget(TEXT("RI_WorkflowSelectionRow")) != nullptr;
}

bool UInspectorTestPageWidget::HasRenderedWorkflowRow(FName WorkflowId) const
{
    return WidgetTree && WidgetTree->FindWidget(FName(*FString::Printf(TEXT("RI_Workflow_%s"), *WorkflowId.ToString()))) != nullptr;
}

UWidget* UInspectorTestPageWidget::CreateAvailableTestRow(const FRISelfTestDefinition& Definition)
{
    const FString DisabledReason = bRunning
        ? TEXT("A workflow or self-test is already running.")
        : (Definition.bEnabled ? TEXT("Select a self-test to run it.") : TEXT("This self-test is currently unavailable."));
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(5.f, 3.f));
    Border->SetBrushColor(Definition.Id == SelectedTestId ? RI_TestSelectedRowColor() : RI_TestRowColor());

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border->SetContent(Row);

    UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* HBoxSlot = Row->AddChildToHorizontalBox(TextBox))
    {
        HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HBoxSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    FString TitleLine = Definition.DisplayName;
    if (!Definition.Category.IsEmpty())
    {
        TitleLine += FString::Printf(TEXT(" [%s]"), *Definition.Category);
    }
    TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, TitleLine, RICompactUI::GetLabelFontSize(), true, RI_TestTextColor()));

    FString MetaLine;
    if (Definition.bRequiresPIE)
    {
        MetaLine = Definition.bEnabled ? TEXT("PIE Required") : TEXT("PIE Required - currently unavailable");
    }
    else
    {
        MetaLine = TEXT("No PIE requirement");
    }
    if (Definition.bMutatesRuntime)
    {
        MetaLine += TEXT(" • Mutates runtime");
    }

    if (UVerticalBoxSlot* VBoxSlot = TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, MetaLine, RICompactUI::GetMutedFontSize(), false, Definition.bEnabled ? RI_TestMutedTextColor() : RI_TestWarningColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    }

    if (!Definition.Description.IsEmpty())
    {
        if (UVerticalBoxSlot* VBoxSlot = TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, Definition.Description, RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
        }
    }

    UHorizontalBox* ButtonsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Row->AddChildToHorizontalBox(ButtonsBox);

    UButton* SelectButton = RICompactUI::MakeLabeledButton(WidgetTree, NAME_None, TEXT("Select"), RICompactUI::ERIButtonVisualStyle::Subtle, 64.f, RICompactUI::GetButtonHeight(), RICompactUI::GetLabelFontSize());
    BindSelectTestButton(SelectButton, Definition.Id);
    RICompactUI::SetWidgetEnabledState(SelectButton, !bRunning, DisabledReason, TEXT("Select this self-test."));
    if (UHorizontalBoxSlot* HBoxSlot = ButtonsBox->AddChildToHorizontalBox(SelectButton))
    {
        HBoxSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    UButton* RunButton = RICompactUI::MakeLabeledButton(WidgetTree, NAME_None, TEXT("Run"), RICompactUI::ERIButtonVisualStyle::Primary, 64.f, RICompactUI::GetButtonHeight(), RICompactUI::GetLabelFontSize());
    BindRunTestButton(RunButton, Definition.Id);
    RICompactUI::SetWidgetEnabledState(RunButton, !bRunning && Definition.bEnabled, DisabledReason, TEXT("Run this self-test."));
    if (UHorizontalBoxSlot* HBoxSlot = ButtonsBox->AddChildToHorizontalBox(RunButton))
    {
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    return Border;
}

UWidget* UInspectorTestPageWidget::CreateResultRow(const FRISelfTestResult& Result)
{
    const FString DisabledReason = bRunning ? TEXT("A workflow or self-test is already running.") : TEXT("Select a result to view its report.");
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(5.f, 3.f));
    Border->SetBrushColor(Result.Id == SelectedResultId ? RI_TestSelectedRowColor() : RI_TestRowColor());

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border->SetContent(Row);

    FString State = Result.bPassed ? TEXT("PASS") : TEXT("FAIL");
    const FLinearColor StateColor = Result.bPassed ? RI_TestSuccessColor() : RI_TestErrorColor();

    UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* HBoxSlot = Row->AddChildToHorizontalBox(TextBox))
    {
        HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HBoxSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, FString::Printf(TEXT("%s • %s"), *State, *Result.DisplayName), RICompactUI::GetLabelFontSize(), true, StateColor));
    if (UVerticalBoxSlot* VBoxSlot = TextBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, Result.Summary, RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    }

    UButton* ViewButton = RICompactUI::MakeLabeledButton(WidgetTree, NAME_None, TEXT("View"), RICompactUI::ERIButtonVisualStyle::Secondary, 64.f, RICompactUI::GetButtonHeight(), RICompactUI::GetLabelFontSize());
    BindResultViewButton(ViewButton, Result.Id);
    RICompactUI::SetWidgetEnabledState(ViewButton, !bRunning, DisabledReason, TEXT("View this report."));
    if (UHorizontalBoxSlot* HBoxSlot = Row->AddChildToHorizontalBox(ViewButton))
    {
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    return Border;
}

void UInspectorTestPageWidget::RebuildAvailableWorkflows()
{
    if (!AvailableWorkflowsBox)
    {
        return;
    }

    AvailableWorkflowsBox->ClearChildren();
    for (const FRIWorkflowDefinition& Definition : AvailableWorkflows)
    {
        if (UVerticalBoxSlot* VBoxSlot = AvailableWorkflowsBox->AddChildToVerticalBox(CreateAvailableWorkflowRow(Definition)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    if (AvailableWorkflowsScroll)
    {
        AvailableWorkflowsScroll->ScrollToStart();
    }
}

void UInspectorTestPageWidget::RebuildAvailableTests()
{
    if (!AvailableTestsBox)
    {
        return;
    }

    AvailableTestsBox->ClearChildren();
    for (const FRISelfTestDefinition& Definition : AvailableTests)
    {
        if (UVerticalBoxSlot* VBoxSlot = AvailableTestsBox->AddChildToVerticalBox(CreateAvailableTestRow(Definition)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    if (AvailableTestsScroll)
    {
        AvailableTestsScroll->ScrollToStart();
    }
}

void UInspectorTestPageWidget::RebuildResults()
{
    if (!ResultsBox)
    {
        return;
    }

    ResultsBox->ClearChildren();
    for (const FRISelfTestResult& Result : Results)
    {
        if (UVerticalBoxSlot* VBoxSlot = ResultsBox->AddChildToVerticalBox(CreateResultRow(Result)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    if (ResultsScroll)
    {
        ResultsScroll->ScrollToStart();
    }
}

void UInspectorTestPageWidget::RebuildActivityLog()
{
    if (!ActivityLogEntriesBox)
    {
        return;
    }

    ActivityLogEntriesBox->ClearChildren();

    const UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    const TArray<FRIActivityLogEntry>& Entries = InspectorSubsystem ? InspectorSubsystem->GetActivityLogEntries() : TArray<FRIActivityLogEntry>();
    if (Entries.Num() <= 0)
    {
        UTextBlock* EmptyText = RI_MakeText(WidgetTree, TEXT("No recent RuntimeInspector actions."), RICompactUI::GetMutedFontSize(), false, RI_TestMutedTextColor(), true);
        ActivityLogEntriesBox->AddChildToVerticalBox(EmptyText);
        return;
    }

    for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
    {
        const FRIActivityLogEntry& Entry = Entries[Index];
        UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        RowBorder->SetPadding(FMargin(5.f, 3.f));
        RowBorder->SetBrushColor(RICompactUI::GetSectionSurfaceBackgroundColor());

        UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        RowBorder->SetContent(RowBox);

        const FString TimeText = Entry.TimestampUtc.ToString(TEXT("%H:%M:%S"));
        const FString CategoryText = Entry.Category.IsEmpty() ? TEXT("RuntimeInspector") : Entry.Category;
        UTextBlock* MetaText = RI_MakeText(
            WidgetTree,
            FString::Printf(TEXT("%s  %s"), *TimeText, *CategoryText),
            RICompactUI::GetMutedFontSize(),
            true,
            RI_TestMutedTextColor(),
            false);
        RowBox->AddChildToVerticalBox(MetaText);

        const FLinearColor MessageColor =
            Entry.Severity == ERIToastType::Error ? RI_TestErrorColor() :
            (Entry.Severity == ERIToastType::Warning ? RI_TestWarningColor() :
            (Entry.Severity == ERIToastType::Success ? RI_TestSuccessColor() : RI_TestTextColor()));

        UTextBlock* MessageText = RI_MakeText(WidgetTree, Entry.Message, RICompactUI::GetMutedFontSize(), false, MessageColor, true);
        if (UVerticalBoxSlot* VBoxSlot = RowBox->AddChildToVerticalBox(MessageText))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
        }

        if (UVerticalBoxSlot* VBoxSlot = ActivityLogEntriesBox->AddChildToVerticalBox(RowBorder))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }
}

void UInspectorTestPageWidget::RefreshFromSubsystem()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        AvailableWorkflows = InspectorSubsystem->GetAvailableWorkflows();
        AvailableTests = InspectorSubsystem->GetAvailableSelfTests();
        Results = InspectorSubsystem->GetLastSelfTestResults();
        AvailableRemoteSessions = InspectorSubsystem->GetAvailableRuntimeSessions();
        LastWorkflowRunResult = InspectorSubsystem->GetLastWorkflowRunResult();
    }
    else
    {
        AvailableWorkflows.Reset();
        AvailableTests.Reset();
        Results.Reset();
        AvailableRemoteSessions.Reset();
        LastWorkflowRunResult = FRIWorkflowRunResult();
        ClearStatusMessage();
    }

    const bool bHasSelectedWorkflow = AvailableWorkflows.ContainsByPredicate([this](const FRIWorkflowDefinition& Definition)
    {
        return Definition.WorkflowId == SelectedWorkflowId;
    });
    if (!bHasSelectedWorkflow)
    {
        SelectedWorkflowId = AvailableWorkflows.Num() > 0 ? AvailableWorkflows[0].WorkflowId : NAME_None;
    }

    const bool bHasSelectedTest = AvailableTests.ContainsByPredicate([this](const FRISelfTestDefinition& Definition)
    {
        return Definition.Id == SelectedTestId;
    });
    if (!bHasSelectedTest)
    {
        SelectedTestId = AvailableTests.Num() > 0 ? AvailableTests[0].Id : NAME_None;
    }

    const bool bHasSelectedResult = Results.ContainsByPredicate([this](const FRISelfTestResult& Result)
    {
        return Result.Id == SelectedResultId;
    });
    if (!bHasSelectedResult)
    {
        SelectedResultId = Results.Num() > 0 ? Results.Last().Id : NAME_None;
    }

    PullRemoteSessionContextFromSubsystem();
    RefreshRemoteSessionSelection();

    if (RemoteSessionWorkflowBox)
    {
        const FString StoredWorkflowText = Subsystem.IsValid() ? Subsystem->GetLastRemoteSessionWorkflowId().TrimStartAndEnd() : FString();
        const FString CurrentWorkflowText = RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd();
        if (StoredWorkflowText.IsEmpty() && CurrentWorkflowText.IsEmpty() && SelectedWorkflowId != NAME_None)
        {
            RemoteSessionWorkflowBox->SetText(FText::FromString(SelectedWorkflowId.ToString()));
        }
    }

    PushRemoteSessionContextToSubsystem();
    RebuildActivityLog();
    UpdateUIFromState();
}

void UInspectorTestPageWidget::ApplyPresentationCollapsedState(bool bCollapseTests, bool bCollapseRemote, bool bCollapseDiagnostics, bool bCollapseOverride, bool bCollapseActivityLog)
{
    bTestsSectionExpanded = !bCollapseTests;
    bRemoteSectionExpanded = !bCollapseRemote;
    bDiagnosticsSectionExpanded = !bCollapseDiagnostics;
    bRemoteOverrideSectionExpanded = !bCollapseOverride;
    bActivityLogSectionExpanded = !bCollapseActivityLog;

    SetSectionExpanded(AvailableTestsSectionWidget, TestsSectionToggleText, bTestsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(RemoteSessionSectionWidget, RemoteSectionToggleText, bRemoteSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(DiagnosticsSectionWidget, DiagnosticsSectionToggleText, bDiagnosticsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(ActivityLogSectionWidget, ActivityLogSectionToggleText, bActivityLogSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(RemoteOverrideSectionWidget, RemoteOverrideToggleText, bRemoteOverrideSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleWorkflowSelected(FName WorkflowId)
{
    SelectedWorkflowId = WorkflowId;
    bViewingWorkflowReport = true;
    if (RemoteSessionWorkflowBox)
    {
        RemoteSessionWorkflowBox->SetText(FText::FromString(WorkflowId != NAME_None ? WorkflowId.ToString() : TEXT("")));
    }
    PushRemoteSessionContextToSubsystem();
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleWorkflowRunRequested(FName WorkflowId)
{
    SelectedWorkflowId = WorkflowId;
    bViewingWorkflowReport = true;
    if (RemoteSessionWorkflowBox)
    {
        RemoteSessionWorkflowBox->SetText(FText::FromString(WorkflowId != NAME_None ? WorkflowId.ToString() : TEXT("")));
    }
    RunSingleWorkflow(WorkflowId);
}

void UInspectorTestPageWidget::HandleSelfTestSelected(FName TestId)
{
    SelectedTestId = TestId;
    bViewingWorkflowReport = false;
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleSelfTestRunRequested(FName TestId)
{
    SelectedTestId = TestId;
    bViewingWorkflowReport = false;
    RunSingleTest(TestId);
}

void UInspectorTestPageWidget::HandleSelfTestResultSelected(FName TestId)
{
    SelectedResultId = TestId;
    bViewingWorkflowReport = false;
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleRemoteSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct && SelectedItem.IsEmpty())
    {
        return;
    }

    SelectedRemoteSessionId = SelectedItem;
    PushRemoteSessionContextToSubsystem();
    UpdateUIFromState();
}

void UInspectorTestPageWidget::UpdateUIFromState()
{
    const UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    const FString RunningReason = TEXT("A workflow or self-test is already running.");
    const FString MissingWorkflowReason = TEXT("Select a workflow first.");
    const FString MissingTestReason = TEXT("Select a self-test first.");
    const FString MissingRemoteSessionReason = TEXT("Select a remote session first.");
    const FString MissingRemoteWorkflowReason = TEXT("Enter a workflow id or select one first.");
    auto SetEnabledState = [](UWidget* Widget, bool bEnabled, const FString& DisabledReason, const FString& EnabledTooltip = FString())
    {
        RICompactUI::SetWidgetEnabledState(Widget, bEnabled, DisabledReason, EnabledTooltip);
    };
    auto HasWorkflowId = [this](const FString& WorkflowIdText) -> bool
    {
        return !WorkflowIdText.IsEmpty() && FindWorkflowById(FName(*WorkflowIdText)) != nullptr;
    };

    if (SelectedWorkflowValueText)
    {
        SelectedWorkflowValueText->SetText(GetSelectedWorkflowDisplayText());
    }
    if (SelectedTestValueText)
    {
        SelectedTestValueText->SetText(GetSelectedTestDisplayText());
    }
    if (SelectedRemoteSessionValueText)
    {
        const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSession(AvailableRemoteSessions, SelectedRemoteSessionId);
        SelectedRemoteSessionValueText->SetText(FText::FromString(
            SelectedSession ? RI_BuildRemoteSessionSummaryText(*SelectedSession) : TEXT("No remote session selected")));
    }

    if (RunSelectedWorkflowButton)
    {
        SetEnabledState(
            RunSelectedWorkflowButton,
            !bRunning && FindWorkflowById(SelectedWorkflowId) != nullptr,
            bRunning ? RunningReason : MissingWorkflowReason,
            TEXT("Run the selected workflow."));
    }
    if (RunSelectedButton)
    {
        const FRISelfTestDefinition* SelectedDefinition = AvailableTests.FindByPredicate([this](const FRISelfTestDefinition& Definition)
        {
            return Definition.Id == SelectedTestId;
        });
        SetEnabledState(
            RunSelectedButton,
            !bRunning && SelectedDefinition && SelectedDefinition->bEnabled,
            bRunning ? RunningReason : MissingTestReason,
            TEXT("Run the selected self-test."));
    }
    if (RunAllButton)
    {
        const bool bHasRunnable = AvailableTests.ContainsByPredicate([](const FRISelfTestDefinition& Definition)
        {
            return Definition.bEnabled;
        });
        SetEnabledState(
            RunAllButton,
            !bRunning && bHasRunnable,
            bRunning ? RunningReason : TEXT("No runnable self-tests are available."),
            TEXT("Run every available self-test."));
    }
    if (RefreshButton)
    {
        SetEnabledState(RefreshButton, !bRunning, RunningReason, TEXT("Refresh workflows, tests, and cached results."));
    }
    if (RemoteSessionRefreshButton)
    {
        SetEnabledState(RemoteSessionRefreshButton, !bRunning, RunningReason, TEXT("Refresh remote sessions."));
    }
    if (RemoteSessionConnectButton)
    {
        SetEnabledState(
            RemoteSessionConnectButton,
            !bRunning && !SelectedRemoteSessionId.IsEmpty(),
            bRunning ? RunningReason : MissingRemoteSessionReason,
            TEXT("Connect to the selected remote session."));
    }
    if (RemoteSessionRunWorkflowButton)
    {
        FString WorkflowIdText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
        if (WorkflowIdText.IsEmpty() && SelectedWorkflowId != NAME_None)
        {
            WorkflowIdText = SelectedWorkflowId.ToString();
        }
        SetEnabledState(
            RemoteSessionRunWorkflowButton,
            !bRunning && HasWorkflowId(WorkflowIdText),
            bRunning ? RunningReason : (WorkflowIdText.IsEmpty() ? MissingRemoteWorkflowReason : TEXT("Remote workflow id is not defined in Tools workflows.")),
            TEXT("Run the selected workflow in the connected remote session."));
    }

    if (StatusMessageText)
    {
        StatusMessageText->SetText(FText::FromString(StatusMessage));
        StatusMessageText->SetColorAndOpacity(FSlateColor(bStatusIsError ? RI_TestErrorColor() : RI_TestMutedTextColor()));
        StatusMessageText->SetVisibility(StatusMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    const FRIRuntimeRoleCompareReport RoleCompareReport = InspectorSubsystem ? InspectorSubsystem->GetLastRuntimeRoleCompareReport() : FRIRuntimeRoleCompareReport();
    const bool bHasRoleCompare = !RoleCompareReport.Summary.IsEmpty() || RoleCompareReport.Lines.Num() > 0;
    if (DiagnosticsRoleCompareSummaryText)
    {
        DiagnosticsRoleCompareSummaryText->SetText(FText::FromString(bHasRoleCompare ? RoleCompareReport.Summary : TEXT("No runtime role compare yet.")));
        DiagnosticsRoleCompareSummaryText->SetColorAndOpacity(FSlateColor(bHasRoleCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }
    if (DiagnosticsRoleCompareAvailableRoleText)
    {
        const FString AvailableRole = bHasRoleCompare ? (RoleCompareReport.AvailableRoleLabel.IsEmpty() ? TEXT("No available role") : RoleCompareReport.AvailableRoleLabel) : TEXT("No available role");
        DiagnosticsRoleCompareAvailableRoleText->SetText(FText::FromString(AvailableRole));
        DiagnosticsRoleCompareAvailableRoleText->SetColorAndOpacity(FSlateColor(bHasRoleCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }
    if (DiagnosticsRoleCompareStatsText)
    {
        const FString Stats = bHasRoleCompare
            ? FString::Printf(
                TEXT("Lines=%d | Mismatch=%d | MissingRoles=%d | Verify=%d"),
                RoleCompareReport.ComparedLineCount,
                RoleCompareReport.MismatchCount,
                RoleCompareReport.MissingRoleCount,
                RoleCompareReport.VerificationMismatchCount)
            : TEXT("No runtime role compare stats.");
        DiagnosticsRoleCompareStatsText->SetText(FText::FromString(Stats));
        DiagnosticsRoleCompareStatsText->SetColorAndOpacity(FSlateColor(bHasRoleCompare && RoleCompareReport.MismatchCount > 0 ? RI_TestWarningColor() : (bHasRoleCompare ? RI_TestTextColor() : RI_TestMutedTextColor())));
    }
    if (DiagnosticsRoleComparePreviewText)
    {
        FString PreviewText = TEXT("No runtime role compare preview.");
        if (bHasRoleCompare)
        {
            if (RoleCompareReport.Lines.Num() > 0)
            {
                const FRIRuntimeRoleCompareLine& FirstLine = RoleCompareReport.Lines[0];
                PreviewText = FirstLine.Summary.IsEmpty()
                    ? FString::Printf(TEXT("%s :: no summary"), *FirstLine.Field.FieldPath)
                    : FirstLine.Summary;
            }
            else
            {
                PreviewText = RoleCompareReport.Summary;
            }
        }
        DiagnosticsRoleComparePreviewText->SetText(FText::FromString(PreviewText));
        DiagnosticsRoleComparePreviewText->SetColorAndOpacity(FSlateColor(bHasRoleCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }

    const FRIRuntimeSessionTargetSetCompareReport SessionCompareReport = InspectorSubsystem ? InspectorSubsystem->GetLastRuntimeSessionTargetSetCompareReport() : FRIRuntimeSessionTargetSetCompareReport();
    const bool bHasSessionCompare = !SessionCompareReport.Summary.IsEmpty() || SessionCompareReport.Lines.Num() > 0;
    if (DiagnosticsSessionCompareSummaryText)
    {
        DiagnosticsSessionCompareSummaryText->SetText(FText::FromString(bHasSessionCompare ? SessionCompareReport.Summary : TEXT("No session compare yet.")));
        DiagnosticsSessionCompareSummaryText->SetColorAndOpacity(FSlateColor(bHasSessionCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }
    if (DiagnosticsSessionComparePairText)
    {
        const FString PairText = bHasSessionCompare
            ? FString::Printf(TEXT("%s -> %s"), *SessionCompareReport.LeftSessionId, *SessionCompareReport.RightSessionId)
            : TEXT("No session pair");
        DiagnosticsSessionComparePairText->SetText(FText::FromString(PairText));
        DiagnosticsSessionComparePairText->SetColorAndOpacity(FSlateColor(bHasSessionCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }
    if (DiagnosticsSessionCompareStatsText)
    {
        const FString Stats = bHasSessionCompare
            ? FString::Printf(
                TEXT("Shared=%d | LeftOnly=%d | RightOnly=%d | Mismatch=%d"),
                SessionCompareReport.SharedTargetCount,
                SessionCompareReport.LeftOnlyCount,
                SessionCompareReport.RightOnlyCount,
                SessionCompareReport.MismatchCount)
            : TEXT("No session compare stats.");
        DiagnosticsSessionCompareStatsText->SetText(FText::FromString(Stats));
        DiagnosticsSessionCompareStatsText->SetColorAndOpacity(FSlateColor(bHasSessionCompare && SessionCompareReport.MismatchCount > 0 ? RI_TestWarningColor() : (bHasSessionCompare ? RI_TestTextColor() : RI_TestMutedTextColor())));
    }
    if (DiagnosticsSessionComparePreviewText)
    {
        FString PreviewText = TEXT("No session compare preview.");
        if (bHasSessionCompare)
        {
            if (SessionCompareReport.Lines.Num() > 0)
            {
                const FRIRuntimeSessionTargetSetCompareLine& FirstLine = SessionCompareReport.Lines[0];
                PreviewText = FString::Printf(TEXT("%s :: L=%d R=%d | %s"), *FirstLine.DisplayLabel, FirstLine.LeftCount, FirstLine.RightCount, *FirstLine.Message);
            }
            else
            {
                PreviewText = SessionCompareReport.Summary;
            }
        }
        DiagnosticsSessionComparePreviewText->SetText(FText::FromString(PreviewText));
        DiagnosticsSessionComparePreviewText->SetColorAndOpacity(FSlateColor(bHasSessionCompare ? RI_TestTextColor() : RI_TestMutedTextColor()));
    }

    if (SelectedResultStateText)
    {
        if (bViewingWorkflowReport && LastWorkflowRunResult.WorkflowId == SelectedWorkflowId && SelectedWorkflowId != NAME_None)
        {
            SelectedResultStateText->SetText(FText::FromString(FString::Printf(
                TEXT("%s • %s"),
                LastWorkflowRunResult.bPassed ? TEXT("PASS") : (LastWorkflowRunResult.bBlocked ? TEXT("BLOCKED") : TEXT("FAIL")),
                *LastWorkflowRunResult.DisplayName)));
            SelectedResultStateText->SetColorAndOpacity(FSlateColor(
                LastWorkflowRunResult.bPassed ? RI_TestSuccessColor() : (LastWorkflowRunResult.bBlocked ? RI_TestWarningColor() : RI_TestErrorColor())));
        }
        else if (const FRISelfTestResult* SelectedResult = FindResultById(SelectedResultId))
        {
            SelectedResultStateText->SetText(FText::FromString(FString::Printf(TEXT("%s • %s"), SelectedResult->bPassed ? TEXT("PASS") : TEXT("FAIL"), *SelectedResult->DisplayName)));
            SelectedResultStateText->SetColorAndOpacity(FSlateColor(SelectedResult->bPassed ? RI_TestSuccessColor() : RI_TestErrorColor()));
        }
        else
        {
            SelectedResultStateText->SetText(FText::FromString(TEXT("No result selected")));
            SelectedResultStateText->SetColorAndOpacity(FSlateColor(RI_TestMutedTextColor()));
        }
    }
    if (SelectedResultReportText)
    {
        if (bViewingWorkflowReport && LastWorkflowRunResult.WorkflowId == SelectedWorkflowId && SelectedWorkflowId != NAME_None)
        {
            SelectedResultReportText->SetText(FText::FromString(LastWorkflowRunResult.FullReport));
        }
        else if (const FRISelfTestResult* SelectedResult = FindResultById(SelectedResultId))
        {
            SelectedResultReportText->SetText(FText::FromString(SelectedResult->FullReport));
        }
        else
        {
            SelectedResultReportText->SetText(FText::FromString(TEXT("Run a test or workflow to see the detailed report.")));
        }
        SelectedResultReportText->SetColorAndOpacity(FSlateColor(RI_TestMutedTextColor()));
    }

    // The dynamic row widgets are replaced as one synchronous batch below.
    // Drop the previous payload proxies first, then retain exactly one proxy
    // for every configured Select/Run/View button created by the new rows.
    ConfiguredActionBindings.Reset();
    RebuildAvailableWorkflows();
    RebuildAvailableTests();
    RebuildResults();
    RebuildActivityLog();
}

void UInspectorTestPageWidget::SetStatusMessage(const FString& InMessage, bool bIsError)
{
    StatusMessage = InMessage;
    bStatusIsError = bIsError;
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        InspectorSubsystem->AppendActivityLog(
            bIsError ? ERIToastType::Error : ERIToastType::Info,
            TEXT("Tools"),
            InMessage);
    }
}

void UInspectorTestPageWidget::ClearStatusMessage()
{
    StatusMessage.Reset();
    bStatusIsError = false;
}

void UInspectorTestPageWidget::PullRemoteSessionContextFromSubsystem()
{
    const UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return;
    }

    const FString PreferredSessionId = InspectorSubsystem->GetPreferredRemoteSessionId().TrimStartAndEnd();
    if (!PreferredSessionId.IsEmpty())
    {
        SelectedRemoteSessionId = PreferredSessionId;
    }

    if (RemoteSessionWorkflowBox)
    {
        const FString StoredWorkflowId = InspectorSubsystem->GetLastRemoteSessionWorkflowId().TrimStartAndEnd();
        const FString CurrentWorkflowId = RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd();
        if (!StoredWorkflowId.IsEmpty() && !StoredWorkflowId.Equals(CurrentWorkflowId, ESearchCase::CaseSensitive))
        {
            RemoteSessionWorkflowBox->SetText(FText::FromString(StoredWorkflowId));
        }
    }

    if (RemoteSessionTargetQueryBox)
    {
        const FString StoredTargetQuery = InspectorSubsystem->GetLastRemoteSessionTargetQuery().TrimStartAndEnd();
        const FString CurrentTargetQuery = RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd();
        if (!StoredTargetQuery.IsEmpty() && !StoredTargetQuery.Equals(CurrentTargetQuery, ESearchCase::CaseSensitive))
        {
            RemoteSessionTargetQueryBox->SetText(FText::FromString(StoredTargetQuery));
        }
    }
}

void UInspectorTestPageWidget::PushRemoteSessionContextToSubsystem()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem || !RemoteSessionWorkflowBox || !RemoteSessionTargetQueryBox)
    {
        return;
    }

    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSession(AvailableRemoteSessions, SelectedRemoteSessionId);
    const FString SelectionSummary = SelectedSession
        ? RI_BuildRemoteSessionSummaryText(*SelectedSession)
        : SelectedRemoteSessionId;
    const FString WorkflowIdText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FString TargetQueryText = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    InspectorSubsystem->SetRemoteSessionUIContext(SelectedRemoteSessionId, SelectionSummary, TargetQueryText, WorkflowIdText);
}

void UInspectorTestPageWidget::RefreshRemoteSessionSelection()
{
    if (RemoteSessionComboBox)
    {
        RemoteSessionComboBox->ClearOptions();
        for (const FRIRuntimeSessionInfo& SessionInfo : AvailableRemoteSessions)
        {
            RemoteSessionComboBox->AddOption(SessionInfo.SessionId);
        }
    }

    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSession(AvailableRemoteSessions, SelectedRemoteSessionId);
    if (!SelectedSession && AvailableRemoteSessions.Num() > 0)
    {
        const FRIRuntimeSessionInfo* PreferredSession = AvailableRemoteSessions.FindByPredicate([](const FRIRuntimeSessionInfo& SessionInfo)
        {
            return !RI_IsLocalRuntimeSessionId(SessionInfo.SessionId);
        });

        SelectedSession = PreferredSession ? PreferredSession : &AvailableRemoteSessions[0];
        SelectedRemoteSessionId = SelectedSession->SessionId;
    }
    else if (!SelectedSession)
    {
        SelectedRemoteSessionId.Reset();
    }

    if (RemoteSessionComboBox)
    {
        if (!SelectedRemoteSessionId.IsEmpty())
        {
            RemoteSessionComboBox->SetSelectedOption(SelectedRemoteSessionId);
        }
        else
        {
            RemoteSessionComboBox->ClearSelection();
        }
    }
}

bool UInspectorTestPageWidget::EnsureRemoteSessionConnected()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Workflow subsystem is unavailable."), true);
        return false;
    }

    if (SelectedRemoteSessionId.IsEmpty() || RI_IsLocalRuntimeSessionId(SelectedRemoteSessionId))
    {
        return true;
    }

    FRIRuntimeSessionInfo ConnectedSession;
    FString Error;
    if (!InspectorSubsystem->ConnectRemoteRuntimeSession(SelectedRemoteSessionId, ConnectedSession, Error))
    {
        SetStatusMessage(Error.IsEmpty() ? TEXT("Failed to connect remote session.") : Error, true);
        UpdateUIFromState();
        return false;
    }

    if (!ConnectedSession.SessionId.IsEmpty())
    {
        SelectedRemoteSessionId = ConnectedSession.SessionId;
    }
    return true;
}

bool UInspectorTestPageWidget::RunSingleTest(FName TestId)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Self-test subsystem is unavailable."), true);
        UpdateUIFromState();
        return false;
    }

    bRunning = true;
    SetStatusMessage(FString::Printf(TEXT("Running %s..."), *TestId.ToString()), false);
    UpdateUIFromState();

    FRISelfTestResult Result;
    const bool bPassed = InspectorSubsystem->RunSelfTestById(TestId, Result);

    bRunning = false;
    Results = InspectorSubsystem->GetLastSelfTestResults();
    SelectedResultId = Result.Id;
    SetStatusMessage(Result.Summary.IsEmpty() ? Result.FullReport : Result.Summary, !bPassed);
    UpdateUIFromState();
    return bPassed;
}

bool UInspectorTestPageWidget::RunSingleWorkflow(FName WorkflowId)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Workflow subsystem is unavailable."), true);
        UpdateUIFromState();
        return false;
    }

    if (!EnsureRemoteSessionConnected())
    {
        return false;
    }

    bRunning = true;
    bViewingWorkflowReport = true;
    SetStatusMessage(FString::Printf(TEXT("Running workflow %s..."), *WorkflowId.ToString()), false);
    UpdateUIFromState();

    const FString ActorQuery = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    const bool bRunOnExplicitSession = !SelectedRemoteSessionId.IsEmpty() && !RI_IsLocalRuntimeSessionId(SelectedRemoteSessionId);

    bool bPassed = false;
    if (bRunOnExplicitSession)
    {
        FString Error;
        bPassed = InspectorSubsystem->RunWorkflowOnRuntimeSession(SelectedRemoteSessionId, WorkflowId, ActorQuery, LastWorkflowRunResult, Error);
        if (!bPassed && LastWorkflowRunResult.Summary.IsEmpty() && !Error.IsEmpty())
        {
            LastWorkflowRunResult.WorkflowId = WorkflowId;
            LastWorkflowRunResult.DisplayName = WorkflowId.ToString();
            LastWorkflowRunResult.Summary = Error;
            LastWorkflowRunResult.FullReport = Error;
        }
    }
    else
    {
        bPassed = InspectorSubsystem->RunWorkflowById(WorkflowId, LastWorkflowRunResult);
    }

    bRunning = false;
    AvailableWorkflows = InspectorSubsystem->GetAvailableWorkflows();
    AvailableTests = InspectorSubsystem->GetAvailableSelfTests();
    Results = InspectorSubsystem->GetLastSelfTestResults();
    SetStatusMessage(LastWorkflowRunResult.Summary.IsEmpty() ? LastWorkflowRunResult.FullReport : LastWorkflowRunResult.Summary, !(bPassed || LastWorkflowRunResult.bBlocked));
    UpdateUIFromState();
    return bPassed;
}

void UInspectorTestPageWidget::BindSelectWorkflowButton(UButton* Button, FName WorkflowId)
{
    if (!Button)
    {
        return;
    }

    UInspectorTestPageButtonBinding* Binding = NewObject<UInspectorTestPageButtonBinding>(this);
    Binding->Initialize(this, WorkflowId, ERIInspectorTestPageButtonAction::SelectWorkflow);
    Button->OnClicked.AddDynamic(Binding, &UInspectorTestPageButtonBinding::HandleClicked);
    ConfiguredActionBindings.Add(Binding);
}

void UInspectorTestPageWidget::BindRunWorkflowButton(UButton* Button, FName WorkflowId)
{
    if (!Button)
    {
        return;
    }

    UInspectorTestPageButtonBinding* Binding = NewObject<UInspectorTestPageButtonBinding>(this);
    Binding->Initialize(this, WorkflowId, ERIInspectorTestPageButtonAction::RunWorkflow);
    Button->OnClicked.AddDynamic(Binding, &UInspectorTestPageButtonBinding::HandleClicked);
    ConfiguredActionBindings.Add(Binding);
}

void UInspectorTestPageWidget::BindSelectTestButton(UButton* Button, FName TestId)
{
    if (!Button)
    {
        return;
    }

    UInspectorTestPageButtonBinding* Binding = NewObject<UInspectorTestPageButtonBinding>(this);
    Binding->Initialize(this, TestId, ERIInspectorTestPageButtonAction::SelectSelfTest);
    Button->OnClicked.AddDynamic(Binding, &UInspectorTestPageButtonBinding::HandleClicked);
    ConfiguredActionBindings.Add(Binding);
}

void UInspectorTestPageWidget::BindRunTestButton(UButton* Button, FName TestId)
{
    if (!Button)
    {
        return;
    }

    UInspectorTestPageButtonBinding* Binding = NewObject<UInspectorTestPageButtonBinding>(this);
    Binding->Initialize(this, TestId, ERIInspectorTestPageButtonAction::RunSelfTest);
    Button->OnClicked.AddDynamic(Binding, &UInspectorTestPageButtonBinding::HandleClicked);
    ConfiguredActionBindings.Add(Binding);
}

void UInspectorTestPageWidget::BindResultViewButton(UButton* Button, FName TestId)
{
    if (!Button)
    {
        return;
    }

    UInspectorTestPageButtonBinding* Binding = NewObject<UInspectorTestPageButtonBinding>(this);
    Binding->Initialize(this, TestId, ERIInspectorTestPageButtonAction::ViewSelfTestResult);
    Button->OnClicked.AddDynamic(Binding, &UInspectorTestPageButtonBinding::HandleClicked);
    ConfiguredActionBindings.Add(Binding);
}

FText UInspectorTestPageWidget::GetSelectedWorkflowDisplayText() const
{
    const FRIWorkflowDefinition* SelectedDefinition = FindWorkflowById(SelectedWorkflowId);
    return FText::FromString(SelectedDefinition ? SelectedDefinition->DisplayName : TEXT("None"));
}

FText UInspectorTestPageWidget::GetSelectedTestDisplayText() const
{
    const FRISelfTestDefinition* SelectedDefinition = AvailableTests.FindByPredicate([this](const FRISelfTestDefinition& Definition)
    {
        return Definition.Id == SelectedTestId;
    });
    return FText::FromString(SelectedDefinition ? SelectedDefinition->DisplayName : TEXT("None"));
}

const FRIWorkflowDefinition* UInspectorTestPageWidget::FindWorkflowById(FName WorkflowId) const
{
    return AvailableWorkflows.FindByPredicate([WorkflowId](const FRIWorkflowDefinition& Definition)
    {
        return Definition.WorkflowId == WorkflowId;
    });
}

const FRISelfTestResult* UInspectorTestPageWidget::FindResultById(FName TestId) const
{
    return Results.FindByPredicate([TestId](const FRISelfTestResult& Result)
    {
        return Result.Id == TestId;
    });
}

void UInspectorTestPageWidget::HandleRunSelectedWorkflowClicked()
{
    if (SelectedWorkflowId != NAME_None)
    {
        RunSingleWorkflow(SelectedWorkflowId);
    }
}

void UInspectorTestPageWidget::HandleSelectSafePatchWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdSafePatch);
}

void UInspectorTestPageWidget::HandleSelectPromoteWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdPromote);
}

void UInspectorTestPageWidget::HandleSelectActorPatchWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdActorPatch);
}

void UInspectorTestPageWidget::HandleSelectActorPromoteWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdActorPromote);
}

void UInspectorTestPageWidget::HandleSelectActorApplyWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdActorApply);
}

void UInspectorTestPageWidget::HandleSelectActorEndToEndWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdActorEndToEnd);
}

void UInspectorTestPageWidget::HandleSelectFullClosureWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdFullClosure);
}

void UInspectorTestPageWidget::HandleSelectRemotePackagedFoundationWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemotePackagedFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemotePackagedPatchPullWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemotePackagedPatchPull);
}

void UInspectorTestPageWidget::HandleSelectRemotePackagedToSourceClosureWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemotePackagedToSourceClosure);
}

void UInspectorTestPageWidget::HandleSelectRemotePackagedMatrixWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemotePackagedMatrix);
}

void UInspectorTestPageWidget::HandleSelectRemoteActorEndToEndWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteActorEndToEnd);
}

void UInspectorTestPageWidget::HandleSelectRoleCompareFoundationWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRoleCompareFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteRuntimeFoundationWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteRuntimeFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionCompareWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionCompareFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionTargetSetCompareWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionTargetSetCompareFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionCompareUIWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionCompareUIFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionCompareScopedUIWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionCompareScopedUIFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionCompareMatrixWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionCompareMatrixFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteSessionContextUIWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteSessionContextUIFoundation);
}

void UInspectorTestPageWidget::HandleSelectRemoteWorkflowMatrixWorkflowClicked()
{
    HandleWorkflowSelected(RI_WorkflowIdRemoteWorkflowMatrixFoundation);
}

void UInspectorTestPageWidget::HandleRunSafePatchWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdSafePatch);
}

void UInspectorTestPageWidget::HandleRunPromoteWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdPromote);
}

void UInspectorTestPageWidget::HandleRunActorPatchWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdActorPatch);
}

void UInspectorTestPageWidget::HandleRunActorPromoteWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdActorPromote);
}

void UInspectorTestPageWidget::HandleRunActorApplyWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdActorApply);
}

void UInspectorTestPageWidget::HandleRunActorEndToEndWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdActorEndToEnd);
}

void UInspectorTestPageWidget::HandleRunFullClosureWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdFullClosure);
}

void UInspectorTestPageWidget::HandleRunRemotePackagedFoundationWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemotePackagedFoundation);
}

void UInspectorTestPageWidget::HandleRunRemotePackagedPatchPullWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemotePackagedPatchPull);
}

void UInspectorTestPageWidget::HandleRunRemotePackagedToSourceClosureWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemotePackagedToSourceClosure);
}

void UInspectorTestPageWidget::HandleRunRemotePackagedMatrixWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemotePackagedMatrix);
}

void UInspectorTestPageWidget::HandleRunRemoteActorEndToEndWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteActorEndToEnd);
}

void UInspectorTestPageWidget::HandleRunRoleCompareFoundationWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRoleCompareFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteRuntimeFoundationWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteRuntimeFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionCompareWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionCompareFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionTargetSetCompareWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionTargetSetCompareFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionCompareUIWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionCompareUIFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionCompareScopedUIWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionCompareScopedUIFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionCompareMatrixWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionCompareMatrixFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteSessionContextUIWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteSessionContextUIFoundation);
}

void UInspectorTestPageWidget::HandleRunRemoteWorkflowMatrixWorkflowClicked()
{
    HandleWorkflowRunRequested(RI_WorkflowIdRemoteWorkflowMatrixFoundation);
}

void UInspectorTestPageWidget::HandleRunSelectedClicked()
{
    if (SelectedTestId != NAME_None)
    {
        bViewingWorkflowReport = false;
        RunSingleTest(SelectedTestId);
    }
}

void UInspectorTestPageWidget::HandleRunAllClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Self-test subsystem is unavailable."), true);
        UpdateUIFromState();
        return;
    }

    bRunning = true;
    bViewingWorkflowReport = false;
    SetStatusMessage(TEXT("Running all self tests..."), false);
    UpdateUIFromState();

    TArray<FRISelfTestResult> RunResults;
    InspectorSubsystem->RunAllSelfTests(RunResults);

    bRunning = false;
    Results = RunResults;
    SelectedResultId = Results.Num() > 0 ? Results.Last().Id : NAME_None;

    int32 PassedCount = 0;
    for (const FRISelfTestResult& Result : Results)
    {
        if (Result.bPassed)
        {
            ++PassedCount;
        }
    }

    SetStatusMessage(
        FString::Printf(TEXT("Completed %d tests. Passed=%d Failed=%d"), Results.Num(), PassedCount, Results.Num() - PassedCount),
        PassedCount != Results.Num());
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleRefreshClicked()
{
    RefreshFromSubsystem();
}

void UInspectorTestPageWidget::HandleToggleTestsSectionClicked()
{
    bTestsSectionExpanded = !bTestsSectionExpanded;
    SetSectionExpanded(AvailableTestsSectionWidget, TestsSectionToggleText, bTestsSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleToggleRemoteSectionClicked()
{
    bRemoteSectionExpanded = !bRemoteSectionExpanded;
    SetSectionExpanded(RemoteSessionSectionWidget, RemoteSectionToggleText, bRemoteSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleToggleDiagnosticsSectionClicked()
{
    bDiagnosticsSectionExpanded = !bDiagnosticsSectionExpanded;
    SetSectionExpanded(DiagnosticsSectionWidget, DiagnosticsSectionToggleText, bDiagnosticsSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleToggleActivityLogSectionClicked()
{
    bActivityLogSectionExpanded = !bActivityLogSectionExpanded;
    SetSectionExpanded(ActivityLogSectionWidget, ActivityLogSectionToggleText, bActivityLogSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleToggleRemoteOverrideSectionClicked()
{
    bRemoteOverrideSectionExpanded = !bRemoteOverrideSectionExpanded;
    SetSectionExpanded(RemoteOverrideSectionWidget, RemoteOverrideToggleText, bRemoteOverrideSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorTestPageWidget::HandleRemoteSessionRefreshClicked()
{
    RefreshFromSubsystem();
}

void UInspectorTestPageWidget::HandleRemoteSessionConnectClicked()
{
    if (SelectedRemoteSessionId.IsEmpty())
    {
        SetStatusMessage(TEXT("No remote session selected."), true);
        UpdateUIFromState();
        return;
    }

    if (RI_IsLocalRuntimeSessionId(SelectedRemoteSessionId))
    {
        SetStatusMessage(FString::Printf(TEXT("Local session %s selected."), *SelectedRemoteSessionId), false);
        UpdateUIFromState();
        return;
    }

    if (!EnsureRemoteSessionConnected())
    {
        return;
    }

    RefreshFromSubsystem();
    PushRemoteSessionContextToSubsystem();
    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSession(AvailableRemoteSessions, SelectedRemoteSessionId);
    SetStatusMessage(
        SelectedSession ? RI_BuildRemoteSessionSummaryText(*SelectedSession) : FString::Printf(TEXT("Connected to %s"), *SelectedRemoteSessionId),
        false);
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleRemoteSessionRunWorkflowClicked()
{
    FString WorkflowIdText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
    if (WorkflowIdText.IsEmpty() && SelectedWorkflowId != NAME_None)
    {
        WorkflowIdText = SelectedWorkflowId.ToString();
        if (RemoteSessionWorkflowBox)
        {
            RemoteSessionWorkflowBox->SetText(FText::FromString(WorkflowIdText));
        }
    }

    if (WorkflowIdText.IsEmpty())
    {
        SetStatusMessage(TEXT("Remote workflow id is empty."), true);
        UpdateUIFromState();
        return;
    }

    if (FindWorkflowById(FName(*WorkflowIdText)) == nullptr)
    {
        SetStatusMessage(FString::Printf(TEXT("Unknown remote workflow id: %s"), *WorkflowIdText), true);
        UpdateUIFromState();
        return;
    }

    SelectedWorkflowId = FName(*WorkflowIdText);
    bViewingWorkflowReport = true;
    PushRemoteSessionContextToSubsystem();
    RunSingleWorkflow(SelectedWorkflowId);
}

void UInspectorTestPageWidget::HandleRemoteSessionWorkflowCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    (void)CommitMethod;

    if (RemoteSessionWorkflowBox)
    {
        RemoteSessionWorkflowBox->SetText(InText);
    }

    PushRemoteSessionContextToSubsystem();
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleRemoteSessionTargetQueryCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    (void)CommitMethod;

    if (RemoteSessionTargetQueryBox)
    {
        RemoteSessionTargetQueryBox->SetText(InText);
    }

    PushRemoteSessionContextToSubsystem();
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleBuildDiagnosticsRoleCompareClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Workflow subsystem is unavailable."), true);
        UpdateUIFromState();
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileBuildRuntimeRoleCompareAction(Summary, Details);
    SetStatusMessage(Summary.IsEmpty() ? (bOk ? TEXT("Role compare built.") : TEXT("Role compare failed.")) : Summary, !bOk);
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleBuildDiagnosticsSessionCompareClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Workflow subsystem is unavailable."), true);
        UpdateUIFromState();
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileBuildRemoteSessionTargetSetCompareAction(Summary, Details);
    SetStatusMessage(Summary.IsEmpty() ? (bOk ? TEXT("Session compare built.") : TEXT("Session compare failed.")) : Summary, !bOk);
    UpdateUIFromState();
}

void UInspectorTestPageWidget::HandleClearActivityLogClicked()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        InspectorSubsystem->ClearActivityLog();
    }

    RefreshFromSubsystem();
}

void UInspectorTestPageWidget::HandleSelectConfirmDialogClicked()
{
    HandleSelfTestSelected(RI_TestIdConfirmDialog);
}

void UInspectorTestPageWidget::HandleSelectSettingsPreviewClicked()
{
    HandleSelfTestSelected(RI_TestIdSettingsPreview);
}

void UInspectorTestPageWidget::HandleSelectSettingsHotkeyClicked()
{
    HandleSelfTestSelected(RI_TestIdSettingsHotkey);
}

void UInspectorTestPageWidget::HandleRunConfirmDialogClicked()
{
    HandleSelfTestRunRequested(RI_TestIdConfirmDialog);
}

void UInspectorTestPageWidget::HandleRunSettingsPreviewClicked()
{
    HandleSelfTestRunRequested(RI_TestIdSettingsPreview);
}

void UInspectorTestPageWidget::HandleRunSettingsHotkeyClicked()
{
    HandleSelfTestRunRequested(RI_TestIdSettingsHotkey);
}

void UInspectorTestPageWidget::HandleViewConfirmDialogResultClicked()
{
    HandleSelfTestResultSelected(RI_TestIdConfirmDialog);
}

void UInspectorTestPageWidget::HandleViewSettingsPreviewResultClicked()
{
    HandleSelfTestResultSelected(RI_TestIdSettingsPreview);
}

void UInspectorTestPageWidget::HandleViewSettingsHotkeyResultClicked()
{
    HandleSelfTestResultSelected(RI_TestIdSettingsHotkey);
}
