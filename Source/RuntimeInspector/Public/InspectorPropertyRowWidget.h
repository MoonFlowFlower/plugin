#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPropertyRowWidget.generated.h"

class UBorder;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UHorizontalBox;
class UInspectorPropertyItem;
class UInspectorWorldSubsystem;
class USizeBox;
class UTextBlock;
class UButton;

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
    bool IsColorSwatchVisibleForAutomation() const;
    bool IsReadOnlyValueVisibleForAutomation() const;
    bool IsValueTextBoxVisibleForAutomation() const;
    bool TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const;
    float GetValueControlHeightForAutomation() const;
    float GetFavoriteButtonHeightForAutomation() const;
    float GetColorButtonHeightForAutomation() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    bool ApplyTextValue(const FString& InValue);

    UFUNCTION()
    void HandleValueCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleBoolChanged(bool bIsChecked);

    UFUNCTION()
    void HandleEnumChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleFavoriteClicked();

    UFUNCTION()
    void HandleColorClicked();

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText = nullptr;

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
    TObjectPtr<UTextBlock> FavoriteText = nullptr;

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
    TObjectPtr<USizeBox> BoolCheckBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> EnumComboBoxSizeBox = nullptr;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorPropertyItem> PropertyItem;
};
