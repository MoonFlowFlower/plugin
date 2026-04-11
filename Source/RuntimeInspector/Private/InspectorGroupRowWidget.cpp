#include "InspectorGroupRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorGroupItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
    static FLinearColor RI_GroupRowBackground()
    {
        return RICompactUI::GetRowSurfaceBackgroundColor();
    }
}

UInspectorGroupRowWidget::UInspectorGroupRowWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorGroupRowWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void UInspectorGroupRowWidget::SetGroupItem(UInspectorGroupItem* InItem)
{
    GroupItem = InItem;
    RefreshRow();
}

TSharedRef<SWidget> UInspectorGroupRowWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorGroupRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshRow();
}

void UInspectorGroupRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_GroupRowButton"));
    RootButton->SetClickMethod(EButtonClickMethod::MouseDown);
    RootButton->SetBackgroundColor(RI_GroupRowBackground());
    RootButton->OnClicked.AddDynamic(this, &UInspectorGroupRowWidget::HandleClicked);

    RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_GroupRowBox"));
    RootButton->AddChild(RootBox);

    ExpanderText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetMutedTextColor(), false);
    if (UHorizontalBoxSlot* ExpanderSlot = RootBox->AddChildToHorizontalBox(ExpanderText))
    {
        ExpanderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ExpanderSlot->SetVerticalAlignment(VAlign_Center);
        ExpanderSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Group"), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetStrongTextColor(), true);
    if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(NameText))
    {
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NameSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RootBox->Slot))
    {
        ButtonSlot->SetPadding(FMargin(6.f, 4.f));
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Fill);
    }

    WidgetTree->RootWidget = RootButton;
}

bool UInspectorGroupRowWidget::CanExpandItem() const
{
    const UInspectorGroupItem* Item = GroupItem.Get();
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

    return Item->TargetObject && !Item->IsMaterialSlot();
}

void UInspectorGroupRowWidget::RefreshRow()
{
    UInspectorGroupItem* Item = GroupItem.Get();
    if (!Item)
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(TEXT("Unavailable")));
        }
        return;
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(Item->DisplayName));
    }

    if (ExpanderText)
    {
        const FString Expander = CanExpandItem() ? (Item->bExpanded ? TEXT("v") : TEXT(">")) : TEXT(" ");
        ExpanderText->SetText(FText::FromString(Expander));
    }

    if (UButtonSlot* ButtonSlot = RootButton ? Cast<UButtonSlot>(RootBox->Slot) : nullptr)
    {
        const float Indent = 6.0f + static_cast<float>(FMath::Max(0, Item->Depth)) * 12.0f;
        ButtonSlot->SetPadding(FMargin(Indent, 4.f, 6.f, 4.f));
    }
}

void UInspectorGroupRowWidget::HandleClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorGroupItem* Item = GroupItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    if (CanExpandItem())
    {
        InspectorSubsystem->SetGroupExpanded(Item->StableKey, !Item->bExpanded);
    }

    InspectorSubsystem->SetSelectedGroupItem(Item);
    InspectorSubsystem->RequestActorPageRefresh();
}
