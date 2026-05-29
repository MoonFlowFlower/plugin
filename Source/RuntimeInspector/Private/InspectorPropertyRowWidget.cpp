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
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SceneComponent.h"
#include "HAL/PlatformTime.h"
#include "Widgets/SWidget.h"

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

    static FString RI_FormatNumericValue(double Value)
    {
        FString Text = FString::Printf(TEXT("%.3f"), Value);
        if (Text.Contains(TEXT(".")))
        {
            while (Text.EndsWith(TEXT("0")))
            {
                Text.LeftChopInline(1, false);
            }
            if (Text.EndsWith(TEXT(".")))
            {
                Text.AppendChar(TEXT('0'));
            }
        }
        return Text;
    }

    static bool RI_ShouldSkipCommit(ETextCommit::Type CommitMethod)
    {
        return CommitMethod == ETextCommit::Default;
    }

    static constexpr float RI_StructuredPreviewDelaySeconds = 0.12f;
    static constexpr float RI_RowFavoriteButtonSize = 16.0f;
    static constexpr float RI_RowFavoriteIconSize = 10.5f;
    static constexpr float RI_RowCheckBoxSize = 16.0f;
    static constexpr float RI_RowValueWidth = 104.0f;
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
    RefreshTickPolicy();
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
    RefreshTickPolicy();
}

void UInspectorPropertyRowWidget::SetAllowNavigation(bool bInAllowNavigation)
{
    bAllowNavigation = bInAllowNavigation;
    if (RootBorder)
    {
        RootBorder->SetToolTipText(bAllowNavigation
            ? FText::FromString(TEXT("Click the row background to navigate to this property."))
            : FText::GetEmpty());
    }
}

