#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "InspectorTypes.h"
#include "RuntimeInspectorSettings.h"
#include "TimerManager.h"
#include "Types/SlateEnums.h"
#include "InspectorSettingsPageWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UInspectorWorldSubsystem;
class UScrollBox;
class USpinBox;
class UTextBlock;

namespace RICompactUI
{
    enum class ERISectionVisualStyle : uint8;
}

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorSettingsPageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorSettingsPageWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    FString GetSessionValueText() const { return SessionValueText ? SessionValueText->GetText().ToString() : FString(); }
    FString GetNetModeValueText() const { return NetModeValueText ? NetModeValueText->GetText().ToString() : FString(); }
    FString GetSelectedActorValueText() const { return SelectedActorValueText ? SelectedActorValueText->GetText().ToString() : FString(); }
    FString GetSelectedRoleValueText() const { return SelectedRoleValueText ? SelectedRoleValueText->GetText().ToString() : FString(); }
    bool HasPageScrollRoot() const { return PageScrollBox != nullptr; }
    bool HasTouchScrollSupportForAutomation() const;
    bool HasFooterControls() const { return SaveButton && ResetButton && DirtyStateText; }
    bool HasStatusSection() const { return RuntimeEnabledValueText && SessionValueText && SelectedActorValueText; }
    bool HasInteractionSection() const { return ToggleKeyButton && PickKeyButton && PickRequiresCtrlCheckBox && PickRequiresShiftCheckBox; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    void RefreshFromSubsystem();

    void ScheduleDeferredRefresh();
    void CancelDeferredRefresh();

#if WITH_EDITOR
    void BuildFallbackWidgetTreeForExport(class UWidgetTree* TargetWidgetTree);
#endif

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    enum class ERIHotkeyCaptureTarget : uint8
    {
        None,
        Toggle,
        Pick
    };

    void BuildWidgetTree();
    bool ShouldIgnoreControlChange() const;
    void ResetControlReferences();
    void BindControlDelegates();
    void EnsureThemePresetOptions();

    UWidget* CreateSectionTitle(const FString& InTitle, bool bEmphasis = false);
    UWidget* CreateKeybindRow(const FString& InLabel, UTextBlock*& OutValueText, UButton*& OutButton, FName ValueTextName = NAME_None, FName ButtonName = NAME_None);
    UWidget* CreateCheckRow(const FString& InLabel, UCheckBox*& OutCheckBox, FName CheckBoxName = NAME_None);
    UWidget* CreateSpinRow(const FString& InLabel, USpinBox*& OutSpinBox, FName SpinBoxName = NAME_None);
    UWidget* CreateThemePresetRow(const FString& InLabel, UComboBoxString*& OutComboBox, FName ComboBoxName = NAME_None);
    UWidget* CreateStatusRow(const FString& InLabel, UTextBlock*& OutValueText, bool bWrapValue = false, FName ValueTextName = NAME_None);

    void UpdateUIFromState();
    void SyncDraftSettingsFromControls();
    void SetStatusMessage(const FString& InMessage, bool bIsError);
    void ClearStatusMessage();
    bool ApplyPreviewSettings(const FRIEditableSettings& CandidateSettings, bool bShowSuccessMessage = false);
    void BeginHotkeyCapture(ERIHotkeyCaptureTarget CaptureTarget);
    void CancelHotkeyCapture(bool bClearStatusMessage);
    FText GetKeyDisplayText(const FKey& InKey) const;

    UFUNCTION()
    void HandleToggleKeyCaptureClicked();

    UFUNCTION()
    void HandlePickKeyCaptureClicked();

    UFUNCTION()
    void HandlePickRequiresCtrlChanged(bool bIsChecked);

    UFUNCTION()
    void HandlePickRequiresShiftChanged(bool bIsChecked);

    UFUNCTION()
    void HandleEnableRightMousePickChanged(bool bIsChecked);

    UFUNCTION()
    void HandleRightMousePickRequiresCtrlChanged(bool bIsChecked);

    UFUNCTION()
    void HandleRightMousePickRequiresShiftChanged(bool bIsChecked);

    UFUNCTION()
    void HandleEnableOutlineChanged(bool bIsChecked);

    UFUNCTION()
    void HandleOutlineWeightChanged(float InValue);

    UFUNCTION()
    void HandleUIScaleChanged(float InValue);

    UFUNCTION()
    void HandleEnableApplyDebounceChanged(bool bIsChecked);

    UFUNCTION()
    void HandleApplyDebounceSecondsChanged(float InValue);

    UFUNCTION()
    void HandleRequireUnlockChanged(bool bIsChecked);

    UFUNCTION()
    void HandleAutoLockOnCloseChanged(bool bIsChecked);

    UFUNCTION()
    void HandleThemePresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    UWidget* HandleGenerateThemePresetOptionWidget(FString InItemText);

    UFUNCTION()
    void HandleSaveClicked();

    UFUNCTION()
    void HandleResetClicked();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    FRIEditableSettings DraftSettings;
    FRISettingsDiagnostics Diagnostics;
    FRIRuntimeSessionSummary RuntimeSessionSummary;
    FRIRuntimeActorRoleSummary RuntimeActorRoleSummary;
    ERuntimeInspectorThemePreset ActiveThemePreset = ERuntimeInspectorThemePreset::StudioSlate;

    bool bRefreshingUI = false;
    bool bControlsReady = false;
    bool bStatusIsError = false;
    FString StatusMessage;
    ERIHotkeyCaptureTarget PendingCaptureTarget = ERIHotkeyCaptureTarget::None;
    bool bDeferredRefreshScheduled = false;
    FTimerHandle DeferredRefreshTimerHandle;

    UFUNCTION()
    void HandleDeferredRefreshTimerElapsed();

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* ToggleKeyValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* PickKeyValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* DirtyStateText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* StatusMessageText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* CaptureHintText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* RuntimeEnabledValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* DisabledReasonValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* SessionValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* NetModeValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* SelectedActorValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* SelectedRoleValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* LockStateValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* UnlockCodeValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* OutlineMaterialValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UTextBlock* CustomDepthValueText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UScrollBox* PageScrollBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UComboBoxString* ThemePresetComboBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UButton* ToggleKeyButton = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UButton* PickKeyButton = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UButton* SaveButton = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UButton* ResetButton = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* PickRequiresCtrlCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* PickRequiresShiftCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* EnableRightMousePickCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* RightMousePickRequiresCtrlCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* RightMousePickRequiresShiftCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* EnableOutlineCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* EnableApplyDebounceCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* RequireUnlockCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    UCheckBox* AutoLockOnCloseCheckBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    USpinBox* OutlineWeightSpinBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    USpinBox* ApplyDebounceSecondsSpinBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    USpinBox* UIScaleSpinBox = nullptr;
};
