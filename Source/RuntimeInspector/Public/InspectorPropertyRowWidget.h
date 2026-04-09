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
class UTextBlock;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertyRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorPropertyRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetPropertyItem(UInspectorPropertyItem* InItem);

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

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorPropertyItem> PropertyItem;
};