void UInspectorPropertyRowWidget::SetStripOwnerPrefixForDisplay(bool bInStripOwnerPrefix)
{
    bStripOwnerPrefixForDisplay = bInStripOwnerPrefix;
    RefreshRow();
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
    if (VectorEditorBox && VectorEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return RICompactUI::GetInputHeight();
    }
    if (RotatorEditorBox && RotatorEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return RICompactUI::GetInputHeight();
    }
    if (TransformEditorBox && TransformEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        return RICompactUI::GetInputHeight();
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

bool UInspectorPropertyRowWidget::IsStructuredVectorVisibleForAutomation() const
{
    return VectorEditorBox && VectorEditorBox->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::IsStructuredRotatorVisibleForAutomation() const
{
    return RotatorEditorBox && RotatorEditorBox->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::IsStructuredTransformVisibleForAutomation() const
{
    return TransformEditorBox && TransformEditorBox->GetVisibility() == ESlateVisibility::Visible;
}

bool UInspectorPropertyRowWidget::CommitVectorValueForAutomation(const FVector& InValue, FString& OutError)
{
    if (!IsStructuredVectorVisibleForAutomation())
    {
        OutError = TEXT("Vector editor is not visible");
        return false;
    }

    if (!ApplyVectorValue(InValue))
    {
        OutError = TEXT("Failed to apply vector value");
        return false;
    }

    OutError.Reset();
    return true;
}

bool UInspectorPropertyRowWidget::CommitRotatorValueForAutomation(const FRotator& InValue, FString& OutError)
{
    if (!IsStructuredRotatorVisibleForAutomation())
    {
        OutError = TEXT("Rotator editor is not visible");
        return false;
    }

    if (!ApplyRotatorValue(InValue))
    {
        OutError = TEXT("Failed to apply rotator value");
        return false;
    }

    OutError.Reset();
    return true;
}

bool UInspectorPropertyRowWidget::CommitTransformValueForAutomation(const FTransform& InValue, FString& OutError)
{
    if (!IsStructuredTransformVisibleForAutomation())
    {
        OutError = TEXT("Transform editor is not visible");
        return false;
    }

    if (!ApplyTransformValue(InValue))
    {
        OutError = TEXT("Failed to apply transform value");
        return false;
    }

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
    RefreshTickPolicy();
}

FReply UInspectorPropertyRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bAllowNavigation)
    {
        HandleNameClicked();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInspectorPropertyRowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        RefreshTickPolicy();
        return;
    }

    const bool bStructuredFocus = HasStructuredEditorFocus();
    if (!bStructuredFocus && !bStructuredPreviewPending)
    {
        RefreshTickPolicy();
        return;
    }

    if (bStructuredPreviewPending)
    {
        if (bStructuredFocus)
        {
            StructuredPreviewAccum += InDeltaTime;
            if (StructuredPreviewAccum >= RI_StructuredPreviewDelaySeconds && TryPreviewStructuredEdit())
            {
                bStructuredPreviewPending = false;
                StructuredPreviewAccum = 0.f;
            }
        }
        else
        {
            bStructuredPreviewPending = false;
            StructuredPreviewAccum = 0.f;
        }
    }
    RefreshTickPolicy();
}

void UInspectorPropertyRowWidget::RefreshTickPolicy()
{
    const bool bShouldTick = bStructuredPreviewPending || HasStructuredEditorFocus();
    if (TSharedPtr<SWidget> CachedWidget = GetCachedWidget())
    {
        CachedWidget->SetCanTick(bShouldTick);
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
    FavoriteSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_PropertyRowFavoriteSize"));
    FavoriteSizeBox->SetWidthOverride(RI_RowFavoriteButtonSize);
    FavoriteSizeBox->SetHeightOverride(RI_RowFavoriteButtonSize);
    FavoriteIcon = RICompactUI::MakeFavoriteIcon(
        WidgetTree,
        TEXT("RI_PropertyRowFavoriteIcon"),
        RI_RowFavoriteIconSize,
        false,
        RI_PropertyFavoriteActiveColor(),
        RI_PropertyMutedColor());
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
    FavoriteButton->OnClicked.AddDynamic(this, &UInspectorPropertyRowWidget::HandleFavoriteClicked);
    if (UHorizontalBoxSlot* FavoriteSlot = RootBox->AddChildToHorizontalBox(FavoriteButton))
    {
        FavoriteSlot->SetVerticalAlignment(VAlign_Top);
        FavoriteSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
    }

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PropertyRowContentBox"));
    if (UHorizontalBoxSlot* ContentSlot = RootBox->AddChildToHorizontalBox(ContentBox))
    {
        ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ContentSlot->SetHorizontalAlignment(HAlign_Fill);
        ContentSlot->SetVerticalAlignment(VAlign_Fill);
    }

    SummaryRowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_PropertyRowSummaryRow"));
    if (UVerticalBoxSlot* SummarySlot = ContentBox->AddChildToVerticalBox(SummaryRowBox))
    {
        SummarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    StructuredValueBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PropertyRowStructuredValueBox"));
    if (UVerticalBoxSlot* StructuredSlot = ContentBox->AddChildToVerticalBox(StructuredValueBox))
    {
        StructuredSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        StructuredSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
    }
    StructuredValueBox->SetVisibility(ESlateVisibility::Collapsed);

    NameText = RICompactUI::MakeText(WidgetTree, TEXT("Property"), RICompactUI::GetLabelFontSize(), true, RI_PropertyTextColor(), false);
    NameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    NameText->SetClipping(EWidgetClipping::ClipToBounds);
    if (UHorizontalBoxSlot* NameSlot = SummaryRowBox->AddChildToHorizontalBox(NameText))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.0f;
        NameSlot->SetSize(SizeRule);
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    ReadOnlyValueText = RICompactUI::MakeText(WidgetTree, TEXT(""), RICompactUI::GetValueFontSize(), false, RI_PropertyMutedColor(), true);
    ReadOnlyValueText->SetJustification(ETextJustify::Right);
    ReadOnlyValueText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    ReadOnlyValueSizeBox = RICompactUI::WrapValueControl(WidgetTree, ReadOnlyValueText, RI_RowValueWidth);
    if (UHorizontalBoxSlot* ValueSlot = SummaryRowBox->AddChildToHorizontalBox(ReadOnlyValueSizeBox))
    {
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ValueSlot->SetHorizontalAlignment(HAlign_Right);
        ValueSlot->SetVerticalAlignment(VAlign_Center);
    }

    ValueTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_PropertyRowTextBox"));
    RICompactUI::ConfigureEditableTextBox(ValueTextBox, RI_PropertyTextColor());
    ValueTextBox->SetJustification(ETextJustify::Right);
    ValueTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleValueCommitted);
    ValueTextBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, ValueTextBox, RI_RowValueWidth);
    if (UHorizontalBoxSlot* TextBoxSlot = SummaryRowBox->AddChildToHorizontalBox(ValueTextBoxSizeBox))
    {
        TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        TextBoxSlot->SetHorizontalAlignment(HAlign_Right);
        TextBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    BoolCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RI_PropertyRowBool"));
    RICompactUI::ConfigureCheckBox(BoolCheckBox);
    BoolCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleBoolChanged);
    BoolCheckBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, BoolCheckBox, 0.f, RI_RowCheckBoxSize, RI_RowCheckBoxSize);
    RICompactUI::CenterSizeBoxContent(BoolCheckBoxSizeBox);
    if (UHorizontalBoxSlot* BoolSlot = SummaryRowBox->AddChildToHorizontalBox(BoolCheckBoxSizeBox))
    {
        BoolSlot->SetVerticalAlignment(VAlign_Center);
    }

    EnumComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RI_PropertyRowEnum"));
    RICompactUI::ConfigureComboBoxString(EnumComboBox, RI_PropertyTextColor());
    EnumComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UInspectorPropertyRowWidget::HandleGenerateEnumOptionWidget);
    EnumComboBox->OnSelectionChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleEnumChanged);
    EnumComboBoxSizeBox = RICompactUI::WrapValueControl(WidgetTree, EnumComboBox, RI_RowValueWidth);
    if (UHorizontalBoxSlot* EnumSlot = SummaryRowBox->AddChildToHorizontalBox(EnumComboBoxSizeBox))
    {
        EnumSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        EnumSlot->SetHorizontalAlignment(HAlign_Right);
        EnumSlot->SetVerticalAlignment(VAlign_Center);
    }

    ColorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RI_PropertyRowColorButton"));
    RICompactUI::ConfigureSwatchButton(ColorButton);
    ColorButton->OnClicked.AddDynamic(this, &UInspectorPropertyRowWidget::HandleColorClicked);
    ColorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_PropertyRowColorSize"));
    ColorSizeBox->SetWidthOverride(30.f);
    ColorSizeBox->SetHeightOverride(RICompactUI::GetInputHeight());
    ColorSwatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_PropertyRowColorSwatch"));
    ColorSwatch->SetPadding(FMargin(0.f));
    ColorSwatch->SetBrushColor(FLinearColor::Black);
    ColorSizeBox->SetContent(ColorSwatch);
    ColorButton->AddChild(ColorSizeBox);
    if (UHorizontalBoxSlot* ColorSlot = SummaryRowBox->AddChildToHorizontalBox(ColorButton))
    {
        ColorSlot->SetVerticalAlignment(VAlign_Center);
        ColorSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
    }

    VectorEditorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_PropertyRowVectorEditor"));
    if (UVerticalBoxSlot* VectorSlot = StructuredValueBox->AddChildToVerticalBox(VectorEditorBox))
    {
        VectorSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }
    VectorEditorBox->SetVisibility(ESlateVisibility::Collapsed);
    VectorAxisEditors.Reset();
    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        UEditableTextBox* AxisTextBox = nullptr;
        UHorizontalBox* AxisRow = RICompactUI::MakeAxisValueRow(
            WidgetTree,
            AxisIndex == 0 ? TEXT("X") : (AxisIndex == 1 ? TEXT("Y") : TEXT("Z")),
            AxisIndex,
            AxisTextBox,
            *FString::Printf(TEXT("RI_VectorAxis%d"), AxisIndex));
        if (AxisTextBox)
        {
            AxisTextBox->OnTextChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleStructuredAxisTextChanged);
            switch (AxisIndex)
            {
            case 0: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleVectorXCommitted); break;
            case 1: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleVectorYCommitted); break;
            case 2:
            default: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleVectorZCommitted); break;
            }
            VectorAxisEditors.Add(AxisTextBox);
        }
        if (UHorizontalBoxSlot* AxisSlot = VectorEditorBox->AddChildToHorizontalBox(AxisRow))
        {
            AxisSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            AxisSlot->SetHorizontalAlignment(HAlign_Fill);
            AxisSlot->SetVerticalAlignment(VAlign_Center);
            AxisSlot->SetPadding(FMargin(AxisIndex > 0 ? 2.f : 0.f, 0.f, 0.f, 0.f));
        }
    }

    RotatorEditorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_PropertyRowRotatorEditor"));
    if (UVerticalBoxSlot* RotatorSlot = StructuredValueBox->AddChildToVerticalBox(RotatorEditorBox))
    {
        RotatorSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }
    RotatorEditorBox->SetVisibility(ESlateVisibility::Collapsed);
    RotatorEditorBox->SetToolTipText(FText::FromString(TEXT("Rotator axis mapping: X=Roll, Y=Pitch, Z=Yaw")));
    RotatorAxisEditors.Reset();
    for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        UEditableTextBox* AxisTextBox = nullptr;
        UHorizontalBox* AxisRow = RICompactUI::MakeAxisValueRow(
            WidgetTree,
            AxisIndex == 0 ? TEXT("X") : (AxisIndex == 1 ? TEXT("Y") : TEXT("Z")),
            AxisIndex,
            AxisTextBox,
            *FString::Printf(TEXT("RI_RotatorAxis%d"), AxisIndex));
        if (AxisTextBox)
        {
            AxisTextBox->OnTextChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleStructuredAxisTextChanged);
            switch (AxisIndex)
            {
            case 0: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleRotatorXCommitted); break;
            case 1: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleRotatorYCommitted); break;
            case 2:
            default: AxisTextBox->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleRotatorZCommitted); break;
            }
            RotatorAxisEditors.Add(AxisTextBox);
        }
        if (UHorizontalBoxSlot* AxisSlot = RotatorEditorBox->AddChildToHorizontalBox(AxisRow))
        {
            AxisSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            AxisSlot->SetHorizontalAlignment(HAlign_Fill);
            AxisSlot->SetVerticalAlignment(VAlign_Center);
            AxisSlot->SetPadding(FMargin(AxisIndex > 0 ? 2.f : 0.f, 0.f, 0.f, 0.f));
        }
    }

    TransformEditorBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PropertyRowTransformEditor"));
    TransformEditorBox->SetToolTipText(FText::FromString(TEXT("Transform editor. Rotation axis mapping: X=Roll, Y=Pitch, Z=Yaw")));
    if (UVerticalBoxSlot* TransformSlot = StructuredValueBox->AddChildToVerticalBox(TransformEditorBox))
    {
        TransformSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        TransformSlot->SetPadding(FMargin(0.f));
    }
    TransformEditorBox->SetVisibility(ESlateVisibility::Collapsed);

    auto AddTransformGroup = [&](const TCHAR* GroupName, const TCHAR* LabelText, TArray<TObjectPtr<UEditableTextBox>>& OutEditors)
    {
        UVerticalBox* GroupBox = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            FName(FString::Printf(TEXT("%sGroup"), GroupName)));
        if (UVerticalBoxSlot* GroupSlot = TransformEditorBox->AddChildToVerticalBox(GroupBox))
        {
            GroupSlot->SetPadding(FMargin(0.f, TransformEditorBox->GetChildrenCount() > 1 ? 4.f : 0.f, 0.f, 0.f));
        }

        UTextBlock* GroupLabel = RICompactUI::MakeText(
            WidgetTree,
            LabelText,
            FMath::Max(6, RICompactUI::GetMutedFontSize()),
            true,
            RICompactUI::GetSecondaryTextColor(),
            false);
        if (UVerticalBoxSlot* LabelSlot = GroupBox->AddChildToVerticalBox(GroupLabel))
        {
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }

        UHorizontalBox* AxisStrip = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            FName(FString::Printf(TEXT("%sAxisStrip"), GroupName)));
        GroupBox->AddChildToVerticalBox(AxisStrip);

        for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            UEditableTextBox* AxisTextBox = nullptr;
            UHorizontalBox* AxisRow = RICompactUI::MakeAxisValueRow(
                WidgetTree,
                AxisIndex == 0 ? TEXT("X") : (AxisIndex == 1 ? TEXT("Y") : TEXT("Z")),
                AxisIndex,
                AxisTextBox,
                *FString::Printf(TEXT("%sAxis%d"), GroupName, AxisIndex));
            if (AxisTextBox)
            {
                AxisTextBox->OnTextChanged.AddDynamic(this, &UInspectorPropertyRowWidget::HandleStructuredAxisTextChanged);
                OutEditors.Add(AxisTextBox);
            }
            if (UHorizontalBoxSlot* AxisSlot = AxisStrip->AddChildToHorizontalBox(AxisRow))
            {
                AxisSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                AxisSlot->SetHorizontalAlignment(HAlign_Fill);
                AxisSlot->SetVerticalAlignment(VAlign_Center);
                AxisSlot->SetPadding(FMargin(AxisIndex > 0 ? 2.f : 0.f, 0.f, 0.f, 0.f));
            }
        }
    };

    TransformLocationEditors.Reset();
    AddTransformGroup(TEXT("RI_TransformLocation"), TEXT("Location"), TransformLocationEditors);
    TransformRotationEditors.Reset();
    AddTransformGroup(TEXT("RI_TransformRotation"), TEXT("Rotation"), TransformRotationEditors);
    TransformScaleEditors.Reset();
    AddTransformGroup(TEXT("RI_TransformScale"), TEXT("Scale"), TransformScaleEditors);

    if (TransformLocationEditors.Num() == 3)
    {
        TransformLocationEditors[0]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformLocationXCommitted);
        TransformLocationEditors[1]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformLocationYCommitted);
        TransformLocationEditors[2]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformLocationZCommitted);
    }
    if (TransformRotationEditors.Num() == 3)
    {
        TransformRotationEditors[0]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformRotationXCommitted);
        TransformRotationEditors[1]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformRotationYCommitted);
        TransformRotationEditors[2]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformRotationZCommitted);
    }
    if (TransformScaleEditors.Num() == 3)
    {
        TransformScaleEditors[0]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformScaleXCommitted);
        TransformScaleEditors[1]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformScaleYCommitted);
        TransformScaleEditors[2]->OnTextCommitted.AddDynamic(this, &UInspectorPropertyRowWidget::HandleTransformScaleZCommitted);
    }

    WidgetTree->RootWidget = RootBorder;
}

