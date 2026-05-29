#include "InspectorPropertiesSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorGroupItem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorMaterialParamRowWidget.h"
#include "InspectorPropertyItem.h"
#include "InspectorPropertyRowWidget.h"
#include "InspectorTouchScrollBox.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

namespace
{
    static FLinearColor RI_PropertySectionMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }

    static FLinearColor RI_PropertySectionColor()
    {
        return RICompactUI::GetSectionSurfaceBackgroundColor();
    }

    static FString RI_SanitizeCategoryToken(const FString& InValue)
    {
        FString Sanitized = InValue;
        Sanitized.ReplaceInline(TEXT(" "), TEXT("_"));
        Sanitized.ReplaceInline(TEXT("/"), TEXT("_"));
        Sanitized.ReplaceInline(TEXT("|"), TEXT("_"));
        Sanitized.ReplaceInline(TEXT(":"), TEXT("_"));
        Sanitized.ReplaceInline(TEXT("."), TEXT("_"));
        return Sanitized;
    }

    static bool RI_CanCategorizePropertyItems(const TArray<UObject*>& Items)
    {
        if (Items.Num() == 0)
        {
            return false;
        }

        for (UObject* ItemObject : Items)
        {
            if (!Cast<UInspectorPropertyItem>(ItemObject))
            {
                return false;
            }
        }

        return true;
    }

    static constexpr int32 RI_PropertyDeferredBatchSize = 6;
}

void UInspectorPropertyCategoryButtonProxy::Initialize(UInspectorPropertiesSectionWidget* InOwner, const FString& InCategoryStateKey, bool bInAllowToggle)
{
    Owner = InOwner;
    CategoryStateKey = InCategoryStateKey;
    bAllowToggle = bInAllowToggle;
}

void UInspectorPropertyCategoryButtonProxy::HandleClicked()
{
    if (!bAllowToggle)
    {
        return;
    }

    if (UInspectorPropertiesSectionWidget* OwnerWidget = Owner.Get())
    {
        OwnerWidget->HandleCategoryToggleRequested(CategoryStateKey);
    }
}

UInspectorPropertiesSectionWidget::UInspectorPropertiesSectionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorPropertiesSectionWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void UInspectorPropertiesSectionWidget::SetOnlyModified(bool bInOnlyModified)
{
    bOnlyModified = bInOnlyModified;
}

int32 UInspectorPropertiesSectionWidget::GetEntryWidgetCountForAutomation() const
{
    return LastVisiblePropertyRowCount + LastVisibleMaterialRowCount;
}

void UInspectorPropertiesSectionWidget::CancelDeferredRefresh()
{
    ++DeferredRefreshSerial;
    bDeferredRefreshPending = false;
    DeferredRefreshMode = ERIDeferredPropertyRefreshMode::None;
    DeferredFlatItems.Reset();
    DeferredCategories.Reset();
    DeferredFlatIndex = 0;
    DeferredCategoryIndex = 0;
}

bool UInspectorPropertiesSectionWidget::FlushDeferredRefreshForAutomation(int32 MaxIterations)
{
    int32 Iterations = 0;
    while (bDeferredRefreshPending && Iterations < MaxIterations)
    {
        ProcessDeferredRefresh(DeferredRefreshSerial);
        ++Iterations;
    }

    return !bDeferredRefreshPending;
}

bool UInspectorPropertiesSectionWidget::HasTouchScrollSupportForAutomation() const
{
    return RIInspectorTouchScroll::HasTouchSupport(ScrollBox);
}

bool UInspectorPropertiesSectionWidget::HasActorTransformBlockForAutomation() const
{
    return bActorTransformBlockVisible;
}

FString UInspectorPropertiesSectionWidget::GetActorTransformDebugSummaryForAutomation() const
{
    return LastActorTransformDebugSummary;
}

FString UInspectorPropertiesSectionWidget::GetCategoryDebugSummaryForAutomation() const
{
    if (LastCategoryOrder.Num() == 0)
    {
        return TEXT("None");
    }

    TArray<FString> Parts;
    for (const FString& CategoryStateKey : LastCategoryOrder)
    {
        const UVerticalBox* CategoryBody = CategoryBodyByKey.FindRef(CategoryStateKey);
        const UTextBlock* CategoryLabel = CategoryLabelTextByKey.FindRef(CategoryStateKey);
        const bool bExpanded = CategoryBody && CategoryBody->GetVisibility() == ESlateVisibility::Visible;
        const int32 ChildCount = CategoryBody ? CategoryBody->GetChildrenCount() : 0;
        Parts.Add(FString::Printf(
            TEXT("%s[%d|children=%d]"),
            CategoryLabel ? *CategoryLabel->GetText().ToString() : *CategoryStateKey,
            bExpanded ? 1 : 0,
            ChildCount));
    }

    return FString::Join(Parts, TEXT(" | "));
}

FString UInspectorPropertiesSectionWidget::FindCategoryStateKeyByPrimaryName(const FString& PrimaryCategory) const
{
    for (const TPair<FString, FString>& Pair : CategoryPrimaryNameByKey)
    {
        if (Pair.Value.Equals(PrimaryCategory, ESearchCase::CaseSensitive))
        {
            return Pair.Key;
        }
    }

    return FString();
}

