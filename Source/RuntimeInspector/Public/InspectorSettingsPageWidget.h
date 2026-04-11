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
    enum class ERISettingsPresentationMode : uint8
    {
        Page,
        EmbeddedSection
    };

    UInspectorSettingsPageWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetPresentationMode(ERISettingsPresentationMode InMode);
    FString GetSessionValueText() const { return SessionValueText ? SessionValueText->GetText().ToString() : FString(); }
    FString GetNetModeValueText() const { return NetModeValueText ? NetModeValueText->GetText().ToString() : FString(); }
    FString GetSelectedActorValueText() const { return SelectedActorValueText ? SelectedActorValueText->GetText().ToString() : FString(); }
    FString GetSelectedRoleValueText() const { return SelectedRoleValueText ? SelectedRoleValueText->GetText().ToString() : FString(); }
    bool HasPageScrollRoot() const { return PageScrollBox != nullptr; }
    bool HasFooterControls() const { return SaveButton && ResetButton && DirtyStateText; }
    bool HasStatusSection() const { return RuntimeEnabledValueText && SessionValueText && SelectedActorValueText; }
    bool HasInteractionSection() const { return ToggleKeyButton && PickKeyButton && PickRequiresCtrlCheckBox && PickRequiresShiftCheckBox; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    void RefreshFromSubsystem();

    void ScheduleDeferredRefresh();
    void CancelDeferredRefresh();

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

    UWidget* CreateSectionTitle(const FString& InTitle, bool bEmphasis = false);
    UWidget* CreateKeybindRow(const FString& InLabel, UTextBlock*& OutValueText, UButton*& OutButton);
    UWidget* CreateCheckRow(const FString& InLabel, UCheckBox*& OutCheckBox);
    UWidget* CreateSpinRow(const FString& InLabel, USpinBox*& OutSpinBox);
    UWidget* CreateThemePresetRow(const FString& InLabel, UComboBoxString*& OutComboBox);
    UWidget* CreateStatusRow(const FString& InLabel, UTextBlock*& OutValueText, bool bWrapValue = false);

    void UpdateUIFromState();
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
    void HandleSaveClicked();

    UFUNCTION()
    void HandleResetClicked();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    ERISettingsPresentationMode PresentationMode = ERISettingsPresentationMode::Page;

    FRIEditableSettings DraftSettings;
    FRISettingsDiagnostics Diagnostics;
    FRIRuntimeSessionSummary RuntimeSessionSummary;
    FRIRuntimeActorRoleSummary RuntimeActorRoleSummary;
    ERuntimeInspectorThemePreset ActiveThemePreset = ERuntimeInspectorThemePreset::StudioSlate;

    bool bRefreshingUI = false;
    bool bStatusIsError = false;
    FString StatusMessage;
    ERIHotkeyCaptureTarget PendingCaptureTarget = ERIHotkeyCaptureTarget::None;
    bool bDeferredRefreshScheduled = false;
    FTimerHandle DeferredRefreshTimerHandle;

    UFUNCTION()
    void HandleDeferredRefreshTimerElapsed();

    UPROPERTY(Transient)
    UTextBlock* ToggleKeyValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* PickKeyValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DirtyStateText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* StatusMessageText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* CaptureHintText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* RuntimeEnabledValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* DisabledReasonValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SessionValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* NetModeValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedActorValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* SelectedRoleValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* LockStateValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* UnlockCodeValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* OutlineMaterialValueText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* CustomDepthValueText = nullptr;

    UPROPERTY(Transient)
    UScrollBox* PageScrollBox = nullptr;

    UPROPERTY(Transient)
    UComboBoxString* ThemePresetComboBox = nullptr;

    UPROPERTY(Transient)
    UButton* ToggleKeyButton = nullptr;

    UPROPERTY(Transient)
    UButton* PickKeyButton = nullptr;

    UPROPERTY(Transient)
    UButton* SaveButton = nullptr;

    UPROPERTY(Transient)
    UButton* ResetButton = nullptr;

    UPROPERTY(Transient)
    UCheckBox* PickRequiresCtrlCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* PickRequiresShiftCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* EnableRightMousePickCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* RightMousePickRequiresCtrlCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* RightMousePickRequiresShiftCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* EnableOutlineCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* EnableApplyDebounceCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* RequireUnlockCheckBox = nullptr;

    UPROPERTY(Transient)
    UCheckBox* AutoLockOnCloseCheckBox = nullptr;

    UPROPERTY(Transient)
    USpinBox* OutlineWeightSpinBox = nullptr;

    UPROPERTY(Transient)
    USpinBox* ApplyDebounceSecondsSpinBox = nullptr;
};