void UInspectorPropertyRowWidget::HideAllValueControls()
{
    if (ReadOnlyValueText) ReadOnlyValueText->SetVisibility(ESlateVisibility::Collapsed);
    if (ValueTextBox) ValueTextBox->SetVisibility(ESlateVisibility::Collapsed);
    if (BoolCheckBox) BoolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
    if (EnumComboBox) EnumComboBox->SetVisibility(ESlateVisibility::Collapsed);
    if (ColorButton) ColorButton->SetVisibility(ESlateVisibility::Collapsed);
    if (VectorEditorBox) VectorEditorBox->SetVisibility(ESlateVisibility::Collapsed);
    if (RotatorEditorBox) RotatorEditorBox->SetVisibility(ESlateVisibility::Collapsed);
    if (TransformEditorBox) TransformEditorBox->SetVisibility(ESlateVisibility::Collapsed);
    if (StructuredValueBox) StructuredValueBox->SetVisibility(ESlateVisibility::Collapsed);
}

void UInspectorPropertyRowWidget::RefreshRow()
{
    if (!WidgetTree || !RootBox || !ContentBox)
    {
        return;
    }

    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(TEXT("Unavailable")));
            NameText->SetToolTipText(FText::GetEmpty());
        }
        HideAllValueControls();
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::GetEmpty());
        }
        if (FavoriteButton) FavoriteButton->SetVisibility(ESlateVisibility::Collapsed);
        RefreshTickPolicy();
        return;
    }

    if (NameText)
    {
        NameText->SetText(FText::FromString(GetDisplayedPropertyName(Item)));
        NameText->SetToolTipText(FText::FromString(Item->GetPropertyName()));
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
    if (FavoriteIcon)
    {
        const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);
        bCachedFavorited = bFavorited;
        RICompactUI::SetFavoriteIconState(
            FavoriteIcon,
            bFavorited,
            RI_RowFavoriteIconSize,
            RI_PropertyFavoriteActiveColor(),
            RI_PropertyMutedColor());
    }

    HideAllValueControls();

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

    switch (ValueType)
    {
    case EInspectorValueType::Vector3:
        RefreshStructuredVector(Item, bEditable);
        return;
    case EInspectorValueType::Rotator:
        RefreshStructuredRotator(Item, bEditable);
        return;
    case EInspectorValueType::Transform:
        RefreshStructuredTransform(Item, bEditable);
        return;
    default:
        break;
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

    RefreshTickPolicy();
}

