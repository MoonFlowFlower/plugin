#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPropertiesSectionWidget.generated.h"

class UInspectorPropertyRowWidget;
class UInspectorMaterialParamItem;
class UInspectorMaterialParamRowWidget;
class UInspectorPropertyItem;
class UInspectorWorldSubsystem;
class UButton;
class UBorder;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertyCategoryButtonProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UInspectorPropertiesSectionWidget* InOwner, const FString& InCategoryStateKey, bool bInAllowToggle);

    UFUNCTION()
    void HandleClicked();

private:
    TWeakObjectPtr<UInspectorPropertiesSectionWidget> Owner;
    FString CategoryStateKey;
    bool bAllowToggle = true;
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertiesSectionWidget : public UUserWidget
{
    GENERATED_BODY()
    friend class UInspectorPropertyCategoryButtonProxy;

public:
    UInspectorPropertiesSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetOnlyModified(bool bInOnlyModified);
    void RefreshFromSubsystem();
    void RefreshValueDisplayFromSubsystem();
    bool RefreshItemDisplay(UObject* ItemObject);
    int32 GetEntryWidgetCountForAutomation() const;
    bool HasSummaryHeader() const { return HeaderBorder != nullptr; }
    bool HasPropertyScrollRoot() const { return ScrollBox != nullptr; }
    bool HasTouchScrollSupportForAutomation() const;
    bool HasActorTransformBlockForAutomation() const;
    FString GetActorTransformDebugSummaryForAutomation() const;
    UInspectorPropertyRowWidget* FindPropertyRowForAutomation(const UInspectorPropertyItem* Item) const;
    UInspectorMaterialParamRowWidget* FindMaterialRowForAutomation(const UInspectorMaterialParamItem* Item) const;
    bool ScrollToItemForAutomation(UObject* ItemObject);
    int32 GetCategorySectionCountForAutomation() const { return LastCategorySectionCount; }
    FString GetCategoryDebugSummaryForAutomation() const;
    bool SetCategoryExpandedForAutomation(const FString& PrimaryCategory, bool bExpanded);
    bool IsCategoryExpandedVisualForAutomation(const FString& PrimaryCategory) const;
    FString GetFirstCategoryNameForAutomation() const;
    FString GetFirstPropertyNameInCategoryForAutomation(const FString& PrimaryCategory) const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    struct FPropertyCategoryViewData
    {
        FString PrimaryCategory;
        FString CategoryStateKey;
        bool bExpanded = true;
        bool bForcedExpandedBySearch = false;
        TArray<UInspectorPropertyItem*> Properties;
        TMap<FString, TArray<UInspectorPropertyItem*>> SubcategoryBuckets;
        TArray<FString> SubcategoryOrder;
    };

    void BuildWidgetTree();
    void RefreshHeaderFromSubsystem();
    UWidget* CreatePropertyRow(UObject* ItemObject);
    UWidget* CreateActorTransformBlock(const TArray<UInspectorPropertyItem*>& TransformItems);
    UWidget* CreatePropertyCategoryHeader(const FPropertyCategoryViewData& CategoryData);
    UWidget* CreatePropertySubcategoryHeader(const FString& SubcategoryLabel);
    void ResetEntryState();
    void BuildCategorizedPropertyRows(const TArray<UObject*>& Items, bool bSearchMode);
    void AddPropertyRowsToCategoryBody(UVerticalBox* CategoryBody, const FPropertyCategoryViewData& CategoryData);
    void SetCategoryExpandedVisual(const FString& CategoryStateKey, bool bExpanded);
    void HandleCategoryToggleRequested(const FString& CategoryStateKey);
    FString FindCategoryStateKeyByPrimaryName(const FString& PrimaryCategory) const;
    FString BuildPropertyItemKey(const UInspectorPropertyItem* Item) const;
    FString BuildPropertyTrackingKey(const UInspectorPropertyItem* Item) const;
    FString BuildMaterialItemKey(const UInspectorMaterialParamItem* Item) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> HeaderBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderActorText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderFocusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderSourceText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderStatusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> EntriesBox = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInspectorPropertyCategoryButtonProxy>> CategoryToggleProxies;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    bool bOnlyModified = false;
    TMap<FString, TObjectPtr<UVerticalBox>> CategoryBodyByKey;
    TMap<FString, TObjectPtr<UTextBlock>> CategoryToggleTextByKey;
    TMap<FString, TObjectPtr<UTextBlock>> CategoryLabelTextByKey;
    TMap<FString, FString> CategoryPrimaryNameByKey;
    TMap<FString, FString> CategoryFirstPropertyNameByKey;
    TMap<FString, TWeakObjectPtr<UObject>> CategoryTargetObjectByKey;
    TMap<FString, TObjectPtr<UInspectorPropertyRowWidget>> PropertyRowsByKey;
    TMap<FString, TObjectPtr<UInspectorPropertyRowWidget>> PropertyRowsByTrackingKey;
    TMap<FString, TObjectPtr<UInspectorMaterialParamRowWidget>> MaterialRowsByKey;
    TMap<FString, FString> PropertyCategoryStateKeyByItemKey;
    TArray<FString> LastCategoryOrder;
    bool bActorTransformBlockVisible = false;
    FString LastActorTransformDebugSummary = TEXT("Hidden");
    int32 LastVisiblePropertyRowCount = 0;
    int32 LastVisibleMaterialRowCount = 0;
    int32 LastCategorySectionCount = 0;
};
