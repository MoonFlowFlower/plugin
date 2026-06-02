#include "InspectorColorPickerWidget.h"

#include "InspectorCompactWidgetUtils.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Layout/SlateRect.h"
#include "Math/Color.h"
#include "Misc/DefaultValueHelper.h"
#include "Rendering/Texture2DResource.h"

namespace
{
    static constexpr int32 RI_ColorPickerSVTextureSize = 192;
    static constexpr int32 RI_ColorPickerHueTextureWidth = 16;
    static constexpr int32 RI_ColorPickerOpacityTextureWidth = 154;
    static constexpr int32 RI_ColorPickerStripHeight = 12;
    static constexpr float RI_ColorPickerSVWidgetSize = 130.0f;
    static constexpr float RI_ColorPickerHueWidgetWidth = 14.0f;
    static constexpr float RI_ColorPickerModalWidth = 350.0f;
    static constexpr float RI_ColorPickerModalHeight = 310.0f;
    static constexpr float RI_ColorPickerModalRadius = 8.0f;
    static constexpr float RI_ColorPickerBottomOffset = 28.0f;
    static constexpr float RI_ColorPickerViewportClampPadding = 8.0f;

    static void RI_SetRoundedBorder(UBorder* Border, const FLinearColor& FillColor, float Radius = 5.0f, float OutlineWidth = 0.0f, const FLinearColor& OutlineColor = FLinearColor::Transparent)
    {
        if (!Border)
        {
            return;
        }

        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(FillColor);
        Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        Brush.OutlineSettings.Width = OutlineWidth;
        Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Border->SetBrush(Brush);
    }

    static bool RI_UsesFixedRadius(const UBorder* Border)
    {
        return Border && Border->Background.OutlineSettings.RoundingType == ESlateBrushRoundingType::FixedRadius;
    }

    static void RI_SetImageTexture(UImage* Image, UTexture2D* Texture, const FVector2D& ImageSize)
    {
        if (!Image || !Texture)
        {
            return;
        }

        Image->SetBrushFromTexture(Texture, true);
        FSlateBrush Brush = Image->GetBrush();
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.ImageSize = ImageSize;
        Image->SetBrush(Brush);
        Image->SetDesiredSizeOverride(ImageSize);
    }

