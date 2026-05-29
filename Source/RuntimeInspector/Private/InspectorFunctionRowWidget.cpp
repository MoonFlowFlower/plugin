#include "InspectorFunctionRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFunctionItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
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

    static FLinearColor RI_FunctionFavoriteActiveColor()
    {
        return RICompactUI::GetWarningTextColor();
    }

    static constexpr float RI_FunctionFavoriteButtonSize = 16.0f;
    static constexpr float RI_FunctionFavoriteIconSize = 10.5f;
    static constexpr float RI_FunctionRunButtonWidth = 42.0f;
    static constexpr float RI_FunctionRunButtonHeight = 18.0f;
    static constexpr float RI_FunctionParameterWidth = 88.0f;
    static constexpr float RI_FunctionParameterCheckSize = 16.0f;
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

void UInspectorFunctionRowWidget::SetAllowNavigation(bool bInAllowNavigation)
{
    bAllowNavigation = bInAllowNavigation;
    if (TitleButton)
    {
        TitleButton->SetIsEnabled(bAllowNavigation);
    }
}

bool UInspectorFunctionRowWidget::IsDisplayingItem(const UInspectorFunctionItem* InItem) const
{
    const UInspectorFunctionItem* CurrentItem = FunctionItem.Get();
    if (!CurrentItem || !InItem)
    {
        return false;
    }

    return CurrentItem == InItem
        || (CurrentItem->GetTargetObject() == InItem->GetTargetObject()
            && CurrentItem->GetFunctionFName() == InItem->GetFunctionFName());
}

void UInspectorFunctionRowWidget::RefreshDisplay()
{
    RefreshRow();
}

float UInspectorFunctionRowWidget::GetFavoriteButtonHeightForAutomation() const
{
    return FavoriteSizeBox ? FavoriteSizeBox->GetHeightOverride() : 0.f;
}

float UInspectorFunctionRowWidget::GetParameterInputHeightForAutomation() const
{
    const USizeBox* SizeBox = PrimaryParameterSizeBox.Get();
    return SizeBox ? SizeBox->GetHeightOverride() : 0.f;
}

bool UInspectorFunctionRowWidget::NavigateForAutomation(FString& OutError)
{
    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        OutError = TEXT("Function item is invalid");
        return false;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("Inspector subsystem is unavailable");
        return false;
    }

    return InspectorSubsystem->NavigateToPinnedItem(Item, OutError);
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

void UInspectorFunctionRowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        return;
    }

    const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
    if (bFavorited != bCachedFavorited)
    {
        RefreshRow();
    }
}

void UInspectorFunctionRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_FunctionRowBorder"));
    RootBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
    RootBorder->SetBrushColor(RI_FunctionRowColor());

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FunctionRowBox"));
    RootBorder->SetContent(RootBox);

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_FunctionRowHeader"));
    if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(HeaderRow))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    FavoriteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_FunctionRowFavoriteButton"));
    FavoriteButton->OnClicked.AddDynamic(this, &UInspectorFunctionRowWidget::HandleFavoriteClicked);
    FavoriteSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_FunctionRowFavoriteSize"));
    FavoriteSizeBox->SetWidthOverride(RI_FunctionFavoriteButtonSize);
    FavoriteSizeBox->SetHeightOverride(RI_FunctionFavoriteButtonSize);
    FavoriteIcon = RICompactUI::MakeFavoriteIcon(
        WidgetTree,
        TEXT("RI_FunctionRowFavoriteIcon"),
        RI_FunctionFavoriteIconSize,
        false,
        RI_FunctionFavoriteActiveColor(),
        RI_FunctionRowMutedColor());
    FavoriteSizeBox->SetContent(FavoriteIcon);
    RICompactUI::CenterSizeBoxContent(FavoriteSizeBox);
    FavoriteButton->AddChild(FavoriteSizeBox);
    RICompactUI::ConfigureGhostIconButton(FavoriteButton);
    if (UButtonSlot* FavoriteButtonSlot = Cast<UButtonSlot>(FavoriteButton->GetContentSlot()))
    {
        FavoriteButtonSlot->SetHorizontalAlignment(HAlign_Center);
        FavoriteButtonSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteButtonSlot->SetPadding(FMargin(0.f));
    }
    if (UHorizontalBoxSlot* FavoriteSlot = HeaderRow->AddChildToHorizontalBox(FavoriteButton))
    {
        FavoriteSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
    }

    TitleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_FunctionRowTitleButton"));
    RICompactUI::ConfigureButton(TitleButton, RICompactUI::ERIButtonVisualStyle::Subtle, false);
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FButtonStyle TitleButtonStyle = TitleButton->WidgetStyle;
        const FSlateColor TransparentTint(FLinearColor::Transparent);
        TitleButtonStyle.Normal.TintColor = TransparentTint;
        TitleButtonStyle.Hovered.TintColor = TransparentTint;
        TitleButtonStyle.Pressed.TintColor = TransparentTint;
        TitleButtonStyle.Disabled.TintColor = TransparentTint;
        TitleButtonStyle.Normal.OutlineSettings.Width = 0.0f;
        TitleButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
        TitleButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
        TitleButtonStyle.Disabled.OutlineSettings.Width = 0.0f;
        TitleButtonStyle.NormalPadding = FMargin(0.f);
        TitleButtonStyle.PressedPadding = FMargin(0.f);
        TitleButton->WidgetStyle = TitleButtonStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }
    TitleButton->SetBackgroundColor(FLinearColor::Transparent);
    TitleButton->OnClicked.AddDynamic(this, &UInspectorFunctionRowWidget::HandleTitleClicked);
    TitleText = RICompactUI::MakeText(WidgetTree, TEXT("Function"), RICompactUI::GetLabelFontSize(), true, RI_FunctionRowTextColor(), false);
    TitleButton->AddChild(TitleText);
    if (UButtonSlot* TitleButtonSlot = Cast<UButtonSlot>(TitleButton->GetContentSlot()))
    {
        TitleButtonSlot->SetHorizontalAlignment(HAlign_Left);
        TitleButtonSlot->SetVerticalAlignment(VAlign_Center);
        TitleButtonSlot->SetPadding(FMargin(0.f));
    }
    if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleButton))
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

    InvokeButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("BTN_InvokeFunction"),
        TEXT("Run"),
        RICompactUI::ERIButtonVisualStyle::Primary,
        RI_FunctionRunButtonWidth,
        RI_FunctionRunButtonHeight,
        RICompactUI::GetValueFontSize());
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
    PrimaryParameterSizeBox.Reset();
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
        if (FavoriteButton)
        {
            FavoriteButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    if (FavoriteButton)
    {
        FavoriteButton->SetVisibility(ESlateVisibility::Visible);
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(Item->GetDisplayName()));
    }
    if (TitleButton)
    {
        TitleButton->SetIsEnabled(bAllowNavigation);
        TitleButton->SetToolTipText(bAllowNavigation
            ? FText::FromString(TEXT("Navigate to this function."))
            : FText::GetEmpty());
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

    const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
    UpdateCachedDisplayState(bFavorited);

    const TArray<FRIFunctionParameterSpec>& Params = Item->GetParameterSpecs();
    for (const FRIFunctionParameterSpec& Param : Params)
    {
        UBorder* ParamBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        ParamBorder->SetPadding(FMargin(4.f, 2.f));
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
        UWidget* WrappedControl = nullptr;
        if (Param.bIsEnum && Param.EnumOptions.Num() > 0)
        {
            UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
            RICompactUI::ConfigureComboBoxString(Combo, RI_FunctionRowTextColor(), 180.0f, RICompactUI::ERIInputVisualStyle::Strong);
            Combo->OnGenerateWidgetEvent.BindDynamic(this, &UInspectorFunctionRowWidget::HandleGenerateParameterComboItem);
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
            WrappedControl = RICompactUI::WrapValueControl(WidgetTree, Combo, RI_FunctionParameterWidth);
        }
        else if (Param.TypeLabel.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
        {
            UCheckBox* CheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
            RICompactUI::ConfigureCheckBox(CheckBox);
            const bool bChecked = Param.DefaultText.Equals(TEXT("True"), ESearchCase::IgnoreCase) || Param.DefaultText == TEXT("1");
            CheckBox->SetIsChecked(bChecked);
            Control = CheckBox;
            WrappedControl = RICompactUI::WrapValueControl(WidgetTree, CheckBox, 0.f, RI_FunctionParameterCheckSize, RI_FunctionParameterCheckSize);
            RICompactUI::CenterSizeBoxContent(Cast<USizeBox>(WrappedControl));
        }
        else
        {
            UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
            RICompactUI::ConfigureEditableTextBox(TextBox, RI_FunctionRowTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Strong);
            TextBox->SetText(FText::FromString(Param.DefaultText));
            Control = TextBox;
            WrappedControl = RICompactUI::WrapValueControl(WidgetTree, TextBox, RI_FunctionParameterWidth);
        }

        if (Control)
        {
            if (!PrimaryParameterSizeBox.IsValid())
            {
                PrimaryParameterSizeBox = Cast<USizeBox>(WrappedControl);
            }

            if (UHorizontalBoxSlot* ControlSlot = ParamRow->AddChildToHorizontalBox(WrappedControl ? WrappedControl : Control))
            {
                ControlSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
                ControlSlot->SetHorizontalAlignment(HAlign_Right);
                ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                ControlSlot->SetVerticalAlignment(VAlign_Center);
            }
            ParameterWidgets.Add(Control);
        }

        if (UVerticalBoxSlot* VBoxSlot = ParametersBox->AddChildToVerticalBox(ParamBorder))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
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

UWidget* UInspectorFunctionRowWidget::CreateParameterComboItemWidget(const FString& InItemText) const
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    return RICompactUI::MakeComboBoxItemText(WidgetTree, InItemText, RI_FunctionRowTextColor());
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

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("Inspector subsystem is unavailable");
        return false;
    }

    const TArray<FString> Args = CollectArgumentTexts();
    if (!InspectorSubsystem->InvokeFunctionItem(Item, Args, OutError))
    {
        InspectorSubsystem->PushToast(ERIToastType::Error, OutError.IsEmpty() ? TEXT("Function call failed") : OutError, 2.0f);
        return false;
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

UWidget* UInspectorFunctionRowWidget::HandleGenerateParameterComboItem(FString InItemText)
{
    return CreateParameterComboItemWidget(InItemText);
}

void UInspectorFunctionRowWidget::UpdateCachedDisplayState(bool bFavorited)
{
    bCachedFavorited = bFavorited;
    if (FavoriteIcon)
    {
        RICompactUI::SetFavoriteIconState(
            FavoriteIcon,
            bFavorited,
            RI_FunctionFavoriteIconSize,
            RI_FunctionFavoriteActiveColor(),
            RI_FunctionRowMutedColor());
    }
}

void UInspectorFunctionRowWidget::HandleFavoriteClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    InspectorSubsystem->ToggleFavoriteForAnyItem(Item);
    RefreshRow();
}

void UInspectorFunctionRowWidget::HandleTitleClicked()
{
    if (!bAllowNavigation)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorFunctionItem* Item = FunctionItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    FString Error;
    InspectorSubsystem->NavigateToPinnedItem(Item, Error);
}

void UInspectorFunctionRowWidget::HandleInvokeClicked()
{
    FString Error;
    if (!InvokeForAutomation(Error))
    {
        return;
    }
}
