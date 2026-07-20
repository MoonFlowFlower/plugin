#include "InspectorSettingsPageWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorTouchScrollBox.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace
{
    static FString RI_GetThemePresetLabel(ERuntimeInspectorThemePreset InPreset)
    {
        switch (InPreset)
        {
        case ERuntimeInspectorThemePreset::SoftContrast:
            return TEXT("Soft Contrast");
        case ERuntimeInspectorThemePreset::StudioSlate:
        default:
            return TEXT("Studio Slate");
        }
    }

    static ERuntimeInspectorThemePreset RI_ParseThemePresetLabel(const FString& InLabel)
    {
        return InLabel.Equals(TEXT("Soft Contrast"), ESearchCase::IgnoreCase)
            ? ERuntimeInspectorThemePreset::SoftContrast
            : ERuntimeInspectorThemePreset::StudioSlate;
    }

    static FLinearColor RI_SettingsSectionColor() { return RICompactUI::GetSectionSurfaceBackgroundColor(); }
    static FLinearColor RI_SettingsRowColor() { return RICompactUI::GetRowSurfaceBackgroundColor(); }
    static FLinearColor RI_SettingsTextColor() { return RICompactUI::GetStrongTextColor(); }
    static FLinearColor RI_SettingsMutedTextColor() { return RICompactUI::GetMutedTextColor(); }
    static FLinearColor RI_SettingsWarningColor() { return RICompactUI::GetWarningTextColor(); }
    static FLinearColor RI_SettingsErrorColor() { return RICompactUI::GetErrorTextColor(); }
    static FLinearColor RI_SettingsSuccessColor() { return RICompactUI::GetSuccessTextColor(); }

    static FString RI_BoolLabel(bool bValue, const TCHAR* TrueLabel, const TCHAR* FalseLabel)
    {
        return bValue ? FString(TrueLabel) : FString(FalseLabel);
    }

    static void RI_ApplyTextStyle(UTextBlock* TextBlock, int32 Size, bool bBold, const FLinearColor& Color)
    {
        RICompactUI::ApplyTextStyle(TextBlock, Size, bBold, Color);
    }
}

UInspectorSettingsPageWidget::UInspectorSettingsPageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

bool UInspectorSettingsPageWidget::HasTouchScrollSupportForAutomation() const
{
    return RIInspectorTouchScroll::HasTouchSupport(PageScrollBox);
}

void UInspectorSettingsPageWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    CancelDeferredRefresh();
    Subsystem = InSubsystem;
    RefreshFromSubsystem();
}

TSharedRef<SWidget> UInspectorSettingsPageWidget::RebuildWidget()
{
    const bool bHasUsableBlueprintLayout = PageScrollBox
        || (WidgetTree && (WidgetTree->FindWidget(TEXT("PageScrollBox")) != nullptr
            || WidgetTree->FindWidget(TEXT("RI_SettingsScroll")) != nullptr));

    if (WidgetTree && (!WidgetTree->RootWidget || !bHasUsableBlueprintLayout))
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorSettingsPageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!PageScrollBox && WidgetTree)
    {
        PageScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("PageScrollBox")));
        if (!PageScrollBox)
        {
            PageScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_SettingsScroll")));
        }
    }
    RIInspectorTouchScroll::Configure(PageScrollBox);
    bControlsReady = false;
    BindControlDelegates();
    RefreshFromSubsystem();
    bControlsReady = true;
}

void UInspectorSettingsPageWidget::NativeDestruct()
{
    CancelDeferredRefresh();
    bControlsReady = false;
    PendingCaptureTarget = ERIHotkeyCaptureTarget::None;
    Super::NativeDestruct();
}

void UInspectorSettingsPageWidget::ScheduleDeferredRefresh()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bDeferredRefreshScheduled = true;
    World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
    World->GetTimerManager().SetTimer(
        DeferredRefreshTimerHandle,
        this,
        &UInspectorSettingsPageWidget::HandleDeferredRefreshTimerElapsed,
        0.0f,
        false);
}

void UInspectorSettingsPageWidget::CancelDeferredRefresh()
{
    bDeferredRefreshScheduled = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
    }
}

void UInspectorSettingsPageWidget::HandleDeferredRefreshTimerElapsed()
{
    CancelDeferredRefresh();
    const double StartSeconds = FPlatformTime::Seconds();
    RefreshFromSubsystem();
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] Snapshot DeferredRefresh %.2f ms"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
}

bool UInspectorSettingsPageWidget::ShouldIgnoreControlChange() const
{
    return bRefreshingUI || !bControlsReady;
}

void UInspectorSettingsPageWidget::ResetControlReferences()
{
    ToggleKeyValueText = nullptr;
    PickKeyValueText = nullptr;
    DirtyStateText = nullptr;
    StatusMessageText = nullptr;
    CaptureHintText = nullptr;
    RuntimeEnabledValueText = nullptr;
    DisabledReasonValueText = nullptr;
    SessionValueText = nullptr;
    NetModeValueText = nullptr;
    SelectedActorValueText = nullptr;
    SelectedRoleValueText = nullptr;
    LockStateValueText = nullptr;
    UnlockCodeValueText = nullptr;
    OutlineMaterialValueText = nullptr;
    CustomDepthValueText = nullptr;
    PageScrollBox = nullptr;
    ThemePresetComboBox = nullptr;
    ToggleKeyButton = nullptr;
    PickKeyButton = nullptr;
    SaveButton = nullptr;
    ResetButton = nullptr;
    PickRequiresCtrlCheckBox = nullptr;
    PickRequiresShiftCheckBox = nullptr;
    EnableRightMousePickCheckBox = nullptr;
    RightMousePickRequiresCtrlCheckBox = nullptr;
    RightMousePickRequiresShiftCheckBox = nullptr;
    EnableOutlineCheckBox = nullptr;
    EnableApplyDebounceCheckBox = nullptr;
    RequireUnlockCheckBox = nullptr;
    AutoLockOnCloseCheckBox = nullptr;
    OutlineWeightSpinBox = nullptr;
    ApplyDebounceSecondsSpinBox = nullptr;
    UIScaleSpinBox = nullptr;
}

