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

int32 UInspectorFunctionsSectionWidget::GetEntryWidgetCountForAutomation() const
{
    return FunctionsEntriesBox ? FunctionsEntriesBox->GetChildrenCount() : INDEX_NONE;
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
    FunctionsSectionBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
    FunctionsSectionBorder->SetBrushColor(RI_FunctionSectionColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsRoot"));
    FunctionsSectionBorder->SetContent(RootBox);

    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Function"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    FocusSummaryText = RICompactUI::MakeText(
        WidgetTree,
        TEXT("Focused target: Actor root"),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_FunctionMutedColor(),
        true);
    if (UVerticalBoxSlot* SummarySlot = RootBox->AddChildToVerticalBox(FocusSummaryText))
    {
        SummarySlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorFunctionsScroll"));

    FunctionsEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsEntriesBox"));
    ScrollBox->AddChild(FunctionsEntriesBox);
    FunctionsScrollBox = ScrollBox;

    USizeBox* BodySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorFunctionsBody"));
    BodySizeBox->SetMinDesiredHeight(220.f);
    BodySizeBox->ClearMaxDesiredHeight();

    UBorder* BodyBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorFunctionsBodyBorder"));
    BodyBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
    BodyBorder->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());
    BodyBorder->SetContent(ScrollBox);
    BodySizeBox->SetContent(BodyBorder);

    if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(BodySizeBox))
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
    Row->TakeWidget();
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

    const UObject* FocusedObject = InspectorSubsystem->GetFocusedInspectObject();
    if (FocusSummaryText)
    {
        const FString FocusSummary = FocusedObject
            ? FString::Printf(TEXT("Focused target: %s"), *FocusedObject->GetName())
            : TEXT("Focused target: Actor root");
        FocusSummaryText->SetText(FText::FromString(FocusSummary));
    }

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
