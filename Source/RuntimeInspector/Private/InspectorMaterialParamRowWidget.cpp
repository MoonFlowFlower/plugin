#include "InspectorMaterialParamRowWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

bool UInspectorMaterialParamRowWidget::IsColorSwatchVisibleForAutomation() const
{
    return ColorButton && ColorButton->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorMaterialParamRowWidget::IsScalarValueVisibleForAutomation() const
{
    return ReadOnlyValueText && ReadOnlyValueText->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorMaterialParamRowWidget::HasFavoriteButtonForAutomation() const
{
    return FavoriteButton && FavoriteButton->GetVisibility() == ESlateVisibility::Visible;
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

void UInspectorMaterialParamRowWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_MaterialParamRowBorder"));
    RootBorder->SetPadding(FMargin(6.f, 4.f));
    RootBorder->SetBrushColor(RI_MaterialRowColor());

    RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_MaterialParamRowBox"));
    RootBorder->SetContent(RootBox);

    FavoriteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_MaterialParamFavoriteButton"));
    RICompactUI::ConfigureButton(FavoriteButton, RICompactUI::ERIButtonVisualStyle::Secondary, false);
    USizeBox* FavoriteSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_MaterialParamFavoriteSize"));
    FavoriteSizeBox->SetWidthOverride(24.f);
    FavoriteSizeBox->SetHeightOverride(24.f);
    FavoriteText = RICompactUI::MakeText(WidgetTree, TEXT("☆"), RICompactUI::GetValueFontSize(), true, RI_MaterialMutedColor(), false);
    FavoriteText->SetJustification(ETextJustify::Center);
    FavoriteSizeBox->SetContent(FavoriteText);
    FavoriteButton->AddChild(FavoriteSizeBox);
    FavoriteButton->OnClicked.AddDynamic(this, &UInspectorMaterialParamRowWidget::HandleFavoriteClicked);
    if (UHorizontalBoxSlot* FavoriteSlot = RootBox->AddChildToHorizontalBox(FavoriteButton))
    {
        FavoriteSlot->SetVerticalAlignment(VAlign_Center);
        FavoriteSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Material Parameter"), RICompactUI::GetLabelFontSize(), true, RI_MaterialTextColor(), true);
    if (UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(NameText))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 0.92f;
        NameSlot->SetSize(SizeRule);
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }

    ReadOnlyValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), false, RI_MaterialMutedColor(), true);
    if (UHorizontalBoxSlot* ValueSlot = RootBox->AddChildToHorizontalBox(ReadOnlyValueText))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.08f;
        ValueSlot->SetSize(SizeRule);
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ColorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_MaterialParamColorButton"));
    RICompactUI::ConfigureButton(ColorButton, RICompactUI::ERIButtonVisualStyle::Subtle, false);
    ColorButton->OnClicked.AddDynamic(this, &UInspectorMaterialParamRowWidget::HandleColorClicked);
    USizeBox* ColorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_MaterialParamColorSize"));
    ColorSizeBox->SetWidthOverride(34.f);
    ColorSizeBox->SetHeightOverride(20.f);
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
    if (FavoriteText)
    {
        const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
        FavoriteText->SetText(FText::FromString(bFavorited ? TEXT("★") : TEXT("☆")));
        FavoriteText->SetColorAndOpacity(bFavorited ? RI_MaterialFavoriteActiveColor() : RI_MaterialMutedColor());
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(Item->GetPropertyName()));
    }

    if (ReadOnlyValueText)
    {
        ReadOnlyValueText->SetVisibility(ESlateVisibility::Collapsed);
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
        ColorSwatch->SetBrushColor(bHasValue ? VectorValue : FLinearColor::Black);
        ColorButton->SetVisibility(ESlateVisibility::Visible);
        ColorButton->SetIsEnabled(bHasValue);
        return;
    }

    if (ReadOnlyValueText)
    {
        float ScalarValue = 0.f;
        const bool bHasScalar = Item->GetScalar(ScalarValue, Error);
        ReadOnlyValueText->SetText(FText::FromString(bHasScalar ? FString::SanitizeFloat(ScalarValue) : Item->GetValueText()));
        ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
    }
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
