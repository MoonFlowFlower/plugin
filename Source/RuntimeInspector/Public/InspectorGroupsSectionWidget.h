#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorGroupsSectionWidget.generated.h"

class UScrollBox;
class UButton;
class UInspectorGroupItem;
class UInspectorMaterialParamItem;
class UInspectorPropertyItem;
class UInspectorWorldSubsystem;
class USizeBox;
class UVerticalBox;
class UWidget;
class UBorder;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorGroupButtonProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UInspectorWorldSubsystem* InSubsystem, UInspectorGroupItem* InItem);

    UFUNCTION()
    void HandleClicked();

    bool MatchesStableKey(const FString& InStableKey) const;
    FString GetStableKey() const;
    void InvokeForAutomation();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UInspectorGroupItem> Item;
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPinnedItemButtonProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UInspectorWorldSubsystem* InSubsystem, UObject* InItem);

    UFUNCTION()
    void HandleClicked();

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    TWeakObjectPtr<UObject> Item;
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorGroupsSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorGroupsSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void RefreshFromSubsystem();
    int32 GetEntryWidgetCountForDebug() const;
    int32 GetPinnedEntryWidgetCountForDebug() const;
    bool InvokeGroupItemClickForAutomation(const FString& StableKey);
    void GetVisibleGroupStableKeysForAutomation(TArray<FString>& OutKeys) const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    void AppendItemRecursive(class UInspectorGroupItem* Item, const FString& SearchText, TArray<class UInspectorGroupItem*>& OutItems);
    UWidget* CreatePinnedRow(UObject* ItemObject);

private:
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> RootSizeBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ComponentSectionBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ComponentScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> EntriesBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PinnedSectionBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> PinnedScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> PinnedEntriesBox = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInspectorGroupButtonProxy>> ClickProxies;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInspectorPinnedItemButtonProxy>> PinnedClickProxies;

    int32 LastEntryWidgetCount = 0;
    int32 LastPinnedEntryWidgetCount = 0;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
};