void UInspectorSettingsPageWidget::EnsureThemePresetOptions()
{
    if (!ThemePresetComboBox)
    {
        return;
    }

    ThemePresetComboBox->ClearOptions();
    ThemePresetComboBox->AddOption(RI_GetThemePresetLabel(ERuntimeInspectorThemePreset::StudioSlate));
    ThemePresetComboBox->AddOption(RI_GetThemePresetLabel(ERuntimeInspectorThemePreset::SoftContrast));
    RICompactUI::ConfigureComboBoxString(ThemePresetComboBox, RI_SettingsTextColor(), 180.0f, RICompactUI::ERIInputVisualStyle::Strong);
}

void UInspectorSettingsPageWidget::BindControlDelegates()
{
    EnsureThemePresetOptions();

    if (ToggleKeyButton)
    {
        ToggleKeyButton->OnClicked.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleToggleKeyCaptureClicked);
        ToggleKeyButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleToggleKeyCaptureClicked);
    }
    if (PickKeyButton)
    {
        PickKeyButton->OnClicked.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandlePickKeyCaptureClicked);
        PickKeyButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickKeyCaptureClicked);
    }
    if (PickRequiresCtrlCheckBox)
    {
        PickRequiresCtrlCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresCtrlChanged);
        PickRequiresCtrlCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresCtrlChanged);
    }
    if (PickRequiresShiftCheckBox)
    {
        PickRequiresShiftCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresShiftChanged);
        PickRequiresShiftCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresShiftChanged);
    }
    if (EnableRightMousePickCheckBox)
    {
        EnableRightMousePickCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleEnableRightMousePickChanged);
        EnableRightMousePickCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableRightMousePickChanged);
    }
    if (RightMousePickRequiresCtrlCheckBox)
    {
        RightMousePickRequiresCtrlCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresCtrlChanged);
        RightMousePickRequiresCtrlCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresCtrlChanged);
    }
    if (RightMousePickRequiresShiftCheckBox)
    {
        RightMousePickRequiresShiftCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresShiftChanged);
        RightMousePickRequiresShiftCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresShiftChanged);
    }
    if (EnableOutlineCheckBox)
    {
        EnableOutlineCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleEnableOutlineChanged);
        EnableOutlineCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableOutlineChanged);
    }
    if (OutlineWeightSpinBox)
    {
        OutlineWeightSpinBox->OnValueChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleOutlineWeightChanged);
        OutlineWeightSpinBox->OnValueChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleOutlineWeightChanged);
    }
    if (UIScaleSpinBox)
    {
        UIScaleSpinBox->OnValueChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleUIScaleChanged);
        UIScaleSpinBox->OnValueChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleUIScaleChanged);
    }
    if (EnableApplyDebounceCheckBox)
    {
        EnableApplyDebounceCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleEnableApplyDebounceChanged);
        EnableApplyDebounceCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableApplyDebounceChanged);
    }
    if (ApplyDebounceSecondsSpinBox)
    {
        ApplyDebounceSecondsSpinBox->OnValueChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleApplyDebounceSecondsChanged);
        ApplyDebounceSecondsSpinBox->OnValueChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleApplyDebounceSecondsChanged);
    }
    if (ThemePresetComboBox)
    {
        ThemePresetComboBox->OnSelectionChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleThemePresetSelectionChanged);
        ThemePresetComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleThemePresetSelectionChanged);
        ThemePresetComboBox->OnGenerateWidgetEvent.Unbind();
        ThemePresetComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UInspectorSettingsPageWidget::HandleGenerateThemePresetOptionWidget);
    }
    if (RequireUnlockCheckBox)
    {
        RequireUnlockCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleRequireUnlockChanged);
        RequireUnlockCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRequireUnlockChanged);
    }
    if (AutoLockOnCloseCheckBox)
    {
        AutoLockOnCloseCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleAutoLockOnCloseChanged);
        AutoLockOnCloseCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleAutoLockOnCloseChanged);
    }
    if (SaveButton)
    {
        SaveButton->OnClicked.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleSaveClicked);
        SaveButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleSaveClicked);
    }
    if (ResetButton)
    {
        ResetButton->OnClicked.RemoveDynamic(this, &UInspectorSettingsPageWidget::HandleResetClicked);
        ResetButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleResetClicked);
    }
}

FReply UInspectorSettingsPageWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (PendingCaptureTarget == ERIHotkeyCaptureTarget::None)
    {
        return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
    }

    if (InKeyEvent.IsRepeat())
    {
        return FReply::Handled();
    }

    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        CancelHotkeyCapture(true);
        return FReply::Handled();
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        CancelHotkeyCapture(false);
        SetStatusMessage(TEXT("Inspector settings are unavailable."), true);
        return FReply::Handled();
    }

    FString Error;
    if (!InspectorSubsystem->ValidateHotkeyCandidate(InKeyEvent.GetKey(), Error))
    {
        SetStatusMessage(Error, true);
        return FReply::Handled();
    }

    FRIEditableSettings Candidate = DraftSettings;
    if (PendingCaptureTarget == ERIHotkeyCaptureTarget::Toggle)
    {
        Candidate.ToggleKey = InKeyEvent.GetKey();
    }
    else
    {
        Candidate.PickKey = InKeyEvent.GetKey();
    }

    if (!ApplyPreviewSettings(Candidate))
    {
        return FReply::Handled();
    }

    CancelHotkeyCapture(false);
    return FReply::Handled();
}

void UInspectorSettingsPageWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    ResetControlReferences();

    UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_SettingsRoot"));
    RootBorder->SetPadding(RICompactUI::GetPanelPadding());
    RootBorder->SetBrushColor(RICompactUI::GetPageBackgroundColor());

    UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsMainBox"));
    RootBorder->SetContent(MainBox);

    PageScrollBox = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("PageScrollBox"));
    RIInspectorTouchScroll::Configure(PageScrollBox);
    if (UVerticalBoxSlot* ScrollSlot = MainBox->AddChildToVerticalBox(PageScrollBox))
    {
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsContentBox"));
    PageScrollBox->AddChild(ContentBox);

    auto AddContentWidget = [ContentBox](UWidget* Child, const FMargin& SlotPadding)
    {
        if (!Child)
        {
            return;
        }

        if (UVerticalBoxSlot* Slot = ContentBox->AddChildToVerticalBox(Child))
        {
            Slot->SetPadding(SlotPadding);
        }
    };

    AddContentWidget(CreateSectionTitle(TEXT("Settings Workspace"), true), FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    AddContentWidget(
        RICompactUI::MakeText(
            WidgetTree,
            TEXT("Configure runtime interaction, appearance, and safety without leaving the active workflow."),
            RICompactUI::GetMutedFontSize(),
            false,
            RI_SettingsMutedTextColor(),
            true),
        FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

    auto AddGroupCard = [this, AddContentWidget](const FString& Title, const FString& Subtitle, bool bEmphasis, const FName& Name) -> UVerticalBox*
    {
        UBorder* Card = RICompactUI::MakeSurfaceCard(WidgetTree, Name, RI_SettingsSectionColor());
        UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Card->SetContent(CardBox);

        if (UVerticalBoxSlot* Slot = CardBox->AddChildToVerticalBox(CreateSectionTitle(Title, bEmphasis)))
        {
            Slot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
        }

        if (!Subtitle.IsEmpty())
        {
            if (UVerticalBoxSlot* Slot = CardBox->AddChildToVerticalBox(
                RICompactUI::MakeText(WidgetTree, Subtitle, RICompactUI::GetMutedFontSize(), false, RI_SettingsMutedTextColor(), true)))
            {
                Slot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
            }
        }

        AddContentWidget(Card, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
        return CardBox;
    };

    auto AddCardChild = [](UVerticalBox* CardBox, UWidget* Child, bool bLast = false)
    {
        if (!CardBox || !Child)
        {
            return;
        }

        if (UVerticalBoxSlot* Slot = CardBox->AddChildToVerticalBox(Child))
        {
            Slot->SetPadding(FMargin(0.f, 0.f, 0.f, bLast ? 0.f : RICompactUI::GetInlineGap()));
        }
    };

    UVerticalBox* InteractionCard = AddGroupCard(
        TEXT("Interaction"),
        TEXT("How the panel opens, picks targets, and responds to input."),
        true,
        TEXT("RI_SettingsInteractionCard"));
    AddCardChild(InteractionCard, CreateKeybindRow(TEXT("Toggle Key"), ToggleKeyValueText, ToggleKeyButton, TEXT("ToggleKeyValueText"), TEXT("ToggleKeyButton")));
    AddCardChild(InteractionCard, CreateKeybindRow(TEXT("Pick Key"), PickKeyValueText, PickKeyButton, TEXT("PickKeyValueText"), TEXT("PickKeyButton")));
    AddCardChild(InteractionCard, CreateCheckRow(TEXT("Pick Requires Ctrl"), PickRequiresCtrlCheckBox, TEXT("PickRequiresCtrlCheckBox")));
    AddCardChild(InteractionCard, CreateCheckRow(TEXT("Pick Requires Shift"), PickRequiresShiftCheckBox, TEXT("PickRequiresShiftCheckBox")));
    AddCardChild(InteractionCard, CreateCheckRow(TEXT("Enable Ctrl+RMB Pick"), EnableRightMousePickCheckBox, TEXT("EnableRightMousePickCheckBox")));
    AddCardChild(InteractionCard, CreateCheckRow(TEXT("RMB Requires Ctrl"), RightMousePickRequiresCtrlCheckBox, TEXT("RightMousePickRequiresCtrlCheckBox")));
    AddCardChild(InteractionCard, CreateCheckRow(TEXT("RMB Requires Shift"), RightMousePickRequiresShiftCheckBox, TEXT("RightMousePickRequiresShiftCheckBox")), true);

    UVerticalBox* AppearanceCard = AddGroupCard(
        TEXT("Appearance"),
        TEXT("Tune outline behavior and theme without leaving the active runtime session."),
        true,
        TEXT("RI_SettingsAppearanceCard"));
    AddCardChild(AppearanceCard, CreateCheckRow(TEXT("Enable Outline"), EnableOutlineCheckBox, TEXT("EnableOutlineCheckBox")));
    AddCardChild(AppearanceCard, CreateSpinRow(TEXT("Outline Weight"), OutlineWeightSpinBox, TEXT("OutlineWeightSpinBox")));
    AddCardChild(AppearanceCard, CreateSpinRow(TEXT("UI Scale"), UIScaleSpinBox, TEXT("UIScaleSpinBox")));
    AddCardChild(AppearanceCard, CreateThemePresetRow(TEXT("Theme Preset"), ThemePresetComboBox, TEXT("ThemePresetComboBox")), true);

    UVerticalBox* ApplyCard = AddGroupCard(
        TEXT("Apply Flow"),
        TEXT("Reduce noisy writes while preserving responsive runtime editing."),
        true,
        TEXT("RI_SettingsApplyCard"));
    AddCardChild(ApplyCard, CreateCheckRow(TEXT("Enable Apply Debounce"), EnableApplyDebounceCheckBox, TEXT("EnableApplyDebounceCheckBox")));
    AddCardChild(ApplyCard, CreateSpinRow(TEXT("Debounce Seconds"), ApplyDebounceSecondsSpinBox, TEXT("ApplyDebounceSecondsSpinBox")), true);

    UVerticalBox* StatusCard = AddGroupCard(
        TEXT("Security & Runtime Status"),
        TEXT("Current guardrails and session health for the active inspector authority."),
        true,
        TEXT("RI_SettingsStatusCard"));
    AddCardChild(StatusCard, CreateCheckRow(TEXT("Require Unlock"), RequireUnlockCheckBox, TEXT("RequireUnlockCheckBox")));
    AddCardChild(StatusCard, CreateCheckRow(TEXT("Auto Lock On Close"), AutoLockOnCloseCheckBox, TEXT("AutoLockOnCloseCheckBox")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Runtime"), RuntimeEnabledValueText, false, TEXT("RuntimeEnabledValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Disabled Reason"), DisabledReasonValueText, true, TEXT("DisabledReasonValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Session"), SessionValueText, false, TEXT("SessionValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Net Mode"), NetModeValueText, false, TEXT("NetModeValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Selected Actor"), SelectedActorValueText, true, TEXT("SelectedActorValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Selected Role"), SelectedRoleValueText, false, TEXT("SelectedRoleValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Lock State"), LockStateValueText, false, TEXT("LockStateValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Unlock Code"), UnlockCodeValueText, false, TEXT("UnlockCodeValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Outline Material"), OutlineMaterialValueText, true, TEXT("OutlineMaterialValueText")));
    AddCardChild(StatusCard, CreateStatusRow(TEXT("Custom Depth Stencil"), CustomDepthValueText, false, TEXT("CustomDepthValueText")), true);

    UBorder* FooterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_SettingsFooter"));
    FooterBorder->SetPadding(RICompactUI::GetSurfaceCardPadding());
    FooterBorder->SetBrushColor(RICompactUI::GetFooterBackgroundColor());
    if (UVerticalBoxSlot* FooterSlot = MainBox->AddChildToVerticalBox(FooterBorder))
    {
        FooterSlot->SetPadding(FMargin(0.f, RICompactUI::GetSectionGap(), 0.f, 0.f));
        FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    UVerticalBox* FooterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsFooterBox"));
    FooterBorder->SetContent(FooterBox);

    StatusMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusMessageText"));
    StatusMessageText->SetAutoWrapText(true);
    RI_ApplyTextStyle(StatusMessageText, RICompactUI::GetValueFontSize(), false, RI_SettingsMutedTextColor());
    if (UVerticalBoxSlot* StatusSlot = FooterBox->AddChildToVerticalBox(StatusMessageText))
    {
        StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    CaptureHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CaptureHintText"));
    CaptureHintText->SetAutoWrapText(true);
    RI_ApplyTextStyle(CaptureHintText, RICompactUI::GetValueFontSize(), false, RI_SettingsWarningColor());
    if (UVerticalBoxSlot* HintSlot = FooterBox->AddChildToVerticalBox(CaptureHintText))
    {
        HintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    DirtyStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DirtyStateText"));
    RI_ApplyTextStyle(DirtyStateText, RICompactUI::GetSectionTitleFontSize(), true, RI_SettingsWarningColor());
    DirtyStateText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* DirtySlot = FooterBox->AddChildToVerticalBox(DirtyStateText))
    {
        DirtySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    UHorizontalBox* FooterButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SettingsFooterButtons"));
    FooterBox->AddChildToVerticalBox(FooterButtonsRow);

    SaveButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("SaveButton"),
        TEXT("Save"),
        RICompactUI::ERIButtonVisualStyle::Primary,
        0.0f);
    if (UHorizontalBoxSlot* SaveSlot = FooterButtonsRow->AddChildToHorizontalBox(SaveButton))
    {
        SaveSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
        SaveSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SaveSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    ResetButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("ResetButton"),
        TEXT("Reset"),
        RICompactUI::ERIButtonVisualStyle::Danger,
        0.0f);
    if (UHorizontalBoxSlot* ResetSlot = FooterButtonsRow->AddChildToHorizontalBox(ResetButton))
    {
        ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ResetSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    WidgetTree->RootWidget = RootBorder;
}

#if WITH_EDITOR
void UInspectorSettingsPageWidget::BuildFallbackWidgetTreeForExport(UWidgetTree* TargetWidgetTree)
{
    if (!TargetWidgetTree)
    {
        return;
    }

    UWidgetTree* OriginalWidgetTree = WidgetTree;
    WidgetTree = TargetWidgetTree;
    TargetWidgetTree->RootWidget = nullptr;
    BuildWidgetTree();
    WidgetTree = OriginalWidgetTree;
}
#endif

UWidget* UInspectorSettingsPageWidget::CreateSectionTitle(const FString& InTitle, bool bEmphasis)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        bEmphasis ? RICompactUI::ERISectionVisualStyle::Emphasis : RICompactUI::ERISectionVisualStyle::Standard);
}

UWidget* UInspectorSettingsPageWidget::CreateKeybindRow(const FString& InLabel, UTextBlock*& OutValueText, UButton*& OutButton, FName ValueTextName, FName ButtonName)
{
    UHorizontalBox* ValueBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ValueTextName);
    RI_ApplyTextStyle(OutValueText, RICompactUI::GetValueFontSize(), true, RI_SettingsTextColor());
    if (UHorizontalBoxSlot* ValueSlot = ValueBox->AddChildToHorizontalBox(OutValueText))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    OutButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        ButtonName,
        TEXT("Rebind"),
        RICompactUI::ERIButtonVisualStyle::Secondary,
        72.0f);
    if (UHorizontalBoxSlot* ButtonSlot = ValueBox->AddChildToHorizontalBox(OutButton))
    {
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    return RICompactUI::MakeStackedContentRow(
        WidgetTree,
        InLabel,
        ValueBox,
        RI_SettingsRowColor(),
        RI_SettingsTextColor());
}

UWidget* UInspectorSettingsPageWidget::CreateCheckRow(const FString& InLabel, UCheckBox*& OutCheckBox, FName CheckBoxName)
{
    UBorder* Border = RICompactUI::MakeSurfaceCard(
        WidgetTree,
        NAME_None,
        RI_SettingsRowColor(),
        FMargin(6.f, 4.f));
    if (!Border)
    {
        return nullptr;
    }

    UHorizontalBox* ValueBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Border->SetContent(ValueBox);

    UTextBlock* LabelText = RICompactUI::MakeText(
        WidgetTree,
        InLabel,
        RICompactUI::GetLabelFontSize(),
        true,
        RI_SettingsTextColor(),
        true);
    if (UHorizontalBoxSlot* LabelSlot = ValueBox->AddChildToHorizontalBox(LabelText))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetHorizontalAlignment(HAlign_Fill);
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    OutCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), CheckBoxName);
    OutCheckBox->SetRenderScale(FVector2D(0.55f, 0.55f));
    if (UHorizontalBoxSlot* CheckBoxSlot = ValueBox->AddChildToHorizontalBox(OutCheckBox))
    {
        CheckBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        CheckBoxSlot->SetHorizontalAlignment(HAlign_Right);
        CheckBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    return Border;
}

UWidget* UInspectorSettingsPageWidget::CreateSpinRow(const FString& InLabel, USpinBox*& OutSpinBox, FName SpinBoxName)
{
    USizeBox* SpinBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SpinBoxSizeBox->SetWidthOverride(92.0f);
    SpinBoxSizeBox->SetHeightOverride(RICompactUI::GetInputHeight());
    OutSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), SpinBoxName);
    OutSpinBox->SetMinDesiredWidth(92.0f);
    OutSpinBox->SetMinFractionalDigits(2);
    OutSpinBox->SetMaxFractionalDigits(3);
    SpinBoxSizeBox->SetContent(OutSpinBox);
    return RICompactUI::MakeStackedContentRow(
        WidgetTree,
        InLabel,
        SpinBoxSizeBox,
        RI_SettingsRowColor(),
        RI_SettingsTextColor());
}

UWidget* UInspectorSettingsPageWidget::CreateThemePresetRow(const FString& InLabel, UComboBoxString*& OutComboBox, FName ComboBoxName)
{
    OutComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), ComboBoxName);
    RICompactUI::ConfigureComboBoxString(OutComboBox, RI_SettingsTextColor(), 180.0f, RICompactUI::ERIInputVisualStyle::Strong);
    EnsureThemePresetOptions();
    return RICompactUI::MakeStackedContentRow(
        WidgetTree,
        InLabel,
        RICompactUI::WrapFixedHeight(WidgetTree, OutComboBox, RICompactUI::GetInputHeight()),
        RI_SettingsRowColor(),
        RI_SettingsTextColor());
}