    static UTexture2D* RI_CreateTransientColorTexture(UObject* Outer, const FName& Name, int32 Width, int32 Height, const TArray<FColor>& Pixels)
    {
        if (!Outer || Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height)
        {
            return nullptr;
        }

        const FName UniqueTextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), Name);
        UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, UniqueTextureName);
        if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
        {
            return nullptr;
        }

        Texture->SRGB = true;
        Texture->NeverStream = true;
        FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
        void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
        Texture->UpdateResource();
        return Texture;
    }

    static FColor RI_CompositeOverChecker(const FLinearColor& InColor, float InAlpha, int32 X, int32 Y)
    {
        const bool bLightSquare = ((X / 6) + (Y / 6)) % 2 == 0;
        const FLinearColor Base = bLightSquare
            ? RICompactUI::MakeTokenColor(TEXT("D9DEE7"))
            : RICompactUI::MakeTokenColor(TEXT("758093"));
        FLinearColor Color = InColor;
        Color.A = 1.0f;
        return FLinearColor(
            Color.R * InAlpha + Base.R * (1.0f - InAlpha),
            Color.G * InAlpha + Base.G * (1.0f - InAlpha),
            Color.B * InAlpha + Base.B * (1.0f - InAlpha),
            1.0f).ToFColorSRGB();
    }

    static FString RI_FormatByte(float UnitValue)
    {
        return FString::FromInt(FMath::Clamp(FMath::RoundToInt(UnitValue * 255.0f), 0, 255));
    }

    static FString RI_FormatPercent(float UnitValue)
    {
        return FString::FromInt(FMath::Clamp(FMath::RoundToInt(UnitValue * 100.0f), 0, 100));
    }

    static void RI_AddVertical(UVerticalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.0f), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
    {
        if (!Box || !Child)
        {
            return;
        }

        if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child))
        {
            Slot->SetPadding(Padding);
            Slot->SetSize(FSlateChildSize(SizeRule));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    static void RI_AddHorizontal(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.0f), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
    {
        if (!Box || !Child)
        {
            return;
        }

        if (UHorizontalBoxSlot* Slot = Box->AddChildToHorizontalBox(Child))
        {
            Slot->SetPadding(Padding);
            Slot->SetSize(FSlateChildSize(SizeRule));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    static bool RI_TryGetWidgetScreenRect(const UWidget* Widget, FSlateRect& OutRect)
    {
        if (!Widget)
        {
            return false;
        }

        const FGeometry Geometry = Widget->GetCachedGeometry();
        const FVector2D LocalSize = Geometry.GetLocalSize();
        if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
        {
            return false;
        }

        const FVector2D TopLeft = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
        const FVector2D BottomRight = Geometry.LocalToAbsolute(LocalSize);
        OutRect = FSlateRect(
            FMath::Min(TopLeft.X, BottomRight.X),
            FMath::Min(TopLeft.Y, BottomRight.Y),
            FMath::Max(TopLeft.X, BottomRight.X),
            FMath::Max(TopLeft.Y, BottomRight.Y));
        return true;
    }
}

UInspectorColorPickerWidget::UInspectorColorPickerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

TSharedRef<SWidget> UInspectorColorPickerWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorColorPickerWidget::InitializePicker(const FLinearColor& InInitialColor, const TArray<FLinearColor>& InRecentColors)
{
    InitialColor = InInitialColor;
    RecentColors = InRecentColors;
    bHasBroadcastPreview = false;
    LastBroadcastPreviewColor = FLinearColor::Black;
    SetColorInternal(InInitialColor, false, true);
}

void UInspectorColorPickerWidget::SetPickerColor(const FLinearColor& InColor, bool bBroadcastPreview)
{
    SetColorInternal(InColor, bBroadcastPreview, true);
}

void UInspectorColorPickerWidget::SetRecentColors(const TArray<FLinearColor>& InRecentColors)
{
    RecentColors = InRecentColors;
    RefreshRecentSwatches();
}

void UInspectorColorPickerWidget::RefreshPickerForAutomation()
{
    RefreshAll(true);
}

bool UInspectorColorPickerWidget::IsNativeColorPickerReadyForAutomation() const
{
    return RootCanvas && ModalBorder && SaturationValueImage && HueImage && OpacityImage
        && HeaderRow
        && CurrentPreviewBorder && PreviousPreviewBorder && InputR && InputG && InputB && InputA
        && InputH && InputS && InputV && InputHsvA && InputHex && RecentSwatchBox && ApplyButton && CancelButton;
}

bool UInspectorColorPickerWidget::HasNativeColorPickerContractForAutomation() const
{
    return IsNativeColorPickerReadyForAutomation()
        && SaturationValueTexture
        && HueTexture
        && OpacityTexture
        && RgbInputsBox
        && HsvInputsBox
        && HasHitTestSafeDragLayersForAutomation()
        && HasFixedRadiusBrushesForAutomation()
        && HasCompactLayoutForAutomation();
}

bool UInspectorColorPickerWidget::SetRgbChannelTextForAutomation(int32 ChannelIndex, const FString& InText)
{
    UEditableTextBox* Target = nullptr;
    switch (ChannelIndex)
    {
    case 0: Target = InputR.Get(); break;
    case 1: Target = InputG.Get(); break;
    case 2: Target = InputB.Get(); break;
    case 3: Target = InputA.Get(); break;
    default: break;
    }

    if (!Target)
    {
        return false;
    }

    Target->SetText(FText::FromString(InText));
    ApplyRgbTextValues(true);
    return true;
}

bool UInspectorColorPickerWidget::SetHexTextForAutomation(const FString& InText)
{
    if (!InputHex)
    {
        return false;
    }

    InputHex->SetText(FText::FromString(InText));
    ApplyHexTextValue(InputHex->GetText(), true);
    return true;
}

bool UInspectorColorPickerWidget::ApplySaturationValueForAutomation(float InSaturation, float InValue)
{
    SetColorFromHSV(Hue, InSaturation, InValue, Alpha, true);
    return true;
}

bool UInspectorColorPickerWidget::ApplyHueForAutomation(float InHue)
{
    SetColorFromHSV(InHue, Saturation, Value, Alpha, true);
    return true;
}

bool UInspectorColorPickerWidget::ApplyOpacityForAutomation(float InAlpha)
{
    SetColorFromHSV(Hue, Saturation, Value, InAlpha, true);
    return true;
}

bool UInspectorColorPickerWidget::HasHitTestSafeDragLayersForAutomation() const
{
    return SaturationValueImage && SaturationValueImage->GetVisibility() == ESlateVisibility::HitTestInvisible
        && HueImage && HueImage->GetVisibility() == ESlateVisibility::HitTestInvisible
        && OpacityImage && OpacityImage->GetVisibility() == ESlateVisibility::HitTestInvisible
        && SaturationValueMarker && SaturationValueMarker->GetVisibility() == ESlateVisibility::HitTestInvisible
        && HueMarker && HueMarker->GetVisibility() == ESlateVisibility::HitTestInvisible
        && OpacityMarker && OpacityMarker->GetVisibility() == ESlateVisibility::HitTestInvisible;
}

bool UInspectorColorPickerWidget::HasFixedRadiusBrushesForAutomation() const
{
    return RI_UsesFixedRadius(ModalBorder.Get())
        && RI_UsesFixedRadius(CurrentPreviewBorder.Get())
        && RI_UsesFixedRadius(PreviousPreviewBorder.Get())
        && RI_UsesFixedRadius(SaturationValueMarker.Get())
        && RI_UsesFixedRadius(HueMarker.Get())
        && RI_UsesFixedRadius(OpacityMarker.Get());
}

bool UInspectorColorPickerWidget::HasCompactLayoutForAutomation() const
{
    return RI_ColorPickerModalWidth <= 370.0f
        && RI_ColorPickerModalHeight <= 320.0f
        && RI_ColorPickerSVWidgetSize <= 132.0f
        && RI_ColorPickerHueWidgetWidth <= 14.0f
        && RI_ColorPickerOpacityTextureWidth <= 156
        && RecentSwatchBox
        && RecentSwatchBox->GetChildrenCount() <= 6;
}

bool UInspectorColorPickerWidget::HasBottomSheetPlacementForAutomation() const
{
    const UCanvasPanelSlot* ModalSlot = GetModalCanvasSlot();
    if (!ModalSlot)
    {
        return false;
    }

    const FAnchors Anchors = ModalSlot->GetAnchors();
    const FVector2D Alignment = ModalSlot->GetAlignment();
    const FVector2D Size = ModalSlot->GetSize();
    const FVector2D Position = ModalSlot->GetPosition();
    return FMath::IsNearlyEqual(Anchors.Minimum.X, 0.5f)
        && FMath::IsNearlyEqual(Anchors.Minimum.Y, 1.0f)
        && FMath::IsNearlyEqual(Anchors.Maximum.X, 0.5f)
        && FMath::IsNearlyEqual(Anchors.Maximum.Y, 1.0f)
        && Alignment.Equals(FVector2D(0.5f, 1.0f), 0.01f)
        && Size.Equals(FVector2D(RI_ColorPickerModalWidth, RI_ColorPickerModalHeight), 0.5f)
        && FMath::Abs(Position.X) <= 0.5f
        && FMath::IsNearlyEqual(Position.Y, -RI_ColorPickerBottomOffset, 0.5f);
}

bool UInspectorColorPickerWidget::HasFooterClearanceForAutomation() const
{
    FSlateRect HexRect;
    FSlateRect CancelRect;
    FSlateRect ApplyRect;
    if (!RI_TryGetWidgetScreenRect(InputHex.Get(), HexRect)
        || !RI_TryGetWidgetScreenRect(CancelButton.Get(), CancelRect)
        || !RI_TryGetWidgetScreenRect(ApplyButton.Get(), ApplyRect))
    {
        return false;
    }

    const float FooterTop = FMath::Min(CancelRect.Top, ApplyRect.Top);
    return HexRect.Bottom + 4.0f <= FooterTop;
}

bool UInspectorColorPickerWidget::DragModalByForAutomation(const FVector2D& Delta)
{
    UCanvasPanelSlot* ModalSlot = GetModalCanvasSlot();
    if (!ModalSlot || Delta.IsNearlyZero())
    {
        return false;
    }

    const FVector2D Before = ModalSlot->GetPosition();
    ModalSlot->SetPosition(ClampModalPosition(Before + Delta));
    const FVector2D After = ModalSlot->GetPosition();
    return !After.Equals(Before, 0.5f);
}

void UInspectorColorPickerWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RI_ColorPickerRoot"));
    ModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ColorPickerModal"));
    ModalBorder->SetPadding(FMargin(10.0f, 8.0f));
    RI_SetRoundedBorder(
        ModalBorder,
        RICompactUI::GetPageBackgroundColor(),
        RI_ColorPickerModalRadius,
        RICompactUI::GetThemeMetrics().BorderWidth,
        RICompactUI::GetInputPalette(RICompactUI::ERIInputVisualStyle::Strong).Focused);

    UVerticalBox* ModalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerModalBox"));
    ModalBorder->SetContent(ModalBox);

    HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerHeader"));
    RI_AddHorizontal(HeaderRow, MakeLabel(TEXT("Color Picker"), 14, true), FMargin(0.0f), ESlateSizeRule::Fill);
    CloseButton = MakePickerButton(TEXT("RI_ColorPickerClose"), TEXT("X"), false);
    RI_AddHorizontal(HeaderRow, CloseButton, FMargin(8.0f, 0.0f, 0.0f, 0.0f));
    RI_AddVertical(ModalBox, HeaderRow, FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerBody"));

    UVerticalBox* LeftBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerLeft"));
    UHorizontalBox* SpectrumRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerSpectrumRow"));

    SaturationValueCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RI_ColorPickerSVCanvas"));
    SaturationValueImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RI_ColorPickerSVImage"));
    SaturationValueImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    SaturationValueMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ColorPickerSVMarker"));
    RI_SetRoundedBorder(SaturationValueMarker, FLinearColor::Transparent, 6.0f, 2.0f, RICompactUI::GetStrongTextColor());
    SaturationValueMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UCanvasPanelSlot* ImageSlot = SaturationValueCanvas->AddChildToCanvas(SaturationValueImage))
    {
        ImageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        ImageSlot->SetOffsets(FMargin(0.0f));
    }
    if (UCanvasPanelSlot* MarkerSlot = SaturationValueCanvas->AddChildToCanvas(SaturationValueMarker))
    {
        MarkerSlot->SetAutoSize(false);
        MarkerSlot->SetSize(FVector2D(12.0f, 12.0f));
        MarkerSlot->SetPosition(FVector2D::ZeroVector);
    }

    USizeBox* SVBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ColorPickerSVBox"));
    SVBox->SetWidthOverride(RI_ColorPickerSVWidgetSize);
    SVBox->SetHeightOverride(RI_ColorPickerSVWidgetSize);
    SVBox->SetContent(SaturationValueCanvas);
    RI_AddHorizontal(SpectrumRow, SVBox);

    HueCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RI_ColorPickerHueCanvas"));
    HueImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RI_ColorPickerHueImage"));
    HueImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    HueMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ColorPickerHueMarker"));
    RI_SetRoundedBorder(HueMarker, RICompactUI::GetStrongTextColor(), 2.0f);
    HueMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UCanvasPanelSlot* HueImageSlot = HueCanvas->AddChildToCanvas(HueImage))
    {
        HueImageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        HueImageSlot->SetOffsets(FMargin(0.0f));
    }
    if (UCanvasPanelSlot* HueMarkerSlot = HueCanvas->AddChildToCanvas(HueMarker))
    {
        HueMarkerSlot->SetAutoSize(false);
        HueMarkerSlot->SetSize(FVector2D(RI_ColorPickerHueWidgetWidth + 6.0f, 3.0f));
        HueMarkerSlot->SetPosition(FVector2D(-3.0f, 0.0f));
    }

    USizeBox* HueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ColorPickerHueBox"));
    HueBox->SetWidthOverride(RI_ColorPickerHueWidgetWidth);
    HueBox->SetHeightOverride(RI_ColorPickerSVWidgetSize);
    HueBox->SetContent(HueCanvas);
    RI_AddHorizontal(SpectrumRow, HueBox, FMargin(8.0f, 0.0f, 0.0f, 0.0f));
    RI_AddVertical(LeftBox, SpectrumRow);

    UTextBlock* OpacityLabel = MakeLabel(TEXT("Opacity"), RICompactUI::GetMutedFontSize(), false);
    RI_AddVertical(LeftBox, OpacityLabel, FMargin(0.0f, 3.0f, 0.0f, 1.0f));
    OpacityCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RI_ColorPickerOpacityCanvas"));
    OpacityImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RI_ColorPickerOpacityImage"));
    OpacityImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    OpacityMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ColorPickerOpacityMarker"));
    RI_SetRoundedBorder(OpacityMarker, RICompactUI::GetStrongTextColor(), 3.0f);
    OpacityMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UCanvasPanelSlot* OpacityImageSlot = OpacityCanvas->AddChildToCanvas(OpacityImage))
    {
        OpacityImageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        OpacityImageSlot->SetOffsets(FMargin(0.0f));
    }
    if (UCanvasPanelSlot* OpacityMarkerSlot = OpacityCanvas->AddChildToCanvas(OpacityMarker))
    {
        OpacityMarkerSlot->SetAutoSize(false);
        OpacityMarkerSlot->SetSize(FVector2D(4.0f, RI_ColorPickerStripHeight + 6.0f));
        OpacityMarkerSlot->SetPosition(FVector2D::ZeroVector);
    }
    USizeBox* OpacityBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ColorPickerOpacityBox"));
    OpacityBox->SetWidthOverride(static_cast<float>(RI_ColorPickerOpacityTextureWidth));
    OpacityBox->SetHeightOverride(static_cast<float>(RI_ColorPickerStripHeight));
    OpacityBox->SetContent(OpacityCanvas);
    RI_AddVertical(LeftBox, OpacityBox);

    RI_AddVertical(LeftBox, MakeLabel(TEXT("Recent Colors"), RICompactUI::GetMutedFontSize(), false), FMargin(0.0f, 3.0f, 0.0f, 2.0f));
    RecentSwatchBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerRecentSwatches"));
    RI_AddVertical(LeftBox, RecentSwatchBox);

    UVerticalBox* RightBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerRight"));
    UHorizontalBox* PreviewRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerPreviewRow"));
    UVerticalBox* CurrentPreviewStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    RI_AddVertical(CurrentPreviewStack, MakeLabel(TEXT("Preview"), RICompactUI::GetMutedFontSize(), false), FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    UBorder* CurrentPreviewRaw = nullptr;
    RI_AddVertical(CurrentPreviewStack, MakeColorBlock(TEXT("RI_ColorPickerCurrentPreview"), 34.0f, 34.0f, CurrentPreviewRaw));
    CurrentPreviewBorder = CurrentPreviewRaw;
    RI_AddHorizontal(PreviewRow, CurrentPreviewStack, FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    UVerticalBox* PreviousPreviewStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    RI_AddVertical(PreviousPreviewStack, MakeLabel(TEXT("Previous"), RICompactUI::GetMutedFontSize(), false), FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    UBorder* PreviousPreviewRaw = nullptr;
    RI_AddVertical(PreviousPreviewStack, MakeColorBlock(TEXT("RI_ColorPickerPreviousPreview"), 34.0f, 34.0f, PreviousPreviewRaw));
    PreviousPreviewBorder = PreviousPreviewRaw;
    RI_AddHorizontal(PreviewRow, PreviousPreviewStack);
    RI_AddVertical(RightBox, PreviewRow, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UVerticalBox* ModeStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerModeStack"));
    RgbModeButton = MakeModeButton(TEXT("RI_ColorPickerRgbMode"), TEXT("RGB"), ERIColorPickerInputMode::RGB);
    HsvModeButton = MakeModeButton(TEXT("RI_ColorPickerHsvMode"), TEXT("HSV"), ERIColorPickerInputMode::HSV);
    RI_AddVertical(ModeStack, RgbModeButton, FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(ModeStack, HsvModeButton);
    RI_AddVertical(RightBox, ModeStack, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    InputR = MakeInputBox(TEXT("InputTXT_R"), TEXT("255"));
    InputG = MakeInputBox(TEXT("InputTXT_G"), TEXT("0"));
    InputB = MakeInputBox(TEXT("InputTXT_B"), TEXT("0"));
    InputA = MakeInputBox(TEXT("InputTXT_A"), TEXT("255"));
    InputH = MakeInputBox(TEXT("InputTXT_H"), TEXT("0"));
    InputS = MakeInputBox(TEXT("InputTXT_S"), TEXT("100"));
    InputV = MakeInputBox(TEXT("InputTXT_V"), TEXT("100"));
    InputHsvA = MakeInputBox(TEXT("InputTXT_HSV_A"), TEXT("255"));
    InputHex = MakeInputBox(TEXT("InputTXT_SRGB"), TEXT("#FF0000FF"));

    RgbInputsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerRgbInputs"));
    RI_AddVertical(RgbInputsBox, MakeChannelRow(TEXT("R"), InputR, RICompactUI::GetAxisAccentColor(0)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(RgbInputsBox, MakeChannelRow(TEXT("G"), InputG, RICompactUI::GetAxisAccentColor(1)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(RgbInputsBox, MakeChannelRow(TEXT("B"), InputB, RICompactUI::GetAxisAccentColor(2)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(RgbInputsBox, MakeChannelRow(TEXT("A"), InputA, RICompactUI::GetSecondaryTextColor()), FMargin(0.0f, 0.0f, 0.0f, 3.0f));

    HsvInputsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ColorPickerHsvInputs"));
    RI_AddVertical(HsvInputsBox, MakeChannelRow(TEXT("H"), InputH, RICompactUI::GetAxisAccentColor(0)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(HsvInputsBox, MakeChannelRow(TEXT("S"), InputS, RICompactUI::GetAxisAccentColor(1)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(HsvInputsBox, MakeChannelRow(TEXT("V"), InputV, RICompactUI::GetAxisAccentColor(2)), FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    RI_AddVertical(HsvInputsBox, MakeChannelRow(TEXT("A"), InputHsvA, RICompactUI::GetSecondaryTextColor()), FMargin(0.0f, 0.0f, 0.0f, 3.0f));

    RI_AddVertical(RightBox, RgbInputsBox);
    RI_AddVertical(RightBox, HsvInputsBox);
    RI_AddVertical(RightBox, MakeLabel(TEXT("HEX"), RICompactUI::GetLabelFontSize(), false), FMargin(0.0f, 2.0f, 0.0f, 3.0f));
    USizeBox* HexInputSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ColorPickerHexInputSize"));
    HexInputSize->SetWidthOverride(142.0f);
    HexInputSize->SetHeightOverride(20.0f);
    HexInputSize->SetContent(InputHex);
    RI_AddVertical(RightBox, HexInputSize);

    USizeBox* LeftSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LeftSize->SetWidthOverride(168.0f);
    LeftSize->SetContent(LeftBox);
    RI_AddHorizontal(BodyRow, LeftSize, FMargin(0.0f, 0.0f, 12.0f, 0.0f));
    RI_AddHorizontal(BodyRow, RightBox, FMargin(0.0f), ESlateSizeRule::Fill);
    RI_AddVertical(ModalBox, BodyRow, FMargin(0.0f), ESlateSizeRule::Fill);

    UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ColorPickerFooter"));
    USizeBox* FooterSpacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    RI_AddHorizontal(FooterRow, FooterSpacer, FMargin(0.0f), ESlateSizeRule::Fill);
    CancelButton = MakePickerButton(TEXT("RI_ColorPickerCancel"), TEXT("Cancel"), false);
    ApplyButton = MakePickerButton(TEXT("RI_ColorPickerApply"), TEXT("Apply"), true);
    RI_AddHorizontal(FooterRow, CancelButton, FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    RI_AddHorizontal(FooterRow, ApplyButton);
    RI_AddVertical(ModalBox, FooterRow, FMargin(0.0f, 7.0f, 0.0f, 0.0f));

    if (UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(ModalBorder))
    {
        ModalSlot->SetAnchors(FAnchors(0.5f, 1.0f));
        ModalSlot->SetAlignment(FVector2D(0.5f, 1.0f));
        ModalSlot->SetSize(FVector2D(RI_ColorPickerModalWidth, RI_ColorPickerModalHeight));
        ModalSlot->SetPosition(FVector2D(0.0f, -RI_ColorPickerBottomOffset));
    }

    WidgetTree->RootWidget = RootCanvas;

    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleApplyClicked);
    }
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleCancelClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleCloseClicked);
    }
    if (RgbModeButton)
    {
        RgbModeButton->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRgbModeClicked);
    }
    if (HsvModeButton)
    {
        HsvModeButton->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleHsvModeClicked);
    }
    for (UEditableTextBox* TextBox : { InputR.Get(), InputG.Get(), InputB.Get(), InputA.Get() })
    {
        if (TextBox)
        {
            TextBox->OnTextCommitted.AddDynamic(this, &UInspectorColorPickerWidget::HandleRgbTextCommitted);
        }
    }
    for (UEditableTextBox* TextBox : { InputH.Get(), InputS.Get(), InputV.Get(), InputHsvA.Get() })
    {
        if (TextBox)
        {
            TextBox->OnTextCommitted.AddDynamic(this, &UInspectorColorPickerWidget::HandleHsvTextCommitted);
        }
    }
    if (InputHex)
    {
        InputHex->OnTextCommitted.AddDynamic(this, &UInspectorColorPickerWidget::HandleHexTextCommitted);
    }

    RefreshAll(true);
}

FReply UInspectorColorPickerWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (BeginPointerDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition()))
        {
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }

    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInspectorColorPickerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (BeginPointerDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition()))
        {
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInspectorColorPickerWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ActiveDragRegion != ERIColorPickerDragRegion::None && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        if (ActiveDragRegion == ERIColorPickerDragRegion::Move)
        {
            ApplyModalDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition());
        }
        else
        {
            ApplyDragAtScreenPosition(InMouseEvent.GetScreenSpacePosition(), ActiveDragRegion, true);
        }
        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInspectorColorPickerWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ActiveDragRegion != ERIColorPickerDragRegion::None)
    {
        ActiveDragRegion = ERIColorPickerDragRegion::None;
        return FReply::Handled().ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInspectorColorPickerWidget::RefreshAll(bool bRegenerateTextures)
{
    RefreshTextFields();
    RefreshTextureBrushes(bRegenerateTextures);
    RefreshMarkers();
    RefreshInputModeVisibility();
    RefreshPreviewBlocks();
    RefreshRecentSwatches();
}

void UInspectorColorPickerWidget::RefreshTextFields()
{
    TGuardValue<bool> GuardUpdating(bUpdatingFields, true);
    if (InputR)
    {
        InputR->SetText(FText::FromString(RI_FormatByte(CurrentColor.R)));
    }
    if (InputG)
    {
        InputG->SetText(FText::FromString(RI_FormatByte(CurrentColor.G)));
    }
    if (InputB)
    {
        InputB->SetText(FText::FromString(RI_FormatByte(CurrentColor.B)));
    }
    if (InputA)
    {
        InputA->SetText(FText::FromString(RI_FormatByte(CurrentColor.A)));
    }
    if (InputHsvA)
    {
        InputHsvA->SetText(FText::FromString(RI_FormatByte(CurrentColor.A)));
    }
    if (InputH)
    {
        InputH->SetText(FText::FromString(FString::FromInt(FMath::Clamp(FMath::RoundToInt(Hue), 0, 360))));
    }
    if (InputS)
    {
        InputS->SetText(FText::FromString(RI_FormatPercent(Saturation)));
    }
    if (InputV)
    {
        InputV->SetText(FText::FromString(RI_FormatPercent(Value)));
    }
    if (InputHex)
    {
        InputHex->SetText(FText::FromString(ToHexText(CurrentColor)));
    }
}

void UInspectorColorPickerWidget::RefreshTextureBrushes(bool bRegenerateTextures)
{
    if (bRegenerateTextures || !SaturationValueTexture)
    {
        SaturationValueTexture = BuildSaturationValueTexture();
    }
    if (bRegenerateTextures || !HueTexture)
    {
        HueTexture = BuildHueTexture();
    }
    if (bRegenerateTextures || !OpacityTexture)
    {
        OpacityTexture = BuildOpacityTexture();
    }

    RI_SetImageTexture(SaturationValueImage.Get(), SaturationValueTexture.Get(), FVector2D(RI_ColorPickerSVWidgetSize, RI_ColorPickerSVWidgetSize));
    RI_SetImageTexture(HueImage.Get(), HueTexture.Get(), FVector2D(RI_ColorPickerHueWidgetWidth, RI_ColorPickerSVWidgetSize));
    RI_SetImageTexture(OpacityImage.Get(), OpacityTexture.Get(), FVector2D(static_cast<float>(RI_ColorPickerOpacityTextureWidth), static_cast<float>(RI_ColorPickerStripHeight)));
}

void UInspectorColorPickerWidget::RefreshMarkers()
{
    if (SaturationValueMarker)
    {
        if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(SaturationValueMarker->Slot))
        {
            MarkerSlot->SetPosition(FVector2D(
                Saturation * RI_ColorPickerSVWidgetSize - 6.0f,
                (1.0f - Value) * RI_ColorPickerSVWidgetSize - 6.0f));
        }
    }

    if (HueMarker)
    {
        if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(HueMarker->Slot))
        {
            const float Y = (1.0f - (Hue / 360.0f)) * RI_ColorPickerSVWidgetSize;
            MarkerSlot->SetPosition(FVector2D(-3.0f, FMath::Clamp(Y - 1.5f, 0.0f, RI_ColorPickerSVWidgetSize - 3.0f)));
        }
    }

    if (OpacityMarker)
    {
        if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(OpacityMarker->Slot))
        {
            const float X = Alpha * static_cast<float>(RI_ColorPickerOpacityTextureWidth);
            MarkerSlot->SetPosition(FVector2D(FMath::Clamp(X - 2.0f, 0.0f, static_cast<float>(RI_ColorPickerOpacityTextureWidth) - 4.0f), -3.0f));
        }
    }
}

void UInspectorColorPickerWidget::RefreshInputModeVisibility()
{
    if (RgbInputsBox)
    {
        RgbInputsBox->SetVisibility(InputMode == ERIColorPickerInputMode::RGB ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (HsvInputsBox)
    {
        HsvInputsBox->SetVisibility(InputMode == ERIColorPickerInputMode::HSV ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (RgbModeButton)
    {
        RICompactUI::ConfigureButton(
            RgbModeButton,
            InputMode == ERIColorPickerInputMode::RGB ? RICompactUI::ERIButtonVisualStyle::TabActive : RICompactUI::ERIButtonVisualStyle::TabInactive,
            true);
    }
    if (HsvModeButton)
    {
        RICompactUI::ConfigureButton(
            HsvModeButton,
            InputMode == ERIColorPickerInputMode::HSV ? RICompactUI::ERIButtonVisualStyle::TabActive : RICompactUI::ERIButtonVisualStyle::TabInactive,
            true);
    }
}

void UInspectorColorPickerWidget::RefreshPreviewBlocks()
{
    if (CurrentPreviewBorder)
    {
        RI_SetRoundedBorder(CurrentPreviewBorder, CurrentColor, 3.0f);
    }
    if (PreviousPreviewBorder)
    {
        RI_SetRoundedBorder(PreviousPreviewBorder, InitialColor, 3.0f);
    }
}

void UInspectorColorPickerWidget::RefreshRecentSwatches()
{
    if (!RecentSwatchBox || !WidgetTree)
    {
        return;
    }

    RecentSwatchBox->ClearChildren();
    const int32 MaxSwatches = 6;
    const int32 VisibleSwatches = FMath::Clamp(FMath::Max(RecentColors.Num(), 4), 1, MaxSwatches);
    for (int32 Index = 0; Index < VisibleSwatches; ++Index)
    {
        const FLinearColor SwatchColor = RecentColors.IsValidIndex(Index)
            ? RecentColors[Index]
            : RICompactUI::GetInputPalette(RICompactUI::ERIInputVisualStyle::Muted).ReadOnly;
        UButton* SwatchButton = MakeRecentColorButton(Index, SwatchColor);
        if (!SwatchButton)
        {
            continue;
        }

        RI_AddHorizontal(RecentSwatchBox, SwatchButton, FMargin(0.0f, 0.0f, 6.0f, 0.0f));
    }
}

UTexture2D* UInspectorColorPickerWidget::BuildSaturationValueTexture()
{
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(RI_ColorPickerSVTextureSize * RI_ColorPickerSVTextureSize);

    for (int32 Y = 0; Y < RI_ColorPickerSVTextureSize; ++Y)
    {
        const float LocalValue = 1.0f - static_cast<float>(Y) / static_cast<float>(RI_ColorPickerSVTextureSize - 1);
        for (int32 X = 0; X < RI_ColorPickerSVTextureSize; ++X)
        {
            const float LocalSaturation = static_cast<float>(X) / static_cast<float>(RI_ColorPickerSVTextureSize - 1);
            Pixels[Y * RI_ColorPickerSVTextureSize + X] = HSVToLinearRgb(Hue, LocalSaturation, LocalValue, 1.0f).ToFColorSRGB();
        }
    }

    return RI_CreateTransientColorTexture(this, TEXT("RI_ColorPickerSVTexture"), RI_ColorPickerSVTextureSize, RI_ColorPickerSVTextureSize, Pixels);
}

UTexture2D* UInspectorColorPickerWidget::BuildHueTexture()
{
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(RI_ColorPickerHueTextureWidth * RI_ColorPickerSVTextureSize);

    for (int32 Y = 0; Y < RI_ColorPickerSVTextureSize; ++Y)
    {
        const float LocalHue = (1.0f - static_cast<float>(Y) / static_cast<float>(RI_ColorPickerSVTextureSize - 1)) * 360.0f;
        const FColor HueColor = HSVToLinearRgb(LocalHue, 1.0f, 1.0f, 1.0f).ToFColorSRGB();
        for (int32 X = 0; X < RI_ColorPickerHueTextureWidth; ++X)
        {
            Pixels[Y * RI_ColorPickerHueTextureWidth + X] = HueColor;
        }
    }

    return RI_CreateTransientColorTexture(this, TEXT("RI_ColorPickerHueTexture"), RI_ColorPickerHueTextureWidth, RI_ColorPickerSVTextureSize, Pixels);
}

UTexture2D* UInspectorColorPickerWidget::BuildOpacityTexture()
{
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(RI_ColorPickerOpacityTextureWidth * RI_ColorPickerStripHeight);

    FLinearColor OpaqueColor = CurrentColor;
    OpaqueColor.A = 1.0f;
    for (int32 Y = 0; Y < RI_ColorPickerStripHeight; ++Y)
    {
        for (int32 X = 0; X < RI_ColorPickerOpacityTextureWidth; ++X)
        {
            const float LocalAlpha = static_cast<float>(X) / static_cast<float>(RI_ColorPickerOpacityTextureWidth - 1);
            Pixels[Y * RI_ColorPickerOpacityTextureWidth + X] = RI_CompositeOverChecker(OpaqueColor, LocalAlpha, X, Y);
        }
    }

    return RI_CreateTransientColorTexture(this, TEXT("RI_ColorPickerOpacityTexture"), RI_ColorPickerOpacityTextureWidth, RI_ColorPickerStripHeight, Pixels);
}

void UInspectorColorPickerWidget::SetColorFromHSV(float InHue, float InSaturation, float InValue, float InAlpha, bool bBroadcastPreview)
{
    Hue = FMath::Fmod(FMath::Max(0.0f, InHue), 360.0f);
    Saturation = FMath::Clamp(InSaturation, 0.0f, 1.0f);
    Value = FMath::Clamp(InValue, 0.0f, 1.0f);
    Alpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
    CurrentColor = HSVToLinearRgb(Hue, Saturation, Value, Alpha);
    RefreshAll(true);
    if (bBroadcastPreview)
    {
        BroadcastPreviewIfNeeded();
    }
}

void UInspectorColorPickerWidget::SetColorInternal(const FLinearColor& InColor, bool bBroadcastPreview, bool bRegenerateTextures)
{
    CurrentColor = InColor.GetClamped();
    LinearRgbToHSV(CurrentColor, Hue, Saturation, Value);
    Alpha = FMath::Clamp(CurrentColor.A, 0.0f, 1.0f);
    RefreshAll(bRegenerateTextures);
    if (bBroadcastPreview)
    {
        BroadcastPreviewIfNeeded();
    }
}

void UInspectorColorPickerWidget::BroadcastPreviewIfNeeded()
{
    if (bHasBroadcastPreview && CurrentColor.Equals(LastBroadcastPreviewColor, KINDA_SMALL_NUMBER))
    {
        return;
    }

    bHasBroadcastPreview = true;
    LastBroadcastPreviewColor = CurrentColor;
    OnPreviewColorChanged.Broadcast(CurrentColor);
}

bool UInspectorColorPickerWidget::BeginPointerDragAtScreenPosition(const FVector2D& ScreenPosition)
{
    if (IsWidgetUnderScreenPosition(SaturationValueCanvas.Get(), ScreenPosition))
    {
        ActiveDragRegion = ERIColorPickerDragRegion::SaturationValue;
    }
    else if (IsWidgetUnderScreenPosition(HueCanvas.Get(), ScreenPosition))
    {
        ActiveDragRegion = ERIColorPickerDragRegion::Hue;
    }
    else if (IsWidgetUnderScreenPosition(OpacityCanvas.Get(), ScreenPosition))
    {
        ActiveDragRegion = ERIColorPickerDragRegion::Opacity;
    }
    else if (IsWidgetUnderScreenPosition(CloseButton.Get(), ScreenPosition))
    {
        ActiveDragRegion = ERIColorPickerDragRegion::None;
    }
    else if (IsWidgetUnderScreenPosition(HeaderRow.Get(), ScreenPosition))
    {
        ActiveDragRegion = ERIColorPickerDragRegion::Move;
        ModalDragStartScreenPosition = ScreenPosition;
        if (UCanvasPanelSlot* ModalSlot = GetModalCanvasSlot())
        {
            ModalDragStartSlotPosition = ModalSlot->GetPosition();
        }
        return true;
    }
    else
    {
        ActiveDragRegion = ERIColorPickerDragRegion::None;
    }

    if (ActiveDragRegion == ERIColorPickerDragRegion::None)
    {
        return false;
    }

    ApplyDragAtScreenPosition(ScreenPosition, ActiveDragRegion, true);
    return true;
}

bool UInspectorColorPickerWidget::ApplyModalDragAtScreenPosition(const FVector2D& ScreenPosition)
{
    UCanvasPanelSlot* ModalSlot = GetModalCanvasSlot();
    if (!ModalSlot)
    {
        return false;
    }

    const FVector2D Delta = ScreenPosition - ModalDragStartScreenPosition;
    ModalSlot->SetPosition(ClampModalPosition(ModalDragStartSlotPosition + Delta));
    return true;
}

bool UInspectorColorPickerWidget::ApplyDragAtScreenPosition(const FVector2D& ScreenPosition, ERIColorPickerDragRegion Region, bool bBroadcastPreview)
{
    if (Region == ERIColorPickerDragRegion::SaturationValue && SaturationValueCanvas)
    {
        const FGeometry Geometry = SaturationValueCanvas->GetCachedGeometry();
        const FVector2D Local = Geometry.AbsoluteToLocal(ScreenPosition);
        const FVector2D Size = Geometry.GetLocalSize();
        if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
        {
            return false;
        }

        SetColorFromHSV(
            Hue,
            FMath::Clamp(Local.X / Size.X, 0.0f, 1.0f),
            FMath::Clamp(1.0f - Local.Y / Size.Y, 0.0f, 1.0f),
            Alpha,
            bBroadcastPreview);
        return true;
    }

    if (Region == ERIColorPickerDragRegion::Hue && HueCanvas)
    {
        const FGeometry Geometry = HueCanvas->GetCachedGeometry();
        const FVector2D Local = Geometry.AbsoluteToLocal(ScreenPosition);
        const FVector2D Size = Geometry.GetLocalSize();
        if (Size.Y <= KINDA_SMALL_NUMBER)
        {
            return false;
        }

        SetColorFromHSV(
            FMath::Clamp(1.0f - Local.Y / Size.Y, 0.0f, 1.0f) * 360.0f,
            Saturation,
            Value,
            Alpha,
            bBroadcastPreview);
        return true;
    }

    if (Region == ERIColorPickerDragRegion::Opacity && OpacityCanvas)
    {
        const FGeometry Geometry = OpacityCanvas->GetCachedGeometry();
        const FVector2D Local = Geometry.AbsoluteToLocal(ScreenPosition);
        const FVector2D Size = Geometry.GetLocalSize();
        if (Size.X <= KINDA_SMALL_NUMBER)
        {
            return false;
        }

        SetColorFromHSV(Hue, Saturation, Value, FMath::Clamp(Local.X / Size.X, 0.0f, 1.0f), bBroadcastPreview);
        return true;
    }

    return false;
}

bool UInspectorColorPickerWidget::IsWidgetUnderScreenPosition(const UWidget* Widget, const FVector2D& ScreenPosition) const
{
    if (!Widget)
    {
        return false;
    }

    const FGeometry Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPosition = Geometry.AbsoluteToLocal(ScreenPosition);
    const FVector2D LocalSize = Geometry.GetLocalSize();
    return LocalPosition.X >= 0.0f && LocalPosition.Y >= 0.0f
        && LocalPosition.X <= LocalSize.X && LocalPosition.Y <= LocalSize.Y;
}

UCanvasPanelSlot* UInspectorColorPickerWidget::GetModalCanvasSlot() const
{
    return ModalBorder ? Cast<UCanvasPanelSlot>(ModalBorder->Slot) : nullptr;
}

FVector2D UInspectorColorPickerWidget::ClampModalPosition(const FVector2D& DesiredPosition) const
{
    const UCanvasPanelSlot* ModalSlot = GetModalCanvasSlot();
    if (!RootCanvas || !ModalSlot)
    {
        return DesiredPosition;
    }

    const FVector2D ViewportSize = RootCanvas->GetCachedGeometry().GetLocalSize();
    const FVector2D ModalSize = ModalSlot->GetSize();
    if (ViewportSize.X <= KINDA_SMALL_NUMBER || ViewportSize.Y <= KINDA_SMALL_NUMBER
        || ModalSize.X <= KINDA_SMALL_NUMBER || ModalSize.Y <= KINDA_SMALL_NUMBER)
    {
        return DesiredPosition;
    }

    const float MinX = -ViewportSize.X * 0.5f + ModalSize.X * 0.5f + RI_ColorPickerViewportClampPadding;
    const float MaxX = ViewportSize.X * 0.5f - ModalSize.X * 0.5f - RI_ColorPickerViewportClampPadding;
    const float MinY = -ViewportSize.Y + ModalSize.Y + RI_ColorPickerViewportClampPadding;
    const float MaxY = -RI_ColorPickerViewportClampPadding;
    return FVector2D(
        FMath::Clamp(DesiredPosition.X, MinX, MaxX),
        FMath::Clamp(DesiredPosition.Y, MinY, MaxY));
}

void UInspectorColorPickerWidget::ApplyRgbTextValues(bool bBroadcastPreview)
{
    if (bUpdatingFields)
    {
        return;
    }

    float NewR = CurrentColor.R;
    float NewG = CurrentColor.G;
    float NewB = CurrentColor.B;
    float NewA = CurrentColor.A;
    bool bChanged = false;
    bChanged |= ParseRgbChannelText(InputR ? InputR->GetText() : FText::GetEmpty(), CurrentColor.R, NewR);
    bChanged |= ParseRgbChannelText(InputG ? InputG->GetText() : FText::GetEmpty(), CurrentColor.G, NewG);
    bChanged |= ParseRgbChannelText(InputB ? InputB->GetText() : FText::GetEmpty(), CurrentColor.B, NewB);
    bChanged |= ParseRgbChannelText(InputA ? InputA->GetText() : FText::GetEmpty(), CurrentColor.A, NewA);
    if (!bChanged)
    {
        RefreshTextFields();
        return;
    }

    SetColorInternal(FLinearColor(NewR, NewG, NewB, NewA), bBroadcastPreview, true);
}

void UInspectorColorPickerWidget::ApplyHsvTextValues(bool bBroadcastPreview)
{
    if (bUpdatingFields)
    {
        return;
    }

    float NewHue = Hue;
    float NewSaturation = Saturation;
    float NewValue = Value;
    float NewAlpha = Alpha;
    bool bChanged = false;
    bChanged |= ParseHsvChannelText(InputH ? InputH->GetText() : FText::GetEmpty(), 0, Hue, NewHue);
    bChanged |= ParseHsvChannelText(InputS ? InputS->GetText() : FText::GetEmpty(), 1, Saturation, NewSaturation);
    bChanged |= ParseHsvChannelText(InputV ? InputV->GetText() : FText::GetEmpty(), 2, Value, NewValue);
    bChanged |= ParseRgbChannelText(InputHsvA ? InputHsvA->GetText() : FText::GetEmpty(), Alpha, NewAlpha);
    if (!bChanged)
    {
        RefreshTextFields();
        return;
    }

    SetColorFromHSV(NewHue, NewSaturation, NewValue, NewAlpha, bBroadcastPreview);
}

void UInspectorColorPickerWidget::ApplyHexTextValue(const FText& InText, bool bBroadcastPreview)
{
    if (bUpdatingFields)
    {
        return;
    }

    FLinearColor ParsedColor;
    if (!ParseHexText(InText, ParsedColor))
    {
        RefreshTextFields();
        return;
    }

    SetColorInternal(ParsedColor, bBroadcastPreview, true);
}

void UInspectorColorPickerWidget::LinearRgbToHSV(const FLinearColor& InColor, float& OutHue, float& OutSaturation, float& OutValue)
{
    const float R = FMath::Clamp(InColor.R, 0.0f, 1.0f);
    const float G = FMath::Clamp(InColor.G, 0.0f, 1.0f);
    const float B = FMath::Clamp(InColor.B, 0.0f, 1.0f);
    const float MaxChannel = FMath::Max3(R, G, B);
    const float MinChannel = FMath::Min3(R, G, B);
    const float Delta = MaxChannel - MinChannel;

    OutValue = MaxChannel;
    OutSaturation = MaxChannel <= KINDA_SMALL_NUMBER ? 0.0f : Delta / MaxChannel;
    if (Delta <= KINDA_SMALL_NUMBER)
    {
        OutHue = 0.0f;
        return;
    }

    if (FMath::IsNearlyEqual(MaxChannel, R))
    {
        OutHue = 60.0f * FMath::Fmod(((G - B) / Delta), 6.0f);
    }
    else if (FMath::IsNearlyEqual(MaxChannel, G))
    {
        OutHue = 60.0f * (((B - R) / Delta) + 2.0f);
    }
    else
    {
        OutHue = 60.0f * (((R - G) / Delta) + 4.0f);
    }

    if (OutHue < 0.0f)
    {
        OutHue += 360.0f;
    }
}

FLinearColor UInspectorColorPickerWidget::HSVToLinearRgb(float InHue, float InSaturation, float InValue, float InAlpha)
{
    const float NormalizedHue = FMath::Fmod(FMath::Max(0.0f, InHue), 360.0f);
    const float C = FMath::Clamp(InValue, 0.0f, 1.0f) * FMath::Clamp(InSaturation, 0.0f, 1.0f);
    const float X = C * (1.0f - FMath::Abs(FMath::Fmod(NormalizedHue / 60.0f, 2.0f) - 1.0f));
    const float M = FMath::Clamp(InValue, 0.0f, 1.0f) - C;

    float RPrime = 0.0f;
    float GPrime = 0.0f;
    float BPrime = 0.0f;
    if (NormalizedHue < 60.0f)
    {
        RPrime = C;
        GPrime = X;
    }
    else if (NormalizedHue < 120.0f)
    {
        RPrime = X;
        GPrime = C;
    }
    else if (NormalizedHue < 180.0f)
    {
        GPrime = C;
        BPrime = X;
    }
    else if (NormalizedHue < 240.0f)
    {
        GPrime = X;
        BPrime = C;
    }
    else if (NormalizedHue < 300.0f)
    {
        RPrime = X;
        BPrime = C;
    }
    else
    {
        RPrime = C;
        BPrime = X;
    }

    return FLinearColor(RPrime + M, GPrime + M, BPrime + M, FMath::Clamp(InAlpha, 0.0f, 1.0f));
}

FString UInspectorColorPickerWidget::ToHexText(const FLinearColor& InColor)
{
    return FString::Printf(TEXT("#%s"), *InColor.ToFColorSRGB().ToHex());
}

bool UInspectorColorPickerWidget::ParseHexText(const FText& InText, FLinearColor& OutColor)
{
    FString HexString = InText.ToString();
    HexString.TrimStartAndEndInline();
    HexString.RemoveFromStart(TEXT("#"));

    if (!(HexString.Len() == 6 || HexString.Len() == 8))
    {
        return false;
    }

    for (const TCHAR Char : HexString)
    {
        if (!FChar::IsHexDigit(Char))
        {
            return false;
        }
    }

    if (HexString.Len() == 6)
    {
        HexString.Append(TEXT("FF"));
    }

    OutColor = FLinearColor::FromSRGBColor(FColor::FromHex(HexString));
    return true;
}

bool UInspectorColorPickerWidget::ParseRgbChannelText(const FText& InText, float CurrentValue, float& OutValue)
{
    FString ValueString = InText.ToString();
    ValueString.TrimStartAndEndInline();
    if (ValueString.IsEmpty())
    {
        OutValue = CurrentValue;
        return false;
    }

    double ParsedValue = 0.0;
    if (!FDefaultValueHelper::ParseDouble(ValueString, ParsedValue))
    {
        OutValue = CurrentValue;
        return false;
    }

    if (ValueString.Contains(TEXT(".")) && ParsedValue <= 1.0)
    {
        OutValue = FMath::Clamp(static_cast<float>(ParsedValue), 0.0f, 1.0f);
    }
    else
    {
        OutValue = FMath::Clamp(static_cast<float>(ParsedValue / 255.0), 0.0f, 1.0f);
    }
    return true;
}

bool UInspectorColorPickerWidget::ParseHsvChannelText(const FText& InText, int32 ChannelIndex, float CurrentValue, float& OutValue)
{
    FString ValueString = InText.ToString();
    ValueString.TrimStartAndEndInline();
    if (ValueString.IsEmpty())
    {
        OutValue = CurrentValue;
        return false;
    }

    double ParsedValue = 0.0;
    if (!FDefaultValueHelper::ParseDouble(ValueString, ParsedValue))
    {
        OutValue = CurrentValue;
        return false;
    }

    if (ChannelIndex == 0)
    {
        OutValue = FMath::Clamp(static_cast<float>(ParsedValue), 0.0f, 360.0f);
    }
    else
    {
        OutValue = FMath::Clamp(static_cast<float>(ParsedValue / 100.0), 0.0f, 1.0f);
    }
    return true;
}

UTextBlock* UInspectorColorPickerWidget::MakeLabel(const FString& Label, int32 Size, bool bBold) const
{
    UTextBlock* Text = RICompactUI::MakeText(WidgetTree, Label, Size, bBold, RICompactUI::GetStrongTextColor());
    RICompactUI::ConfigureEllipsisText(Text, Label);
    return Text;
}

UEditableTextBox* UInspectorColorPickerWidget::MakeInputBox(const FName& Name, const FString& InitialText) const
{
    UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), Name);
    TextBox->SetText(FText::FromString(InitialText));
    TextBox->SetJustification(ETextJustify::Center);
    TextBox->SetSelectAllTextWhenFocused(true);
    TextBox->SetMinDesiredWidth(58.0f);
    RICompactUI::ConfigureEditableTextBox(TextBox, RICompactUI::GetStrongTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Strong);
    return TextBox;
}

UButton* UInspectorColorPickerWidget::MakePickerButton(const FName& Name, const FString& Label, bool bPrimary) const
{
    const bool bCloseButton = Name == TEXT("RI_ColorPickerClose");
    return RICompactUI::MakeLabeledButton(
        WidgetTree,
        Name,
        Label,
        bPrimary ? RICompactUI::ERIButtonVisualStyle::Primary : RICompactUI::ERIButtonVisualStyle::Secondary,
        bCloseButton ? 28.0f : (bPrimary ? 96.0f : 84.0f),
        bCloseButton ? 22.0f : 24.0f,
        RICompactUI::GetLabelFontSize());
}

UWidget* UInspectorColorPickerWidget::MakeColorBlock(const FName& Name, float Width, float Height, UBorder*& OutBorder) const
{
    OutBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    RI_SetRoundedBorder(OutBorder, RICompactUI::GetInputPalette(RICompactUI::ERIInputVisualStyle::Muted).Background, 3.0f);

    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SizeBox->SetWidthOverride(Width);
    SizeBox->SetHeightOverride(Height);
    SizeBox->SetContent(OutBorder);
    return SizeBox;
}

UWidget* UInspectorColorPickerWidget::MakeChannelRow(const FString& Label, UEditableTextBox* Input, const FLinearColor& LabelColor) const
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* LabelText = RICompactUI::MakeText(WidgetTree, Label, RICompactUI::GetLabelFontSize(), true, LabelColor);
    LabelText->SetJustification(ETextJustify::Center);
    USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelBox->SetWidthOverride(20.0f);
    LabelBox->SetContent(LabelText);
    RICompactUI::CenterSizeBoxContent(LabelBox);
    RI_AddHorizontal(Row, LabelBox, FMargin(0.0f, 0.0f, 4.0f, 0.0f));
    USizeBox* InputSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    InputSize->SetWidthOverride(88.0f);
    InputSize->SetHeightOverride(18.0f);
    InputSize->SetContent(Input);
    RI_AddHorizontal(Row, InputSize);
    return Row;
}

UButton* UInspectorColorPickerWidget::MakeModeButton(const FName& Name, const FString& Label, ERIColorPickerInputMode Mode) const
{
    return RICompactUI::MakeLabeledButton(
        WidgetTree,
        Name,
        Label,
        Mode == InputMode ? RICompactUI::ERIButtonVisualStyle::TabActive : RICompactUI::ERIButtonVisualStyle::TabInactive,
        0.0f,
        21.0f,
        RICompactUI::GetValueFontSize());
}

UButton* UInspectorColorPickerWidget::MakeRecentColorButton(int32 Index, const FLinearColor& Color) const
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(FString::Printf(TEXT("RI_ColorPickerRecent_%d"), Index)));
    RICompactUI::ConfigureSwatchButton(Button);

    UBorder* SwatchBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RI_SetRoundedBorder(SwatchBorder, Color, 3.0f, 1.0f, RICompactUI::GetInputPalette(RICompactUI::ERIInputVisualStyle::Strong).Focused);
    SwatchBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SizeBox->SetWidthOverride(16.0f);
    SizeBox->SetHeightOverride(16.0f);
    SizeBox->SetContent(SwatchBorder);
    SizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
    Button->AddChild(SizeBox);

    switch (Index)
    {
    case 0: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor0Clicked); break;
    case 1: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor1Clicked); break;
    case 2: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor2Clicked); break;
    case 3: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor3Clicked); break;
    case 4: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor4Clicked); break;
    case 5: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor5Clicked); break;
    case 6: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor6Clicked); break;
    case 7: Button->OnClicked.AddDynamic(this, &UInspectorColorPickerWidget::HandleRecentColor7Clicked); break;
    default: break;
    }

    return Button;
}

