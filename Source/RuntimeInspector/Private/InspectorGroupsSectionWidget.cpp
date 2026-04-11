#include "InspectorGroupsSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorGroupItem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorPropertyItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"

namespace
{
    static bool RI_GroupItemCanExpand(const UInspectorGroupItem* Item)
    {
        if (!Item)
        {
            return false;
        }

        if (Item->StableKey == TEXT("ROOT_COMPONENTS"))
        {
            return true;
        }

        if (Item->IsMaterialsRoot())
        {
            return true;
        }

        return Cast<UStaticMeshComponent>(Item->TargetObject) != nullptr && !Item->IsMaterialSlot();
    }
}

void UInspectorGroupButtonProxy::Initialize(UInspectorWorldSubsystem* InSubsystem, UInspectorGroupItem* InItem)
{
    Subsystem = InSubsystem;
    Item = InItem;
}

void UInspectorGroupButtonProxy::HandleClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorGroupItem* GroupItem = Item.Get();
    if (!InspectorSubsystem || !GroupItem)
    {
        return;
    }

    if (RI_GroupItemCanExpand(GroupItem))
    {
        InspectorSubsystem->SetGroupExpanded(GroupItem->StableKey, !GroupItem->bExpanded);
    }

    InspectorSubsystem->SetSelectedGroupItem(GroupItem);
    InspectorSubsystem->RequestActorPageRefresh();
}

bool UInspectorGroupButtonProxy::MatchesStableKey(const FString& InStableKey) const
{
    const UInspectorGroupItem* GroupItem = Item.Get();
    return GroupItem && GroupItem->StableKey == InStableKey;
}

FString UInspectorGroupButtonProxy::GetStableKey() const
{
    const UInspectorGroupItem* GroupItem = Item.Get();
    return GroupItem ? GroupItem->StableKey : FString();
}

void UInspectorGroupButtonProxy::InvokeForAutomation()
{
    HandleClicked();
}

void UInspectorPinnedItemButtonProxy::Initialize(UInspectorWorldSubsystem* InSubsystem, UObject* InItem)
{
    Subsystem = InSubsystem;
    Item = InItem;
}

void UInspectorPinnedItemButtonProxy::HandleClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return;
    }

    UObject* TargetItem = Item.Get();
    if (!TargetItem)
    {
        return;
    }

    FString Error;
    if (!InspectorSubsystem->NavigateToPinnedItem(TargetItem, Error))
    {
        InspectorSubsystem->PushToast(
            ERIToastType::Warning,
            Error.IsEmpty() ? TEXT("Pinned target is no longer available.") : Error,
            2.0f);
    }
}

UInspectorGroupsSectionWidget::UInspectorGroupsSectionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorGroupsSectionWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

int32 UInspectorGroupsSectionWidget::GetEntryWidgetCountForDebug() const
{
    return LastEntryWidgetCount;
}

int32 UInspectorGroupsSectionWidget::GetPinnedEntryWidgetCountForDebug() const
{
    return LastPinnedEntryWidgetCount;
}

bool UInspectorGroupsSectionWidget::InvokeGroupItemClickForAutomation(const FString& StableKey)
{
    if (StableKey.IsEmpty())
    {
        return false;
    }

    for (UInspectorGroupButtonProxy* Proxy : ClickProxies)
    {
        if (Proxy && Proxy->MatchesStableKey(StableKey))
        {
            Proxy->InvokeForAutomation();
            return true;
        }
    }

    return false;
}

void UInspectorGroupsSectionWidget::GetVisibleGroupStableKeysForAutomation(TArray<FString>& OutKeys) const
{
    OutKeys.Reset();

    for (UInspectorGroupButtonProxy* Proxy : ClickProxies)
    {
        const FString StableKey = Proxy ? Proxy->GetStableKey() : FString();
        if (!StableKey.IsEmpty())
        {
            OutKeys.Add(StableKey);
        }
    }
}