UWidget* UInspectorSettingsPageWidget::CreateStatusRow(const FString& InLabel, UTextBlock*& OutValueText, bool bWrapValue, FName ValueTextName)
{
    return RICompactUI::MakeStackedValueRow(
        WidgetTree,
        InLabel,
        OutValueText,
        RI_SettingsRowColor(),
        RI_SettingsTextColor(),
        RI_SettingsMutedTextColor(),
        bWrapValue,
        ValueTextName);
}

void UInspectorSettingsPageWidget::RefreshFromSubsystem()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        DraftSettings = InspectorSubsystem->GetEditableSettings();
        Diagnostics = InspectorSubsystem->GetSettingsDiagnostics();
        RuntimeSessionSummary = InspectorSubsystem->GetRuntimeSessionSummary();
        RuntimeActorRoleSummary = InspectorSubsystem->GetSelectedActorRoleSummary();
        ActiveThemePreset = InspectorSubsystem->GetThemePreset();
        UE_LOG(
            LogRuntimeInspector,
            Log,
            TEXT("[RI][SettingsUI] Refresh Outline=%.3f Theme=%d Dirty=%d"),
            DraftSettings.OutlinePPWeight,
            static_cast<int32>(ActiveThemePreset),
            InspectorSubsystem->HasUnsavedSettingsChanges() ? 1 : 0);
    }
    else
    {
        Diagnostics = FRISettingsDiagnostics();
        RuntimeSessionSummary = FRIRuntimeSessionSummary();
        RuntimeActorRoleSummary = FRIRuntimeActorRoleSummary();
        ActiveThemePreset = ERuntimeInspectorThemePreset::StudioSlate;
        ClearStatusMessage();
    }

    CancelHotkeyCapture(false);
    UpdateUIFromState();
}