void UInspectorColorPickerWidget::HandleApplyClicked()
{
    OnColorAccepted.Broadcast(CurrentColor);
}

void UInspectorColorPickerWidget::HandleCancelClicked()
{
    OnColorCanceled.Broadcast();
}

void UInspectorColorPickerWidget::HandleCloseClicked()
{
    OnColorCanceled.Broadcast();
}

void UInspectorColorPickerWidget::HandleRgbTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    ApplyRgbTextValues(true);
}

void UInspectorColorPickerWidget::HandleHsvTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    ApplyHsvTextValues(true);
}

void UInspectorColorPickerWidget::HandleHexTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    ApplyHexTextValue(InText, true);
}

void UInspectorColorPickerWidget::HandleRgbModeClicked()
{
    InputMode = ERIColorPickerInputMode::RGB;
    RefreshInputModeVisibility();
}

void UInspectorColorPickerWidget::HandleHsvModeClicked()
{
    InputMode = ERIColorPickerInputMode::HSV;
    RefreshInputModeVisibility();
}

void UInspectorColorPickerWidget::HandleRecentColorClicked(int32 Index)
{
    if (!RecentColors.IsValidIndex(Index))
    {
        return;
    }

    SetColorInternal(RecentColors[Index], true, true);
}

void UInspectorColorPickerWidget::HandleRecentColor0Clicked()
{
    HandleRecentColorClicked(0);
}

void UInspectorColorPickerWidget::HandleRecentColor1Clicked()
{
    HandleRecentColorClicked(1);
}

void UInspectorColorPickerWidget::HandleRecentColor2Clicked()
{
    HandleRecentColorClicked(2);
}

void UInspectorColorPickerWidget::HandleRecentColor3Clicked()
{
    HandleRecentColorClicked(3);
}

void UInspectorColorPickerWidget::HandleRecentColor4Clicked()
{
    HandleRecentColorClicked(4);
}

void UInspectorColorPickerWidget::HandleRecentColor5Clicked()
{
    HandleRecentColorClicked(5);
}

void UInspectorColorPickerWidget::HandleRecentColor6Clicked()
{
    HandleRecentColorClicked(6);
}

void UInspectorColorPickerWidget::HandleRecentColor7Clicked()
{
    HandleRecentColorClicked(7);
}
