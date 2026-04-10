#include "InspectorFunctionRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFunctionItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    static FLinearColor RI_FunctionRowColor()
    {
        return RICompactUI::GetRowSurfaceBackgroundColor();
    }

    static FLinearColor RI_FunctionRowTextColor()
    {
        return RICompactUI::GetStrongTextColor();
    }

    static FLinearColor RI_FunctionRowMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }
}

UInspectorFunctionRowWidget::UInspectorFunctionRowWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorFunctionRowWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void UInspectorFunctionRowWidget::SetFunctionItem(UInspectorFunctionItem* InItem)
{
    FunctionItem = InItem;
    RefreshRow();
}

TSharedRef<SWidget> UInspectorFunctionRowWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorFunctionRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshRow();
}

void UInspectorFunctionRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_FunctionRowBorder"));
    RootBorder->SetPadding(FMargin(6.f, 5.f));
    RootBorder->SetBrushColor(RI_FunctionRowColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FunctionRowBox"));
    RootBorder->SetContent(RootBox);

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_FunctionRowHeader"));
    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(HeaderRow))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    TitleText = RICompactUI::MakeText(WidgetTree, TEXT("Function"), RICompactUI::GetLabelFontSize(), true, RI_FunctionRowTextColor(), true);
    if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText))
    {
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TitleSlot->SetVerticalAlignment(VAlign_Center);
    }

    OwnerText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetMutedFontSize(), false, RI_FunctionRowMutedColor(), true);
    OwnerText->SetVisibility(ESlateVisibility::Collapsed);
    if (UHorizontalBoxSlot* OwnerSlot = HeaderRow->AddChildToHorizontalBox(OwnerText))
    {
        OwnerSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f));
        OwnerSlot->SetVerticalAlignment(VAlign_Center);
    }

    InvokeButton = RICompactUI::MakeLabeledButton(WidgetTree, TEXT("BTN_InvokeFunction"), TEXT("Run"), RICompactUI::ERIButtonVisualStyle::Primary, 44.f);
    InvokeButton->OnClicked.AddDynamic(this, &UInspectorFunctionRowWidget::HandleInvokeClicked);
    if (UHorizontalBoxSlot* ButtonSlot = HeaderRow->AddChildToHorizontalBox(InvokeButton))
    {
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    ParametersBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FunctionRowParameters"));
    RootBox->AddChildToVerticalBox(ParametersBox);

    WidgetTree->RootWidget = RootBorder;
}

void UInspectorFunctionRowWidget::ClearParameterWidgets()
{
    ParameterWidgets.Reset();
    if (ParametersBox)
    {
        ParametersBox->ClearChildren();
    }
}

