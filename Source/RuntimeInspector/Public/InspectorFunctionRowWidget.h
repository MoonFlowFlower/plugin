#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorTypes.h"
#include "InspectorFunctionRowWidget.generated.h"

class UButton;
class UBorder;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UInspectorFunctionItem;
class UInspectorWorldSubsystem;
class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorFunctionRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorFunctionRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetFunctionItem(UInspectorFunctionItem* InItem);

    bool HasFunctionItem() const { return FunctionItem.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Function")
    bool InvokeForAutomation(FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetFunctionTitleForAutomation() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    void ClearParameterWidgets();
    TArray<FString> CollectArgumentTexts() const;
    UWidget* CreateParameterComboItemWidget(const FString& InItemText) const;

    UFUNCTION()
    void HandleInvokeClicked();

    UFUNCTION()
    UWidget* HandleGenerateParameterComboItem(FString InItemText);

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorFunctionItem> FunctionItem;

    UPROPERTY(Transient)
    UBorder* RootBorder = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* RootBox = nullptr;

    UPROPERTY(Transient)
    UTextBlock* TitleText = nullptr;

    UPROPERTY(Transient)
    UTextBlock* OwnerText = nullptr;

    UPROPERTY(Transient)
    UButton* InvokeButton = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* ParametersBox = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UWidget>> ParameterWidgets;
};
