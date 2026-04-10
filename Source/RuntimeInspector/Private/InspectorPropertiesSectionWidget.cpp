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
#include "GameFramework/Actor.h"

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

    static UWidget* RI_CreateHeaderInfoCell(
        UWidgetTree* WidgetTree,
        const FString& Label,
        TObjectPtr<UTextBlock>& OutValueText,
        const FLinearColor& BackgroundColor)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Border->SetPadding(FMargin(6.f, 4.f));
        Border->SetBrushColor(BackgroundColor);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Border->SetContent(Box);

        if (UVerticalBoxSlot* LabelSlot = Box->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, Label, RICompactUI::GetMutedFontSize(), true, RICompactUI::GetMutedTextColor())))
        {
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }

        OutValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), true, RICompactUI::GetStrongTextColor(), true);
        Box->AddChildToVerticalBox(OutValueText);
        return Border;
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

    HeaderBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_InspectHeaderBorder"));
    HeaderBorder->SetPadding(FMargin(7.f, 6.f));
    HeaderBorder->SetBrushColor(RICompactUI::GetContextStripBackgroundColor());

    UVerticalBox* HeaderRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_InspectHeaderRoot"));
    HeaderBorder->SetContent(HeaderRoot);

    if (UVerticalBoxSlot* HeaderTitleSlot = HeaderRoot->AddChildToVerticalBox(
        RICompactUI::MakeText(WidgetTree, TEXT("Selection"), RICompactUI::GetMutedFontSize(), true, RICompactUI::GetMutedTextColor())))
    {
        HeaderTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    HeaderActorText = RICompactUI::MakeText(WidgetTree, TEXT("No selected actor"), 10, true, RICompactUI::GetStrongTextColor(), true);
    if (UVerticalBoxSlot* ActorSlot = HeaderRoot->AddChildToVerticalBox(HeaderActorText))
    {
        ActorSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UHorizontalBox* HeaderInfoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_InspectHeaderInfoRow"));
    if (UVerticalBoxSlot* InfoRowSlot = HeaderRoot->AddChildToVerticalBox(HeaderInfoRow))
    {
        InfoRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
    }

    auto AddHeaderCell = [HeaderInfoRow](UWidget* Cell, float FillWeight)
    {
        if (!HeaderInfoRow || !Cell)
        {
            return;
        }

        if (UHorizontalBoxSlot* Slot = HeaderInfoRow->AddChildToHorizontalBox(Cell))
        {
            FSlateChildSize SizeRule(ESlateSizeRule::Fill);
            SizeRule.Value = FillWeight;
            Slot->SetSize(SizeRule);
            Slot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
    };

    AddHeaderCell(
        RI_CreateHeaderInfoCell(
            WidgetTree,
            TEXT("Focus"),
            HeaderFocusText,
            RICompactUI::GetContextPrimaryCellBackgroundColor()),
        1.0f);
    AddHeaderCell(
        RI_CreateHeaderInfoCell(
            WidgetTree,
            TEXT("Source Asset"),
            HeaderSourceText,
            RICompactUI::GetContextSecondaryCellBackgroundColor()),
        1.4f);
    AddHeaderCell(
        RI_CreateHeaderInfoCell(
            WidgetTree,
            TEXT("Snapshot"),
            HeaderStatusText,
            RICompactUI::GetContextStatusCellBackgroundColor()),
        0.9f);

    HeaderBorder->SetVisibility(ESlateVisibility::Collapsed);

    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Property"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorPropertiesScroll"));
    EntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertiesEntries"));
    ScrollBox->AddChild(EntriesBox);

    if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(ScrollBox))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
    }
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
        RefreshHeaderFromSubsystem();
        EntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_PropertySectionMutedColor(), true));
        return;
    }

    RefreshHeaderFromSubsystem();

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

void UInspectorPropertiesSectionWidget::RefreshHeaderFromSubsystem()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();

    const AActor* SelectedActor = InspectorSubsystem ? InspectorSubsystem->GetSelectedActor() : nullptr;
    const UObject* FocusedObject = InspectorSubsystem ? InspectorSubsystem->GetFocusedInspectObject() : nullptr;

    const FString ActorLabel = SelectedActor ? SelectedActor->GetActorLabel() : TEXT("No selected actor");
    const FString FocusLabel = FocusedObject
        ? FocusedObject->GetName()
        : TEXT("Actor");
    const FString SourcePath = (SelectedActor && SelectedActor->GetClass())
        ? SelectedActor->GetClass()->GetPathName()
        : TEXT("No source asset");
    const FString SnapshotLabel = (InspectorSubsystem && InspectorSubsystem->HasStagedPatch())
        ? FString::Printf(TEXT("Staged (%d ops)"), InspectorSubsystem->GetStagedPatch().Operations.Num())
        : TEXT("No staged patch");

    if (HeaderActorText)
    {
        HeaderActorText->SetText(FText::FromString(ActorLabel));
        HeaderActorText->SetColorAndOpacity(SelectedActor ? RICompactUI::GetStrongTextColor() : RICompactUI::GetMutedTextColor());
    }

    auto ApplyHeaderValue = [](UTextBlock* TextWidget, const FString& Value, const FLinearColor& Color)
    {
        if (!TextWidget)
        {
            return;
        }

        TextWidget->SetText(FText::FromString(Value));
        TextWidget->SetColorAndOpacity(Color);
    };

    ApplyHeaderValue(HeaderFocusText, FocusLabel, RICompactUI::GetStrongTextColor());
    ApplyHeaderValue(HeaderSourceText, SourcePath, SelectedActor ? RICompactUI::GetSecondaryTextColor() : RICompactUI::GetMutedTextColor());
    ApplyHeaderValue(
        HeaderStatusText,
        SnapshotLabel,
        (InspectorSubsystem && InspectorSubsystem->HasStagedPatch()) ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetStrongTextColor());
}