bool UInspectorPropertiesSectionWidget::SetCategoryExpandedForAutomation(const FString& PrimaryCategory, bool bExpanded)
{
    const FString CategoryStateKey = FindCategoryStateKeyByPrimaryName(PrimaryCategory);
    if (CategoryStateKey.IsEmpty())
    {
        return false;
    }

    SetCategoryExpandedVisual(CategoryStateKey, bExpanded);
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        const FString* StoredPrimaryCategory = CategoryPrimaryNameByKey.Find(CategoryStateKey);
        const TWeakObjectPtr<UObject>* TargetObject = CategoryTargetObjectByKey.Find(CategoryStateKey);
        InspectorSubsystem->SetPropertyCategoryExpanded(
            TargetObject ? TargetObject->Get() : nullptr,
            StoredPrimaryCategory ? *StoredPrimaryCategory : PrimaryCategory,
            bExpanded);
    }

    return IsCategoryExpandedVisualForAutomation(PrimaryCategory) == bExpanded;
}

bool UInspectorPropertiesSectionWidget::IsCategoryExpandedVisualForAutomation(const FString& PrimaryCategory) const
{
    const FString CategoryStateKey = FindCategoryStateKeyByPrimaryName(PrimaryCategory);
    if (CategoryStateKey.IsEmpty())
    {
        return false;
    }

    const UVerticalBox* CategoryBody = CategoryBodyByKey.FindRef(CategoryStateKey);
    return CategoryBody && CategoryBody->GetVisibility() == ESlateVisibility::Visible;
}

FString UInspectorPropertiesSectionWidget::GetFirstCategoryNameForAutomation() const
{
    for (const FString& CategoryStateKey : LastCategoryOrder)
    {
        const FString* PrimaryCategory = CategoryPrimaryNameByKey.Find(CategoryStateKey);
        if (PrimaryCategory && !PrimaryCategory->Equals(TEXT("Default"), ESearchCase::CaseSensitive))
        {
            return *PrimaryCategory;
        }
    }

    for (const FString& CategoryStateKey : LastCategoryOrder)
    {
        if (const FString* PrimaryCategory = CategoryPrimaryNameByKey.Find(CategoryStateKey))
        {
            return *PrimaryCategory;
        }
    }

    return FString();
}

FString UInspectorPropertiesSectionWidget::GetFirstPropertyNameInCategoryForAutomation(const FString& PrimaryCategory) const
{
    const FString CategoryStateKey = FindCategoryStateKeyByPrimaryName(PrimaryCategory);
    if (CategoryStateKey.IsEmpty())
    {
        return FString();
    }

    const FString* PropertyName = CategoryFirstPropertyNameByKey.Find(CategoryStateKey);
    return PropertyName ? *PropertyName : FString();
}

TSharedRef<SWidget> UInspectorPropertiesSectionWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorPropertiesSectionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WidgetTree && WidgetTree->RootWidget && !EntriesBox)
    {
        RootBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_ActorPropertiesBorder")));
        RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_ActorPropertiesRoot")));
        HeaderBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_ActorPropertiesHeader")));
        ScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_ActorPropertiesScroll")));
        EntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_ActorPropertiesEntries")));
    }

    RIInspectorTouchScroll::Configure(ScrollBox);
    RefreshFromSubsystem();
}

void UInspectorPropertiesSectionWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorPropertiesBorder"));
    RootBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
    RootBorder->SetBrushColor(RI_PropertySectionColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesRoot"));
    RootBorder->SetContent(RootBox);

    if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Actor Attributes"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    UBorder* BodyBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorPropertiesBodyBorder"));
    BodyBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
    BodyBorder->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());

    ScrollBox = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_ActorPropertiesScroll"));
    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesEntries"));
    ScrollBox->AddChild(EntriesBox);
    RIInspectorTouchScroll::Configure(ScrollBox);
    BodyBorder->SetContent(ScrollBox);

    if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(BodyBorder))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BodySlot->SetPadding(FMargin(0.f));
    }

    WidgetTree->RootWidget = RootBorder;
}

void UInspectorPropertiesSectionWidget::ResetEntryState()
{
    if (EntriesBox)
    {
        EntriesBox->ClearChildren();
    }

    CategoryToggleProxies.Reset();
    CategoryBodyByKey.Reset();
    CategoryToggleTextByKey.Reset();
    CategoryLabelTextByKey.Reset();
    CategoryPrimaryNameByKey.Reset();
    CategoryFirstPropertyNameByKey.Reset();
    CategoryTargetObjectByKey.Reset();
    PropertyRowsByKey.Reset();
    PropertyRowsByTrackingKey.Reset();
    MaterialRowsByKey.Reset();
    PropertyCategoryStateKeyByItemKey.Reset();
    LastCategoryOrder.Reset();
    bActorTransformBlockVisible = false;
    LastActorTransformDebugSummary = TEXT("Hidden");
    LastVisiblePropertyRowCount = 0;
    LastVisibleMaterialRowCount = 0;
    LastCategorySectionCount = 0;
}