void UInspectorPropertyRowWidget::RefreshStructuredVector(UInspectorPropertyItem* Item, bool bEditable)
{
    FVector Value = FVector::ZeroVector;
    if (!Item->GetVector(Value))
    {
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::FromString(Item->GetValueText()));
        }
        return;
    }

    PopulateAxisEditors(VectorAxisEditors, { Value.X, Value.Y, Value.Z }, bEditable);
    if (VectorEditorBox)
    {
        VectorEditorBox->SetVisibility(ESlateVisibility::Visible);
    }
    if (StructuredValueBox)
    {
        StructuredValueBox->SetVisibility(ESlateVisibility::Visible);
    }
}

void UInspectorPropertyRowWidget::RefreshStructuredRotator(UInspectorPropertyItem* Item, bool bEditable)
{
    FRotator Value = FRotator::ZeroRotator;
    if (!Item->GetRotator(Value))
    {
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::FromString(Item->GetValueText()));
        }
        return;
    }

    PopulateAxisEditors(RotatorAxisEditors, { Value.Roll, Value.Pitch, Value.Yaw }, bEditable);
    if (RotatorEditorBox)
    {
        RotatorEditorBox->SetVisibility(ESlateVisibility::Visible);
    }
    if (StructuredValueBox)
    {
        StructuredValueBox->SetVisibility(ESlateVisibility::Visible);
    }
    if (NameText)
    {
        NameText->SetToolTipText(FText::FromString(TEXT("Rotator axis mapping: X=Roll, Y=Pitch, Z=Yaw")));
    }
}

