#include "InspectorFunctionsSectionWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFunctionItem.h"
#include "InspectorFunctionRowWidget.h"
#include "InspectorTouchScrollBox.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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

    static constexpr int32 RI_FunctionDeferredBatchSize = 3;
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

void UInspectorFunctionsSectionWidget::CancelDeferredRefresh()
{
    ++DeferredRefreshSerial;
    bDeferredRefreshPending = false;
    bDeferredRefreshCollectPending = false;
    DeferredFunctionItems.Reset();
    DeferredFunctionIndex = 0;
}

bool UInspectorFunctionsSectionWidget::FlushDeferredRefreshForAutomation(int32 MaxIterations)
{
    int32 Iterations = 0;
    while (bDeferredRefreshPending && Iterations < MaxIterations)
    {
        ProcessDeferredRefresh(DeferredRefreshSerial);
        ++Iterations;
    }

    return !bDeferredRefreshPending;
}

bool UInspectorFunctionsSectionWidget::HasTouchScrollSupportForAutomation() const
{
    return RIInspectorTouchScroll::HasTouchSupport(FunctionsScrollBox);
}

UInspectorFunctionRowWidget* UInspectorFunctionsSectionWidget::FindFunctionRowForAutomation(const UInspectorFunctionItem* Item) const
{
    if (!FunctionsEntriesBox || !Item)
    {
        return nullptr;
    }

    for (int32 ChildIndex = 0; ChildIndex < FunctionsEntriesBox->GetChildrenCount(); ++ChildIndex)
    {
        if (UInspectorFunctionRowWidget* Row = Cast<UInspectorFunctionRowWidget>(FunctionsEntriesBox->GetChildAt(ChildIndex)))
        {
            if (Row->IsDisplayingItem(Item))
            {
                return Row;
            }
        }
    }

    return nullptr;
}

bool UInspectorFunctionsSectionWidget::ScrollToItemForAutomation(UInspectorFunctionItem* Item)
{
    if (!FunctionsScrollBox || !FunctionsEntriesBox || !Item)
    {
        return false;
    }

    if (UInspectorFunctionRowWidget* Row = FindFunctionRowForAutomation(Item))
    {
        FunctionsScrollBox->ScrollWidgetIntoView(Row, true, EDescendantScrollDestination::Center, 0.0f);
        return true;
    }

    return false;
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
    RIInspectorTouchScroll::Configure(FunctionsScrollBox);
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
        RICompactUI::MakeSectionTitle(WidgetTree, TEXT("Callable Functions"), RICompactUI::ERISectionVisualStyle::Emphasis)))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    UInspectorTouchScrollBox* ScrollBox = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_ActorFunctionsScroll"));

    FunctionsEntriesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsEntriesBox"));
    ScrollBox->AddChild(FunctionsEntriesBox);
    FunctionsScrollBox = ScrollBox;
    RIInspectorTouchScroll::Configure(FunctionsScrollBox);

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

    CancelDeferredRefresh();
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

void UInspectorFunctionsSectionWidget::RefreshFromSubsystemDeferred()
{
    if (!WidgetTree || !FunctionsEntriesBox)
    {
        return;
    }

    CancelDeferredRefresh();
    RIInspectorTouchScroll::Configure(FunctionsScrollBox);
    FunctionsEntriesBox->ClearChildren();
    FunctionsEntriesBox->AddChildToVerticalBox(
        RICompactUI::MakeText(WidgetTree, TEXT("Loading functions..."), RICompactUI::GetMutedFontSize(), false, RI_FunctionMutedColor(), true));

    bDeferredRefreshPending = true;
    bDeferredRefreshCollectPending = true;
    ++DeferredRefreshSerial;
    ScheduleDeferredRefreshTick();
}

void UInspectorFunctionsSectionWidget::ScheduleDeferredRefreshTick()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        ProcessDeferredRefresh(DeferredRefreshSerial);
        return;
    }

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &UInspectorFunctionsSectionWidget::ProcessDeferredRefresh, DeferredRefreshSerial);
    World->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UInspectorFunctionsSectionWidget::ProcessDeferredRefresh(int32 Serial)
{
    if (Serial != DeferredRefreshSerial || !bDeferredRefreshPending || !WidgetTree || !FunctionsEntriesBox)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        FunctionsEntriesBox->ClearChildren();
        FunctionsEntriesBox->AddChildToVerticalBox(
            RICompactUI::MakeText(WidgetTree, TEXT("Inspector subsystem unavailable."), RICompactUI::GetMutedFontSize(), false, RI_FunctionMutedColor(), true));
        FinishDeferredRefresh();
        return;
    }

    if (bDeferredRefreshCollectPending)
    {
        FunctionsEntriesBox->ClearChildren();
        TArray<UInspectorFunctionItem*> Items;
        InspectorSubsystem->GetFunctionItemsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), Items);
        for (UInspectorFunctionItem* Item : Items)
        {
            if (Item)
            {
                DeferredFunctionItems.Add(Item);
            }
        }
        DeferredFunctionIndex = 0;
        bDeferredRefreshCollectPending = false;

        if (DeferredFunctionItems.Num() == 0)
        {
            FunctionsEntriesBox->AddChildToVerticalBox(RICompactUI::MakeText(
                WidgetTree,
                TEXT("No callable functions match the current selection."),
                RICompactUI::GetMutedFontSize(),
                false,
                RI_FunctionMutedColor(),
                true));
            FinishDeferredRefresh();
            return;
        }
    }

    if (BuildNextDeferredFunctionBatch())
    {
        FinishDeferredRefresh();
        return;
    }

    ScheduleDeferredRefreshTick();
}

bool UInspectorFunctionsSectionWidget::BuildNextDeferredFunctionBatch()
{
    int32 BuiltCount = 0;
    while (DeferredFunctionIndex < DeferredFunctionItems.Num() && BuiltCount < RI_FunctionDeferredBatchSize)
    {
        UInspectorFunctionItem* Item = DeferredFunctionItems[DeferredFunctionIndex].Get();
        ++DeferredFunctionIndex;
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
            ++BuiltCount;
        }
    }

    return DeferredFunctionIndex >= DeferredFunctionItems.Num();
}

void UInspectorFunctionsSectionWidget::FinishDeferredRefresh()
{
    bDeferredRefreshPending = false;
    bDeferredRefreshCollectPending = false;
    DeferredFunctionItems.Reset();
    DeferredFunctionIndex = 0;
}
