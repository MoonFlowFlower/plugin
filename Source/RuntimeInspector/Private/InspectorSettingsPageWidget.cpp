#include "InspectorSettingsPageWidget.h"

#include "InspectorCompactWidgetUtils.h"
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

void UInspectorSettingsPageWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    CancelDeferredRefresh();
    Subsystem = InSubsystem;
}

void UInspectorSettingsPageWidget::SetPresentationMode(ERISettingsPresentationMode InMode)
{
    PresentationMode = InMode;
}

TSharedRef<SWidget> UInspectorSettingsPageWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorSettingsPageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshFromSubsystem();
}

void UInspectorSettingsPageWidget::NativeDestruct()
{
    CancelDeferredRefresh();
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
        SetStatusMessage(TEXT("Snapshot controls are unavailable."), true);
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

    const bool bEmbeddedSection = PresentationMode == ERISettingsPresentationMode::EmbeddedSection;

    UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), bEmbeddedSection ? TEXT("RI_SnapshotSectionRoot") : TEXT("RI_SettingsRoot"));
    RootBorder->SetPadding(bEmbeddedSection ? FMargin(4.0f) : FMargin(6.0f));
    RootBorder->SetBrushColor(bEmbeddedSection ? RI_SettingsSectionColor() : RICompactUI::GetPageBackgroundColor());

    UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), bEmbeddedSection ? TEXT("RI_SnapshotSectionBox") : TEXT("RI_SettingsMainBox"));
    RootBorder->SetContent(MainBox);

    UVerticalBox* ContentBox = nullptr;
    if (bEmbeddedSection)
    {
        ContentBox = MainBox;
    }
    else
    {
        PageScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_SettingsScroll"));
        if (UVerticalBoxSlot* ScrollSlot = MainBox->AddChildToVerticalBox(PageScrollBox))
        {
            ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsContentBox"));
        PageScrollBox->AddChild(ContentBox);
    }

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

    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Controls"), true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Interaction"), true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateKeybindRow(TEXT("Toggle Key"), ToggleKeyValueText, ToggleKeyButton), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateKeybindRow(TEXT("Pick Key"), PickKeyValueText, PickKeyButton), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Pick Requires Ctrl"), PickRequiresCtrlCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Pick Requires Shift"), PickRequiresShiftCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Enable Ctrl+RMB Pick"), EnableRightMousePickCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("RMB Requires Ctrl"), RightMousePickRequiresCtrlCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("RMB Requires Shift"), RightMousePickRequiresShiftCheckBox), FMargin(0.f, 0.f, 0.f, 8.f));

    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Outline")), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Enable Outline"), EnableOutlineCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateSpinRow(TEXT("Outline Weight"), OutlineWeightSpinBox), FMargin(0.f, 0.f, 0.f, 8.f));

    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Apply"), true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Enable Apply Debounce"), EnableApplyDebounceCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateSpinRow(TEXT("Debounce Seconds"), ApplyDebounceSecondsSpinBox), FMargin(0.f, 0.f, 0.f, 8.f));

    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Appearance"), true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateThemePresetRow(TEXT("Theme Preset"), ThemePresetComboBox), FMargin(0.f, 0.f, 0.f, 8.f));

    AddContentWidget(CreateSectionTitle(TEXT("Snapshot Security & Status"), true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Require Unlock"), RequireUnlockCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateCheckRow(TEXT("Auto Lock On Close"), AutoLockOnCloseCheckBox), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Runtime"), RuntimeEnabledValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Disabled Reason"), DisabledReasonValueText, true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Session"), SessionValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Net Mode"), NetModeValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Selected Actor"), SelectedActorValueText, true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Selected Role"), SelectedRoleValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Lock State"), LockStateValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Unlock Code"), UnlockCodeValueText), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Outline Material"), OutlineMaterialValueText, true), FMargin(0.f, 0.f, 0.f, 4.f));
    AddContentWidget(CreateStatusRow(TEXT("Custom Depth Stencil"), CustomDepthValueText), FMargin(0.f, 0.f, 0.f, 4.f));

    UBorder* FooterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_SettingsFooter"));
    FooterBorder->SetPadding(FMargin(6.f, 5.f));
    FooterBorder->SetBrushColor(RICompactUI::GetFooterBackgroundColor());
    if (UVerticalBoxSlot* FooterSlot = MainBox->AddChildToVerticalBox(FooterBorder))
    {
        FooterSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
        FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    UVerticalBox* FooterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsFooterBox"));
    FooterBorder->SetContent(FooterBox);

    StatusMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SettingsStatus"));
    StatusMessageText->SetAutoWrapText(true);
    RI_ApplyTextStyle(StatusMessageText, RICompactUI::GetValueFontSize(), false, RI_SettingsMutedTextColor());
    if (UVerticalBoxSlot* StatusSlot = FooterBox->AddChildToVerticalBox(StatusMessageText))
    {
        StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    CaptureHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SettingsCaptureHint"));
    CaptureHintText->SetAutoWrapText(true);
    RI_ApplyTextStyle(CaptureHintText, RICompactUI::GetValueFontSize(), false, RI_SettingsWarningColor());
    if (UVerticalBoxSlot* HintSlot = FooterBox->AddChildToVerticalBox(CaptureHintText))
    {
        HintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    UHorizontalBox* FooterButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SettingsFooterButtons"));
    FooterBox->AddChildToVerticalBox(FooterButtonsRow);

    DirtyStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SettingsDirty"));
    RI_ApplyTextStyle(DirtyStateText, RICompactUI::GetSectionTitleFontSize(), true, RI_SettingsWarningColor());
    DirtyStateText->SetAutoWrapText(true);
    if (UHorizontalBoxSlot* DirtySlot = FooterButtonsRow->AddChildToHorizontalBox(DirtyStateText))
    {
        DirtySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        DirtySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
        DirtySlot->SetVerticalAlignment(VAlign_Center);
    }

    SaveButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("BTN_SettingsSave"),
        TEXT("Save Snapshot"),
        RICompactUI::ERIButtonVisualStyle::Primary,
        96.0f);
    SaveButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleSaveClicked);
    if (UHorizontalBoxSlot* SaveSlot = FooterButtonsRow->AddChildToHorizontalBox(SaveButton))
    {
        SaveSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        SaveSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ResetButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("BTN_SettingsReset"),
        TEXT("Reload Snapshot"),
        RICompactUI::ERIButtonVisualStyle::Danger,
        104.0f);
    ResetButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleResetClicked);
    FooterButtonsRow->AddChildToHorizontalBox(ResetButton);

    if (ToggleKeyButton)
    {
        ToggleKeyButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandleToggleKeyCaptureClicked);
    }
    if (PickKeyButton)
    {
        PickKeyButton->OnClicked.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickKeyCaptureClicked);
    }
    if (PickRequiresCtrlCheckBox)
    {
        PickRequiresCtrlCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresCtrlChanged);
    }
    if (PickRequiresShiftCheckBox)
    {
        PickRequiresShiftCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandlePickRequiresShiftChanged);
    }
    if (EnableRightMousePickCheckBox)
    {
        EnableRightMousePickCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableRightMousePickChanged);
    }
    if (RightMousePickRequiresCtrlCheckBox)
    {
        RightMousePickRequiresCtrlCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresCtrlChanged);
    }
    if (RightMousePickRequiresShiftCheckBox)
    {
        RightMousePickRequiresShiftCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRightMousePickRequiresShiftChanged);
    }
    if (EnableOutlineCheckBox)
    {
        EnableOutlineCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableOutlineChanged);
    }
    if (OutlineWeightSpinBox)
    {
        OutlineWeightSpinBox->OnValueChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleOutlineWeightChanged);
    }
    if (EnableApplyDebounceCheckBox)
    {
        EnableApplyDebounceCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleEnableApplyDebounceChanged);
    }
    if (ApplyDebounceSecondsSpinBox)
    {
        ApplyDebounceSecondsSpinBox->OnValueChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleApplyDebounceSecondsChanged);
    }
    if (ThemePresetComboBox)
    {
        ThemePresetComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleThemePresetSelectionChanged);
    }
    if (RequireUnlockCheckBox)
    {
        RequireUnlockCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleRequireUnlockChanged);
    }
    if (AutoLockOnCloseCheckBox)
    {
        AutoLockOnCloseCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorSettingsPageWidget::HandleAutoLockOnCloseChanged);
    }

    WidgetTree->RootWidget = RootBorder;
}