void UInspectorPropertyRowWidget::RefreshStructuredTransform(UInspectorPropertyItem* Item, bool bEditable)
{
    FTransform Value = FTransform::Identity;
    if (!Item->GetTransform(Value))
    {
        if (ReadOnlyValueText)
        {
            ReadOnlyValueText->SetVisibility(ESlateVisibility::Visible);
            ReadOnlyValueText->SetText(FText::FromString(Item->GetValueText()));
        }
        return;
    }

    const FVector Location = Value.GetLocation();
    const FRotator Rotation = Value.Rotator();
    const FVector Scale = Value.GetScale3D();

    PopulateAxisEditors(TransformLocationEditors, { Location.X, Location.Y, Location.Z }, bEditable);
    PopulateAxisEditors(TransformRotationEditors, { Rotation.Roll, Rotation.Pitch, Rotation.Yaw }, bEditable);
    PopulateAxisEditors(TransformScaleEditors, { Scale.X, Scale.Y, Scale.Z }, bEditable);

    if (TransformEditorBox)
    {
        TransformEditorBox->SetVisibility(ESlateVisibility::Visible);
    }
    if (StructuredValueBox)
    {
        StructuredValueBox->SetVisibility(ESlateVisibility::Visible);
    }
    if (NameText)
    {
        NameText->SetToolTipText(FText::FromString(TEXT("Transform groups: Location, Rotation, Scale. Rotation uses X=Roll, Y=Pitch, Z=Yaw.")));
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
        if (InspectorSubsystem && TargetObject)
        {
            InspectorSubsystem->RefreshActorPropertyValue(TargetObject, Item->GetPropertyFName());
        }
    }
    return bApplied;
}

FString UInspectorPropertyRowWidget::GetDisplayedPropertyName(const UInspectorPropertyItem* Item) const
{
    if (!Item)
    {
        return TEXT("Unavailable");
    }

    return bStripOwnerPrefixForDisplay
        ? Item->GetPropertyNameWithoutOwnerPrefix()
        : Item->GetPropertyName();
}

bool UInspectorPropertyRowWidget::ApplyVectorValue(const FVector& InValue)
{
    return ApplyVectorValueInternal(InValue, true);
}