FString UInspectorPropertiesSectionWidget::BuildPropertyItemKey(const UInspectorPropertyItem* Item) const
{
    if (!Item)
    {
        return FString();
    }

    return FString::Printf(
        TEXT("PROP:%s:%s"),
        *GetPathNameSafe(Item->GetTargetObject()),
        *Item->GetPropertyFName().ToString());
}

FString UInspectorPropertiesSectionWidget::BuildPropertyTrackingKey(const UInspectorPropertyItem* Item) const
{
    if (!Item)
    {
        return FString();
    }

    return FString::Printf(
        TEXT("PROP_TRACK:%s:%s"),
        *GetPathNameSafe(Item->GetTrackingTargetObject()),
        *Item->GetTrackingPropertyFName().ToString());
}

FString UInspectorPropertiesSectionWidget::BuildMaterialItemKey(const UInspectorMaterialParamItem* Item) const
{
    if (!Item)
    {
        return FString();
    }

    return FString::Printf(
        TEXT("MAT:%s:%d:%d:%s"),
        *GetPathNameSafe(Item->GetMeshComponent()),
        Item->GetSlotIndex(),
        static_cast<int32>(Item->GetParamType()),
        *Item->GetParamName().ToString());
}

UWidget* UInspectorPropertiesSectionWidget::CreatePropertyRow(UObject* ItemObject)
{
    if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ItemObject))
    {
        return RICompactUI::MakeSectionTitle(WidgetTree, GroupItem->DisplayName, RICompactUI::ERISectionVisualStyle::Emphasis);
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        UInspectorMaterialParamRowWidget* Row = nullptr;
        if (APlayerController* PC = GetOwningPlayer())
        {
            Row = CreateWidget<UInspectorMaterialParamRowWidget>(PC, UInspectorMaterialParamRowWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            Row = CreateWidget<UInspectorMaterialParamRowWidget>(World, UInspectorMaterialParamRowWidget::StaticClass());
        }

        if (!Row)
        {
            return nullptr;
        }

        Row->SetInspectorSubsystem(Subsystem.Get());
        Row->SetMaterialItem(MaterialItem);
        Row->TakeWidget();
        MaterialRowsByKey.Add(BuildMaterialItemKey(MaterialItem), Row);
        return Row;
    }

    UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject);
    if (!PropertyItem)
    {
        return nullptr;
    }

    UInspectorPropertyRowWidget* Row = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        Row = CreateWidget<UInspectorPropertyRowWidget>(PC, UInspectorPropertyRowWidget::StaticClass());
    }
    else if (UWorld* World = GetWorld())
    {
        Row = CreateWidget<UInspectorPropertyRowWidget>(World, UInspectorPropertyRowWidget::StaticClass());
    }

    if (!Row)
    {
        return nullptr;
    }

    Row->SetInspectorSubsystem(Subsystem.Get());
    Row->SetStripOwnerPrefixForDisplay(true);
    Row->SetPropertyItem(PropertyItem);
    Row->TakeWidget();
    PropertyRowsByKey.Add(BuildPropertyItemKey(PropertyItem), Row);
    const FString TrackingKey = BuildPropertyTrackingKey(PropertyItem);
    if (!TrackingKey.IsEmpty())
    {
        PropertyRowsByTrackingKey.Add(TrackingKey, Row);
    }
    return Row;
}

UWidget* UInspectorPropertiesSectionWidget::CreateActorTransformBlock(const TArray<UInspectorPropertyItem*>& TransformItems)
{
    if (!WidgetTree || TransformItems.Num() == 0)
    {
        return nullptr;
    }

    UBorder* BlockBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorTransformBlock"));
    BlockBorder->SetPadding(FMargin(0.f));
    BlockBorder->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());

    UVerticalBox* BlockBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorTransformBlockBox"));
    BlockBorder->SetContent(BlockBox);

    if (UVerticalBoxSlot* TitleSlot = BlockBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Transform"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    TArray<FString> SummaryParts;
    for (UInspectorPropertyItem* TransformItem : TransformItems)
    {
        if (!TransformItem)
        {
            continue;
        }

        SummaryParts.Add(FString::Printf(
            TEXT("%s->%s.%s"),
            *TransformItem->GetPropertyNameWithoutOwnerPrefix(),
            *GetNameSafe(TransformItem->GetTrackingTargetObject()),
            *TransformItem->GetTrackingPropertyFName().ToString()));

        if (UWidget* RowWidget = CreatePropertyRow(TransformItem))
        {
            if (UVerticalBoxSlot* RowSlot = BlockBox->AddChildToVerticalBox(RowWidget))
            {
                RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }
            ++LastVisiblePropertyRowCount;
        }
    }

    bActorTransformBlockVisible = SummaryParts.Num() > 0;
    LastActorTransformDebugSummary = SummaryParts.Num() > 0
        ? FString::Join(SummaryParts, TEXT(" | "))
        : TEXT("Hidden");

    return bActorTransformBlockVisible ? BlockBorder : nullptr;
}

