#include "InspectorMaterialParamRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
    static FLinearColor RI_MaterialRowColor()
    {
        return RICompactUI::GetRowSurfaceBackgroundColor();
    }

    static FLinearColor RI_MaterialTextColor()
    {
        return RICompactUI::GetStrongTextColor();
    }

    static FLinearColor RI_MaterialMutedColor()
    {
        return RICompactUI::GetMutedTextColor();
    }

    static FLinearColor RI_MaterialFavoriteActiveColor()
    {
        return RICompactUI::GetWarningTextColor();
    }
}

UInspectorMaterialParamRowWidget::UInspectorMaterialParamRowWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorMaterialParamRowWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void UInspectorMaterialParamRowWidget::SetMaterialItem(UInspectorMaterialParamItem* InItem)
{
    MaterialItem = InItem;
    RefreshRow();
}

bool UInspectorMaterialParamRowWidget::IsDisplayingItem(const UInspectorMaterialParamItem* InItem) const
{
    const UInspectorMaterialParamItem* CurrentItem = MaterialItem.Get();
    if (!CurrentItem || !InItem)
    {
        return false;
    }

    return CurrentItem == InItem
        || (CurrentItem->GetMeshComponent() == InItem->GetMeshComponent()
            && CurrentItem->GetSlotIndex() == InItem->GetSlotIndex()
            && CurrentItem->GetParamName() == InItem->GetParamName()
            && CurrentItem->GetParamType() == InItem->GetParamType());
}

void UInspectorMaterialParamRowWidget::RefreshDisplay()
{
    RefreshRow();
}

void UInspectorMaterialParamRowWidget::SetAllowNavigation(bool bInAllowNavigation)
{
    bAllowNavigation = bInAllowNavigation;
    if (RootBorder)
    {
        RootBorder->SetToolTipText(bAllowNavigation
            ? FText::FromString(TEXT("Click the row background to navigate to this material parameter."))
            : FText::GetEmpty());
    }
}

float UInspectorMaterialParamRowWidget::GetValueControlHeightForAutomation() const
{
    if (ColorSizeBox && ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible)
    {
        return ColorSizeBox->GetHeightOverride();
    }
    if (ScalarValueTextBoxSizeBox && ScalarValueTextBox && ScalarValueTextBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return ScalarValueTextBoxSizeBox->GetHeightOverride();
    }
    if (ReadOnlyValueSizeBox && ReadOnlyValueText && ReadOnlyValueText->GetVisibility() == ESlateVisibility::Visible)
    {
        return ReadOnlyValueSizeBox->GetHeightOverride();
    }

    return 0.f;
}

float UInspectorMaterialParamRowWidget::GetFavoriteButtonHeightForAutomation() const
{
    return FavoriteSizeBox ? FavoriteSizeBox->GetHeightOverride() : 0.f;
}

float UInspectorMaterialParamRowWidget::GetColorButtonHeightForAutomation() const
{
    return (ColorSizeBox && ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible)
        ? ColorSizeBox->GetHeightOverride()
        : 0.f;
}

bool UInspectorMaterialParamRowWidget::CommitScalarValueForAutomation(const FString& InValue, FString& OutError)
{
    if (!MaterialItem.IsValid())
    {
        OutError = TEXT("Material item is invalid");
        return false;
    }

    if (!ScalarValueTextBox || ScalarValueTextBox->GetVisibility() != ESlateVisibility::Visible)
    {
        OutError = TEXT("Material row is not exposing an editable scalar text box");
        return false;
    }

    if (!ApplyScalarValue(InValue))
    {
        OutError = FString::Printf(TEXT("Failed to apply scalar value '%s'"), *InValue);
        return false;
    }

    RefreshRow();
    OutError.Reset();
    return true;
}

bool UInspectorMaterialParamRowWidget::NavigateForAutomation(FString& OutError)
{
    if (!MaterialItem.IsValid())
    {
        OutError = TEXT("Material item is invalid");
        return false;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("Inspector subsystem is unavailable");
        return false;
    }

    return InspectorSubsystem->NavigateToPinnedItem(MaterialItem.Get(), OutError);
}