TSharedRef<SWidget> UInspectorGroupsSectionWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorGroupsSectionWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree && WidgetTree->RootWidget && !EntriesBox)
    {
        RootSizeBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionSizeBox")));
        RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionRoot")));
        ComponentSectionBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_GroupSectionBorder")));
        ComponentScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionScroll")));
        EntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionEntries")));
        PinnedSectionBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionBorder")));
        PinnedScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionScroll")));
        PinnedEntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionEntries")));
    }
    RefreshFromSubsystem();
}

void UInspectorGroupsSectionWidget::BuildWidgetTree()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_GroupSectionSizeBox"));
    RootSizeBox->SetMinDesiredWidth(220.0f);
    RootSizeBox->SetMinDesiredHeight(420.0f);

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_GroupSectionRoot"));
    ComponentSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_GroupSectionBorder"));
    ComponentSectionBorder->SetPadding(FMargin(6.f, 5.f));
    ComponentSectionBorder->SetBrushColor(RICompactUI::GetSectionSurfaceBackgroundColor());

    UVerticalBox* ComponentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_GroupSectionComponentBox"));
    ComponentSectionBorder->SetContent(ComponentBox);

    if (UVerticalBoxSlot* HeaderSlot = ComponentBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Component"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    ComponentScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_GroupSectionScroll"));
    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_GroupSectionEntries"));
    ComponentScrollBox->AddChild(EntriesBox);
    USizeBox* ComponentBodySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_GroupSectionBodySize"));
    ComponentBodySizeBox->SetHeightOverride(180.0f);
    ComponentBodySizeBox->SetMinDesiredHeight(180.0f);
    ComponentBodySizeBox->SetContent(ComponentScrollBox);
    if (UVerticalBoxSlot* BodySlot = ComponentBox->AddChildToVerticalBox(ComponentBodySizeBox))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ComponentSectionBorder))
    {
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    PinnedSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_PinnedSectionBorder"));
    PinnedSectionBorder->SetPadding(FMargin(6.f, 5.f));
    PinnedSectionBorder->SetBrushColor(RICompactUI::GetSectionSurfaceBackgroundColor());

    UVerticalBox* PinnedBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PinnedSectionBox"));
    PinnedSectionBorder->SetContent(PinnedBox);

    if (UVerticalBoxSlot* HeaderSlot = PinnedBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Star"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    PinnedScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_PinnedSectionScroll"));
    PinnedEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PinnedSectionEntries"));
    PinnedScrollBox->AddChild(PinnedEntriesBox);
    USizeBox* PinnedBodySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_PinnedSectionBodySize"));
    PinnedBodySizeBox->SetHeightOverride(120.0f);
    PinnedBodySizeBox->SetMinDesiredHeight(120.0f);
    PinnedBodySizeBox->SetContent(PinnedScrollBox);
    if (UVerticalBoxSlot* BodySlot = PinnedBox->AddChildToVerticalBox(PinnedBodySizeBox))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    if (UVerticalBoxSlot* PinnedSlot = RootBox->AddChildToVerticalBox(PinnedSectionBorder))
    {
        PinnedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    RootSizeBox->SetContent(RootBox);
    WidgetTree->RootWidget = RootSizeBox;
}

void UInspectorGroupsSectionWidget::AppendItemRecursive(UInspectorGroupItem* Item, const FString& SearchText, TArray<UInspectorGroupItem*>& OutItems)
{
    if (!Item)
    {
        return;
    }

    OutItems.Add(Item);
    if (!Item->bExpanded)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return;
    }

    TArray<UObject*> ChildObjects;
    InspectorSubsystem->GetGroupTreeChildrenForItem(Item, SearchText, ChildObjects);
    for (UObject* ChildObject : ChildObjects)
    {
        if (UInspectorGroupItem* ChildItem = Cast<UInspectorGroupItem>(ChildObject))
        {
            AppendItemRecursive(ChildItem, SearchText, OutItems);
        }
    }
}