void UInspectorFunctionRowWidget::RefreshRow()
{
    if (!WidgetTree || !RootBox || !ParametersBox)
    {
        return;
    }

    ClearParameterWidgets();

    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        if (TitleText)
        {
            TitleText->SetText(FText::FromString(TEXT("Unavailable")));
        }
        if (OwnerText)
        {
            OwnerText->SetText(FText::FromString(TEXT("")));
        }
        if (InvokeButton)
        {
            InvokeButton->SetIsEnabled(false);
        }
        return;
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(Item->GetDisplayName()));
    }

    if (OwnerText)
    {
        OwnerText->SetText(FText::GetEmpty());
        OwnerText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (InvokeButton)
    {
        InvokeButton->SetIsEnabled(true);
    }

    const TArray<FRIFunctionParameterSpec>& Params = Item->GetParameterSpecs();
    for (const FRIFunctionParameterSpec& Param : Params)
    {
        UBorder* ParamBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        ParamBorder->SetPadding(FMargin(4.f, 3.f));
        ParamBorder->SetBrushColor(RICompactUI::GetCellSurfaceBackgroundColor());

        UHorizontalBox* ParamRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        ParamBorder->SetContent(ParamRow);

        const FString LabelText = FString::Printf(TEXT("%s (%s)"), *Param.DisplayName, *Param.TypeLabel);
        UTextBlock* Label = RICompactUI::MakeText(WidgetTree, LabelText, RICompactUI::GetMutedFontSize(), true, RI_FunctionRowMutedColor(), true);
        if (UHorizontalBoxSlot* LabelSlot = ParamRow->AddChildToHorizontalBox(Label))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
        }

        UWidget* Control = nullptr;
        if (Param.bIsEnum && Param.EnumOptions.Num() > 0)
        {
            UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
            for (const FString& Opt : Param.EnumOptions)
            {
                Combo->AddOption(Opt);
            }
            if (!Param.DefaultText.IsEmpty())
            {
                Combo->SetSelectedOption(Param.DefaultText);
            }
            else if (Param.EnumOptions.Num() > 0)
            {
                Combo->SetSelectedOption(Param.EnumOptions[0]);
            }
            Control = Combo;
        }
        else if (Param.TypeLabel.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
        {
            UCheckBox* CheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
            const bool bChecked = Param.DefaultText.Equals(TEXT("True"), ESearchCase::IgnoreCase) || Param.DefaultText == TEXT("1");
            CheckBox->SetIsChecked(bChecked);
            Control = CheckBox;
        }
        else
        {
            UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
            TextBox->SetText(FText::FromString(Param.DefaultText));
            Control = TextBox;
        }

        if (Control)
        {
            if (UHorizontalBoxSlot* ControlSlot = ParamRow->AddChildToHorizontalBox(Control))
            {
                ControlSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
                ControlSlot->SetHorizontalAlignment(HAlign_Fill);
                ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }
            ParameterWidgets.Add(Control);
        }

        if (UVerticalBoxSlot* VBoxSlot = ParametersBox->AddChildToVerticalBox(ParamBorder))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    ParametersBox->SetVisibility(ParameterWidgets.Num() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

TArray<FString> UInspectorFunctionRowWidget::CollectArgumentTexts() const
{
    TArray<FString> Args;
    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!Item)
    {
        return Args;
    }

    const TArray<FRIFunctionParameterSpec>& Params = Item->GetParameterSpecs();
    Args.Reserve(Params.Num());

    for (int32 Index = 0; Index < Params.Num(); ++Index)
    {
        const FRIFunctionParameterSpec& Param = Params[Index];
        const UWidget* Control = ParameterWidgets.IsValidIndex(Index) ? ParameterWidgets[Index].Get() : nullptr;
        if (!Control)
        {
            Args.Add(Param.DefaultText);
            continue;
        }

        if (const UCheckBox* CheckBox = Cast<UCheckBox>(Control))
        {
            Args.Add(CheckBox->IsChecked() ? TEXT("True") : TEXT("False"));
        }
        else if (const UComboBoxString* Combo = Cast<UComboBoxString>(Control))
        {
            Args.Add(Combo->GetSelectedOption());
        }
        else if (const UEditableTextBox* TextBox = Cast<UEditableTextBox>(Control))
        {
            Args.Add(TextBox->GetText().ToString());
        }
        else
        {
            Args.Add(Param.DefaultText);
        }
    }

    return Args;
}

bool UInspectorFunctionRowWidget::InvokeForAutomation(FString& OutError)
{
    OutError.Reset();

    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        OutError = TEXT("Function item invalid");
        return false;
    }

    const TArray<FString> Args = CollectArgumentTexts();
    if (!Item->Invoke(Args, OutError))
    {
        if (UInspectorWorldSubsystem* Sub = Subsystem.Get())
        {
            Sub->PushToast(ERIToastType::Error, OutError.IsEmpty() ? TEXT("Function call failed") : OutError, 2.0f);
        }
        return false;
    }

    if (UInspectorWorldSubsystem* Sub = Subsystem.Get())
    {
        Sub->PushToast(ERIToastType::Success, FString::Printf(TEXT("Invoked %s"), *Item->GetQualifiedDisplayName()), 1.5f);
    }

    return true;
}

FString UInspectorFunctionRowWidget::GetFunctionTitleForAutomation() const
{
    if (const UInspectorFunctionItem* Item = FunctionItem.Get())
    {
        return Item->GetQualifiedDisplayName();
    }

    return FString();
}

void UInspectorFunctionRowWidget::HandleInvokeClicked()
{
    FString Error;
    if (!InvokeForAutomation(Error))
    {
        return;
    }
}
