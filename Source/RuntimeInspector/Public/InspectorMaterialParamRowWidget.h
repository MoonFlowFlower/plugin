#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorMaterialParamRowWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class UInspectorMaterialParamItem;
class UInspectorWorldSubsystem;
class USizeBox;
class UTextBlock;
class UWidget;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorMaterialParamRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorMaterialParamRowWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetMaterialItem(UInspectorMaterialParamItem* InItem);
    bool IsDisplayingItem(const UInspectorMaterialParamItem* InItem) const;
    void RefreshDisplay();
    void SetAllowNavigation(bool bInAllowNavigation);

    bool IsColorSwatchVisibleForAutomation() const;
    bool IsScalarValueVisibleForAutomation() const;
    bool IsScalarTextBoxVisibleForAutomation() const;
    bool HasFavoriteButtonForAutomation() const;
    bool HasFavoriteVisualContractForAutomation() const;
    bool TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const;
    float GetValueControlHeightForAutomation() const;
    float GetFavoriteButtonHeightForAutomation() const;
    float GetColorButtonHeightForAutomation() const;
    bool CommitScalarValueForAutomation(const FString& InValue, FString& OutError);
    bool NavigateForAutomation(FString& OutError);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    void BuildWidgetTree();
    void RefreshRow();
    void UpdateCachedScalarDisplay();

    bool ApplyScalarValue(const FString& InValue);

    UFUNCTION()
    void HandleFavoriteClicked();

    UFUNCTION()
    void HandleColorClicked();

    UFUNCTION()
    void HandleScalarCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleNameClicked();

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> FavoriteButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> FavoriteIcon = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> FavoriteSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReadOnlyValueText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ReadOnlyValueSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> ScalarValueTextBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ScalarValueTextBoxSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ColorButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ColorSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ColorSwatch = nullptr;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorMaterialParamItem> MaterialItem;
    bool bHasCachedDisplayColor = false;
    FLinearColor CachedDisplayColor = FLinearColor::Transparent;
    bool bCachedFavorited = false;
    FString CachedScalarValue;
    bool bAllowNavigation = true;
};