UWidget* UInspectorGroupsSectionWidget::CreatePinnedRow(UObject* ItemObject)
{
    if (!WidgetTree || !ItemObject)
    {
        return nullptr;
    }

    FString Label = TEXT("Pinned");
    FString Value;
    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        Label = PropertyItem->GetPropertyName();
        Value = PropertyItem->GetValueText();
    }
    else if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        Label = MaterialItem->GetPropertyName();
        Value = MaterialItem->GetValueText();
    }
    else
    {
        return nullptr;
    }

    UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    if (!RowButton)
    {
        return nullptr;
    }

    RowButton->SetClickMethod(EButtonClickMethod::MouseDown);
    RowButton->SetBackgroundColor(RICompactUI::GetRowSurfaceBackgroundColor());

    UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!RowBox)
    {
        return nullptr;
    }
    RowButton->AddChild(RowBox);

    UTextBlock* StarText = RICompactUI::MakeText(WidgetTree, TEXT("*"), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetWarningTextColor(), false);
    if (UHorizontalBoxSlot* StarSlot = RowBox->AddChildToHorizontalBox(StarText))
    {
        StarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        StarSlot->SetVerticalAlignment(VAlign_Top);
        StarSlot->SetPadding(FMargin(0.f, 1.f, 6.f, 0.f));
    }

    UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UHorizontalBoxSlot* TextSlot = RowBox->AddChildToHorizontalBox(TextBox))
    {
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* LabelSlot = TextBox->AddChildToVerticalBox(
        RICompactUI::MakeText(WidgetTree, Label, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetStrongTextColor(), true)))
    {
        LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    if (!Value.IsEmpty())
    {
        TextBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, Value, RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor(), true));
    }

    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RowBox->Slot))
    {
        ButtonSlot->SetPadding(FMargin(8.f, 4.f, 6.f, 4.f));
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Fill);
    }

    UInspectorPinnedItemButtonProxy* Proxy = NewObject<UInspectorPinnedItemButtonProxy>(this);
    Proxy->Initialize(Subsystem.Get(), ItemObject);
    RowButton->OnClicked.AddDynamic(Proxy, &UInspectorPinnedItemButtonProxy::HandleClicked);
    PinnedClickProxies.Add(Proxy);
    return RowButton;
}

