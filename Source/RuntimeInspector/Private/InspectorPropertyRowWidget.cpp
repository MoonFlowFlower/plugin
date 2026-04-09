#include "InspectorPropertyRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorPropertyItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
    static FLinearColor RI_PropertyRowColor()
    {
        return RICompactUI::GetRowSurfaceBackgroundColor();
    }

    static FLinearColor RI_PropertyTextColor()
    {
        return RICompactUI::GetStrongTextColor();
    }

    static FLinearColor RI_PropertyMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }
}

UInspectorPropertyRowWidget::UInspectorPropertyRowWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorPropertyRowWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void UInspectorPropertyRowWidget::SetPropertyItem(UInspectorPropertyItem* InItem)
{
    PropertyItem = InItem;
    RefreshRow();
}

TSharedRef<SWidget> UInspectorPropertyRowWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorPropertyRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshRow();
}

void UInspectorPropertyRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_PropertyRowBorder"));
    RootBorder->SetPadding(FMargin(6.f, 4.f));
    RootBorder->SetBrushColor(RI_PropertyRowColor());

    RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_PropertyRowBox"));
    RootBorder->SetContent(RootBox);

    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Property"), RICompactUI::GetLabelFontSize(), true, RI_PropertyTextColor(), true);
    if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(NameText))
    {
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    ReadOnlyValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), false, RI_PropertyMutedColor(), true);
    if (UHorizontalBoxSlot* ValueSlot = RootBox->AddChildToHorizontalBox(ReadOnlyValueText))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ValueTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_PropertyRowTextBox"));
    RICompactUI::ConfigureEditableTextBox(ValueTextBox, RI_PropertyTextColor());
    ValueTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleValueCommitted);
    if (UHorizontalBoxSlot* TextBoxSlot = RootBox->AddChildToHorizontalBox(ValueTextBox))
    {
        TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    BoolCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RI_PropertyRowBool"));
    BoolCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleBoolChanged);
    if (UHorizontalBoxSlot* BoolSlot = RootBox->AddChildToHorizontalBox(BoolCheckBox))
    {
        BoolSlot->SetVerticalAlignment(VAlign_Center);
    }

    EnumComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RI_PropertyRowEnum"));
    RICompactUI::ConfigureComboBoxString(EnumComboBox, RI_PropertyTextColor());
    EnumComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleEnumChanged);
    if (UHorizontalBoxSlot* EnumSlot = RootBox->AddChildToHorizontalBox(EnumComboBox))
    {
        EnumSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        EnumSlot->SetVerticalAlignment(VAlign_Center);
    }

    WidgetTree->RootWidget = RootBorder;
}

void UInspectorPropertyRowWidget::RefreshRow()
{
    if (!WidgetTree || !RootBox)
    {
        return;
    }

    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(TEXT("Unavailable")));
        }
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::GetEmpty());
        }
        if (ValueTextBox) ValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
        if (BoolCheckBox) BoolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
        if (EnumComboBox) EnumComboBox->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(Item->GetPropertyName()));
    }

    const FString CurrentValue = Item->GetValueText();
    const bool bEditable = Item->IsEditable();
    const EInspectorValueType ValueType = Item->GetValueType();

    if (ReadOnlyValueText) ReadOnlyValueText->SetVisibility(ESlateVisibility::Collapsed);
    if (ValueTextBox) ValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
    if (BoolCheckBox) BoolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
    if (EnumComboBox) EnumComboBox->SetVisibility(ESlateVisibility::Collapsed);

    if (!bEditable)
    {
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::FromString(CurrentValue));
        }
        return;
    }

    if (ValueType == EInspectorValueType::Bool && BoolCheckBox)
    {
        const bool bChecked = CurrentValue.Equals(TEXT("True"), ESearchCase::IgnoreCase) || CurrentValue == TEXT("1");
        BoolCheckBox->SetVisibility(ESlateVisibility::Visible);
        BoolCheckBox->SetIsChecked(bChecked);
        return;
    }

    if (ValueType == EInspectorValueType::Enum && EnumComboBox)
    {
        EnumComboBox->ClearOptions();
        TArray<FString> Options;
        Item->GetEnumOptions(Options);
        for (const FString& Option : Options)
        {
            EnumComboBox->AddOption(Option);
        }
        EnumComboBox->SetVisibility(ESlateVisibility::Visible);
        if (Options.Contains(CurrentValue))
        {
            EnumComboBox->SetSelectedOption(CurrentValue);
        }
        else if (Options.Num() > 0)
        {
            EnumComboBox->SetSelectedOption(Options[0]);
        }
        return;
    }

    if (ValueTextBox)
    {
        ValueTextBox->SetVisibility(ESlateVisibility::Visible);
        ValueTextBox->SetText(FText::FromString(CurrentValue));
    }
}

bool UInspectorPropertyRowWidget::ApplyTextValue(const FString& InValue)
{
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item)
    {
        return false;
    }

    FString Error;
    const bool bApplied = Item->ApplyFromText(InValue, Error);
    if (bApplied)
    {
        RefreshRow();
    }
    return bApplied;
}

void UInspectorPropertyRowWidget::HandleValueCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::Default)
    {
        return;
    }

    ApplyTextValue(InText.ToString());
}

void UInspectorPropertyRowWidget::HandleBoolChanged(bool bIsChecked)
{
    ApplyTextValue(bIsChecked ? TEXT("True") : TEXT("False"));
}

void UInspectorPropertyRowWidget::HandleEnumChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    ApplyTextValue(SelectedItem);
}
