#include "InspectorModalBlockerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

UInspectorModalBlockerWidget::UInspectorModalBlockerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

TSharedRef<SWidget> UInspectorModalBlockerWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorModalBlockerWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RI_ModalBlockerCanvas"));
    BlockerButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_ModalBlockerButton"));
    BlockerButton->SetBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.28f));
    BlockerButton->OnClicked.AddDynamic(this, &UInspectorModalBlockerWidget::HandleBlockerClicked);

    if (UCanvasPanelSlot* BlockerSlot = RootCanvas->AddChildToCanvas(BlockerButton))
    {
        BlockerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        BlockerSlot->SetOffsets(FMargin(0.f));
    }

    WidgetTree->RootWidget = RootCanvas;
}

void UInspectorModalBlockerWidget::HandleBlockerClicked()
{
}
