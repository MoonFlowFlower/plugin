#include "InspectorGroupsSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorGroupItem.h"
#include "InspectorGroupRowWidget.h"
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

    UObject* FocusTarget = nullptr;
    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(Item.Get()))
    {
        FocusTarget = PropertyItem->GetTargetObject();
    }
    else if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(Item.Get()))
    {
        FocusTarget = MaterialItem->GetMeshComponent();
    }

    if (!FocusTarget)
    {
        return;
    }

    if (UActorComponent* Component = Cast<UActorComponent>(FocusTarget))
    {
        FString Error;
        InspectorSubsystem->FocusSelectedActorComponentByName(Component->GetName(), Error);
        return;
    }

    if (AActor* Actor = Cast<AActor>(FocusTarget))
    {
        InspectorSubsystem->SetSelectedActor(Actor);
        return;
    }

    InspectorSubsystem->RequestActorPageRefresh();
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
        ScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionScroll")));
        EntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionEntries")));
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
    ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_GroupSectionScroll"));
    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_GroupSectionEntries"));
    ScrollBox->AddChild(EntriesBox);
    if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ScrollBox))
    {
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
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
        ScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionScroll")));
        EntriesBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("RI_GroupSectionEntries")));
    }

    if (!EntriesBox || !WidgetTree)
    {
        return;
    }

    EntriesBox->ClearChildren();
    ClickProxies.Reset();
    PinnedClickProxies.Reset();
    LastEntryWidgetCount = 0;

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

    if (UVerticalBoxSlot* HeaderSlot = EntriesBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Component"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
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

        UInspectorGroupRowWidget* RowWidget = nullptr;
        if (APlayerController* PC = GetOwningPlayer())
        {
            RowWidget = CreateWidget<UInspectorGroupRowWidget>(PC, UInspectorGroupRowWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            RowWidget = CreateWidget<UInspectorGroupRowWidget>(World, UInspectorGroupRowWidget::StaticClass());
        }
        if (!RowWidget)
        {
            continue;
        }

        RowWidget->SetInspectorSubsystem(InspectorSubsystem);
        RowWidget->SetGroupItem(Item);

        if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowWidget))
        {
            EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
            ++LastEntryWidgetCount;
        }
    }

    TArray<UObject*> PinnedItems;
    InspectorSubsystem->GetPinnedItemsForSelected(SearchText, PinnedItems);

    if (UVerticalBoxSlot* HeaderSlot = EntriesBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Star"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, FlatItems.Num() > 0 ? 8.f : 6.f, 0.f, 4.f));
    }

    if (PinnedItems.Num() == 0)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No starred properties yet."), RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor(), true));
    }
    else
    {
        for (UObject* PinnedItem : PinnedItems)
        {
            if (UWidget* PinnedRow = CreatePinnedRow(PinnedItem))
            {
                if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(PinnedRow))
                {
                    EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
                    ++LastEntryWidgetCount;
                }
            }
        }
    }

    if (ScrollBox)
    {
        ScrollBox->SetScrollOffset(0.0f);
        if (EntriesBox->GetChildrenCount() > 0)
        {
            ScrollBox->ScrollWidgetIntoView(EntriesBox->GetChildAt(0), true, EDescendantScrollDestination::TopOrLeft, 0.0f);
        }
    }

    EntriesBox->InvalidateLayoutAndVolatility();
    ScrollBox->InvalidateLayoutAndVolatility();
    InvalidateLayoutAndVolatility();
    ForceLayoutPrepass();
}