bool UInspectorMaterialParamRowWidget::IsColorSwatchVisibleForAutomation() const
{
    return ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorMaterialParamRowWidget::IsScalarValueVisibleForAutomation() const
{
    return (ScalarValueTextBox && ScalarValueTextBox->GetVisibility() == ESlateVisibility::Visible)
        || (ReadOnlyValueText && ReadOnlyValueText->GetVisibility() == ESlateVisibility::Visible);
}

bool UInspectorMaterialParamRowWidget::IsScalarTextBoxVisibleForAutomation() const
{
    return ScalarValueTextBox && ScalarValueTextBox->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorMaterialParamRowWidget::HasFavoriteButtonForAutomation() const
{
    return FavoriteButton && FavoriteButton->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorMaterialParamRowWidget::TryGetDisplayedColorSwatchForAutomation(FLinearColor& OutColor) const
{
    OutColor = FLinearColor::Black;
    if (!ColorSwatch || !ColorButton || ColorButton->GetVisibility() != ESlateVisibility::Visible)
    {
        return false;
    }

    OutColor = ColorSwatch->GetBrushColor();
    return true;
}

TSharedRef<SWidget> UInspectorMaterialParamRowWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorMaterialParamRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshRow();
}

FReply UInspectorMaterialParamRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bAllowNavigation)
    {
        HandleNameClicked();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInspectorMaterialParamRowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!Item)
    {
        return;
    }

    const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
    if (bFavorited != bCachedFavorited)
    {
        RefreshRow();
        return;
    }

    if (Item->GetParamType() == EInspectorMatParamType::Vector)
    {
        FString Error;
        FLinearColor VectorValue = FLinearColor::Black;
        if (Item->GetVector(VectorValue, Error))
        {
            if (!bHasCachedDisplayColor || !VectorValue.Equals(CachedDisplayColor, KINDA_SMALL_NUMBER))
            {
                RefreshRow();
            }
        }
    }
    else
    {
        const FString CurrentValue = Item->GetValueText();
        if (CurrentValue != CachedScalarValue)
        {
            RefreshRow();
        }
    }
}

void UInspectorMaterialParamRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_MaterialParamRowBorder"));
    RootBorder->SetPadding(FMargin(4.f, 2.f));
    RootBorder->SetBrushColor(RI_MaterialRowColor());

    RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_MaterialParamRowBox"));
    RootBorder->SetContent(RootBox);

    FavoriteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_MaterialParamFavoriteButton"));
    RICompactUI::ConfigureButton(FavoriteButton, RICompactUI::ERIButtonVisualStyle::Secondary, false);
    FavoriteSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_MaterialParamFavoriteSize"));
    const float FavoriteButtonSize = FMath::Max(18.f, RICompactUI::GetInputHeight() - 4.f);
    const float FavoriteIconSize = FavoriteButtonSize * 0.58f;
    FavoriteSizeBox->SetWidthOverride(FavoriteButtonSize);
    FavoriteSizeBox->SetHeightOverride(FavoriteButtonSize);
    FavoriteIcon = RICompactUI::MakeFavoriteIcon(
        WidgetTree,
        TEXT("RI_MaterialParamFavoriteIcon"),
        FavoriteIconSize,
        false,
        RI_MaterialFavoriteActiveColor(),
        RI_MaterialMutedColor());
    FavoriteSizeBox->SetContent(FavoriteIcon);
    FavoriteButton->AddChild(FavoriteSizeBox);
    if (UButtonSlot* FavoriteButtonSlot = Cast<UButtonSlot>(FavoriteButton->GetContentSlot()))
    {
        FavoriteButtonSlot->SetHorizontalAlignment(HAlign_Center);
        FavoriteButtonSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteButtonSlot->SetPadding(FMargin(0.f));
    }
    FavoriteButton->OnClicked.AddDynamic(this, &UInspectorMaterialParamRowWidget::HandleFavoriteClicked);
    if (UHorizontalBoxSlot* FavoriteSlot = RootBox->AddChildToHorizontalBox(FavoriteButton))
    {
        FavoriteSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Material Parameter"), RICompactUI::GetLabelFontSize(), true, RI_MaterialTextColor(), false);
    NameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(NameText))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.0f;
        NameSlot->SetSize(SizeRule);
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    ReadOnlyValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), false, RI_MaterialMutedColor(), true);
    ReadOnlyValueText->SetJustification(ETextJustify::Right);
    ReadOnlyValueText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    ReadOnlyValueSizeBox = RICompactUI::WrapValueControl(WidgetTree, ReadOnlyValueText, 132.f);
    if (UHorizontalBoxSlot* ValueSlot = RootBox->AddChildToHorizontalBox(ReadOnlyValueSizeBox))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ValueSlot->SetHorizontalAlignment(HAlign_Right);
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ScalarValueTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_MaterialParamScalarTextBox"));
    RICompactUI::ConfigureEditableTextBox(ScalarValueTextBox, RI_MaterialTextColor());
    ScalarValueTextBox->SetJustification(ETextJustify::Right);
    ScalarValueTextBox->OnTextCommitted.AddDynamic(this, &UInspectorMaterialParamRowWidget::HandleScalarCommitted);
    ScalarValueTextBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, ScalarValueTextBox, 132.f);
    if (UHorizontalBoxSlot* ValueSlot = RootBox->AddChildToHorizontalBox(ScalarValueTextBoxSizeBox))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ValueSlot->SetHorizontalAlignment(HAlign_Right);
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ColorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_MaterialParamColorButton"));
    RICompactUI::ConfigureSwatchButton(ColorButton);
    ColorButton->OnClicked.AddDynamic(this, &UInspectorMaterialParamRowWidget::HandleColorClicked);
    ColorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_MaterialParamColorSize"));
    ColorSizeBox->SetWidthOverride(30.f);
    ColorSizeBox->SetHeightOverride(RICompactUI::GetInputHeight());
    ColorSwatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_MaterialParamColorSwatch"));
    ColorSwatch->SetPadding(FMargin(0.f));
    ColorSwatch->SetBrushColor(FLinearColor::Black);
    ColorSizeBox->SetContent(ColorSwatch);
    ColorButton->AddChild(ColorSizeBox);
    if (UHorizontalBoxSlot* ColorSlot = RootBox->AddChildToHorizontalBox(ColorButton))
    {
        ColorSlot->SetVerticalAlignment(VAlign_Center);
    }

    WidgetTree->RootWidget = RootBorder;
}

