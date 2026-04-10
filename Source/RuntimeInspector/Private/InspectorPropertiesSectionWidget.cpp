#include "InspectorPropertiesSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
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

    static FLinearColor RI_PropertySectionColor()
    {
        return RICompactUI::GetSectionSurfaceBackgroundColor();
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

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorPropertiesBorder"));
    RootBorder->SetPadding(FMargin(6.f, 5.f));
    RootBorder->SetBrushColor(RI_PropertySectionColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesRoot"));
    RootBorder->SetContent(RootBox);

    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Property"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorPropertiesScroll"));
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorPropertiesSize"));
    SizeBox->SetMaxDesiredHeight(320.f);
    SizeBox->SetContent(ScrollBox);

    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesEntries"));
    ScrollBox->AddChild(EntriesBox);

    RootBox->AddChildToVerticalBox(SizeBox);
    WidgetTree->RootWidget = RootBorder;
}

UWidget* UInspectorPropertiesSectionWidget::CreatePropertyRow(UObject* ItemObject)
{
    if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ItemObject))
    {
        return RICompactUI::MakeSectionTitle(WidgetTree, GroupItem->DisplayName, RICompactUI::ERISectionVisualStyle::Emphasis);
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

    if (AddedRows == 0)
    {
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("No visible properties match the current selection."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
    }
}