bool UInspectorPropertyRowWidget::ApplyVectorValueInternal(const FVector& InValue, bool bRefreshRow)
{
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item)
    {
        return false;
    }

    FString Error;
    const bool bApplied = Item->SetVector(InValue, Error);
    if (bRefreshRow)
    {
        RefreshRow();
        if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
        {
            if (UObject* TargetObject = Item->GetTargetObject())
            {
                InspectorSubsystem->RefreshActorPropertyValue(TargetObject, Item->GetPropertyFName());
            }
        }
    }
    return bApplied;
}

bool UInspectorPropertyRowWidget::ApplyRotatorValue(const FRotator& InValue)
{
    return ApplyRotatorValueInternal(InValue, true);
}

bool UInspectorPropertyRowWidget::ApplyRotatorValueInternal(const FRotator& InValue, bool bRefreshRow)
{
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item)
    {
        return false;
    }

    FString Error;
    const bool bApplied = Item->SetRotator(InValue, Error);
    if (bRefreshRow)
    {
        RefreshRow();
        if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
        {
            if (UObject* TargetObject = Item->GetTargetObject())
            {
                InspectorSubsystem->RefreshActorPropertyValue(TargetObject, Item->GetPropertyFName());
            }
        }
    }
    return bApplied;
}

bool UInspectorPropertyRowWidget::ApplyTransformValue(const FTransform& InValue)
{
    return ApplyTransformValueInternal(InValue, true);
}

bool UInspectorPropertyRowWidget::ApplyTransformValueInternal(const FTransform& InValue, bool bRefreshRow)
{
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item)
    {
        return false;
    }

    if (USceneComponent* SceneComponent = Cast<USceneComponent>(Item->GetTargetObject()))
    {
        if (Item->GetPropertyFName() == TEXT("RelativeTransform"))
        {
            return ApplySceneComponentTransformValue(SceneComponent, InValue, bRefreshRow);
        }
    }

    FString Error;
    const bool bApplied = Item->SetTransform(InValue, Error);
    if (bRefreshRow)
    {
        RefreshRow();
        if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
        {
            if (UObject* TargetObject = Item->GetTargetObject())
            {
                InspectorSubsystem->RefreshActorPropertyValue(TargetObject, Item->GetPropertyFName());
            }
        }
    }
    return bApplied;
}

UInspectorPropertyItem* UInspectorPropertyRowWidget::MakeSiblingPropertyItem(UObject* TargetObject, FName PropertyFName) const
{
    if (!TargetObject || PropertyFName.IsNone())
    {
        return nullptr;
    }

    UObject* Outer = Subsystem.IsValid() ? static_cast<UObject*>(Subsystem.Get()) : GetTransientPackage();
    UInspectorPropertyItem* SiblingItem = NewObject<UInspectorPropertyItem>(Outer);
    if (!SiblingItem)
    {
        return nullptr;
    }

    SiblingItem->Init(TargetObject, PropertyFName);
    return SiblingItem->IsValidItem() ? SiblingItem : nullptr;
}

bool UInspectorPropertyRowWidget::ApplySceneComponentTransformValue(USceneComponent* SceneComponent, const FTransform& InValue, bool bRefreshRow)
{
    if (!SceneComponent)
    {
        return false;
    }

    const FVector OriginalLocation = SceneComponent->GetRelativeLocation();
    const FRotator OriginalRotation = SceneComponent->GetRelativeRotation();
    const FVector OriginalScale = SceneComponent->GetRelativeScale3D();

    const FVector NewLocation = InValue.GetLocation();
    const FRotator NewRotation = InValue.Rotator();
    const FVector NewScale = InValue.GetScale3D();

    const bool bLocationChanged = !OriginalLocation.Equals(NewLocation, 0.001f);
    const bool bRotationChanged = !OriginalRotation.Equals(NewRotation, 0.001f);
    const bool bScaleChanged = !OriginalScale.Equals(NewScale, 0.001f);

    if (!bLocationChanged && !bRotationChanged && !bScaleChanged)
    {
        if (bRefreshRow)
        {
            RefreshRow();
        }
        return true;
    }

    UInspectorPropertyItem* LocationItem = bLocationChanged
        ? MakeSiblingPropertyItem(SceneComponent, TEXT("RelativeLocation"))
        : nullptr;
    UInspectorPropertyItem* RotationItem = bRotationChanged
        ? MakeSiblingPropertyItem(SceneComponent, TEXT("RelativeRotation"))
        : nullptr;
    UInspectorPropertyItem* ScaleItem = bScaleChanged
        ? MakeSiblingPropertyItem(SceneComponent, TEXT("RelativeScale3D"))
        : nullptr;

    if ((bLocationChanged && (!LocationItem || LocationItem->GetValueType() != EInspectorValueType::Vector3))
        || (bRotationChanged && (!RotationItem || RotationItem->GetValueType() != EInspectorValueType::Rotator))
        || (bScaleChanged && (!ScaleItem || ScaleItem->GetValueType() != EInspectorValueType::Vector3)))
    {
        if (bRefreshRow)
        {
            RefreshRow();
        }
        return false;
    }

    FString Error;
    bool bLocationApplied = false;
    bool bRotationApplied = false;
    bool bScaleApplied = false;

    if (bLocationChanged)
    {
        bLocationApplied = LocationItem->SetVector(NewLocation, Error);
        if (!bLocationApplied)
        {
            if (bRefreshRow)
            {
                RefreshRow();
            }
            return false;
        }
    }

    if (bRotationChanged)
    {
        bRotationApplied = RotationItem->SetRotator(NewRotation, Error);
        if (!bRotationApplied)
        {
            if (bLocationApplied)
            {
                FString RestoreError;
                LocationItem->SetVector(OriginalLocation, RestoreError);
            }
            if (bRefreshRow)
            {
                RefreshRow();
            }
            return false;
        }
    }

    if (bScaleChanged)
    {
        bScaleApplied = ScaleItem->SetVector(NewScale, Error);
        if (!bScaleApplied)
        {
            if (bRotationApplied)
            {
                FString RestoreError;
                RotationItem->SetRotator(OriginalRotation, RestoreError);
            }
            if (bLocationApplied)
            {
                FString RestoreError;
                LocationItem->SetVector(OriginalLocation, RestoreError);
            }
            if (bRefreshRow)
            {
                RefreshRow();
            }
            return false;
        }
    }

    if (bRefreshRow)
    {
        RefreshRow();
    }
    return true;
}