UWidget* UInspectorPropertiesSectionWidget::CreatePropertySubcategoryHeader(const FString& SubcategoryLabel)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        *FString::Printf(TEXT("RI_PropertySubcategory_%s"), *RI_SanitizeCategoryToken(SubcategoryLabel)));
    Border->SetPadding(FMargin(6.f, 2.f, 6.f, 2.f));
    Border->SetBrushColor(RICompactUI::GetCellSurfaceBackgroundColor());
    Border->SetContent(RICompactUI::MakeText(
        WidgetTree,
        SubcategoryLabel,
        RICompactUI::GetMutedFontSize(),
        true,
        RICompactUI::GetSecondaryTextColor(),
        true));
    return Border;
}

UWidget* UInspectorPropertiesSectionWidget::CreatePropertyCategoryHeader(const FPropertyCategoryViewData& CategoryData)
{
    const FString Token = RI_SanitizeCategoryToken(CategoryData.PrimaryCategory);
    UButton* HeaderButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(),
        *FString::Printf(TEXT("RI_PropertyCategoryButton_%s"), *Token));

    UBorder* HeaderSurface = RICompactUI::MakeSurfaceCard(
        WidgetTree,
        *FString::Printf(TEXT("RI_PropertyCategorySurface_%s"), *Token),
        RICompactUI::GetContextSecondaryCellBackgroundColor(),
        FMargin(5.f, 2.f));

    UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),
        *FString::Printf(TEXT("RI_PropertyCategoryRow_%s"), *Token));
    HeaderSurface->SetContent(RowBox);
    HeaderButton->SetContent(HeaderSurface);
    RICompactUI::ConfigureSwatchButton(HeaderButton);

    UTextBlock* ToggleText = RICompactUI::MakeText(
        WidgetTree,
        CategoryData.bExpanded ? TEXT("-") : TEXT("+"),
        RICompactUI::GetMutedFontSize(),
        true,
        RICompactUI::GetStrongTextColor());
    ToggleText->SetJustification(ETextJustify::Center);
    if (UHorizontalBoxSlot* ToggleSlot = RowBox->AddChildToHorizontalBox(ToggleText))
    {
        ToggleSlot->SetPadding(FMargin(1.f, 0.f, 6.f, 0.f));
        ToggleSlot->SetHorizontalAlignment(HAlign_Center);
        ToggleSlot->SetVerticalAlignment(VAlign_Center);
        ToggleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    UTextBlock* LabelText = RICompactUI::MakeText(
        WidgetTree,
        CategoryData.PrimaryCategory,
        RICompactUI::GetLabelFontSize(),
        true,
        RICompactUI::GetStrongTextColor(),
        true);
    RICompactUI::ConfigureEllipsisText(LabelText, CategoryData.PrimaryCategory);
    if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(LabelText))
    {
        LabelSlot->SetHorizontalAlignment(HAlign_Fill);
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    CategoryToggleTextByKey.Add(CategoryData.CategoryStateKey, ToggleText);
    CategoryLabelTextByKey.Add(CategoryData.CategoryStateKey, LabelText);
    CategoryPrimaryNameByKey.Add(CategoryData.CategoryStateKey, CategoryData.PrimaryCategory);
    if (CategoryData.Properties.Num() > 0)
    {
        CategoryTargetObjectByKey.Add(CategoryData.CategoryStateKey, CategoryData.Properties[0]->GetTargetObject());
    }

    UInspectorPropertyCategoryButtonProxy* Proxy = NewObject<UInspectorPropertyCategoryButtonProxy>(this);
    Proxy->Initialize(this, CategoryData.CategoryStateKey, !CategoryData.bForcedExpandedBySearch);
    HeaderButton->OnClicked.AddDynamic(Proxy, &UInspectorPropertyCategoryButtonProxy::HandleClicked);
    CategoryToggleProxies.Add(Proxy);

    return HeaderButton;
}

void UInspectorPropertiesSectionWidget::AddPropertyRowsToCategoryBody(UVerticalBox* CategoryBody, const FPropertyCategoryViewData& CategoryData)
{
    if (!CategoryBody)
    {
        return;
    }

    TSet<FString> RenderedSubcategories;
    for (UInspectorPropertyItem* PropertyItem : CategoryData.Properties)
    {
        if (!PropertyItem)
        {
            continue;
        }

        const FString SubcategoryPath = PropertyItem->GetSubcategoryPath();
        if (!SubcategoryPath.IsEmpty() && !RenderedSubcategories.Contains(SubcategoryPath))
        {
            RenderedSubcategories.Add(SubcategoryPath);
            if (UWidget* SubcategoryHeader = CreatePropertySubcategoryHeader(SubcategoryPath))
            {
                if (UVerticalBoxSlot* HeaderSlot = CategoryBody->AddChildToVerticalBox(SubcategoryHeader))
                {
                    HeaderSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 3.f));
                }
            }
        }

        if (UWidget* RowWidget = CreatePropertyRow(PropertyItem))
        {
            if (UVerticalBoxSlot* EntrySlot = CategoryBody->AddChildToVerticalBox(RowWidget))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }
            ++LastVisiblePropertyRowCount;
        }
    }
}