UWidget* UInspectorSettingsPageWidget::CreateSectionTitle(const FString& InTitle, bool bEmphasis)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        bEmphasis ? RICompactUI::ERISectionVisualStyle::Emphasis : RICompactUI::ERISectionVisualStyle::Standard);
}

UWidget* UInspectorSettingsPageWidget::CreateKeybindRow(const FString& InLabel, UTextBlock*& OutValueText, UButton*& OutButton)
{
    UHorizontalBox* ValueBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RI_ApplyTextStyle(OutValueText, RICompactUI::GetValueFontSize(), true, RI_SettingsTextColor());
    if (UHorizontalBoxSlot* ValueSlot = ValueBox->AddChildToHorizontalBox(OutValueText))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    OutButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        NAME_None,
        TEXT("Rebind"),
        RICompactUI::ERIButtonVisualStyle::Secondary,
        56.0f);
    if (UHorizontalBoxSlot* ButtonSlot = ValueBox->AddChildToHorizontalBox(OutButton))
    {
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(104.0f);

    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(FText::FromString(InLabel));
    RI_ApplyTextStyle(LabelText, RICompactUI::GetLabelFontSize(), true, RI_SettingsTextColor());
    LabelBox->SetContent(LabelText);

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RI_SettingsRowColor());
    Border->SetContent(Row);
    return Border;
}