bool UInspectorPropertyRowWidget::TryParseEditableNumber(UEditableTextBox* TextBox, double& OutValue) const
{
    OutValue = 0.0;
    if (!TextBox)
    {
        return false;
    }

    FString Text = TextBox->GetText().ToString();
    Text.TrimStartAndEndInline();
    return !Text.IsEmpty() && LexTryParseString(OutValue, *Text);
}

void UInspectorPropertyRowWidget::PopulateAxisEditors(const TArray<TObjectPtr<UEditableTextBox>>& Editors, const TArray<double>& Values, bool bEditable) const
{
    const int32 Count = FMath::Min(Editors.Num(), Values.Num());
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (UEditableTextBox* Editor = Editors[Index])
        {
            Editor->SetText(FText::FromString(RI_FormatNumericValue(Values[Index])));
            Editor->SetIsReadOnly(!bEditable);
            Editor->SetIsEnabled(true);
        }
    }
}

bool UInspectorPropertyRowWidget::ExtractAxisValues(const TArray<TObjectPtr<UEditableTextBox>>& Editors, TArray<double>& OutValues) const
{
    OutValues.Reset();
    OutValues.Reserve(Editors.Num());
    for (UEditableTextBox* Editor : Editors)
    {
        double ParsedValue = 0.0;
        if (!TryParseEditableNumber(Editor, ParsedValue))
        {
            return false;
        }
        OutValues.Add(ParsedValue);
    }
    return OutValues.Num() == Editors.Num();
}

bool UInspectorPropertyRowWidget::CommitVectorEditors()
{
    bStructuredPreviewPending = false;
    StructuredPreviewAccum = 0.f;
    TArray<double> Values;
    if (!ExtractAxisValues(VectorAxisEditors, Values) || Values.Num() != 3)
    {
        RefreshRow();
        return false;
    }

    return ApplyVectorValue(FVector(Values[0], Values[1], Values[2]));
}

bool UInspectorPropertyRowWidget::CommitRotatorEditors()
{
    bStructuredPreviewPending = false;
    StructuredPreviewAccum = 0.f;
    TArray<double> Values;
    if (!ExtractAxisValues(RotatorAxisEditors, Values) || Values.Num() != 3)
    {
        RefreshRow();
        return false;
    }

    return ApplyRotatorValue(FRotator(Values[1], Values[2], Values[0]));
}

bool UInspectorPropertyRowWidget::CommitTransformEditors()
{
    bStructuredPreviewPending = false;
    StructuredPreviewAccum = 0.f;
    TArray<double> LocationValues;
    TArray<double> RotationValues;
    TArray<double> ScaleValues;
    if (!ExtractAxisValues(TransformLocationEditors, LocationValues)
        || !ExtractAxisValues(TransformRotationEditors, RotationValues)
        || !ExtractAxisValues(TransformScaleEditors, ScaleValues)
        || LocationValues.Num() != 3
        || RotationValues.Num() != 3
        || ScaleValues.Num() != 3)
    {
        RefreshRow();
        return false;
    }

    const FVector Location(LocationValues[0], LocationValues[1], LocationValues[2]);
    const FRotator Rotation(RotationValues[1], RotationValues[2], RotationValues[0]);
    const FVector Scale(ScaleValues[0], ScaleValues[1], ScaleValues[2]);
    return ApplyTransformValue(FTransform(Rotation, Location, Scale));
}

bool UInspectorPropertyRowWidget::HasFocusedEditorInSet(const TArray<TObjectPtr<UEditableTextBox>>& Editors) const
{
    for (UEditableTextBox* Editor : Editors)
    {
        if (Editor && Editor->HasKeyboardFocus())
        {
            return true;
        }
    }

    return false;
}

