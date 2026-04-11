#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorMaterialParamRowWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UHorizontalBox;
class UInspectorMaterialParamItem;
class UInspectorWorldSubsystem;
class USizeBox;
class UTextBlock;

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

    bool IsColorSwatchVisibleForAutomation() const;
    bool IsScalarValueVisibleForAutomation() const;
    bool IsScalarTextBoxVisibleForAutomation() const;
    bool HasFavoriteButtonForAutomation() const;
    bool TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const;
    float GetValueControlHeightForAutomation() const;
    float GetFavoriteButtonHeightForAutomation() const;
    float GetColorButtonHeightForAutomation() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildWidgetTree();
    void RefreshRow();

    bool ApplyScalarValue(const FString& InValue);

    UFUNCTION()
    void HandleFavoriteClicked();

    UFUNCTION()
    void HandleColorClicked();

    UFUNCTION()
    void HandleScalarCommitted(const FText& InText, ETextCommit::Type CommitMethod);

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> FavoriteButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FavoriteText = nullptr;

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
};