void UInspectorPropertiesSectionWidget::SetCategoryExpandedVisual(const FString& CategoryStateKey, bool bExpanded)
{
    if (UVerticalBox* CategoryBody = CategoryBodyByKey.FindRef(CategoryStateKey))
    {
        CategoryBody->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (UTextBlock* ToggleText = CategoryToggleTextByKey.FindRef(CategoryStateKey))
    {
        ToggleText->SetText(FText::FromString(bExpanded ? TEXT("-") : TEXT("+")));
    }
}

void UInspectorPropertiesSectionWidget::HandleCategoryToggleRequested(const FString& CategoryStateKey)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UVerticalBox* CategoryBody = CategoryBodyByKey.FindRef(CategoryStateKey);
    if (!InspectorSubsystem || !CategoryBody)
    {
        return;
    }

    const bool bExpanded = CategoryBody->GetVisibility() == ESlateVisibility::Visible;
    const bool bNewExpanded = !bExpanded;
    SetCategoryExpandedVisual(CategoryStateKey, bNewExpanded);

    const FString* PrimaryCategory = CategoryPrimaryNameByKey.Find(CategoryStateKey);
    const TWeakObjectPtr<UObject>* TargetObject = CategoryTargetObjectByKey.Find(CategoryStateKey);
    InspectorSubsystem->SetPropertyCategoryExpanded(
        TargetObject ? TargetObject->Get() : nullptr,
        PrimaryCategory ? *PrimaryCategory : FString(),
        bNewExpanded);
}

void UInspectorPropertiesSectionWidget::BuildCategorizedPropertyRows(const TArray<UObject*>& Items, bool bSearchMode)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem || !WidgetTree || !EntriesBox)
    {
        return;
    }

    TMap<FString, FPropertyCategoryViewData> CategoryMap;
    TArray<FString> CategoryOrder;

    for (UObject* ItemObject : Items)
    {
        UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject);
        if (!PropertyItem)
        {
            continue;
        }

        const FString PrimaryCategory = PropertyItem->GetPrimaryCategoryName();
        const FString CategoryStateKey = InspectorSubsystem->BuildPropertyCategoryStateKey(PropertyItem->GetTargetObject(), PrimaryCategory);
        FPropertyCategoryViewData* CategoryData = CategoryMap.Find(CategoryStateKey);
        if (!CategoryData)
        {
            FPropertyCategoryViewData NewCategoryData;
            NewCategoryData.PrimaryCategory = PrimaryCategory;
            NewCategoryData.CategoryStateKey = CategoryStateKey;
            NewCategoryData.bForcedExpandedBySearch = bSearchMode;
            NewCategoryData.bExpanded = bSearchMode
                ? true
                : InspectorSubsystem->GetPropertyCategoryExpanded(PropertyItem->GetTargetObject(), PrimaryCategory, true);
            CategoryOrder.Add(CategoryStateKey);
            CategoryData = &CategoryMap.Add(CategoryStateKey, MoveTemp(NewCategoryData));
        }

        CategoryData->Properties.Add(PropertyItem);
        if (!CategoryFirstPropertyNameByKey.Contains(CategoryStateKey))
        {
            CategoryFirstPropertyNameByKey.Add(CategoryStateKey, PropertyItem->GetPropertyName());
        }
        PropertyCategoryStateKeyByItemKey.Add(BuildPropertyItemKey(PropertyItem), CategoryStateKey);
    }

    LastCategoryOrder = CategoryOrder;
    LastCategorySectionCount = CategoryOrder.Num();

    for (const FString& CategoryStateKey : CategoryOrder)
    {
        const FPropertyCategoryViewData* CategoryData = CategoryMap.Find(CategoryStateKey);
        if (!CategoryData)
        {
            continue;
        }

        if (UWidget* HeaderWidget = CreatePropertyCategoryHeader(*CategoryData))
        {
            if (UVerticalBoxSlot* HeaderSlot = EntriesBox->AddChildToVerticalBox(HeaderWidget))
            {
                HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
            }
        }

        UVerticalBox* CategoryBody = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            *FString::Printf(TEXT("RI_PropertyCategoryBody_%s"), *RI_SanitizeCategoryToken(CategoryData->PrimaryCategory)));
        CategoryBodyByKey.Add(CategoryStateKey, CategoryBody);

        AddPropertyRowsToCategoryBody(CategoryBody, *CategoryData);

        if (UVerticalBoxSlot* BodySlot = EntriesBox->AddChildToVerticalBox(CategoryBody))
        {
            BodySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 6.f));
        }

        SetCategoryExpandedVisual(CategoryStateKey, CategoryData->bExpanded);
    }
}

