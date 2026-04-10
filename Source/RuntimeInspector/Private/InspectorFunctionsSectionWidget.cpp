#include "InspectorFunctionsSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFunctionItem.h"
#include "InspectorFunctionRowWidget.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"

namespace
{
    static FLinearColor RI_FunctionSectionColor()
    {
        return RICompactUI::GetSectionSurfaceBackgroundColor();
    }

    static FLinearColor RI_FunctionTextColor()
    {
        return RICompactUI::GetStrongTextColor();
    }

    static FLinearColor RI_FunctionMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }
}

UInspectorFunctionsSectionWidget::UInspectorFunctionsSectionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorFunctionsSectionWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

TSharedRef<SWidget> UInspectorFunctionsSectionWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorFunctionsSectionWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshFromSubsystem();
}

void UInspectorFunctionsSectionWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    FunctionsSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorFunctionsBorder"));
    FunctionsSectionBorder->SetPadding(FMargin(6.f, 5.f));
    FunctionsSectionBorder->SetBrushColor(RI_FunctionSectionColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsRoot"));
    FunctionsSectionBorder->SetContent(RootBox);

    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Functions"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    FocusSummaryText = RICompactUI::MakeText(WidgetTree, TEXT("Focused object"), RICompactUI::GetMutedFontSize(), false, RI_FunctionMutedColor(), true);
    FocusSummaryText->SetVisibility(ESlateVisibility::Collapsed);
    if (UVerticalBoxSlot* SummarySlot = RootBox->AddChildToVerticalBox(FocusSummaryText))
    {
        SummarySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorFunctionsScroll"));

    FunctionsEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsEntriesBox"));
    ScrollBox->AddChild(FunctionsEntriesBox);
    FunctionsScrollBox = ScrollBox;

    if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(ScrollBox))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
    }

    WidgetTree->RootWidget = FunctionsSectionBorder;
}

UWidget* UInspectorFunctionsSectionWidget::CreateSectionTitle(const FString& InTitle)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        RICompactUI::ERISectionVisualStyle::Emphasis);
}

UWidget* UInspectorFunctionsSectionWidget::CreateFunctionRow(UInspectorFunctionItem* Item)
{
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
    Row->SetFunctionItem(Item);
    return Row;
}

void UInspectorFunctionsSectionWidget::RefreshFromSubsystem()
{
    if (!WidgetTree || !FunctionsEntriesBox)
    {
        return;
    }

    FunctionsEntriesBox->ClearChildren();

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        UTextBlock* ErrorText = RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_FunctionMutedColor(), true);
        FunctionsEntriesBox->AddChildToVerticalBox(ErrorText);
        return;
    }

    TArray<UInspectorFunctionItem*> Items;
    InspectorSubsystem->GetFunctionItemsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), Items);

    if (Items.Num() == 0)
    {
        UTextBlock* EmptyText = RICompactUI::MakeText(
            WidgetTree,
            TEXT("No callable functions match the current selection."),
            RICompactUI::GetMutedFontSize(),
            false,
            RI_FunctionMutedColor(),
            true);
        FunctionsEntriesBox->AddChildToVerticalBox(EmptyText);
        return;
    }

    for (UInspectorFunctionItem* Item : Items)
    {
        if (!Item)
        {
            continue;
        }

        if (UWidget* RowWidget = CreateFunctionRow(Item))
        {
            if (UVerticalBoxSlot* EntrySlot = FunctionsEntriesBox->AddChildToVerticalBox(RowWidget))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }
        }
    }
}
