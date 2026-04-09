#include "InspectorPropertiesSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFunctionItem.h"
#include "InspectorFunctionRowWidget.h"
#include "InspectorGroupItem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorPropertyItem.h"
#include "InspectorPropertyRowWidget.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    static FLinearColor RI_PropertySectionMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }

    static UWidget* RI_CreateReadOnlyRow(UWidgetTree* WidgetTree, const FString& Name, const FString& Value)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Border->SetPadding(FMargin(6.f, 4.f));
        Border->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());

        UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        Border->SetContent(RowBox);

        UTextBlock* NameText = RICompactUI::MakeText(WidgetTree, Name, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetStrongTextColor(), true);
        if (UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(NameText))
        {
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NameSlot->SetVerticalAlignment(VAlign_Center);
            NameSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        }

        UTextBlock* ValueText = RICompactUI::MakeText(WidgetTree, Value, RICompactUI::GetValueFontSize(), false, RICompactUI::GetMutedTextColor(), true);
        if (UHorizontalBoxSlot* ValueSlot = RowBox->AddChildToHorizontalBox(ValueText))
        {
            ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            ValueSlot->SetVerticalAlignment(VAlign_Center);
        }

        return Border;
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
    RefreshFromSubsystem();
}

void UInspectorPropertiesSectionWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesRoot"));

    ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorPropertiesScroll"));
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorPropertiesSize"));
    SizeBox->SetHeightOverride(240.f);
    SizeBox->SetMaxDesiredHeight(240.f);
    SizeBox->SetContent(ScrollBox);

    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesEntries"));
    ScrollBox->AddChild(EntriesBox);

    RootBox->AddChildToVerticalBox(SizeBox);
    WidgetTree->RootWidget = RootBox;
}

UWidget* UInspectorPropertiesSectionWidget::CreatePropertyRow(UObject* ItemObject)
{
    if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ItemObject))
    {
        return CreateSectionTitle(GroupItem->DisplayName);
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        return RI_CreateReadOnlyRow(WidgetTree, MaterialItem->GetPropertyName(), MaterialItem->GetValueText());
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
    Row->SetPropertyItem(PropertyItem);
    return Row;
}

UWidget* UInspectorPropertiesSectionWidget::CreateFunctionRow(UInspectorFunctionItem* ItemObject)
{
    if (!ItemObject)
    {
        return nullptr;
    }

    UInspectorFunctionRowWidget* Row = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        Row = CreateWidget<UInspectorFunctionRowWidget>(PC, UInspectorFunctionRowWidget::StaticClass());
    }
    else if (UWorld* World = GetWorld())
    {
        Row = CreateWidget<UInspectorFunctionRowWidget>(World, UInspectorFunctionRowWidget::StaticClass());
    }

    if (!Row)
    {
        return nullptr;
    }

    Row->SetInspectorSubsystem(Subsystem.Get());
    Row->SetFunctionItem(ItemObject);
    return Row;
}

UWidget* UInspectorPropertiesSectionWidget::CreateSectionTitle(const FString& InTitle)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        RICompactUI::ERISectionVisualStyle::Emphasis);
}

void UInspectorPropertiesSectionWidget::RefreshFromSubsystem()
{
    if (!EntriesBox || !WidgetTree)
    {
        return;
    }

    EntriesBox->ClearChildren();

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
        return;
    }

    TArray<UObject*> Items;
    InspectorSubsystem->GetPropertyItemsForSelectedEx(InspectorSubsystem->GetCurrentActorSearchText(), bOnlyModified, Items);

    int32 AddedRows = 0;
    if (Items.Num() > 0)
    {
        if (UVerticalBoxSlot* TitleSlot = EntriesBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Properties"))))
        {
            TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    for (UObject* ItemObject : Items)
    {
        if (UWidget* RowWidget = CreatePropertyRow(ItemObject))
        {
            if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowWidget))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }
            ++AddedRows;
        }
    }

    TArray<UInspectorFunctionItem*> FunctionItems;
    InspectorSubsystem->GetFunctionItemsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), FunctionItems);
    if (FunctionItems.Num() > 0)
    {
        if (UVerticalBoxSlot* TitleSlot = EntriesBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Functions"))))
        {
            TitleSlot->SetPadding(FMargin(0.f, AddedRows > 0 ? 8.f : 0.f, 0.f, 4.f));
        }

        for (UInspectorFunctionItem* FunctionItem : FunctionItems)
        {
            if (UWidget* RowWidget = CreateFunctionRow(FunctionItem))
            {
                if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowWidget))
                {
                    EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
                }
            }
        }
    }

    if (FunctionItems.Num() == 0 && AddedRows == 0)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No visible properties or callable functions match the current filter."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
    }
}