void UInspectorPropertiesSectionWidget::RefreshFromSubsystemDeferred()
{
    if (!EntriesBox || !WidgetTree)
    {
        return;
    }

    CancelDeferredRefresh();
    RIInspectorTouchScroll::Configure(ScrollBox);
    ResetEntryState();
    RefreshHeaderFromSubsystem();
    EntriesBox->AddChildToVerticalBox(
        RICompactUI::MakeText(WidgetTree, TEXT("Loading attributes..."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));

    bDeferredRefreshPending = true;
    DeferredRefreshMode = ERIDeferredPropertyRefreshMode::Collect;
    ++DeferredRefreshSerial;
    ScheduleDeferredRefreshTick();
}

void UInspectorPropertiesSectionWidget::ScheduleDeferredRefreshTick()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        ProcessDeferredRefresh(DeferredRefreshSerial);
        return;
    }

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &UInspectorPropertiesSectionWidget::ProcessDeferredRefresh, DeferredRefreshSerial);
    World->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UInspectorPropertiesSectionWidget::ProcessDeferredRefresh(int32 Serial)
{
    if (Serial != DeferredRefreshSerial || !bDeferredRefreshPending || !EntriesBox || !WidgetTree)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        ResetEntryState();
        RefreshHeaderFromSubsystem();
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
        FinishDeferredRefresh();
        return;
    }

    if (DeferredRefreshMode == ERIDeferredPropertyRefreshMode::Collect)
    {
        ResetEntryState();
        RefreshHeaderFromSubsystem();

        const FString SearchText = InspectorSubsystem->GetCurrentActorSearchText();
        const bool bSearchMode = !SearchText.IsEmpty();

        TArray<UInspectorPropertyItem*> ActorTransformItems;
        InspectorSubsystem->GetActorWorldTransformPropertyItems(ActorTransformItems);
        if (UWidget* TransformBlock = CreateActorTransformBlock(ActorTransformItems))
        {
            if (UVerticalBoxSlot* TransformSlot = EntriesBox->AddChildToVerticalBox(TransformBlock))
            {
                TransformSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
            }
        }

        TArray<UObject*> Items;
        InspectorSubsystem->GetPropertyItemsForSelectedEx(SearchText, bOnlyModified, Items);
        if (RI_CanCategorizePropertyItems(Items))
        {
            PrepareDeferredCategorizedPropertyRows(Items, bSearchMode);
            DeferredRefreshMode = ERIDeferredPropertyRefreshMode::Categories;
        }
        else
        {
            DeferredFlatItems.Reset();
            for (UObject* ItemObject : Items)
            {
                if (ItemObject)
                {
                    DeferredFlatItems.Add(ItemObject);
                }
            }
            DeferredFlatIndex = 0;
            DeferredRefreshMode = ERIDeferredPropertyRefreshMode::Flat;
        }
    }

    const bool bComplete = DeferredRefreshMode == ERIDeferredPropertyRefreshMode::Categories
        ? BuildNextDeferredPropertyBatch()
        : BuildNextDeferredFlatBatch();

    if (bComplete)
    {
        AddDeferredEmptyStateIfNeeded();
        FinishDeferredRefresh();
        return;
    }

    ScheduleDeferredRefreshTick();
}

void UInspectorPropertiesSectionWidget::PrepareDeferredCategorizedPropertyRows(const TArray<UObject*>& Items, bool bSearchMode)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem || !WidgetTree || !EntriesBox)
    {
        return;
    }

    TMap<FString, int32> CategoryIndexByKey;
    DeferredCategories.Reset();
    LastCategoryOrder.Reset();
    LastCategorySectionCount = 0;

    for (UObject* ItemObject : Items)
    {
        UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject);
        if (!PropertyItem)
        {
            continue;
        }

        const FString PrimaryCategory = PropertyItem->GetPrimaryCategoryName();
        const FString CategoryStateKey = InspectorSubsystem->BuildPropertyCategoryStateKey(PropertyItem->GetTargetObject(), PrimaryCategory);
        int32* ExistingIndex = CategoryIndexByKey.Find(CategoryStateKey);
        if (!ExistingIndex)
        {
            FDeferredPropertyCategoryViewData NewCategoryData;
            NewCategoryData.PrimaryCategory = PrimaryCategory;
            NewCategoryData.CategoryStateKey = CategoryStateKey;
            NewCategoryData.bForcedExpandedBySearch = bSearchMode;
            NewCategoryData.bExpanded = bSearchMode
                ? true
                : InspectorSubsystem->GetPropertyCategoryExpanded(PropertyItem->GetTargetObject(), PrimaryCategory, true);
            const int32 NewIndex = DeferredCategories.Add(MoveTemp(NewCategoryData));
            CategoryIndexByKey.Add(CategoryStateKey, NewIndex);
            ExistingIndex = CategoryIndexByKey.Find(CategoryStateKey);
            LastCategoryOrder.Add(CategoryStateKey);
        }

        FDeferredPropertyCategoryViewData& CategoryData = DeferredCategories[*ExistingIndex];
        CategoryData.Properties.Add(PropertyItem);
        if (!CategoryFirstPropertyNameByKey.Contains(CategoryStateKey))
        {
            CategoryFirstPropertyNameByKey.Add(CategoryStateKey, PropertyItem->GetPropertyName());
        }
        PropertyCategoryStateKeyByItemKey.Add(BuildPropertyItemKey(PropertyItem), CategoryStateKey);
    }

    LastCategorySectionCount = DeferredCategories.Num();
    DeferredCategoryIndex = 0;

    for (const FDeferredPropertyCategoryViewData& CategoryData : DeferredCategories)
    {
        FPropertyCategoryViewData HeaderData;
        HeaderData.PrimaryCategory = CategoryData.PrimaryCategory;
        HeaderData.CategoryStateKey = CategoryData.CategoryStateKey;
        HeaderData.bExpanded = CategoryData.bExpanded;
        HeaderData.bForcedExpandedBySearch = CategoryData.bForcedExpandedBySearch;
        for (const TWeakObjectPtr<UInspectorPropertyItem>& PropertyPtr : CategoryData.Properties)
        {
            if (UInspectorPropertyItem* PropertyItem = PropertyPtr.Get())
            {
                HeaderData.Properties.Add(PropertyItem);
            }
        }

        if (UWidget* HeaderWidget = CreatePropertyCategoryHeader(HeaderData))
        {
            if (UVerticalBoxSlot* HeaderSlot = EntriesBox->AddChildToVerticalBox(HeaderWidget))
            {
                HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
            }
        }

        UVerticalBox* CategoryBody = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            *FString::Printf(TEXT("RI_PropertyCategoryBody_%s"), *RI_SanitizeCategoryToken(CategoryData.PrimaryCategory)));
        CategoryBodyByKey.Add(CategoryData.CategoryStateKey, CategoryBody);

        if (UVerticalBoxSlot* BodySlot = EntriesBox->AddChildToVerticalBox(CategoryBody))
        {
            BodySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 6.f));
        }

        SetCategoryExpandedVisual(CategoryData.CategoryStateKey, CategoryData.bExpanded);
    }
}