UWidget* UInspectorSettingsPageWidget::HandleGenerateThemePresetOptionWidget(FString InItemText)
{
    return WidgetTree
        ? RICompactUI::MakeComboBoxItemText(WidgetTree, InItemText, RI_SettingsTextColor())
        : nullptr;
}

void UInspectorSettingsPageWidget::UpdateUIFromState()
{
    TGuardValue<bool> RefreshGuard(bRefreshingUI, true);

    const bool bEditable = Diagnostics.bRuntimeEnabled && Subsystem.IsValid();
    const bool bDirty = Subsystem.IsValid() ? Subsystem->HasUnsavedSettingsChanges() : false;
    const FString EditDisabledReason = Diagnostics.bRuntimeEnabled
        ? TEXT("Open Runtime Inspector before editing settings.")
        : (Diagnostics.DisabledReason.IsEmpty() ? TEXT("Runtime Inspector is disabled.") : Diagnostics.DisabledReason);
    const FString SaveDisabledReason = bDirty ? FString() : TEXT("No unsaved settings changes to save.");
    const FString ResetDisabledReason = bDirty ? FString() : TEXT("No unsaved settings changes to reset.");
    const FString OutlineDisabledReason = !DraftSettings.bEnableOutlinePP ? TEXT("Enable outline first.") : EditDisabledReason;
    const FString DebounceDisabledReason = !DraftSettings.bEnableApplyDebounce ? TEXT("Enable apply debounce first.") : EditDisabledReason;
    const FString UnlockDisabledReason = bEditable ? FString() : EditDisabledReason;
    const FString AutoLockDisabledReason = !DraftSettings.bRequireUnlock ? TEXT("Enable Require Unlock first.") : EditDisabledReason;
    auto SetEnabledState = [](UWidget* Widget, bool bEnabled, const FString& DisabledReason, const FString& EnabledTooltip = FString())
    {
        RICompactUI::SetWidgetEnabledState(Widget, bEnabled, DisabledReason, EnabledTooltip);
    };

    if (ToggleKeyValueText)
    {
        ToggleKeyValueText->SetText(PendingCaptureTarget == ERIHotkeyCaptureTarget::Toggle ? FText::FromString(TEXT("Capturing...")) : GetKeyDisplayText(DraftSettings.ToggleKey));
    }
    if (PickKeyValueText)
    {
        PickKeyValueText->SetText(PendingCaptureTarget == ERIHotkeyCaptureTarget::Pick ? FText::FromString(TEXT("Capturing...")) : GetKeyDisplayText(DraftSettings.PickKey));
    }
    if (PickRequiresCtrlCheckBox)
    {
        PickRequiresCtrlCheckBox->SetIsChecked(DraftSettings.bPickKeyRequiresCtrl);
        SetEnabledState(PickRequiresCtrlCheckBox, bEditable, EditDisabledReason, TEXT("Toggle whether object-based mouse-position pick requires Ctrl."));
    }
    if (PickRequiresShiftCheckBox)
    {
        PickRequiresShiftCheckBox->SetIsChecked(DraftSettings.bPickKeyRequiresShift);
        SetEnabledState(PickRequiresShiftCheckBox, bEditable, EditDisabledReason, TEXT("Toggle whether object-based mouse-position pick requires Shift."));
    }
    if (EnableRightMousePickCheckBox)
    {
        EnableRightMousePickCheckBox->SetIsChecked(DraftSettings.bEnableRightMousePick);
        SetEnabledState(EnableRightMousePickCheckBox, bEditable, EditDisabledReason, TEXT("Enable or disable object-based right mouse picking at the current mouse position, including player characters."));
    }
    if (RightMousePickRequiresCtrlCheckBox)
    {
        RightMousePickRequiresCtrlCheckBox->SetIsChecked(DraftSettings.bRightMousePickRequiresCtrl);
        SetEnabledState(RightMousePickRequiresCtrlCheckBox, bEditable && DraftSettings.bEnableRightMousePick, EditDisabledReason.IsEmpty() ? TEXT("Enable right mouse pick first.") : EditDisabledReason, TEXT("Object-based mouse-position right mouse pick requires Ctrl."));
    }
    if (RightMousePickRequiresShiftCheckBox)
    {
        RightMousePickRequiresShiftCheckBox->SetIsChecked(DraftSettings.bRightMousePickRequiresShift);
        SetEnabledState(RightMousePickRequiresShiftCheckBox, bEditable && DraftSettings.bEnableRightMousePick, EditDisabledReason.IsEmpty() ? TEXT("Enable right mouse pick first.") : EditDisabledReason, TEXT("Object-based mouse-position right mouse pick requires Shift."));
    }
    if (EnableOutlineCheckBox)
    {
        EnableOutlineCheckBox->SetIsChecked(DraftSettings.bEnableOutlinePP);
        SetEnabledState(EnableOutlineCheckBox, bEditable, EditDisabledReason, TEXT("Toggle the outline post-process effect."));
    }
    if (OutlineWeightSpinBox)
    {
        OutlineWeightSpinBox->SetMinValue(0.0f);
        OutlineWeightSpinBox->SetMaxValue(5.0f);
        OutlineWeightSpinBox->SetDelta(0.05f);
        OutlineWeightSpinBox->SetValue(DraftSettings.OutlinePPWeight);
        SetEnabledState(OutlineWeightSpinBox, bEditable && DraftSettings.bEnableOutlinePP, OutlineDisabledReason, TEXT("Adjust the outline post-process weight."));
    }
    if (UIScaleSpinBox)
    {
        UIScaleSpinBox->SetMinValue(0.8f);
        UIScaleSpinBox->SetMaxValue(1.5f);
        UIScaleSpinBox->SetDelta(0.05f);
        UIScaleSpinBox->SetValue(DraftSettings.UIScale);
        SetEnabledState(UIScaleSpinBox, bEditable, EditDisabledReason, TEXT("Scale the inspector UI fonts and controls."));
    }
    if (EnableApplyDebounceCheckBox)
    {
        EnableApplyDebounceCheckBox->SetIsChecked(DraftSettings.bEnableApplyDebounce);
        SetEnabledState(EnableApplyDebounceCheckBox, bEditable, EditDisabledReason, TEXT("Toggle apply debounce for runtime changes."));
    }
    if (ApplyDebounceSecondsSpinBox)
    {
        ApplyDebounceSecondsSpinBox->SetMinValue(0.0f);
        ApplyDebounceSecondsSpinBox->SetMaxValue(0.20f);
        ApplyDebounceSecondsSpinBox->SetDelta(0.01f);
        ApplyDebounceSecondsSpinBox->SetValue(DraftSettings.ApplyDebounceSeconds);
        SetEnabledState(ApplyDebounceSecondsSpinBox, bEditable && DraftSettings.bEnableApplyDebounce, DebounceDisabledReason, TEXT("Adjust how long apply debounce waits."));
    }
    if (ThemePresetComboBox)
    {
        const FString DesiredOption = RI_GetThemePresetLabel(ActiveThemePreset);
        if (ThemePresetComboBox->GetSelectedOption() != DesiredOption)
        {
            ThemePresetComboBox->SetSelectedOption(DesiredOption);
        }
        SetEnabledState(ThemePresetComboBox, Subsystem.IsValid(), EditDisabledReason, TEXT("Switch the active UI theme preset."));
    }
    if (RequireUnlockCheckBox)
    {
        RequireUnlockCheckBox->SetIsChecked(DraftSettings.bRequireUnlock);
        SetEnabledState(RequireUnlockCheckBox, bEditable, UnlockDisabledReason, TEXT("Require unlock before editing runtime settings."));
    }
    if (AutoLockOnCloseCheckBox)
    {
        AutoLockOnCloseCheckBox->SetIsChecked(DraftSettings.bAutoLockOnClose);
        SetEnabledState(AutoLockOnCloseCheckBox, bEditable && DraftSettings.bRequireUnlock, AutoLockDisabledReason, TEXT("Automatically lock the panel when it closes."));
    }

    if (ToggleKeyButton)
    {
        SetEnabledState(ToggleKeyButton, bEditable, EditDisabledReason, TEXT("Rebind the toggle key."));
    }
    if (PickKeyButton)
    {
        SetEnabledState(PickKeyButton, bEditable, EditDisabledReason, TEXT("Rebind the object-based mouse-position pick key."));
    }
    if (SaveButton)
    {
        SetEnabledState(SaveButton, bEditable && bDirty, SaveDisabledReason, TEXT("Save the current settings to config."));
    }
    if (ResetButton)
    {
        SetEnabledState(ResetButton, bEditable && bDirty, ResetDisabledReason, TEXT("Restore the saved settings from config."));
    }

    if (DirtyStateText)
    {
        DirtyStateText->SetText(FText::FromString(bDirty ? TEXT("Unsaved Settings Changes") : TEXT("Settings Saved")));
        DirtyStateText->SetColorAndOpacity(FSlateColor(bDirty ? RI_SettingsWarningColor() : RI_SettingsSuccessColor()));
    }
    if (StatusMessageText)
    {
        StatusMessageText->SetText(FText::FromString(StatusMessage));
        StatusMessageText->SetColorAndOpacity(FSlateColor(bStatusIsError ? RI_SettingsErrorColor() : RI_SettingsSuccessColor()));
        StatusMessageText->SetVisibility(StatusMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    if (CaptureHintText)
    {
        const bool bCapturing = PendingCaptureTarget != ERIHotkeyCaptureTarget::None;
        CaptureHintText->SetText(FText::FromString(bCapturing ? TEXT("Press a key, Esc to cancel") : TEXT("")));
        CaptureHintText->SetVisibility(bCapturing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (RuntimeEnabledValueText)
    {
        RuntimeEnabledValueText->SetText(FText::FromString(Diagnostics.bRuntimeEnabled ? TEXT("Enabled") : TEXT("Disabled")));
        RuntimeEnabledValueText->SetColorAndOpacity(FSlateColor(Diagnostics.bRuntimeEnabled ? RI_SettingsSuccessColor() : RI_SettingsErrorColor()));
    }
    if (DisabledReasonValueText)
    {
        DisabledReasonValueText->SetText(FText::FromString(Diagnostics.DisabledReason.IsEmpty() ? TEXT("-") : Diagnostics.DisabledReason));
        DisabledReasonValueText->SetColorAndOpacity(FSlateColor(Diagnostics.DisabledReason.IsEmpty() ? RI_SettingsMutedTextColor() : RI_SettingsWarningColor()));
    }
    if (SessionValueText)
    {
        SessionValueText->SetText(FText::FromString(RuntimeSessionSummary.bSessionAvailable ? RuntimeSessionSummary.WorldTypeLabel : TEXT("Unavailable")));
    }
    if (NetModeValueText)
    {
        NetModeValueText->SetText(FText::FromString(RuntimeSessionSummary.NetModeLabel.IsEmpty() ? TEXT("-") : RuntimeSessionSummary.NetModeLabel));
    }
    if (SelectedActorValueText)
    {
        SelectedActorValueText->SetText(FText::FromString(RuntimeActorRoleSummary.bHasActor ? RuntimeActorRoleSummary.ActorPath : TEXT("No selected actor")));
    }
    if (SelectedRoleValueText)
    {
        SelectedRoleValueText->SetText(FText::FromString(RuntimeActorRoleSummary.bHasActor
            ? FString::Printf(TEXT("%s / %s"), *RuntimeActorRoleSummary.LocalRoleLabel, *RuntimeActorRoleSummary.RemoteRoleLabel)
            : TEXT("-")));
    }
    if (LockStateValueText)
    {
        const FString LockState = Diagnostics.bUnlockRequired
            ? RI_BoolLabel(Diagnostics.bUnlocked, TEXT("Unlocked"), TEXT("Locked"))
            : TEXT("Not Required");
        LockStateValueText->SetText(FText::FromString(LockState));
    }
    if (UnlockCodeValueText)
    {
        UnlockCodeValueText->SetText(FText::FromString(Diagnostics.bHasUnlockCode ? TEXT("Configured") : TEXT("Not Configured")));
    }
    if (OutlineMaterialValueText)
    {
        OutlineMaterialValueText->SetText(FText::FromString(Diagnostics.bOutlineMaterialAssigned ? Diagnostics.OutlineMaterialPath : TEXT("Not Set")));
    }
    if (CustomDepthValueText)
    {
        CustomDepthValueText->SetText(FText::FromString(Diagnostics.bCustomDepthStencilReady ? TEXT("Ready") : TEXT("Restart Required / Disabled")));
        CustomDepthValueText->SetColorAndOpacity(FSlateColor(Diagnostics.bCustomDepthStencilReady ? RI_SettingsSuccessColor() : RI_SettingsWarningColor()));
    }
}

void UInspectorSettingsPageWidget::SetStatusMessage(const FString& InMessage, bool bIsError)
{
    StatusMessage = InMessage;
    bStatusIsError = bIsError;
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        InspectorSubsystem->AppendActivityLog(
            bIsError ? ERIToastType::Error : ERIToastType::Info,
            TEXT("Settings"),
            InMessage);
    }
    UpdateUIFromState();
}

void UInspectorSettingsPageWidget::SyncDraftSettingsFromControls()
{
    if (PickRequiresCtrlCheckBox)
    {
        DraftSettings.bPickKeyRequiresCtrl = PickRequiresCtrlCheckBox->IsChecked();
    }
    if (PickRequiresShiftCheckBox)
    {
        DraftSettings.bPickKeyRequiresShift = PickRequiresShiftCheckBox->IsChecked();
    }
    if (EnableRightMousePickCheckBox)
    {
        DraftSettings.bEnableRightMousePick = EnableRightMousePickCheckBox->IsChecked();
    }
    if (RightMousePickRequiresCtrlCheckBox)
    {
        DraftSettings.bRightMousePickRequiresCtrl = RightMousePickRequiresCtrlCheckBox->IsChecked();
    }
    if (RightMousePickRequiresShiftCheckBox)
    {
        DraftSettings.bRightMousePickRequiresShift = RightMousePickRequiresShiftCheckBox->IsChecked();
    }
    if (EnableOutlineCheckBox)
    {
        DraftSettings.bEnableOutlinePP = EnableOutlineCheckBox->IsChecked();
    }
    if (OutlineWeightSpinBox)
    {
        DraftSettings.OutlinePPWeight = OutlineWeightSpinBox->GetValue();
    }
    if (UIScaleSpinBox)
    {
        DraftSettings.UIScale = UIScaleSpinBox->GetValue();
    }
    if (EnableApplyDebounceCheckBox)
    {
        DraftSettings.bEnableApplyDebounce = EnableApplyDebounceCheckBox->IsChecked();
    }
    if (ApplyDebounceSecondsSpinBox)
    {
        DraftSettings.ApplyDebounceSeconds = ApplyDebounceSecondsSpinBox->GetValue();
    }
    if (RequireUnlockCheckBox)
    {
        DraftSettings.bRequireUnlock = RequireUnlockCheckBox->IsChecked();
    }
    if (AutoLockOnCloseCheckBox)
    {
        DraftSettings.bAutoLockOnClose = AutoLockOnCloseCheckBox->IsChecked();
    }

    if (ThemePresetComboBox)
    {
        const FString SelectedTheme = ThemePresetComboBox->GetSelectedOption();
        if (!SelectedTheme.IsEmpty())
        {
            ActiveThemePreset = RI_ParseThemePresetLabel(SelectedTheme);
        }
    }
}

void UInspectorSettingsPageWidget::ClearStatusMessage()
{
    StatusMessage.Reset();
    bStatusIsError = false;
    UpdateUIFromState();
}

bool UInspectorSettingsPageWidget::ApplyPreviewSettings(const FRIEditableSettings& CandidateSettings, bool bShowSuccessMessage)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Inspector settings are unavailable."), true);
        return false;
    }

    FString Error;
    if (!InspectorSubsystem->PreviewApplySettings(CandidateSettings, Error))
    {
        SetStatusMessage(Error, true);
        return false;
    }

    DraftSettings = InspectorSubsystem->GetEditableSettings();
    Diagnostics = InspectorSubsystem->GetSettingsDiagnostics();

    if (bShowSuccessMessage)
    {
        SetStatusMessage(TEXT("Settings preview applied."), false);
    }
    else
    {
        StatusMessage.Reset();
        bStatusIsError = false;
        UpdateUIFromState();
    }

    return true;
}

void UInspectorSettingsPageWidget::BeginHotkeyCapture(ERIHotkeyCaptureTarget CaptureTarget)
{
    if (!Subsystem.IsValid())
    {
        SetStatusMessage(TEXT("Inspector settings are unavailable."), true);
        return;
    }

    PendingCaptureTarget = CaptureTarget;
    StatusMessage.Reset();
    bStatusIsError = false;
    UpdateUIFromState();
    SetKeyboardFocus();
}

void UInspectorSettingsPageWidget::CancelHotkeyCapture(bool bClearStatusMessage)
{
    PendingCaptureTarget = ERIHotkeyCaptureTarget::None;
    if (bClearStatusMessage)
    {
        StatusMessage.Reset();
        bStatusIsError = false;
    }
    UpdateUIFromState();
}

FText UInspectorSettingsPageWidget::GetKeyDisplayText(const FKey& InKey) const
{
    if (!InKey.IsValid())
    {
        return FText::FromString(TEXT("Unassigned"));
    }

    return InKey.GetDisplayName();
}

void UInspectorSettingsPageWidget::HandleToggleKeyCaptureClicked()
{
    BeginHotkeyCapture(ERIHotkeyCaptureTarget::Toggle);
}

void UInspectorSettingsPageWidget::HandlePickKeyCaptureClicked()
{
    BeginHotkeyCapture(ERIHotkeyCaptureTarget::Pick);
}

void UInspectorSettingsPageWidget::HandlePickRequiresCtrlChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bPickKeyRequiresCtrl = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandlePickRequiresShiftChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bPickKeyRequiresShift = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableRightMousePickChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableRightMousePick = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRightMousePickRequiresCtrlChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRightMousePickRequiresCtrl = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRightMousePickRequiresShiftChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRightMousePickRequiresShift = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableOutlineChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableOutlinePP = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleOutlineWeightChanged(float InValue)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.OutlinePPWeight = InValue;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleUIScaleChanged(float InValue)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.UIScale = InValue;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableApplyDebounceChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableApplyDebounce = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleApplyDebounceSecondsChanged(float InValue)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.ApplyDebounceSeconds = InValue;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRequireUnlockChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRequireUnlock = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleAutoLockOnCloseChanged(bool bIsChecked)
{
    if (ShouldIgnoreControlChange()) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bAutoLockOnClose = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleThemePresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (ShouldIgnoreControlChange() || !Subsystem.IsValid() || SelectedItem.IsEmpty())
    {
        return;
    }

    FString Error;
    if (!Subsystem->PreviewApplyThemePreset(RI_ParseThemePresetLabel(SelectedItem), Error))
    {
        SetStatusMessage(Error, true);
        return;
    }

    ActiveThemePreset = Subsystem->GetThemePreset();
    SetStatusMessage(TEXT("Theme preview applied."), false);
}

void UInspectorSettingsPageWidget::HandleSaveClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Inspector settings are unavailable."), true);
        return;
    }

    SyncDraftSettingsFromControls();

    if (!ApplyPreviewSettings(DraftSettings, false))
    {
        return;
    }

    FString Error;
    if (!InspectorSubsystem->SaveSettings(Error))
    {
        SetStatusMessage(Error, true);
        return;
    }

    Diagnostics = InspectorSubsystem->GetSettingsDiagnostics();
    DraftSettings = InspectorSubsystem->GetEditableSettings();
    SetStatusMessage(TEXT("Saved settings to config."), false);
}

void UInspectorSettingsPageWidget::HandleResetClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Inspector settings are unavailable."), true);
        return;
    }

    InspectorSubsystem->ReloadSettingsFromConfig();
    RefreshFromSubsystem();
    SetStatusMessage(TEXT("Reloaded settings from config."), false);
}
