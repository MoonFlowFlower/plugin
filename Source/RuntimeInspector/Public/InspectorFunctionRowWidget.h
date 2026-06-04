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
class UImage;
class UInspectorFunctionItem;
class UInspectorWorldSubsystem;
class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorFunctionRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorFunctionRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetFunctionItem(UInspectorFunctionItem* InItem);
    void SetAllowNavigation(bool bInAllowNavigation);

    bool HasFunctionItem() const { return FunctionItem.IsValid(); }
    bool IsDisplayingItem(const UInspectorFunctionItem* InItem) const;
    void RefreshDisplay();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Function")
    bool InvokeForAutomation(FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetFunctionTitleForAutomation() const;

    float GetFavoriteButtonHeightForAutomation() const;
    float GetParameterInputHeightForAutomation() const;
    bool ToggleFavoriteForAutomation(FString& OutError);
    bool HasFavoriteVisualContractForAutomation() const;
    bool HasOverflowLayoutForAutomation() const;
    bool HasCompactRunButtonForAutomation() const;
    bool NavigateForAutomation(FString& OutError);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    void ClearParameterWidgets();
    TArray<FString> CollectArgumentTexts() const;
    UWidget* CreateParameterComboItemWidget(const FString& InItemText) const;
    void UpdateCachedDisplayState(bool bFavorited);

    UFUNCTION()
    void HandleInvokeClicked();

    UFUNCTION()
    UWidget* HandleGenerateParameterComboItem(FString InItemText);

    UFUNCTION()
    void HandleFavoriteClicked();

    UFUNCTION()
    void HandleTitleClicked();

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
    UButton* TitleButton = nullptr;

    UPROPERTY(Transient)
    USizeBox* TitleSizeBox = nullptr;

    UPROPERTY(Transient)
    UButton* FavoriteButton = nullptr;

    UPROPERTY(Transient)
    UImage* FavoriteIcon = nullptr;

    UPROPERTY(Transient)
    USizeBox* FavoriteSizeBox = nullptr;

    UPROPERTY(Transient)
    UTextBlock* OwnerText = nullptr;

    UPROPERTY(Transient)
    UButton* InvokeButton = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* ParametersBox = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UWidget>> ParameterWidgets;

    TWeakObjectPtr<USizeBox> PrimaryParameterSizeBox;
    bool bCachedFavorited = false;
    bool bAllowNavigation = true;
};