bool UInspectorPropertiesSectionWidget::BuildNextDeferredPropertyBatch()
{
    int32 BuiltCount = 0;
    while (DeferredCategoryIndex < DeferredCategories.Num() && BuiltCount < RI_PropertyDeferredBatchSize)
    {
        FDeferredPropertyCategoryViewData& CategoryData = DeferredCategories[DeferredCategoryIndex];
        if (!CategoryData.bExpanded)
        {
            ++DeferredCategoryIndex;
            continue;
        }

        UVerticalBox* CategoryBody = CategoryBodyByKey.FindRef(CategoryData.CategoryStateKey);
        if (!CategoryBody)
        {
            ++DeferredCategoryIndex;
            continue;
        }

        while (CategoryData.NextPropertyIndex < CategoryData.Properties.Num() && BuiltCount < RI_PropertyDeferredBatchSize)
        {
            UInspectorPropertyItem* PropertyItem = CategoryData.Properties[CategoryData.NextPropertyIndex].Get();
            ++CategoryData.NextPropertyIndex;
            if (!PropertyItem)
            {
                continue;
            }

            const FString SubcategoryPath = PropertyItem->GetSubcategoryPath();
            if (!SubcategoryPath.IsEmpty() && !CategoryData.RenderedSubcategories.Contains(SubcategoryPath))
            {
                CategoryData.RenderedSubcategories.Add(SubcategoryPath);
                if (UWidget* SubcategoryHeader = CreatePropertySubcategoryHeader(SubcategoryPath))
                {
                    if (UVerticalBoxSlot* HeaderSlot = CategoryBody->AddChildToVerticalBox(SubcategoryHeader))
                    {
                        HeaderSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 3.f));
                    }
                }
            }

            if (UWidget* RowWidget = CreatePropertyRow(PropertyItem))
            {
                if (UVerticalBoxSlot* EntrySlot = CategoryBody->AddChildToVerticalBox(RowWidget))
                {
                    EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
                }
                ++LastVisiblePropertyRowCount;
                ++BuiltCount;
            }
        }

        if (CategoryData.NextPropertyIndex >= CategoryData.Properties.Num())
        {
            ++DeferredCategoryIndex;
        }
    }

    return DeferredCategoryIndex >= DeferredCategories.Num();
}

bool UInspectorPropertiesSectionWidget::BuildNextDeferredFlatBatch()
{
    int32 BuiltCount = 0;
    while (DeferredFlatIndex < DeferredFlatItems.Num() && BuiltCount < RI_PropertyDeferredBatchSize)
    {
        UObject* ItemObject = DeferredFlatItems[DeferredFlatIndex].Get();
        ++DeferredFlatIndex;
        if (!ItemObject)
        {
            continue;
        }

        if (UWidget* RowWidget = CreatePropertyRow(ItemObject))
        {
            if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowWidget))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }

            if (Cast<UInspectorPropertyItem>(ItemObject))
            {
                ++LastVisiblePropertyRowCount;
            }
            else if (Cast<UInspectorMaterialParamItem>(ItemObject))
            {
                ++LastVisibleMaterialRowCount;
            }
            ++BuiltCount;
        }
    }

    return DeferredFlatIndex >= DeferredFlatItems.Num();
}

void UInspectorPropertiesSectionWidget::AddDeferredEmptyStateIfNeeded()
{
    if ((LastVisiblePropertyRowCount + LastVisibleMaterialRowCount) == 0 && !bActorTransformBlockVisible)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No visible properties match the current selection."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
    }
}

void UInspectorPropertiesSectionWidget::FinishDeferredRefresh()
{
    bDeferredRefreshPending = false;
    DeferredRefreshMode = ERIDeferredPropertyRefreshMode::None;
    DeferredFlatItems.Reset();
    DeferredCategories.Reset();
    DeferredFlatIndex = 0;
    DeferredCategoryIndex = 0;
}