UWidget* UInspectorSettingsPageWidget::CreateCheckRow(const FString& InLabel, UCheckBox*& OutCheckBox)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(104.0f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(FText::FromString(InLabel));
    RI_ApplyTextStyle(LabelText, RICompactUI::GetLabelFontSize(), true, RI_SettingsTextColor());
    LabelBox->SetContent(LabelText);

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
    OutCheckBox->SetRenderScale(FVector2D(0.55f, 0.55f));
    if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutCheckBox))
    {
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RI_SettingsRowColor());
    Border->SetContent(Row);
    return Border;
}

UWidget* UInspectorSettingsPageWidget::CreateSpinRow(const FString& InLabel, USpinBox*& OutSpinBox)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(104.0f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(FText::FromString(InLabel));
    RI_ApplyTextStyle(LabelText, RICompactUI::GetLabelFontSize(), true, RI_SettingsTextColor());
    LabelBox->SetContent(LabelText);

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    USizeBox* SpinBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SpinBoxSizeBox->SetWidthOverride(70.0f);
    SpinBoxSizeBox->SetHeightOverride(RICompactUI::GetInputHeight());
    OutSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass());
    OutSpinBox->SetMinDesiredWidth(70.0f);
    OutSpinBox->SetMinFractionalDigits(2);
    OutSpinBox->SetMaxFractionalDigits(3);
    SpinBoxSizeBox->SetContent(OutSpinBox);
    if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(SpinBoxSizeBox))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RI_SettingsRowColor());
    Border->SetContent(Row);
    return Border;
}

UWidget* UInspectorSettingsPageWidget::CreateThemePresetRow(const FString& InLabel, UComboBoxString*& OutComboBox)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(104.0f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(FText::FromString(InLabel));
    RI_ApplyTextStyle(LabelText, RICompactUI::GetLabelFontSize(), true, RI_SettingsTextColor());
    LabelBox->SetContent(LabelText);

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    OutComboBox->AddOption(RI_GetThemePresetLabel(ERuntimeInspectorThemePreset::StudioSlate));
    OutComboBox->AddOption(RI_GetThemePresetLabel(ERuntimeInspectorThemePreset::SoftContrast));
    RICompactUI::ConfigureComboBoxString(OutComboBox, RI_SettingsTextColor(), 180.0f, RICompactUI::ERIInputVisualStyle::Strong);
    if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(RICompactUI::WrapFixedHeight(WidgetTree, OutComboBox, RICompactUI::GetInputHeight())))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RI_SettingsRowColor());
    Border->SetContent(Row);
    return Border;
}

UWidget* UInspectorSettingsPageWidget::CreateStatusRow(const FString& InLabel, UTextBlock*& OutValueText, bool bWrapValue)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(104.0f);
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(FText::FromString(InLabel));
    RI_ApplyTextStyle(LabelText, RICompactUI::GetLabelFontSize(), true, RI_SettingsTextColor());
    LabelBox->SetContent(LabelText);

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LabelSlot->SetVerticalAlignment(VAlign_Top);
    }

    OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RI_ApplyTextStyle(OutValueText, RICompactUI::GetValueFontSize(), false, RI_SettingsMutedTextColor());
    OutValueText->SetAutoWrapText(bWrapValue);
    if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValueText))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetPadding(FMargin(6.f, 3.f));
    Border->SetBrushColor(RI_SettingsRowColor());
    Border->SetContent(Row);
    return Border;
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