void UInspectorGroupsSectionWidget::RefreshFromSubsystem()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    if (WidgetTree && WidgetTree->RootWidget && !EntriesBox)
    {
        RootSizeBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionSizeBox")));
        RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionRoot")));
        ComponentSectionBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_GroupSectionBorder")));
        ComponentScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionScroll")));
        EntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionEntries")));
        PinnedSectionBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionBorder")));
        PinnedScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionScroll")));
        PinnedEntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_PinnedSectionEntries")));
    }

    if (!EntriesBox || !PinnedEntriesBox || !WidgetTree)
    {
        return;
    }

    EntriesBox->ClearChildren();
    PinnedEntriesBox->ClearChildren();
    ClickProxies.Reset();
    PinnedClickProxies.Reset();
    LastEntryWidgetCount = 0;
    LastPinnedEntryWidgetCount = 0;

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return;
    }

    const FString SearchText = InspectorSubsystem->GetCurrentActorSearchText();

    TArray<UObject*> RootObjects;
    InspectorSubsystem->GetGroupTreeRootsForSelected(SearchText, RootObjects);

    TArray<UInspectorGroupItem*> FlatItems;
    for (UObject* RootObject : RootObjects)
    {
        if (UInspectorGroupItem* RootItem = Cast<UInspectorGroupItem>(RootObject))
        {
            if (RootItem->StableKey == TEXT("PINNED_ROOT"))
            {
                continue;
            }
            AppendItemRecursive(RootItem, SearchText, FlatItems);
        }
    }

    if (FlatItems.Num() == 0)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No components available."), RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor(), true));
    }

    for (UInspectorGroupItem* Item : FlatItems)
    {
        if (!Item)
        {
            continue;
        }

        UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        if (!RowButton || !RowBox)
        {
            continue;
        }

        RowButton->SetClickMethod(EButtonClickMethod::MouseDown);
        RowButton->SetBackgroundColor(RICompactUI::GetRowSurfaceBackgroundColor());
        RowButton->AddChild(RowBox);

        const FString Expander = RI_GroupItemCanExpand(Item) ? (Item->bExpanded ? TEXT("v") : TEXT(">")) : TEXT(" ");
        UTextBlock* ExpanderText = RICompactUI::MakeText(WidgetTree, Expander, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetMutedTextColor(), false);
        if (UHorizontalBoxSlot* ExpanderSlot = RowBox->AddChildToHorizontalBox(ExpanderText))
        {
            ExpanderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            ExpanderSlot->SetVerticalAlignment(VAlign_Center);
            ExpanderSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
        }

        UTextBlock* NameText = RICompactUI::MakeText(WidgetTree, Item->DisplayName, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetStrongTextColor(), true);
        if (UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(NameText))
        {
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NameSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RowBox->Slot))
        {
            const float Indent = 6.0f + static_cast<float>(FMath::Max(0, Item->Depth)) * 12.0f;
            ButtonSlot->SetPadding(FMargin(Indent, 4.f, 6.f, 4.f));
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
            ButtonSlot->SetVerticalAlignment(VAlign_Fill);
        }

        UInspectorGroupButtonProxy* Proxy = NewObject<UInspectorGroupButtonProxy>(this);
        Proxy->Initialize(InspectorSubsystem, Item);
        RowButton->OnClicked.AddDynamic(Proxy, &UInspectorGroupButtonProxy::HandleClicked);
        ClickProxies.Add(Proxy);

        if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowButton))
        {
            EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
            ++LastEntryWidgetCount;
        }
    }

    TArray<UObject*> PinnedItems;
    InspectorSubsystem->GetPinnedItemsForSelected(SearchText, PinnedItems);

    if (PinnedItems.Num() == 0)
    {
        PinnedEntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No starred properties yet."), RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor(), true));
    }
    else
    {
        for (UObject* PinnedItem : PinnedItems)
        {
            if (UWidget* PinnedRow = CreatePinnedRow(PinnedItem))
            {
                if (UVerticalBoxSlot* EntrySlot = PinnedEntriesBox->AddChildToVerticalBox(PinnedRow))
                {
                    EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
                    ++LastPinnedEntryWidgetCount;
                }
            }
        }
    }

    if (ComponentScrollBox)
    {
        ComponentScrollBox->SetScrollOffset(0.0f);
        if (EntriesBox->GetChildrenCount() > 0)
        {
            ComponentScrollBox->ScrollWidgetIntoView(EntriesBox->GetChildAt(0), true, EDescendantScrollDestination::TopOrLeft, 0.0f);
        }
    }

    if (PinnedScrollBox)
    {
        PinnedScrollBox->SetScrollOffset(0.0f);
        if (PinnedEntriesBox->GetChildrenCount() > 0)
        {
            PinnedScrollBox->ScrollWidgetIntoView(PinnedEntriesBox->GetChildAt(0), true, EDescendantScrollDestination::TopOrLeft, 0.0f);
        }
    }

    EntriesBox->InvalidateLayoutAndVolatility();
    PinnedEntriesBox->InvalidateLayoutAndVolatility();
    if (ComponentScrollBox)
    {
        ComponentScrollBox->InvalidateLayoutAndVolatility();
    }
    if (PinnedScrollBox)
    {
        PinnedScrollBox->InvalidateLayoutAndVolatility();
    }
    InvalidateLayoutAndVolatility();
    ForceLayoutPrepass();
}