void UInspectorPropertiesSectionWidget::RefreshFromSubsystem()
{
    if (!EntriesBox || !WidgetTree)
    {
        return;
    }

    RIInspectorTouchScroll::Configure(ScrollBox);
    CancelDeferredRefresh();
    ResetEntryState();

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        RefreshHeaderFromSubsystem();
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
        return;
    }

    RefreshHeaderFromSubsystem();

    const FString SearchText = InspectorSubsystem->GetCurrentActorSearchText();
    const bool bSearchMode = !SearchText.IsEmpty();
    TArray<UInspectorPropertyItem*> ActorTransformItems;
    InspectorSubsystem->GetActorWorldTransformPropertyItems(ActorTransformItems);
    if (UWidget* TransformBlock = CreateActorTransformBlock(ActorTransformItems))
    {
        if (UVerticalBoxSlot* TransformSlot = EntriesBox->AddChildToVerticalBox(TransformBlock))
        {
            TransformSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
        }
    }

    TArray<UObject*> Items;
    InspectorSubsystem->GetPropertyItemsForSelectedEx(SearchText, bOnlyModified, Items);

    if (RI_CanCategorizePropertyItems(Items))
    {
        BuildCategorizedPropertyRows(Items, bSearchMode);
    }
    else
    {
        for (UObject* ItemObject : Items)
        {
            if (UWidget* RowWidget = CreatePropertyRow(ItemObject))
            {
                if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowWidget))
                {
                    EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
                }

                if (Cast<UInspectorPropertyItem>(ItemObject))
                {
                    ++LastVisiblePropertyRowCount;
                }
                else if (Cast<UInspectorMaterialParamItem>(ItemObject))
                {
                    ++LastVisibleMaterialRowCount;
                }
            }
        }
    }

    if ((LastVisiblePropertyRowCount + LastVisibleMaterialRowCount) == 0 && !bActorTransformBlockVisible)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No visible properties match the current selection."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
    }
}

void UInspectorPropertiesSectionWidget::RefreshValueDisplayFromSubsystem()
{
    RefreshHeaderFromSubsystem();

    for (const TPair<FString, TObjectPtr<UInspectorPropertyRowWidget>>& Pair : PropertyRowsByKey)
    {
        if (Pair.Value)
        {
            Pair.Value->RefreshDisplay();
        }
    }

    for (const TPair<FString, TObjectPtr<UInspectorMaterialParamRowWidget>>& Pair : MaterialRowsByKey)
    {
        if (Pair.Value)
        {
            Pair.Value->RefreshDisplay();
        }
    }
}

bool UInspectorPropertiesSectionWidget::RefreshItemDisplay(UObject* ItemObject)
{
    if (!ItemObject)
    {
        return false;
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        if (UInspectorMaterialParamRowWidget* Row = FindMaterialRowForAutomation(MaterialItem))
        {
            Row->RefreshDisplay();
            return true;
        }
        return false;
    }

    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        if (UInspectorPropertyRowWidget* Row = FindPropertyRowForAutomation(PropertyItem))
        {
            Row->RefreshDisplay();
            LastActorTransformDebugSummary = bActorTransformBlockVisible ? LastActorTransformDebugSummary : TEXT("Hidden");
            return true;
        }
    }

    return false;
}

UInspectorMaterialParamRowWidget* UInspectorPropertiesSectionWidget::FindMaterialRowForAutomation(const UInspectorMaterialParamItem* Item) const
{
    return Item ? MaterialRowsByKey.FindRef(BuildMaterialItemKey(Item)) : nullptr;
}

UInspectorPropertyRowWidget* UInspectorPropertiesSectionWidget::FindPropertyRowForAutomation(const UInspectorPropertyItem* Item) const
{
    if (!Item)
    {
        return nullptr;
    }

    if (UInspectorPropertyRowWidget* DirectRow = PropertyRowsByKey.FindRef(BuildPropertyItemKey(Item)))
    {
        return DirectRow;
    }

    return PropertyRowsByTrackingKey.FindRef(BuildPropertyTrackingKey(Item));
}

bool UInspectorPropertiesSectionWidget::ScrollToItemForAutomation(UObject* ItemObject)
{
    if (!ScrollBox || !ItemObject)
    {
        return false;
    }

    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        const FString ItemKey = BuildPropertyItemKey(PropertyItem);
        if (const FString* CategoryStateKey = PropertyCategoryStateKeyByItemKey.Find(ItemKey))
        {
            if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
            {
                InspectorSubsystem->SetPropertyCategoryExpanded(PropertyItem->GetTargetObject(), PropertyItem->GetPrimaryCategoryName(), true);
            }
            SetCategoryExpandedVisual(*CategoryStateKey, true);
        }

        if (UInspectorPropertyRowWidget* Row = FindPropertyRowForAutomation(PropertyItem))
        {
            ScrollBox->ScrollWidgetIntoView(Row, true, EDescendantScrollDestination::Center, 0.0f);
            return true;
        }
    }
    else if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        if (UInspectorMaterialParamRowWidget* Row = FindMaterialRowForAutomation(MaterialItem))
        {
            ScrollBox->ScrollWidgetIntoView(Row, true, EDescendantScrollDestination::Center, 0.0f);
            return true;
        }
    }

    return false;
}

void UInspectorPropertiesSectionWidget::RefreshHeaderFromSubsystem()
{
    if (HeaderBorder)
    {
        HeaderBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
}