void UInspectorSettingsPageWidget::UpdateUIFromState()
{
    TGuardValue<bool> RefreshGuard(bRefreshingUI, true);

    const bool bEditable = Diagnostics.bRuntimeEnabled && Subsystem.IsValid();
    const bool bDirty = Subsystem.IsValid() ? Subsystem->HasUnsavedSettingsChanges() : false;
    const FString EditDisabledReason = Diagnostics.bRuntimeEnabled
        ? TEXT("Open Runtime Inspector before editing snapshot controls.")
        : (Diagnostics.DisabledReason.IsEmpty() ? TEXT("Runtime Inspector is disabled.") : Diagnostics.DisabledReason);
    const FString SaveDisabledReason = bDirty ? FString() : TEXT("No unsaved snapshot changes to save.");
    const FString ResetDisabledReason = bDirty ? FString() : TEXT("No unsaved snapshot changes to reset.");
    const FString OutlineDisabledReason = !DraftSettings.bEnableOutlinePP ? TEXT("Enable snapshot outline first.") : EditDisabledReason;
    const FString DebounceDisabledReason = !DraftSettings.bEnableApplyDebounce ? TEXT("Enable snapshot apply debounce first.") : EditDisabledReason;
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
        SetEnabledState(PickRequiresCtrlCheckBox, bEditable, EditDisabledReason, TEXT("Toggle whether the snapshot pick key requires Ctrl."));
    }
    if (PickRequiresShiftCheckBox)
    {
        PickRequiresShiftCheckBox->SetIsChecked(DraftSettings.bPickKeyRequiresShift);
        SetEnabledState(PickRequiresShiftCheckBox, bEditable, EditDisabledReason, TEXT("Toggle whether the snapshot pick key requires Shift."));
    }
    if (EnableRightMousePickCheckBox)
    {
        EnableRightMousePickCheckBox->SetIsChecked(DraftSettings.bEnableRightMousePick);
        SetEnabledState(EnableRightMousePickCheckBox, bEditable, EditDisabledReason, TEXT("Enable or disable snapshot right mouse pick."));
    }
    if (RightMousePickRequiresCtrlCheckBox)
    {
        RightMousePickRequiresCtrlCheckBox->SetIsChecked(DraftSettings.bRightMousePickRequiresCtrl);
        SetEnabledState(RightMousePickRequiresCtrlCheckBox, bEditable && DraftSettings.bEnableRightMousePick, EditDisabledReason.IsEmpty() ? TEXT("Enable snapshot right mouse pick first.") : EditDisabledReason, TEXT("Snapshot right mouse pick requires Ctrl."));
    }
    if (RightMousePickRequiresShiftCheckBox)
    {
        RightMousePickRequiresShiftCheckBox->SetIsChecked(DraftSettings.bRightMousePickRequiresShift);
        SetEnabledState(RightMousePickRequiresShiftCheckBox, bEditable && DraftSettings.bEnableRightMousePick, EditDisabledReason.IsEmpty() ? TEXT("Enable snapshot right mouse pick first.") : EditDisabledReason, TEXT("Snapshot right mouse pick requires Shift."));
    }
    if (EnableOutlineCheckBox)
    {
        EnableOutlineCheckBox->SetIsChecked(DraftSettings.bEnableOutlinePP);
        SetEnabledState(EnableOutlineCheckBox, bEditable, EditDisabledReason, TEXT("Toggle the snapshot outline post-process effect."));
    }
    if (OutlineWeightSpinBox)
    {
        OutlineWeightSpinBox->SetMinValue(0.0f);
        OutlineWeightSpinBox->SetMaxValue(5.0f);
        OutlineWeightSpinBox->SetDelta(0.05f);
        OutlineWeightSpinBox->SetValue(DraftSettings.OutlinePPWeight);
        SetEnabledState(OutlineWeightSpinBox, bEditable && DraftSettings.bEnableOutlinePP, OutlineDisabledReason, TEXT("Adjust the snapshot outline post-process weight."));
    }
    if (EnableApplyDebounceCheckBox)
    {
        EnableApplyDebounceCheckBox->SetIsChecked(DraftSettings.bEnableApplyDebounce);
        SetEnabledState(EnableApplyDebounceCheckBox, bEditable, EditDisabledReason, TEXT("Toggle snapshot apply debounce for runtime changes."));
    }
    if (ApplyDebounceSecondsSpinBox)
    {
        ApplyDebounceSecondsSpinBox->SetMinValue(0.0f);
        ApplyDebounceSecondsSpinBox->SetMaxValue(0.20f);
        ApplyDebounceSecondsSpinBox->SetDelta(0.01f);
        ApplyDebounceSecondsSpinBox->SetValue(DraftSettings.ApplyDebounceSeconds);
        SetEnabledState(ApplyDebounceSecondsSpinBox, bEditable && DraftSettings.bEnableApplyDebounce, DebounceDisabledReason, TEXT("Adjust how long snapshot apply debounce waits."));
    }
    if (ThemePresetComboBox)
    {
        const FString DesiredOption = RI_GetThemePresetLabel(ActiveThemePreset);
        if (ThemePresetComboBox->GetSelectedOption() != DesiredOption)
        {
            ThemePresetComboBox->SetSelectedOption(DesiredOption);
        }
        SetEnabledState(ThemePresetComboBox, Subsystem.IsValid(), EditDisabledReason, TEXT("Switch the active snapshot UI theme preset."));
    }
    if (RequireUnlockCheckBox)
    {
        RequireUnlockCheckBox->SetIsChecked(DraftSettings.bRequireUnlock);
        SetEnabledState(RequireUnlockCheckBox, bEditable, UnlockDisabledReason, TEXT("Require unlock before editing snapshot controls."));
    }
    if (AutoLockOnCloseCheckBox)
    {
        AutoLockOnCloseCheckBox->SetIsChecked(DraftSettings.bAutoLockOnClose);
        SetEnabledState(AutoLockOnCloseCheckBox, bEditable && DraftSettings.bRequireUnlock, AutoLockDisabledReason, TEXT("Automatically lock snapshot controls when the panel closes."));
    }

    if (ToggleKeyButton)
    {
        SetEnabledState(ToggleKeyButton, bEditable, EditDisabledReason, TEXT("Rebind the snapshot toggle key."));
    }
    if (PickKeyButton)
    {
        SetEnabledState(PickKeyButton, bEditable, EditDisabledReason, TEXT("Rebind the snapshot pick key."));
    }
    if (SaveButton)
    {
        SetEnabledState(SaveButton, bEditable && bDirty, SaveDisabledReason, TEXT("Save the current snapshot settings to config."));
    }
    if (ResetButton)
    {
        SetEnabledState(ResetButton, bEditable && bDirty, ResetDisabledReason, TEXT("Restore the saved snapshot settings from config."));
    }

    if (DirtyStateText)
    {
        DirtyStateText->SetText(FText::FromString(bDirty ? TEXT("Unsaved Snapshot Changes") : TEXT("Snapshot Saved")));
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
            TEXT("Snapshot"),
            InMessage);
    }
    UpdateUIFromState();
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
        SetStatusMessage(TEXT("Snapshot controls are unavailable."), true);
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
        SetStatusMessage(TEXT("Snapshot preview applied."), false);
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
        SetStatusMessage(TEXT("Snapshot controls are unavailable."), true);
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
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bPickKeyRequiresCtrl = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandlePickRequiresShiftChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bPickKeyRequiresShift = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableRightMousePickChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableRightMousePick = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRightMousePickRequiresCtrlChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRightMousePickRequiresCtrl = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRightMousePickRequiresShiftChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRightMousePickRequiresShift = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableOutlineChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableOutlinePP = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleOutlineWeightChanged(float InValue)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.OutlinePPWeight = InValue;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleEnableApplyDebounceChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bEnableApplyDebounce = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleApplyDebounceSecondsChanged(float InValue)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.ApplyDebounceSeconds = InValue;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleRequireUnlockChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bRequireUnlock = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleAutoLockOnCloseChanged(bool bIsChecked)
{
    if (bRefreshingUI) return;
    FRIEditableSettings Candidate = DraftSettings;
    Candidate.bAutoLockOnClose = bIsChecked;
    ApplyPreviewSettings(Candidate);
}

void UInspectorSettingsPageWidget::HandleThemePresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (bRefreshingUI || !Subsystem.IsValid() || SelectedItem.IsEmpty())
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
    SetStatusMessage(TEXT("Snapshot theme preview applied."), false);
}

void UInspectorSettingsPageWidget::HandleSaveClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Snapshot controls are unavailable."), true);
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
    SetStatusMessage(TEXT("Saved snapshot controls to config."), false);
}

void UInspectorSettingsPageWidget::HandleResetClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetStatusMessage(TEXT("Snapshot controls are unavailable."), true);
        return;
    }

    InspectorSubsystem->ReloadSettingsFromConfig();
    RefreshFromSubsystem();
    SetStatusMessage(TEXT("Reloaded snapshot controls from config."), false);
}