bool UInspectorPropertyRowWidget::HasStructuredEditorFocus() const
{
    return HasFocusedEditorInSet(VectorAxisEditors)
        || HasFocusedEditorInSet(RotatorAxisEditors)
        || HasFocusedEditorInSet(TransformLocationEditors)
        || HasFocusedEditorInSet(TransformRotationEditors)
        || HasFocusedEditorInSet(TransformScaleEditors);
}

void UInspectorPropertyRowWidget::MarkStructuredPreviewDirty()
{
    bStructuredPreviewPending = true;
    StructuredPreviewAccum = 0.f;
    RefreshTickPolicy();
}

bool UInspectorPropertyRowWidget::TryPreviewStructuredEdit()
{
    UInspectorPropertyItem* Item = PropertyItem.Get();
    if (!Item || !Item->IsValidItem())
    {
        return false;
    }

    const bool bFavorited = Subsystem.IsValid() && Subsystem->IsFavoriteForAnyItem(Item);

    const double StartSeconds = FPlatformTime::Seconds();

    if (VectorEditorBox && VectorEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        TArray<double> Values;
        if (!ExtractAxisValues(VectorAxisEditors, Values) || Values.Num() != 3)
        {
            return false;
        }

        const FVector NewValue(Values[0], Values[1], Values[2]);
        FVector CurrentValue = FVector::ZeroVector;
        if (Item->GetVector(CurrentValue) && CurrentValue.Equals(NewValue, 0.001f))
        {
            return true;
        }

        if (!ApplyVectorValueInternal(NewValue, false))
        {
            return false;
        }

        UpdateCachedDisplayState(Item->GetValueText(), bFavorited);
        UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] StructuredPreview %.2f ms | Property=%s"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *Item->GetPropertyFName().ToString());
        return true;
    }

    if (RotatorEditorBox && RotatorEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        TArray<double> Values;
        if (!ExtractAxisValues(RotatorAxisEditors, Values) || Values.Num() != 3)
        {
            return false;
        }

        const FRotator NewValue(Values[1], Values[2], Values[0]);
        FRotator CurrentValue = FRotator::ZeroRotator;
        if (Item->GetRotator(CurrentValue) && CurrentValue.Equals(NewValue, 0.001f))
        {
            return true;
        }

        if (!ApplyRotatorValueInternal(NewValue, false))
        {
            return false;
        }

        UpdateCachedDisplayState(Item->GetValueText(), bFavorited);
        UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] StructuredPreview %.2f ms | Property=%s"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *Item->GetPropertyFName().ToString());
        return true;
    }

    if (TransformEditorBox && TransformEditorBox->GetVisibility() == ESlateVisibility::Visible)
    {
        TArray<double> LocationValues;
        TArray<double> RotationValues;
        TArray<double> ScaleValues;
        if (!ExtractAxisValues(TransformLocationEditors, LocationValues)
            || !ExtractAxisValues(TransformRotationEditors, RotationValues)
            || !ExtractAxisValues(TransformScaleEditors, ScaleValues)
            || LocationValues.Num() != 3
            || RotationValues.Num() != 3
            || ScaleValues.Num() != 3)
        {
            return false;
        }

        const FVector NewLocation(LocationValues[0], LocationValues[1], LocationValues[2]);
        const FRotator NewRotation(RotationValues[1], RotationValues[2], RotationValues[0]);
        const FVector NewScale(ScaleValues[0], ScaleValues[1], ScaleValues[2]);
        const FTransform NewValue(NewRotation.Quaternion(), NewLocation, NewScale);

        FTransform CurrentValue = FTransform::Identity;
        if (Item->GetTransform(CurrentValue)
            && CurrentValue.GetLocation().Equals(NewLocation, 0.001f)
            && CurrentValue.GetScale3D().Equals(NewScale, 0.001f)
            && CurrentValue.Rotator().Equals(NewRotation, 0.001f))
        {
            return true;
        }

        if (!ApplyTransformValueInternal(NewValue, false))
        {
            return false;
        }

        UpdateCachedDisplayState(Item->GetValueText(), bFavorited);
        UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] StructuredPreview %.2f ms | Property=%s"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *Item->GetPropertyFName().ToString());
        return true;
    }

    return false;
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
    if (RI_ShouldSkipCommit(CommitMethod))
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

void UInspectorPropertyRowWidget::HandleStructuredAxisTextChanged(const FText& InText)
{
    MarkStructuredPreviewDirty();
}

void UInspectorPropertyRowWidget::HandleVectorXCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitVectorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleVectorYCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitVectorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleVectorZCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitVectorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleRotatorXCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitRotatorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleRotatorYCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitRotatorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleRotatorZCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitRotatorEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformLocationXCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformLocationYCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformLocationZCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformRotationXCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformRotationYCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformRotationZCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformScaleXCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformScaleYCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}

void UInspectorPropertyRowWidget::HandleTransformScaleZCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    if (!RI_ShouldSkipCommit(CommitMethod))
    {
        CommitTransformEditors();
    }
}
