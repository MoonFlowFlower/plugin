#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPropertyRowWidget.generated.h"

class UBorder;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class UInspectorPropertyItem;
class UInspectorWorldSubsystem;
class USizeBox;
class UTextBlock;
class UButton;
class UWidget;
class UVerticalBox;
class USceneComponent;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertyRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorPropertyRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetPropertyItem(UInspectorPropertyItem* InItem);
    bool IsDisplayingItem(const UInspectorPropertyItem* InItem) const;
    void RefreshDisplay();
    void SetAllowNavigation(bool bInAllowNavigation);
    void SetStripOwnerPrefixForDisplay(bool bInStripOwnerPrefix);
    bool IsColorSwatchVisibleForAutomation() const;
    bool IsReadOnlyValueVisibleForAutomation() const;
    bool IsValueTextBoxVisibleForAutomation() const;
    bool TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const;
    float GetValueControlHeightForAutomation() const;
    float GetFavoriteButtonHeightForAutomation() const;
    float GetColorButtonHeightForAutomation() const;
    bool ToggleFavoriteForAutomation(FString& OutError);
    bool HasOverflowLayoutForAutomation() const;
    bool HasExpandedValueEditorForAutomation() const;
    bool CommitTextValueForAutomation(const FString& InValue, FString& OutError);
    bool IsStructuredVectorVisibleForAutomation() const;
    bool IsStructuredRotatorVisibleForAutomation() const;
    bool IsStructuredTransformVisibleForAutomation() const;
    bool CommitVectorValueForAutomation(const FVector& InValue, FString& OutError);
    bool CommitRotatorValueForAutomation(const FRotator& InValue, FString& OutError);
    bool CommitTransformValueForAutomation(const FTransform& InValue, FString& OutError);
    bool NavigateForAutomation(FString& OutError);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    FString GetDisplayedPropertyName(const UInspectorPropertyItem* Item) const;
    void RefreshTickPolicy();
    void UpdateExpandedValueEditorFocusState();
    void ShowExpandedValueEditor(bool bVisible);
    bool ApplyTextValue(const FString& InValue);
    bool ApplyVectorValueInternal(const FVector& InValue, bool bRefreshRow);
    bool ApplyVectorValue(const FVector& InValue);
    bool ApplyRotatorValueInternal(const FRotator& InValue, bool bRefreshRow);
    bool ApplyRotatorValue(const FRotator& InValue);
    bool ApplyTransformValueInternal(const FTransform& InValue, bool bRefreshRow);
    bool ApplyTransformValue(const FTransform& InValue);
    bool ApplySceneComponentTransformValue(USceneComponent* SceneComponent, const FTransform& InValue, bool bRefreshRow);
    UInspectorPropertyItem* MakeSiblingPropertyItem(UObject* TargetObject, FName PropertyFName) const;
    void HideAllValueControls();
    void RefreshStructuredVector(UInspectorPropertyItem* Item, bool bEditable);
    void RefreshStructuredRotator(UInspectorPropertyItem* Item, bool bEditable);
    void RefreshStructuredTransform(UInspectorPropertyItem* Item, bool bEditable);
    bool TryParseEditableNumber(UEditableTextBox* TextBox, double& OutValue) const;
    void PopulateAxisEditors(const TArray<TObjectPtr<UEditableTextBox>>& Editors, const TArray<double>& Values, bool bEditable) const;
    bool ExtractAxisValues(const TArray<TObjectPtr<UEditableTextBox>>& Editors, TArray<double>& OutValues) const;
    bool CommitVectorEditors();
    bool CommitRotatorEditors();
    bool CommitTransformEditors();
    bool HasFocusedEditorInSet(const TArray<TObjectPtr<UEditableTextBox>>& Editors) const;
    bool HasStructuredEditorFocus() const;
    void MarkStructuredPreviewDirty();
    bool TryPreviewStructuredEdit();
    UWidget* CreateEnumOptionWidget(const FString& InItemText) const;
    void UpdateCachedDisplayState(const FString& InCurrentValue, bool bFavorited);

    UFUNCTION()
    void HandleValueCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleBoolChanged(bool bIsChecked);

    UFUNCTION()
    void HandleEnumChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    UWidget* HandleGenerateEnumOptionWidget(FString InItemText);

    UFUNCTION()
    void HandleFavoriteClicked();

    UFUNCTION()
    void HandleColorClicked();

    UFUNCTION()
    void HandleNameClicked();

    UFUNCTION()
    void HandleExpandedValueCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleStructuredAxisTextChanged(const FText& InText);

    UFUNCTION()
    void HandleVectorXCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleVectorYCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleVectorZCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRotatorXCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRotatorYCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRotatorZCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformLocationXCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformLocationYCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformLocationZCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformRotationXCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformRotationYCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformRotationZCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformScaleXCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformScaleYCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleTransformScaleZCommitted(const FText& InText, ETextCommit::Type CommitMethod);

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ContentBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> SummaryRowBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> StructuredValueBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> NameButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> NameSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReadOnlyValueText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> ValueTextBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> BoolCheckBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> EnumComboBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> FavoriteButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> FavoriteIcon = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> FavoriteSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ColorButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ColorSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ColorSwatch = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ReadOnlyValueSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ValueTextBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> ExpandedValueTextBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ExpandedValueTextBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> BoolCheckBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> EnumComboBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> VectorEditorBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RotatorEditorBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> TransformEditorBox = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> VectorAxisEditors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> RotatorAxisEditors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> TransformLocationEditors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> TransformRotationEditors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> TransformScaleEditors;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorPropertyItem> PropertyItem;
    FString CachedDisplayValue;
    bool bCachedFavorited = false;
    bool bAllowNavigation = true;
    bool bStripOwnerPrefixForDisplay = false;
    bool bStructuredPreviewPending = false;
    bool bExpandedValueEditorActive = false;
    float StructuredPreviewAccum = 0.f;
};
