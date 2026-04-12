#include "InspectorPropertyRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorPropertyItem.h"
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
#include "Components/SizeBox.h"
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

    static FLinearColor RI_PropertyFavoriteActiveColor()
    {
        return RICompactUI::GetWarningTextColor();
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

bool UInspectorPropertyRowWidget::IsDisplayingItem(const UInspectorPropertyItem* InItem) const
{
    const UInspectorPropertyItem* CurrentItem = PropertyItem.Get();
    if (!CurrentItem || !InItem)
    {
        return false;
    }

    return CurrentItem == InItem
        || (CurrentItem->GetTargetObject() == InItem->GetTargetObject()
            && CurrentItem->GetPropertyFName() == InItem->GetPropertyFName());
}

void UInspectorPropertyRowWidget::RefreshDisplay()
{
    RefreshRow();
}

void UInspectorPropertyRowWidget::SetAllowNavigation(bool bInAllowNavigation)
{
    bAllowNavigation = bInAllowNavigation;
    if (NameButton)
    {
        NameButton->SetIsEnabled(bAllowNavigation);
    }
}

float UInspectorPropertyRowWidget::GetValueControlHeightForAutomation() const
{
    if (ValueTextBoxSizeBox && ValueTextBox && ValueTextBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return ValueTextBoxSizeBox->GetHeightOverride();
    }
    if (EnumComboBoxSizeBox && EnumComboBox && EnumComboBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return EnumComboBoxSizeBox->GetHeightOverride();
    }
    if (BoolCheckBoxSizeBox && BoolCheckBox && BoolCheckBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return BoolCheckBoxSizeBox->GetHeightOverride();
    }
    if (ColorSizeBox && ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible)
    {
        return ColorSizeBox->GetHeightOverride();
    }
    if (ReadOnlyValueSizeBox && ReadOnlyValueText && ReadOnlyValueText->GetVisibility() == ESlateVisibility::Visible)
    {
        return ReadOnlyValueSizeBox->GetHeightOverride();
    }

    return 0.f;
}

float UInspectorPropertyRowWidget::GetFavoriteButtonHeightForAutomation() const
{
    return FavoriteSizeBox ? FavoriteSizeBox->GetHeightOverride() : 0.f;
}

float UInspectorPropertyRowWidget::GetColorButtonHeightForAutomation() const
{
    return (ColorSizeBox && ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible)
        ? ColorSizeBox->GetHeightOverride()
        : 0.f;
}

bool UInspectorPropertyRowWidget::CommitTextValueForAutomation(const FString& InValue, FString& OutError)
{
    if (!PropertyItem.IsValid())
    {
        OutError = TEXT("Property item is invalid");
        return false;
    }

    if (!ValueTextBox || ValueTextBox->GetVisibility() != ESlateVisibility::Visible)
    {
        OutError = TEXT("Property row is not exposing an editable text box");
        return false;
    }

    if (!ApplyTextValue(InValue))
    {
        OutError = FString::Printf(TEXT("Failed to apply property value '%s'"), *InValue);
        return false;
    }

    RefreshRow();
    OutError.Reset();
    return true;
}

bool UInspectorPropertyRowWidget::NavigateForAutomation(FString& OutError)
{
    if (!PropertyItem.IsValid())
    {
        OutError = TEXT("Property item is invalid");
        return false;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("Inspector subsystem is unavailable");
        return false;
    }

    return InspectorSubsystem->NavigateToPinnedItem(PropertyItem.Get(), OutError);
}

bool UInspectorPropertyRowWidget::IsColorSwatchVisibleForAutomation() const
{
    return ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::IsReadOnlyValueVisibleForAutomation() const
{
    return ReadOnlyValueText && ReadOnlyValueText->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::IsValueTextBoxVisibleForAutomation() const
{
    return ValueTextBox && ValueTextBox->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const
{
    OutColor = FLinearColor::Black;
    if (!ColorSwatch || !ColorButton || ColorButton->GetVisibility() != ESlateVisibility::Visible)
    {
        return false;
    }

    OutColor = ColorSwatch->GetBrushColor();
    return true;
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

void UInspectorPropertyRowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        return;
    }

    const FString CurrentValue = Item->GetValueText();
    const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
    if (CurrentValue != CachedDisplayValue || bFavorited != bCachedFavorited)
    {
        RefreshRow();
    }
}

void UInspectorPropertyRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_PropertyRowBorder"));
    RootBorder->SetPadding(FMargin(4.f, 2.f));
    RootBorder->SetBrushColor(RI_PropertyRowColor());

    RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_PropertyRowBox"));
    RootBorder->SetContent(RootBox);

    FavoriteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_PropertyRowFavoriteButton"));
    RICompactUI::ConfigureButton(FavoriteButton, RICompactUI::ERIButtonVisualStyle::Secondary, false);
    FavoriteSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_PropertyRowFavoriteSize"));
    const float FavoriteButtonSize = FMath::Max(18.f, RICompactUI::GetInputHeight() - 4.f);
    FavoriteSizeBox->SetWidthOverride(FavoriteButtonSize);
    FavoriteSizeBox->SetHeightOverride(FavoriteButtonSize);
    FavoriteText = RICompactUI::MakeText(WidgetTree, TEXT("☆"), RICompactUI::GetValueFontSize(), true, RI_PropertyMutedColor(), false);
    FavoriteText->SetJustification(ETextJustify::Center);
    FavoriteSizeBox->SetContent(FavoriteText);
    FavoriteButton->AddChild(FavoriteSizeBox);
    FavoriteButton->OnClicked.AddDynamic(this, &UInspectorPropertyRowWidget::HandleFavoriteClicked);
    if (UHorizontalBoxSlot* FavoriteSlot = RootBox->AddChildToHorizontalBox(FavoriteButton))
    {
        FavoriteSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    NameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_PropertyRowNameButton"));
    RICompactUI::ConfigureButton(NameButton, RICompactUI::ERIButtonVisualStyle::Subtle, false);
    NameButton->OnClicked.AddDynamic(this, &UInspectorPropertyRowWidget::HandleNameClicked);
    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Property"), RICompactUI::GetLabelFontSize(), true, RI_PropertyTextColor(), false);
    NameButton->AddChild(NameText);
    if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(NameButton))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 0.90f;
        NameSlot->SetSize(SizeRule);
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    ReadOnlyValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), false, RI_PropertyMutedColor(), true);
    ReadOnlyValueSizeBox = RICompactUI::WrapValueControl(WidgetTree, ReadOnlyValueText, 120.f);
    if (UHorizontalBoxSlot* ValueSlot = RootBox->AddChildToHorizontalBox(ReadOnlyValueSizeBox))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.10f;
        ValueSlot->SetSize(SizeRule);
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ValueTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_PropertyRowTextBox"));
    RICompactUI::ConfigureEditableTextBox(ValueTextBox, RI_PropertyTextColor());
    ValueTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleValueCommitted);
    ValueTextBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, ValueTextBox, 120.f);
    if (UHorizontalBoxSlot* TextBoxSlot = RootBox->AddChildToHorizontalBox(ValueTextBoxSizeBox))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.10f;
        TextBoxSlot->SetSize(SizeRule);
        TextBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    BoolCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RI_PropertyRowBool"));
    BoolCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleBoolChanged);
    BoolCheckBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, BoolCheckBox, 0.f, RICompactUI::GetInputHeight(), RICompactUI::GetInputHeight());
    if (UHorizontalBoxSlot* BoolSlot = RootBox->AddChildToHorizontalBox(BoolCheckBoxSizeBox))
    {
        BoolSlot->SetVerticalAlignment(VAlign_Center);
    }

    EnumComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RI_PropertyRowEnum"));
    RICompactUI::ConfigureComboBoxString(EnumComboBox, RI_PropertyTextColor());
    EnumComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UInspectorPropertyRowWidget::HandleGenerateEnumOptionWidget);
    EnumComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleEnumChanged);
    EnumComboBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, EnumComboBox, 120.f);
    if (UHorizontalBoxSlot* EnumSlot = RootBox->AddChildToHorizontalBox(EnumComboBoxSizeBox))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.10f;
        EnumSlot->SetSize(SizeRule);
        EnumSlot->SetVerticalAlignment(VAlign_Center);
    }

    ColorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_PropertyRowColorButton"));
    RICompactUI::ConfigureButton(ColorButton, RICompactUI::ERIButtonVisualStyle::Subtle, false);
    ColorButton->OnClicked.AddDynamic(this, &UInspectorPropertyRowWidget::HandleColorClicked);
    ColorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_PropertyRowColorSize"));
    ColorSizeBox->SetWidthOverride(30.f);
    ColorSizeBox->SetHeightOverride(RICompactUI::GetInputHeight());
    ColorSwatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_PropertyRowColorSwatch"));
    ColorSwatch->SetPadding(FMargin(0.f, 2.f));
    ColorSwatch->SetBrushColor(FLinearColor::Black);
    ColorSizeBox->SetContent(ColorSwatch);
    ColorButton->AddChild(ColorSizeBox);
    if (UHorizontalBoxSlot* ColorSlot = RootBox->AddChildToHorizontalBox(ColorButton))
    {
        ColorSlot->SetVerticalAlignment(VAlign_Center);
        ColorSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
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
        if (ColorButton) ColorButton->SetVisibility(ESlateVisibility::Collapsed);
        if (FavoriteButton) FavoriteButton->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(Item->GetPropertyName()));
    }
    if (NameButton)
    {
        NameButton->SetIsEnabled(bAllowNavigation);
        NameButton->SetToolTipText(bAllowNavigation
            ? FText::FromString(TEXT("Navigate to this property."))
            : FText::GetEmpty());
    }

    const FString CurrentValue = Item->GetValueText();
    CachedDisplayValue = CurrentValue;
    const bool bEditable = Item->IsEditable();
    const EInspectorValueType ValueType = Item->GetValueType();
    const bool bIsColorType = ValueType == EInspectorValueType::LinearColor || ValueType == EInspectorValueType::Color;

    if (FavoriteButton)
    {
        FavoriteButton->SetVisibility(ESlateVisibility::Visible);
    }
    if (FavoriteText)
    {
        const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
        bCachedFavorited = bFavorited;
        FavoriteText->SetText(FText::FromString(bFavorited ? TEXT("★") : TEXT("☆")));
        FavoriteText->SetColorAndOpacity(bFavorited ? RI_PropertyFavoriteActiveColor() : RI_PropertyMutedColor());
    }

    if (ReadOnlyValueText) ReadOnlyValueText->SetVisibility(ESlateVisibility::Collapsed);
    if (ValueTextBox) ValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
    if (BoolCheckBox) BoolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
    if (EnumComboBox) EnumComboBox->SetVisibility(ESlateVisibility::Collapsed);
    if (ColorButton) ColorButton->SetVisibility(ESlateVisibility::Collapsed);

    if (bIsColorType && ColorButton && ColorSwatch)
    {
        FLinearColor SwatchColor = FLinearColor::Black;
        bool bHasColor = false;
        if (ValueType == EInspectorValueType::LinearColor)
        {
            bHasColor = Item->GetLinearColor(SwatchColor);
        }
        else
        {
            FColor SRGBColor = FColor::Black;
            bHasColor = Item->GetColor(SRGBColor);
            SwatchColor = FLinearColor::FromSRGBColor(SRGBColor);
        }

        ColorSwatch->SetBrushColor(bHasColor ? SwatchColor : FLinearColor::Black);
        ColorButton->SetVisibility(ESlateVisibility::Visible);
        ColorButton->SetIsEnabled(bEditable);
        return;
    }

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

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UObject* TargetObject = Item->GetTargetObject();
    FString Error;
    const bool bApplied = (InspectorSubsystem && TargetObject)
        ? InspectorSubsystem->ApplyPropertyTextImmediate(TargetObject, Item->GetPropertyFName(), InValue, Error)
        : Item->ApplyFromText(InValue, Error);
    if (bApplied)
    {
        RefreshRow();
    }
    return bApplied;
}

UWidget* UInspectorPropertyRowWidget::CreateEnumOptionWidget(const FString& InItemText) const
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    return RICompactUI::MakeComboBoxItemText(WidgetTree, InItemText, RI_PropertyTextColor());
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

UWidget* UInspectorPropertyRowWidget::HandleGenerateEnumOptionWidget(FString InItemText)
{
    return CreateEnumOptionWidget(InItemText);
}

void UInspectorPropertyRowWidget::HandleFavoriteClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    InspectorSubsystem->ToggleFavoriteForAnyItem(Item);
    RefreshRow();
}

void UInspectorPropertyRowWidget::HandleColorClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    InspectorSubsystem->OpenColorEditorForAnyItem(Item);
}

void UInspectorPropertyRowWidget::UpdateCachedDisplayState(const FString& InCurrentValue, bool bFavorited)
{
    CachedDisplayValue = InCurrentValue;
    bCachedFavorited = bFavorited;
}

void UInspectorPropertyRowWidget::HandleNameClicked()
{
    if (!bAllowNavigation)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    FString Error;
    InspectorSubsystem->NavigateToPinnedItem(Item, Error);
}