void UInspectorMaterialParamRowWidget::RefreshRow()
{
    if (!WidgetTree || !RootBox)
    {
        return;
    }

    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!Item)
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
        if (FavoriteButton)
        {
            FavoriteButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (ScalarValueTextBox)
        {
            ScalarValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (ColorButton)
        {
            ColorButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    if (FavoriteButton)
    {
        FavoriteButton->SetVisibility(ESlateVisibility::Visible);
    }
    if (FavoriteIcon)
    {
        const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
        bCachedFavorited = bFavorited;
        RICompactUI::SetFavoriteIconState(
            FavoriteIcon,
            bFavorited,
            FavoriteSizeBox ? FavoriteSizeBox->GetWidthOverride() * 0.58f : 0.f,
            RI_MaterialFavoriteActiveColor(),
            RI_MaterialMutedColor());
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(Item->GetPropertyName()));
    }
    if (ReadOnlyValueText)
    {
        ReadOnlyValueText->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (ScalarValueTextBox)
    {
        ScalarValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (ColorButton)
    {
        ColorButton->SetVisibility(ESlateVisibility::Collapsed);
    }

    FString Error;
    if (Item->GetParamType() == EInspectorMatParamType::Vector && ColorButton && ColorSwatch)
    {
        FLinearColor VectorValue = FLinearColor::Black;
        const bool bHasValue = Item->GetVector(VectorValue, Error);
        bHasCachedDisplayColor = bHasValue;
        CachedDisplayColor = bHasValue ? VectorValue : FLinearColor::Black;
        CachedScalarValue.Reset();
        ColorSwatch->SetBrushColor(bHasValue ? VectorValue : FLinearColor::Black);
        ColorButton->SetVisibility(ESlateVisibility::Visible);
        ColorButton->SetIsEnabled(bHasValue);
        return;
    }

    bHasCachedDisplayColor = false;
    CachedDisplayColor = FLinearColor::Transparent;

    if (ReadOnlyValueText)
    {
        float ScalarValue = 0.f;
        const bool bHasScalar = Item->GetScalar(ScalarValue, Error);
        const FString DisplayValue = bHasScalar ? FString::SanitizeFloat(ScalarValue) : Item->GetValueText();
        CachedScalarValue = DisplayValue;
        if (ScalarValueTextBox)
        {
            ScalarValueTextBox->SetVisibility(ESlateVisibility::Visible);
            ScalarValueTextBox->SetText(FText::FromString(DisplayValue));
        }
        else
        {
            ReadOnlyValueText->SetText(FText::FromString(DisplayValue));
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

bool UInspectorMaterialParamRowWidget::ApplyScalarValue(const FString& InValue)
{
    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!Item || Item->GetParamType() != EInspectorMatParamType::Scalar)
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

void UInspectorMaterialParamRowWidget::HandleFavoriteClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    InspectorSubsystem->ToggleFavoriteForAnyItem(Item);
    RefreshRow();
}

void UInspectorMaterialParamRowWidget::HandleColorClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!InspectorSubsystem || !Item || Item->GetParamType() != EInspectorMatParamType::Vector)
    {
        return;
    }

    InspectorSubsystem->OpenColorEditorForAnyItem(Item);
}

void UInspectorMaterialParamRowWidget::HandleScalarCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::Default)
    {
        return;
    }

    ApplyScalarValue(InText.ToString());
}

void UInspectorMaterialParamRowWidget::HandleNameClicked()
{
    if (!bAllowNavigation)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    UInspectorMaterialParamItem* Item = MaterialItem.Get();
    if (!InspectorSubsystem || !Item)
    {
        return;
    }

    FString Error;
    InspectorSubsystem->NavigateToPinnedItem(Item, Error);
}
