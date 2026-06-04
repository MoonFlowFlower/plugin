#include "InspectorDockRootWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorFilePageWidget.h"
#include "InspectorFunctionsSectionWidget.h"
#include "InspectorPropertiesSectionWidget.h"
#include "InspectorSettingsPageWidget.h"
#include "InspectorTestPageWidget.h"
#include "RuntimeInspector.h"
#include "RuntimeInspectorController.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "InspectorWorldSubsystem.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"

namespace
{
    static constexpr float RI_DockExpandedSidePanelMinPhysicalWidth = 320.0f;
    static constexpr float RI_DockExpandedSidePanelMaxPhysicalWidth = 360.0f;
    static constexpr float RI_DockExpandedSidePanelPhysicalWidthRatio = 0.20f;
    static constexpr float RI_DockLeftCompactPhysicalWidth = 64.0f;
    static constexpr float RI_DockOuterPadding = 0.0f;
    static constexpr float RI_DockPanelGapPhysical = 16.0f;
    static constexpr float RI_DockMinimumExpandedCenterPhysicalWidth = 520.0f;
    static constexpr float RI_DockFavoritesFramePhysicalHeight = 190.0f;
    static constexpr float RI_DockFunctionsFramePhysicalHeight = 220.0f;
    static constexpr float RI_DockPanelBorderPhysicalThickness = 1.0f;
    static constexpr float RI_DockPanelBlurStrength = 8.0f;
    static constexpr float RI_DockPanelGridTileSize = 256.0f;
    static constexpr float RI_DockHeaderBarPhysicalHeight = 40.0f;
    static constexpr float RI_DockRightHeaderStackPhysicalGap = 8.0f;

    static FString RI_TabLabel(ERIInspectorTab Tab)
    {
        switch (Tab)
        {
        case ERIInspectorTab::Changes:
            return TEXT("Changes");
        case ERIInspectorTab::Settings:
            return TEXT("Settings");
        case ERIInspectorTab::Tools:
            return TEXT("Tools");
        case ERIInspectorTab::Actor:
        default:
            return TEXT("Actor");
        }
    }

    static void RI_AddVertical(UVerticalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.f), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
    {
        if (!Box || !Child)
        {
            return;
        }

        if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child))
        {
            Slot->SetPadding(Padding);
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
            Slot->SetSize(FSlateChildSize(SizeRule));
        }
    }

    static void RI_AddHorizontal(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.f), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
    {
        if (!Box || !Child)
        {
            return;
        }

        if (UHorizontalBoxSlot* Slot = Box->AddChildToHorizontalBox(Child))
        {
            Slot->SetPadding(Padding);
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
            Slot->SetSize(FSlateChildSize(SizeRule));
        }
    }

    static void RI_AddHorizontalCentered(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.f), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
    {
        if (!Box || !Child)
        {
            return;
        }

        if (UHorizontalBoxSlot* Slot = Box->AddChildToHorizontalBox(Child))
        {
            Slot->SetPadding(Padding);
            Slot->SetHorizontalAlignment(SizeRule == ESlateSizeRule::Fill ? HAlign_Fill : HAlign_Center);
            Slot->SetVerticalAlignment(VAlign_Center);
            Slot->SetSize(FSlateChildSize(SizeRule));
        }
    }

    static void RI_AddOverlayFill(UOverlay* Overlay, UWidget* Child, const FMargin& Padding = FMargin(0.f))
    {
        if (!Overlay || !Child)
        {
            return;
        }

        if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Child))
        {
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
            Slot->SetPadding(Padding);
        }
    }

    static FLinearColor RI_GetDockPanelBorderColor()
    {
        return FLinearColor(0.196f, 0.784f, 1.000f, 0.75f);
    }

    static FLinearColor RI_GetDockPanelWashColor()
    {
        return FLinearColor(0.001518f, 0.001518f, 0.001518f, 0.0f);
    }

    static FLinearColor RI_GetDockPanelBaseColor()
    {
        return FLinearColor(0.001518f, 0.001518f, 0.001518f, 0.70f);
    }

    static FLinearColor RI_GetDockPanelGridColor()
    {
        return FLinearColor(0.196f, 0.784f, 1.000f, 0.20f);
    }

    static FLinearColor RI_GetDockHeaderBarColor()
    {
        return FLinearColor(0.022f, 0.024f, 0.026f, 0.86f);
    }

    static FLinearColor RI_GetDockSectionSurfaceColor()
    {
        return FLinearColor(0.001518f, 0.001518f, 0.001518f, 0.06f);
    }

    static FLinearColor RI_GetDockRowSurfaceColor()
    {
        return FLinearColor(0.001518f, 0.001518f, 0.001518f, 0.12f);
    }

    static FLinearColor RI_GetDockSelectedRowSurfaceColor()
    {
        return FLinearColor(0.045f, 0.180f, 0.280f, 0.36f);
    }

    static FMargin RI_GetDockBodyMargin(float Top = 0.f, float Bottom = 0.f)
    {
        const FMargin PanelPadding = RICompactUI::GetPanelPadding();
        return FMargin(PanelPadding.Left, Top, PanelPadding.Right, Bottom);
    }

    static FMargin RI_GetDockSectionTitleColumnMargin(float Top = 0.f, float Bottom = 0.f)
    {
        const FMargin PanelPadding = RICompactUI::GetPanelPadding();
        const FMargin SectionPadding = RICompactUI::GetSurfaceCardPadding(true);
        return FMargin(PanelPadding.Left + SectionPadding.Left, Top, PanelPadding.Right + SectionPadding.Right, Bottom);
    }

    static float RI_GetDockHeaderIconColumnInset()
    {
        const FMargin PanelPadding = RICompactUI::GetPanelPadding();
        return PanelPadding.Left;
    }

    static void RI_AlignBorderContentCenterFill(UBorder* Border)
    {
        if (Border)
        {
            if (UBorderSlot* ContentSlot = Cast<UBorderSlot>(Border->GetContentSlot()))
            {
                ContentSlot->SetHorizontalAlignment(HAlign_Fill);
                ContentSlot->SetVerticalAlignment(VAlign_Center);
            }
        }
    }

    static USizeBox* RI_WrapDockHeaderBar(UWidgetTree* WidgetTree, UWidget* HeaderWidget, const FName& Name)
    {
        if (!WidgetTree || !HeaderWidget)
        {
            return nullptr;
        }

        USizeBox* HeaderFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
        HeaderFrame->SetHeightOverride(RI_DockHeaderBarPhysicalHeight);
        HeaderFrame->SetContent(HeaderWidget);
        if (USizeBoxSlot* HeaderSlot = Cast<USizeBoxSlot>(HeaderFrame->GetContentSlot()))
        {
            HeaderSlot->SetPadding(FMargin(0.f));
            HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
            HeaderSlot->SetVerticalAlignment(VAlign_Fill);
        }
        return HeaderFrame;
    }

    static bool RI_DockWidgetExists(const UWidgetTree* WidgetTree, const TCHAR* WidgetName)
    {
        return WidgetTree && WidgetName && WidgetTree->FindWidget(FName(WidgetName)) != nullptr;
    }

    static UBorder* RI_MakePanelSurface(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FName& EdgeBorderName,
        const FName& BlurName,
        const FName& BaseName,
        const FName& WashName,
        const FName& GridName,
        EHorizontalAlignment EdgeAlignment,
        UWidget* ContentWidget)
    {
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Border->SetPadding(FMargin(0.f));
        Border->SetBrushColor(FLinearColor::Transparent);
        Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        UOverlay* ChromeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
        ChromeOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Border->SetContent(ChromeOverlay);

        UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), BlurName);
        Blur->SetBlurStrength(RI_DockPanelBlurStrength);
        Blur->SetApplyAlphaToBlur(true);
        FSlateBrush BlurFallbackBrush;
        BlurFallbackBrush.DrawAs = ESlateBrushDrawType::Box;
        BlurFallbackBrush.TintColor = FSlateColor(RI_GetDockPanelBaseColor());
        Blur->SetLowQualityFallbackBrush(BlurFallbackBrush);
        Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
        RI_AddOverlayFill(ChromeOverlay, Blur);

        UBorder* Base = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BaseName);
        Base->SetPadding(FMargin(0.f));
        Base->SetBrushColor(RI_GetDockPanelBaseColor());
        Base->SetVisibility(ESlateVisibility::HitTestInvisible);
        RI_AddOverlayFill(ChromeOverlay, Base);

        UBorder* Wash = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), WashName);
        Wash->SetPadding(FMargin(0.f));
        Wash->SetBrushColor(RI_GetDockPanelWashColor());
        Wash->SetVisibility(ESlateVisibility::HitTestInvisible);
        RI_AddOverlayFill(ChromeOverlay, Wash);

        UImage* Grid = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), GridName);
        if (UTexture2D* GridTexture = RICompactUI::LoadIconTexture(TEXT("grid_white_tiling_256")))
        {
            Grid->SetBrushFromTexture(GridTexture, false);
        }
        FSlateBrush GridBrush = Grid->GetBrush();
        GridBrush.DrawAs = ESlateBrushDrawType::Image;
        GridBrush.Tiling = ESlateBrushTileType::Both;
        GridBrush.ImageSize = FVector2D(RI_DockPanelGridTileSize, RI_DockPanelGridTileSize);
        Grid->SetBrush(GridBrush);
        Grid->SetColorAndOpacity(RI_GetDockPanelGridColor());
        Grid->SetVisibility(ESlateVisibility::HitTestInvisible);
        RI_AddOverlayFill(ChromeOverlay, Grid);

        if (ContentWidget)
        {
            ContentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            RI_AddOverlayFill(ChromeOverlay, ContentWidget);
        }

        const FName EdgeBoxName(*(EdgeBorderName.ToString() + TEXT("Size")));
        USizeBox* EdgeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), EdgeBoxName);
        EdgeBox->SetWidthOverride(RI_DockPanelBorderPhysicalThickness);
        EdgeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
        UBorder* EdgeBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), EdgeBorderName);
        EdgeBorder->SetPadding(FMargin(0.f));
        EdgeBorder->SetBrushColor(RI_GetDockPanelBorderColor());
        EdgeBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
        EdgeBox->SetContent(EdgeBorder);
        if (UOverlaySlot* EdgeSlot = ChromeOverlay->AddChildToOverlay(EdgeBox))
        {
            EdgeSlot->SetHorizontalAlignment(EdgeAlignment);
            EdgeSlot->SetVerticalAlignment(VAlign_Fill);
        }

        return Border;
    }

    static UBorder* RI_MakeSectionCard(UWidgetTree* WidgetTree, const FName& Name)
    {
        return RICompactUI::MakeSurfaceCard(WidgetTree, Name, RI_GetDockSectionSurfaceColor(), RICompactUI::GetSurfaceCardPadding());
    }

    static float RI_GetDockViewportScale(UWidget* Widget)
    {
        const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(Widget);
        return ViewportScale > KINDA_SMALL_NUMBER ? ViewportScale : 1.0f;
    }

    static FVector2D RI_GetDockPhysicalViewportSize(UWidget* Widget)
    {
        return UWidgetLayoutLibrary::GetViewportSize(Widget);
    }

    static float RI_PhysicalToLogical(float PhysicalSize, float ViewportScale)
    {
        return PhysicalSize / FMath::Max(ViewportScale, 0.01f);
    }

    static float RI_GetDockReadableScale(float ViewportScale)
    {
        return FMath::Clamp(1.0f / FMath::Max(ViewportScale, 0.01f), 1.0f, 1.25f);
    }

    static float RI_GetDockExpandedSidePanelPhysicalWidth(float PhysicalViewportWidth)
    {
        if (PhysicalViewportWidth <= 1.0f)
        {
            return RI_DockExpandedSidePanelMinPhysicalWidth;
        }

        return FMath::Clamp(
            PhysicalViewportWidth * RI_DockExpandedSidePanelPhysicalWidthRatio,
            RI_DockExpandedSidePanelMinPhysicalWidth,
            RI_DockExpandedSidePanelMaxPhysicalWidth);
    }

    static float RI_GetDockCenterPhysicalWidth(float PhysicalViewportWidth, bool bLeftCompact)
    {
        const float LeftWidth = bLeftCompact ? RI_DockLeftCompactPhysicalWidth : RI_GetDockExpandedSidePanelPhysicalWidth(PhysicalViewportWidth);
        return PhysicalViewportWidth - LeftWidth - RI_GetDockExpandedSidePanelPhysicalWidth(PhysicalViewportWidth) - (RI_DockPanelGapPhysical * 2.0f);
    }

    static bool RI_ShouldUseCompactLeftPanel(float PhysicalViewportWidth)
    {
        return PhysicalViewportWidth > 1.0f
            && RI_GetDockCenterPhysicalWidth(PhysicalViewportWidth, false) < RI_DockMinimumExpandedCenterPhysicalWidth;
    }

    static void RI_UpdateDockReadableScale(UWidget* Widget)
    {
        RICompactUI::SetReadableScaleOverride(RI_GetDockReadableScale(RI_GetDockViewportScale(Widget)));
    }

    static void RI_SetNamedSizeBoxHeight(UWidgetTree* WidgetTree, const FName& Name, float HeightOverride)
    {
        if (!WidgetTree)
        {
            return;
        }

        if (USizeBox* SizeBox = Cast<USizeBox>(WidgetTree->FindWidget(Name)))
        {
            SizeBox->SetHeightOverride(HeightOverride);
        }
    }

    static USizeBox* RI_MakeIconBox(UWidgetTree* WidgetTree, const FName& Name, const TCHAR* IconAssetName, float IconSize, const FLinearColor& Tint);

    static UHorizontalBox* RI_MakeIconLabel(
        UWidgetTree* WidgetTree,
        const TCHAR* IconAssetName,
        const FString& Label,
        float IconSize,
        int32 FontSize,
        bool bBold,
        const FLinearColor& TextColor,
        const FLinearColor& IconColor)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        if (USizeBox* Icon = RI_MakeIconBox(WidgetTree, NAME_None, IconAssetName, IconSize, IconColor))
        {
            RI_AddHorizontal(Row, Icon, FMargin(0.f, 0.f, RICompactUI::GetInlineGap() + 2.f, 0.f));
        }
        RI_AddHorizontal(Row, RICompactUI::MakeText(WidgetTree, Label, FontSize, bBold, TextColor, true), FMargin(0.f), ESlateSizeRule::Fill);
        return Row;
    }

    static UBorder* RI_MakeIconSectionTitle(
        UWidgetTree* WidgetTree,
        const TCHAR* IconAssetName,
        const FString& Label,
        RICompactUI::ERISectionVisualStyle Style,
        const FName& Name)
    {
        const RICompactUI::FRIThemeMetrics& Metrics = RICompactUI::GetThemeMetrics();
        const RICompactUI::FRISectionPalette Palette = RICompactUI::GetSectionPalette(Style);
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Border->SetPadding(Metrics.SectionPadding);
        Border->SetBrushColor(Palette.Background);
        Border->SetContent(RI_MakeIconLabel(
            WidgetTree,
            IconAssetName,
            Label,
            12.0f,
            RICompactUI::GetSectionTitleFontSize(),
            true,
            Palette.Text,
            Palette.Text));
        return Border;
    }

    static USizeBox* RI_MakeIconBox(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const TCHAR* IconAssetName,
        float IconSize,
        const FLinearColor& Tint)
    {
        UWidget* Icon = RICompactUI::MakeIconWidget(WidgetTree, Name, IconAssetName, IconSize, Tint);
        if (!Icon)
        {
            return nullptr;
        }

        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        IconBox->SetWidthOverride(IconSize);
        IconBox->SetHeightOverride(IconSize);
        IconBox->SetContent(Icon);
        RICompactUI::CenterSizeBoxContent(IconBox);
        return IconBox;
    }

}

void UInspectorDockFunctionRunProxy::HandleClicked()
{
    if (Owner)
    {
        Owner->HandleFunctionRunProxyClicked(FunctionName);
    }
}

void UInspectorDockPatchActionProxy::HandleRevertClicked()
{
    if (Owner)
    {
        Owner->HandlePatchRevertProxyClicked(PatchId);
    }
}

void UInspectorDockComponentActionProxy::HandleClicked()
{
    if (Owner)
    {
        Owner->HandleComponentProxyClicked(ComponentName);
    }
}

void UInspectorDockFavoriteActionProxy::HandleClicked()
{
    if (Owner)
    {
        if (bToggleFavorite)
        {
            Owner->HandleFavoriteToggleProxyClicked(SourceItem);
        }
        else
        {
            Owner->HandleFavoriteProxyClicked(SourceItem);
        }
    }
}

void UInspectorDockRootWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTreeIfNeeded();
}

void UInspectorDockRootWidget::SetController(URuntimeInspectorController* InController)
{
    Controller = InController;
    BuildWidgetTreeIfNeeded();
}

void UInspectorDockRootWidget::BuildWidgetTreeIfNeeded()
{
    if (bWidgetTreeBuilt || !WidgetTree)
    {
        return;
    }

    RI_UpdateDockReadableScale(this);

    RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_DockRootOverlay"));
    RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = RootOverlay;

    UBorder* BackgroundLayer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_DockTransparentBackground"));
    BackgroundLayer->SetBrushColor(FLinearColor::Transparent);
    BackgroundLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(BackgroundLayer))
    {
        BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
        BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
        BackgroundSlot->SetPadding(FMargin(0.f));
    }

    DockBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_DockMainLayout"));
    DockBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (UOverlaySlot* DockSlot = RootOverlay->AddChildToOverlay(DockBox))
    {
        DockSlot->SetHorizontalAlignment(HAlign_Fill);
        DockSlot->SetVerticalAlignment(VAlign_Fill);
        DockSlot->SetPadding(FMargin(RI_DockOuterPadding));
    }

    const float InitialViewportScale = RI_GetDockViewportScale(this);
    const FVector2D InitialPhysicalViewportSize = RI_GetDockPhysicalViewportSize(this);
    const float InitialExpandedSideLogicalWidth = RI_PhysicalToLogical(RI_GetDockExpandedSidePanelPhysicalWidth(InitialPhysicalViewportSize.X), InitialViewportScale);
    const float InitialPanelGapLogical = RI_PhysicalToLogical(RI_DockPanelGapPhysical, InitialViewportScale);

    LeftPanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_DockLeftPanelSize"));
    LeftPanelSizeBox->SetWidthOverride(InitialExpandedSideLogicalWidth);
    LeftPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_DockLeftPanel"));
    UBorder* LeftSurface = RI_MakePanelSurface(
        WidgetTree,
        TEXT("RI_DockLeftPanelSurface"),
        TEXT("RI_DockLeftPanelViewportBorder"),
        TEXT("RI_DockLeftPanelBackgroundBlur"),
        TEXT("RI_DockLeftPanelBackgroundBase"),
        TEXT("RI_DockLeftPanelBackgroundWash"),
        TEXT("RI_DockLeftPanelBackgroundGrid"),
        HAlign_Right,
        LeftPanelBox);
    LeftPanelSizeBox->SetContent(LeftSurface);
    RI_AddHorizontal(DockBox, LeftPanelSizeBox, FMargin(0.f, 0.f, InitialPanelGapLogical, 0.f));
    BuildLeftPanel(LeftPanelBox);

    UOverlay* CenterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_DockViewportOverlay"));
    CenterOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    RI_AddHorizontal(DockBox, CenterOverlay, FMargin(0.f), ESlateSizeRule::Fill);
    BuildCenterOverlay(CenterOverlay);

    RightPanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_DockRightPanelSize"));
    RightPanelSizeBox->SetWidthOverride(InitialExpandedSideLogicalWidth);
    RightPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_DockRightInspector"));
    UBorder* RightSurface = RI_MakePanelSurface(
        WidgetTree,
        TEXT("RI_DockRightPanelSurface"),
        TEXT("RI_DockRightPanelViewportBorder"),
        TEXT("RI_DockRightPanelBackgroundBlur"),
        TEXT("RI_DockRightPanelBackgroundBase"),
        TEXT("RI_DockRightPanelBackgroundWash"),
        TEXT("RI_DockRightPanelBackgroundGrid"),
        HAlign_Left,
        RightPanelBox);
    RightPanelSizeBox->SetContent(RightSurface);
    RI_AddHorizontal(DockBox, RightPanelSizeBox, FMargin(InitialPanelGapLogical, 0.f, 0.f, 0.f));
    BuildRightInspector(RightPanelBox);

    UOverlay* ModalLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_DockModalToastLayer"));
    ModalLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot* ModalSlot = RootOverlay->AddChildToOverlay(ModalLayer))
    {
        ModalSlot->SetHorizontalAlignment(HAlign_Fill);
        ModalSlot->SetVerticalAlignment(VAlign_Fill);
    }

    bWidgetTreeBuilt = true;
    RefreshLayoutForViewport();
}

void UInspectorDockRootWidget::BuildLeftPanel(UVerticalBox* OutPanel)
{
    if (!OutPanel || !WidgetTree)
    {
        return;
    }

    UBorder* ActorCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_SelectedActorCard"));
    ActorCard->SetBrushColor(RI_GetDockHeaderBarColor());
    ActorCard->SetPadding(FMargin(RI_GetDockHeaderIconColumnInset(), 2.f, 6.f, 2.f));
    UVerticalBox* ActorCardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    ActorCard->SetContent(ActorCardBox);
    UHorizontalBox* ActorTopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SelectedActorTopRow"));
    if (USizeBox* ActorIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_SelectedActorIcon"), TEXT("shape:object"), 16.0f, RICompactUI::GetSuccessTextColor()))
    {
        RI_AddHorizontal(ActorTopRow, ActorIcon, FMargin(0.f, 0.f, RICompactUI::GetInlineGap() + 3.f, 0.f));
    }
    UVerticalBox* ActorTextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SelectedActorTextStack"));
    ActorNameText = MakeBoundText(TEXT("RI_SelectedActorName"));
    ActorClassText = MakeBoundText(TEXT("RI_SelectedActorClass"));
    ActorPathText = MakeBoundText(TEXT("RI_SelectedActorPath"));
    ActorNameText->SetAutoWrapText(false);
    ActorClassText->SetAutoWrapText(false);
    ActorPathText->SetAutoWrapText(false);
    RI_AddVertical(ActorTextStack, ActorNameText);
    RI_AddVertical(ActorTextStack, ActorClassText, FMargin(0.f, 2.f, 0.f, 0.f));
    RI_AddHorizontal(ActorTopRow, ActorTextStack, FMargin(0.f), ESlateSizeRule::Fill);
    RI_AddVertical(ActorCardBox, ActorTopRow);
    RI_AddVertical(ActorCardBox, ActorPathText, FMargin(0.f, 1.f, 0.f, 0.f));
    RI_AlignBorderContentCenterFill(ActorCard);
    RI_AddVertical(OutPanel, RI_WrapDockHeaderBar(WidgetTree, ActorCard, TEXT("RI_SelectedActorHeaderFrame")), FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

    UBorder* StagedBanner = RI_MakeSectionCard(WidgetTree, TEXT("RI_StagedStateBanner"));
    StagedBanner->SetBrushColor(RICompactUI::GetContextStatusCellBackgroundColor());
    UHorizontalBox* StagedRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_StagedStateRow"));
    if (USizeBox* StagedIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_StagedStateIcon"), TEXT("shape:status"), 13.0f, RICompactUI::GetWarningTextColor()))
    {
        RI_AddHorizontal(StagedRow, StagedIcon, FMargin(0.f, 0.f, RICompactUI::GetInlineGap() + 2.f, 0.f));
    }
    StagedBannerText = MakeBoundText(TEXT("RI_StagedStateText"));
    RI_AddHorizontal(StagedRow, StagedBannerText, FMargin(0.f), ESlateSizeRule::Fill);
    StagedBanner->SetContent(StagedRow);
    RI_AddVertical(OutPanel, StagedBanner, RI_GetDockBodyMargin(0.f, RICompactUI::GetSectionGap()));

    SearchTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_SearchBar"));
    RICompactUI::ConfigureEditableTextBox(SearchTextBox, RICompactUI::GetStrongTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Muted);
    SearchTextBox->SetHintText(FText::FromString(TEXT("Search actor context")));
    SearchTextBox->OnTextChanged.AddDynamic(this, &UInspectorDockRootWidget::HandleSearchTextChanged);
    UBorder* SearchSurface = RI_MakeSectionCard(WidgetTree, TEXT("RI_SearchBarSurface"));
    SearchSurface->SetBrushColor(RICompactUI::GetContextSecondaryCellBackgroundColor());
    UHorizontalBox* SearchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SearchBarRow"));
    if (USizeBox* SearchIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_SearchIcon"), TEXT("shape:search"), 13.0f, RICompactUI::GetMutedTextColor()))
    {
        RI_AddHorizontal(SearchRow, SearchIcon, FMargin(0.f, 3.f, RICompactUI::GetInlineGap() + 2.f, 0.f));
    }
    RI_AddHorizontal(SearchRow, RICompactUI::WrapValueControl(WidgetTree, SearchTextBox), FMargin(0.f), ESlateSizeRule::Fill);
    SearchSurface->SetContent(SearchRow);
    RI_AddVertical(OutPanel, SearchSurface, RI_GetDockBodyMargin(0.f, RICompactUI::GetSectionGap()));

    ComponentTitleWidget = RI_MakeIconSectionTitle(WidgetTree, TEXT("shape:components"), TEXT("Components"), RICompactUI::ERISectionVisualStyle::Standard, TEXT("RI_ComponentTreeTitle"));
    RI_AddVertical(OutPanel, ComponentTitleWidget, RI_GetDockBodyMargin());
    UBorder* ComponentCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_ComponentTreeCard"));
    UScrollBox* ComponentScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ComponentTreeScroll"));
    ComponentListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ComponentTree"));
    ComponentScrollBox->AddChild(ComponentListBox);
    ComponentCard->SetContent(ComponentScrollBox);
    RI_AddVertical(OutPanel, ComponentCard, RI_GetDockBodyMargin(2.f, RICompactUI::GetSectionGap()), ESlateSizeRule::Fill);

    FavoritesFrameSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_FavoritesFrameSize"));
    FavoritesFrameSizeBox->SetHeightOverride(RI_DockFavoritesFramePhysicalHeight);
    FavoritesFrameSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
    UVerticalBox* FavoritesFrameBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FavoritesFrame"));
    FavoritesFrameSizeBox->SetContent(FavoritesFrameBox);

    FavoritesTitleWidget = RI_MakeIconSectionTitle(WidgetTree, TEXT("shape:star-outline"), TEXT("Favorites"), RICompactUI::ERISectionVisualStyle::Standard, TEXT("RI_FavoritesTitle"));
    RI_AddVertical(FavoritesFrameBox, FavoritesTitleWidget);
    UBorder* FavoritesCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_FavoritesCard"));
    FavoritesCard->SetClipping(EWidgetClipping::ClipToBounds);
    FavoritesScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_FavoritesScroll"));
    FavoritesListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FavoritesPanel"));
    FavoritesScrollBox->AddChild(FavoritesListBox);
    FavoritesCard->SetContent(FavoritesScrollBox);
    RI_AddVertical(FavoritesFrameBox, FavoritesCard, FMargin(0.f, 2.f, 0.f, 0.f), ESlateSizeRule::Fill);
    RI_AddVertical(OutPanel, FavoritesFrameSizeBox, RI_GetDockBodyMargin());
}

void UInspectorDockRootWidget::BuildCenterOverlay(UOverlay* OutOverlay)
{
    if (!OutOverlay || !WidgetTree)
    {
        return;
    }
}

void UInspectorDockRootWidget::BuildRightInspector(UVerticalBox* OutPanel)
{
    if (!OutPanel || !WidgetTree)
    {
        return;
    }

    UBorder* HeaderSurface = RI_MakeSectionCard(WidgetTree, TEXT("RI_RightInspectorHeaderSurface"));
    HeaderSurface->SetBrushColor(RI_GetDockHeaderBarColor());
    HeaderSurface->SetPadding(FMargin(RI_GetDockHeaderIconColumnInset(), 0.f, 6.f, 0.f));
    UOverlay* HeaderOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_RightInspectorHeaderOverlay"));
    if (USizeBox* HeaderIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_RightInspectorHeaderIcon"), TEXT("shape:object"), 16.0f, RICompactUI::GetSuccessTextColor()))
    {
        if (UOverlaySlot* IconSlot = HeaderOverlay->AddChildToOverlay(HeaderIcon))
        {
            IconSlot->SetHorizontalAlignment(HAlign_Left);
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetPadding(FMargin(0.f));
        }
    }
    HeaderText = MakeBoundText(TEXT("RI_RightInspectorHeader"));
    RICompactUI::ApplyTextStyle(HeaderText, RICompactUI::GetSectionTitleFontSize() + 3, true, RICompactUI::GetStrongTextColor());
    HeaderText->SetAutoWrapText(false);
    HeaderText->SetJustification(ETextJustify::Left);
    if (UOverlaySlot* TextSlot = HeaderOverlay->AddChildToOverlay(HeaderText))
    {
        TextSlot->SetHorizontalAlignment(HAlign_Left);
        TextSlot->SetVerticalAlignment(VAlign_Center);
        TextSlot->SetPadding(FMargin(16.f + RICompactUI::GetInlineGap() + 3.f, 0.f, 0.f, 0.f));
    }
    HeaderSurface->SetContent(HeaderOverlay);
    RI_AlignBorderContentCenterFill(HeaderSurface);
    RI_AddVertical(OutPanel, RI_WrapDockHeaderBar(WidgetTree, HeaderSurface, TEXT("RI_RightInspectorHeaderFrame")), FMargin(0.f, 0.f, 0.f, RI_DockRightHeaderStackPhysicalGap));

    TabButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_TabBar"));
    ActorTabButton = MakeDockButton(TEXT("RI_TabActor"), TEXT("Actor"), false, 0.0f, RICompactUI::GetMutedFontSize() + 1);
    ChangesTabButton = MakeDockButton(TEXT("RI_TabChanges"), TEXT("Changes"), false, 0.0f, RICompactUI::GetMutedFontSize() + 1);
    SettingsTabButton = MakeDockButton(TEXT("RI_TabSettings"), TEXT("Settings"), false, 0.0f, RICompactUI::GetMutedFontSize() + 1);
    ToolsTabButton = MakeDockButton(TEXT("RI_TabTools"), TEXT("Tools"), false, 0.0f, RICompactUI::GetMutedFontSize() + 1);
    ActorTabButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleActorTabClicked);
    ChangesTabButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleChangesTabClicked);
    SettingsTabButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleSettingsTabClicked);
    ToolsTabButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleToolsTabClicked);
    RI_AddHorizontal(TabButtonBox, ActorTabButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f), ESlateSizeRule::Fill);
    RI_AddHorizontal(TabButtonBox, ChangesTabButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f), ESlateSizeRule::Fill);
    RI_AddHorizontal(TabButtonBox, SettingsTabButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f), ESlateSizeRule::Fill);
    RI_AddHorizontal(TabButtonBox, ToolsTabButton, FMargin(0.f), ESlateSizeRule::Fill);
    RI_AddVertical(OutPanel, TabButtonBox, RI_GetDockSectionTitleColumnMargin(0.f, RI_DockRightHeaderStackPhysicalGap));

    TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("RI_TabContent"));
    ActorTabPageBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorTabPage"));
    ChangesTabScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ChangesTabScroll"));
    SettingsHostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsTabHost"));
    ToolsHostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsTabHost"));
    BuildActorTab(ActorTabPageBox);
    BuildChangesTab(ChangesTabScrollBox);
    TabSwitcher->AddChild(ActorTabPageBox);
    TabSwitcher->AddChild(ChangesTabScrollBox);
    TabSwitcher->AddChild(SettingsHostBox);
    TabSwitcher->AddChild(ToolsHostBox);
    RI_AddVertical(OutPanel, TabSwitcher, RI_GetDockBodyMargin(0.f, RICompactUI::GetSectionGap()), ESlateSizeRule::Fill);

    ActionBarBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActionBar"));
    UHorizontalBox* ActionTopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ActionBarTopRow"));
    UHorizontalBox* ActionBottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ActionBarBottomRow"));
    OnlyModifyCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RI_OnlyModify"));
    RICompactUI::ConfigureCheckBox(OnlyModifyCheckBox);
    OnlyModifyCheckBox->OnCheckStateChanged.AddDynamic(this, &UInspectorDockRootWidget::HandleOnlyModifyChanged);
    UHorizontalBox* OnlyModifyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    USizeBox* OnlyModifyCheckSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_OnlyModifyCheckSize"));
    OnlyModifyCheckSize->SetWidthOverride(16.0f);
    OnlyModifyCheckSize->SetHeightOverride(16.0f);
    OnlyModifyCheckSize->SetContent(OnlyModifyCheckBox);
    RICompactUI::CenterSizeBoxContent(OnlyModifyCheckSize);
    RI_AddHorizontalCentered(OnlyModifyBox, OnlyModifyCheckSize);
    RI_AddHorizontalCentered(OnlyModifyBox, RICompactUI::MakeText(WidgetTree, TEXT("Only Modify"), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetSecondaryTextColor()), FMargin(4.f, 0.f, 0.f, 0.f));
    RI_AddHorizontalCentered(ActionTopRow, OnlyModifyBox, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f), ESlateSizeRule::Fill);

    const int32 ActionFontSize = RICompactUI::GetMutedFontSize();
    UButton* RefreshButton = MakeDockIconButton(TEXT("RI_ActionRefresh"), TEXT("Refresh"), TEXT(""), false, 52.0f, ActionFontSize, 0.0f);
    ResetButton = MakeDockIconButton(TEXT("RI_ActionReset"), TEXT("Reset"), TEXT(""), false, 44.0f, ActionFontSize, 0.0f);
    UndoButton = MakeDockButton(TEXT("RI_ActionUndo"), TEXT("Undo"), false, 42.0f, ActionFontSize);
    RedoButton = MakeDockButton(TEXT("RI_ActionRedo"), TEXT("Redo"), false, 42.0f, ActionFontSize);
    ApplyButton = MakeDockIconButton(TEXT("RI_ActionApply"), TEXT("Apply"), TEXT(""), true, 44.0f, ActionFontSize, 0.0f);
    RefreshButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleRefreshClicked);
    ResetButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleResetClicked);
    UndoButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleUndoClicked);
    RedoButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleRedoClicked);
    ApplyButton->OnClicked.AddDynamic(this, &UInspectorDockRootWidget::HandleApplyClicked);
    RI_AddHorizontalCentered(ActionTopRow, RefreshButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontalCentered(ActionTopRow, ResetButton);
    RI_AddHorizontalCentered(ActionBottomRow, UndoButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontalCentered(ActionBottomRow, RedoButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontalCentered(ActionBottomRow, ApplyButton);
    RI_AddVertical(ActionBarBox, ActionTopRow, FMargin(0.f, 0.f, 0.f, 3.f));
    RI_AddVertical(ActionBarBox, ActionBottomRow);
    RI_AddVertical(OutPanel, ActionBarBox, RI_GetDockBodyMargin());

    ActionStatusText = MakeBoundText(TEXT("RI_ActionStatusText"));
    RICompactUI::ApplyTextStyle(ActionStatusText, RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor());
    RI_AddVertical(OutPanel, ActionStatusText, RI_GetDockBodyMargin(RICompactUI::GetInlineGap(), 0.f));
}

void UInspectorDockRootWidget::BuildActorTab(UVerticalBox* OutPanel)
{
    if (!OutPanel || !WidgetTree)
    {
        return;
    }

    ActorAttributesFrameSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorAttributesFrameSize"));
    ActorAttributesFrameSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
    ActorAttributesHostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorAttributesHost"));
    ActorAttributesFrameSizeBox->SetContent(ActorAttributesHostBox);
    RI_AddVertical(OutPanel, ActorAttributesFrameSizeBox, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()), ESlateSizeRule::Fill);

    ActorFunctionsFrameSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorFunctionsFrameSize"));
    ActorFunctionsFrameSizeBox->SetHeightOverride(RI_DockFunctionsFramePhysicalHeight);
    ActorFunctionsFrameSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
    ActorFunctionsHostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorFunctionsHost"));
    ActorFunctionsFrameSizeBox->SetContent(ActorFunctionsHostBox);
    RI_AddVertical(OutPanel, ActorFunctionsFrameSizeBox);
}

void UInspectorDockRootWidget::BuildChangesTab(UScrollBox* OutScrollBox)
{
    if (!OutScrollBox || !WidgetTree)
    {
        return;
    }

    UVerticalBox* PageBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ChangesTabPage"));
    OutScrollBox->AddChild(PageBox);
    RI_AddVertical(PageBox, RI_MakeIconSectionTitle(WidgetTree, TEXT("shape:status"), TEXT("Staged Patches"), RICompactUI::ERISectionVisualStyle::Emphasis, TEXT("RI_ChangesTitle")));
    UBorder* PatchCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_PatchListCard"));
    PatchListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_PatchList"));
    PatchCard->SetContent(PatchListBox);
    RI_AddVertical(PageBox, PatchCard, FMargin(0.f, 2.f, 0.f, RICompactUI::GetSectionGap()));
}

void UInspectorDockRootWidget::BuildHostedPageTabs()
{
    const double StartSeconds = FPlatformTime::Seconds();
    if (!TabSwitcher || !Controller || !WidgetTree)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Controller->GetSubsystem();
    if (!InspectorSubsystem)
    {
        return;
    }

    if (CurrentViewModel.ActiveTab == ERIInspectorTab::Actor)
    {
        BuildHostedActorSections(InspectorSubsystem);
    }

    if (CurrentViewModel.ActiveTab == ERIInspectorTab::Changes && !FilePageWidget)
    {
        FilePageWidget = CreateWidget<UInspectorFilePageWidget>(GetWorld(), UInspectorFilePageWidget::StaticClass());
        if (FilePageWidget)
        {
            FilePageWidget->SetInspectorSubsystem(InspectorSubsystem);
            if (ChangesTabScrollBox)
            {
                ChangesTabScrollBox->AddChild(FilePageWidget);
            }
        }
    }

    if (CurrentViewModel.ActiveTab == ERIInspectorTab::Settings && !SettingsPageWidget)
    {
        SettingsPageWidget = CreateWidget<UInspectorSettingsPageWidget>(GetWorld(), UInspectorSettingsPageWidget::StaticClass());
        if (SettingsPageWidget)
        {
            SettingsPageWidget->SetInspectorSubsystem(InspectorSubsystem);
            if (SettingsHostBox)
            {
                RI_AddVertical(SettingsHostBox, SettingsPageWidget, FMargin(0.f), ESlateSizeRule::Fill);
            }
        }
    }

    if (CurrentViewModel.ActiveTab == ERIInspectorTab::Tools && !ToolsPageWidget)
    {
        ToolsPageWidget = CreateWidget<UInspectorTestPageWidget>(GetWorld(), UInspectorTestPageWidget::StaticClass());
        if (ToolsPageWidget)
        {
            ToolsPageWidget->SetInspectorSubsystem(InspectorSubsystem);
            if (ToolsHostBox)
            {
                RI_AddVertical(ToolsHostBox, ToolsPageWidget, FMargin(0.f), ESlateSizeRule::Fill);
            }
        }
    }

    InspectorSubsystem->RegisterDockHostedPages(FilePageWidget.Get(), SettingsPageWidget.Get(), ToolsPageWidget.Get());
    LastHostedCreateMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
}

void UInspectorDockRootWidget::BuildHostedActorSections(UInspectorWorldSubsystem* InspectorSubsystem)
{
    if (!InspectorSubsystem || !ActorAttributesHostBox || !ActorFunctionsHostBox)
    {
        return;
    }

    if (!ActorAttributesWidget)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            ActorAttributesWidget = CreateWidget<UInspectorPropertiesSectionWidget>(PC, UInspectorPropertiesSectionWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            ActorAttributesWidget = CreateWidget<UInspectorPropertiesSectionWidget>(World, UInspectorPropertiesSectionWidget::StaticClass());
        }

        if (ActorAttributesWidget)
        {
            ActorAttributesWidget->SetAutoRefreshOnConstruct(false);
            ActorAttributesWidget->SetInspectorSubsystem(InspectorSubsystem);
            ActorAttributesWidget->TakeWidget();
            RI_AddVertical(ActorAttributesHostBox, ActorAttributesWidget, FMargin(0.f), ESlateSizeRule::Fill);
        }
    }

    if (!ActorFunctionsWidget)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            ActorFunctionsWidget = CreateWidget<UInspectorFunctionsSectionWidget>(PC, UInspectorFunctionsSectionWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            ActorFunctionsWidget = CreateWidget<UInspectorFunctionsSectionWidget>(World, UInspectorFunctionsSectionWidget::StaticClass());
        }

        if (ActorFunctionsWidget)
        {
            ActorFunctionsWidget->SetAutoRefreshOnConstruct(false);
            ActorFunctionsWidget->SetInspectorSubsystem(InspectorSubsystem);
            ActorFunctionsWidget->TakeWidget();
            RI_AddVertical(ActorFunctionsHostBox, ActorFunctionsWidget, FMargin(0.f), ESlateSizeRule::Fill);
        }
    }

    if (ActorAttributesWidget)
    {
        ActorAttributesWidget->SetInspectorSubsystem(InspectorSubsystem);
    }
    if (ActorFunctionsWidget)
    {
        ActorFunctionsWidget->SetInspectorSubsystem(InspectorSubsystem);
    }
    InspectorSubsystem->RegisterDockHostedActorSections(ActorAttributesWidget.Get(), ActorFunctionsWidget.Get());
}

UTextBlock* UInspectorDockRootWidget::MakeBoundText(const FName& Name) const
{
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Text->SetAutoWrapText(true);
    Text->SetClipping(EWidgetClipping::ClipToBounds);
    RICompactUI::ApplyTextStyle(Text, RICompactUI::GetValueFontSize(), false, RICompactUI::GetStrongTextColor());
    return Text;
}

UButton* UInspectorDockRootWidget::MakeDockButton(const FName& Name, const FString& Label, bool bPrimary, float WidthOverride, int32 FontSize) const
{
    return RICompactUI::MakeLabeledButton(
        WidgetTree,
        Name,
        Label,
        bPrimary ? RICompactUI::ERIButtonVisualStyle::Primary : RICompactUI::ERIButtonVisualStyle::Secondary,
        WidthOverride,
        RICompactUI::GetButtonHeight(),
        FontSize > 0 ? FontSize : RICompactUI::GetLabelFontSize());
}

UButton* UInspectorDockRootWidget::MakeDockIconButton(const FName& Name, const FString& Label, const TCHAR* IconAssetName, bool bPrimary, float WidthOverride, int32 FontSize, float IconSize) const
{
    const RICompactUI::ERIButtonVisualStyle Style = bPrimary
        ? RICompactUI::ERIButtonVisualStyle::Primary
        : RICompactUI::ERIButtonVisualStyle::Secondary;
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    if (WidthOverride > 0.0f)
    {
        SizeBox->SetWidthOverride(WidthOverride);
    }
    else
    {
        SizeBox->SetMinDesiredWidth(76.0f);
    }
    SizeBox->SetHeightOverride(RICompactUI::GetButtonHeight());

    UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (USizeBox* Icon = RI_MakeIconBox(WidgetTree, NAME_None, IconAssetName, IconSize, RICompactUI::GetButtonTextColor(Style)))
    {
        RI_AddHorizontal(ContentRow, Icon, FMargin(0.f, 0.f, RICompactUI::GetInlineGap() + 1.f, 0.f));
    }
    UTextBlock* LabelText = RICompactUI::MakeText(WidgetTree, Label, FontSize > 0 ? FontSize : RICompactUI::GetLabelFontSize(), true, RICompactUI::GetButtonTextColor(Style));
    LabelText->SetJustification(ETextJustify::Center);
    RI_AddHorizontal(ContentRow, LabelText, FMargin(0.f), ESlateSizeRule::Fill);

    SizeBox->SetContent(ContentRow);
    Button->AddChild(SizeBox);
    RICompactUI::ConfigureButton(Button, Style, false);
    return Button;
}

void UInspectorDockRootWidget::RefreshFromController()
{
    RefreshFromController(EInspectorRefreshReason::StructureChanged);
}

void UInspectorDockRootWidget::RefreshFromController(EInspectorRefreshReason Reason)
{
    RefreshFromController(Reason, ERIViewModelHydrationMode::DockHydrated);
}

void UInspectorDockRootWidget::RefreshFromController(EInspectorRefreshReason Reason, ERIViewModelHydrationMode HydrationMode)
{
    BuildWidgetTreeIfNeeded();
    if (!Controller)
    {
        return;
    }

    const double ViewModelStartSeconds = FPlatformTime::Seconds();
    CurrentViewModel = Controller->GetCurrentViewModel(HydrationMode);
    LastViewModelRefreshMs = (FPlatformTime::Seconds() - ViewModelStartSeconds) * 1000.0;
    RefreshLayoutForViewport();
    BuildHostedPageTabs();
    RefreshActorContext(CurrentViewModel);
    RefreshViewportOverlay(CurrentViewModel);
    if (CurrentViewModel.ActiveTab == ERIInspectorTab::Actor)
    {
        RefreshHostedActorSectionsDeferred(CurrentViewModel);
    }
    RefreshChangesTab(CurrentViewModel);
    RefreshTabPresentation(CurrentViewModel);
    RefreshActionBar(CurrentViewModel);

    if (FilePageWidget && CurrentViewModel.ActiveTab == ERIInspectorTab::Changes)
    {
        if (Reason == EInspectorRefreshReason::ValuesChanged)
        {
            FilePageWidget->RefreshFastFromSubsystem();
        }
        else
        {
            FilePageWidget->RefreshFromSubsystem();
        }
    }
    if (SettingsPageWidget && CurrentViewModel.ActiveTab == ERIInspectorTab::Settings)
    {
        SettingsPageWidget->RefreshFromSubsystem();
    }
    if (ToolsPageWidget && CurrentViewModel.ActiveTab == ERIInspectorTab::Tools)
    {
        ToolsPageWidget->RefreshFromSubsystem();
    }

    if (HydrationMode == ERIViewModelHydrationMode::ShellOnly)
    {
        ScheduleOpenHydrationRefresh();
    }

    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] RefreshDockRootViewModel %.2f ms | Mode=%d HostedCreate=%.2f OpenHydrationPending=%d"),
        LastViewModelRefreshMs,
        static_cast<int32>(HydrationMode),
        LastHostedCreateMs,
        bOpenHydrationPending ? 1 : 0);
}

void UInspectorDockRootWidget::ScheduleOpenHydrationRefresh()
{
    ++OpenHydrationSerial;
    bOpenHydrationPending = true;

    UWorld* World = GetWorld();
    if (!World)
    {
        ProcessOpenHydrationRefresh(OpenHydrationSerial);
        return;
    }

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &UInspectorDockRootWidget::ProcessOpenHydrationRefresh, OpenHydrationSerial);
    World->GetTimerManager().SetTimerForNextTick(Delegate);
}

void UInspectorDockRootWidget::CancelOpenHydrationRefresh()
{
    ++OpenHydrationSerial;
    bOpenHydrationPending = false;
}

void UInspectorDockRootWidget::ProcessOpenHydrationRefresh(int32 Serial)
{
    if (Serial != OpenHydrationSerial || !bOpenHydrationPending || !Controller)
    {
        return;
    }

    bOpenHydrationPending = false;
    const double StartSeconds = FPlatformTime::Seconds();
    CurrentViewModel = Controller->GetCurrentViewModel(ERIViewModelHydrationMode::DockHydrated);
    LastOpenHydrationViewModelMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    LastViewModelRefreshMs = LastOpenHydrationViewModelMs;

    RefreshLayoutForViewport();
    RefreshActorContext(CurrentViewModel);
    RefreshChangesTab(CurrentViewModel);
    RefreshTabPresentation(CurrentViewModel);
    RefreshActionBar(CurrentViewModel);

    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] OpenHydrationViewModel %.2f ms | Favorites=%d Functions=%d Pending=%d"),
        LastOpenHydrationViewModelMs,
        CurrentViewModel.Favorites.Num(),
        CurrentViewModel.Functions.Num(),
        bOpenHydrationPending ? 1 : 0);
}

void UInspectorDockRootWidget::RefreshLayoutForViewport()
{
    const FVector2D PhysicalViewportSize = RI_GetDockPhysicalViewportSize(this);
    const float ViewportScale = RI_GetDockViewportScale(this);
    const float ReadableScale = RI_GetDockReadableScale(ViewportScale);
    RICompactUI::SetReadableScaleOverride(ReadableScale);

    const float ExpandedSidePhysicalWidth = RI_GetDockExpandedSidePanelPhysicalWidth(PhysicalViewportSize.X);
    const float ExpandedSideLogicalWidth = RI_PhysicalToLogical(ExpandedSidePhysicalWidth, ViewportScale);
    const float CompactLeftLogicalWidth = RI_PhysicalToLogical(RI_DockLeftCompactPhysicalWidth, ViewportScale);
    const float PanelGapLogical = RI_PhysicalToLogical(RI_DockPanelGapPhysical, ViewportScale);
    const float HeaderLogicalHeight = RI_PhysicalToLogical(RI_DockHeaderBarPhysicalHeight, ViewportScale);
    const float FavoritesFrameLogicalHeight = RI_PhysicalToLogical(RI_DockFavoritesFramePhysicalHeight, ViewportScale);
    const float FunctionsFrameLogicalHeight = RI_PhysicalToLogical(RI_DockFunctionsFramePhysicalHeight, ViewportScale);
    const float BorderLogicalThickness = RI_PhysicalToLogical(RI_DockPanelBorderPhysicalThickness, ViewportScale);

    LastDockViewportScale = ViewportScale;
    LastDockReadableScale = ReadableScale;
    LastDockSidePanelLogicalWidth = ExpandedSideLogicalWidth;
    LastDockSidePanelPhysicalWidth = ExpandedSidePhysicalWidth;
    LastDockCompactLeftLogicalWidth = CompactLeftLogicalWidth;
    LastDockCompactLeftPhysicalWidth = RI_DockLeftCompactPhysicalWidth;
    LastDockPanelGapLogical = PanelGapLogical;
    LastDockPanelGapPhysical = RI_DockPanelGapPhysical;

    bLeftPanelCompact = RI_ShouldUseCompactLeftPanel(PhysicalViewportSize.X);
    if (LeftPanelSizeBox)
    {
        LeftPanelSizeBox->SetWidthOverride(bLeftPanelCompact ? CompactLeftLogicalWidth : ExpandedSideLogicalWidth);
        if (UHorizontalBoxSlot* DockSlot = Cast<UHorizontalBoxSlot>(LeftPanelSizeBox->Slot))
        {
            DockSlot->SetPadding(FMargin(0.f, 0.f, PanelGapLogical, 0.f));
        }
    }
    if (RightPanelSizeBox)
    {
        RightPanelSizeBox->SetWidthOverride(ExpandedSideLogicalWidth);
        if (UHorizontalBoxSlot* DockSlot = Cast<UHorizontalBoxSlot>(RightPanelSizeBox->Slot))
        {
            DockSlot->SetPadding(FMargin(PanelGapLogical, 0.f, 0.f, 0.f));
        }
    }

    RI_SetNamedSizeBoxHeight(WidgetTree, TEXT("RI_SelectedActorHeaderFrame"), HeaderLogicalHeight);
    RI_SetNamedSizeBoxHeight(WidgetTree, TEXT("RI_RightInspectorHeaderFrame"), HeaderLogicalHeight);
    if (FavoritesFrameSizeBox)
    {
        FavoritesFrameSizeBox->SetHeightOverride(FavoritesFrameLogicalHeight);
    }
    if (ActorFunctionsFrameSizeBox)
    {
        ActorFunctionsFrameSizeBox->SetHeightOverride(FunctionsFrameLogicalHeight);
    }
    if (WidgetTree)
    {
        if (USizeBox* LeftEdge = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("RI_DockLeftPanelViewportBorderSize"))))
        {
            LeftEdge->SetWidthOverride(BorderLogicalThickness);
        }
        if (USizeBox* RightEdge = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("RI_DockRightPanelViewportBorderSize"))))
        {
            RightEdge->SetWidthOverride(BorderLogicalThickness);
        }
    }
}

void UInspectorDockRootWidget::RefreshActorContext(const FRIInspectorViewModel& ViewModel)
{
    if (ActorNameText)
    {
        ActorNameText->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        ActorNameText->SetText(ViewModel.SelectedActor.ActorDisplayName);
        RICompactUI::ApplyTextStyle(ActorNameText, RICompactUI::GetSectionTitleFontSize() + 2, true, RICompactUI::GetStrongTextColor());
    }
    if (ActorClassText)
    {
        ActorClassText->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        ActorClassText->SetText(ViewModel.SelectedActor.ActorClassName);
        RICompactUI::ApplyTextStyle(ActorClassText, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetSecondaryTextColor());
    }
    if (ActorPathText)
    {
        const bool bHidePathForTightPanel = bLeftPanelCompact || LastDockSidePanelPhysicalWidth <= 300.0f;
        ActorPathText->SetVisibility(bHidePathForTightPanel ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        ActorPathText->SetText(bHidePathForTightPanel ? FText::GetEmpty() : ViewModel.SelectedActor.ActorPath);
        RICompactUI::ApplyTextStyle(ActorPathText, RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor());
    }
    if (StagedBannerText)
    {
        const int32 PatchCount = ViewModel.StagedPatches.Num();
        StagedBannerText->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        StagedBannerText->SetText(FText::FromString(PatchCount > 0 ? FString::Printf(TEXT("%d staged changes"), PatchCount) : TEXT("No staged patch")));
        RICompactUI::ApplyTextStyle(StagedBannerText, RICompactUI::GetLabelFontSize(), true, PatchCount > 0 ? RICompactUI::GetWarningTextColor() : RICompactUI::GetMutedTextColor());
    }
    if (SearchTextBox && Controller)
    {
        SearchTextBox->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        const FText DesiredSearchText = FText::FromString(Controller->GetSearchText());
        if (!SearchTextBox->GetText().EqualTo(DesiredSearchText))
        {
            bSuppressSearchTextChanged = true;
            SearchTextBox->SetText(DesiredSearchText);
            bSuppressSearchTextChanged = false;
        }
    }
    if (ComponentTitleWidget)
    {
        ComponentTitleWidget->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
    if (FavoritesTitleWidget)
    {
        FavoritesTitleWidget->SetVisibility(bLeftPanelCompact ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    ActionProxies.Reset();
    ComponentRowSurfaces.Reset();
    ComponentRowTexts.Reset();

    if (ComponentListBox)
    {
        ComponentListBox->ClearChildren();
        if (bLeftPanelCompact)
        {
            if (USizeBox* CompactIcon = RI_MakeIconBox(WidgetTree, NAME_None, TEXT("shape:components"), 13.0f, RICompactUI::GetMutedTextColor()))
            {
                RI_AddVertical(ComponentListBox, CompactIcon, FMargin(0.f, 2.f, 0.f, 2.f));
            }
        }
        else if (ViewModel.Components.Num() == 0)
        {
            RI_AddVertical(ComponentListBox, RI_MakeIconLabel(WidgetTree, TEXT("shape:components"), TEXT("No components"), 11.0f, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetMutedTextColor(), RICompactUI::GetMutedTextColor()));
        }
        else
        {
            for (const FRIComponentNodeViewModel& Component : ViewModel.Components)
            {
                UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NAME_None);
                UBorder* RowSurface = RICompactUI::MakeSurfaceCard(
                    WidgetTree,
                    NAME_None,
                    Component.bSelected ? RI_GetDockSelectedRowSurfaceColor() : RI_GetDockRowSurfaceColor(),
                    RICompactUI::GetSurfaceCardPadding(true));
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NAME_None);
                const FLinearColor RowColor = Component.bSelected ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetSecondaryTextColor();
                const int32 Depth = FMath::Max(0, Component.Depth);
                if (Depth > 0)
                {
                    USizeBox* Indent = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
                    Indent->SetWidthOverride(static_cast<float>(Depth) * 12.0f);
                    Indent->SetHeightOverride(1.0f);
                    RI_AddHorizontal(Row, Indent);
                }

                const TCHAR* ExpanderIcon = Component.bCanExpand
                    ? (Component.bExpanded ? TEXT("shape:chevron-down") : TEXT("shape:chevron-right"))
                    : TEXT("shape:blank");
                const FLinearColor ExpanderColor = Component.bCanExpand ? RowColor : RICompactUI::GetMutedTextColor();
                if (USizeBox* RowIcon = RI_MakeIconBox(WidgetTree, NAME_None, ExpanderIcon, 11.0f, ExpanderColor))
                {
                    RI_AddHorizontal(Row, RowIcon, FMargin(0.f, 2.f, RICompactUI::GetInlineGap() + 1.f, 0.f));
                }
                FString ComponentLabel = Component.DisplayName.ToString();
                if (Component.Kind == ERIComponentNodeKind::Component && !Component.ClassName.IsEmpty())
                {
                    ComponentLabel = FString::Printf(TEXT("%s  |  %s"), *ComponentLabel, *Component.ClassName.ToString());
                }
                UTextBlock* RowText = RICompactUI::MakeEllipsisText(
                    WidgetTree,
                    ComponentLabel,
                    RICompactUI::GetLabelFontSize(),
                    Component.bSelected,
                    RowColor);
                RowText->SetToolTipText(FText::FromString(ComponentLabel));
                RI_AddHorizontal(Row, RowText, FMargin(0.f), ESlateSizeRule::Fill);
                RowSurface->SetContent(Row);
                RowButton->AddChild(RowSurface);
                RICompactUI::ConfigureSwatchButton(RowButton);

                const FString RouteToken = Component.StableKey.IsEmpty()
                    ? (Component.ComponentName.IsEmpty() ? Component.DisplayName.ToString() : Component.ComponentName)
                    : Component.StableKey;
                ComponentRowSurfaces.Add(RouteToken, RowSurface);
                ComponentRowTexts.Add(RouteToken, RowText);

                UInspectorDockComponentActionProxy* Proxy = NewObject<UInspectorDockComponentActionProxy>(this);
                Proxy->Owner = this;
                Proxy->ComponentName = RouteToken;
                ActionProxies.Add(Proxy);
                RowButton->OnClicked.AddDynamic(Proxy, &UInspectorDockComponentActionProxy::HandleClicked);
                RI_AddVertical(ComponentListBox, RowButton, FMargin(0.f, 0.f, 0.f, 3.f));
            }
        }
    }

    if (FavoritesListBox)
    {
        FavoritesListBox->ClearChildren();
        bLastLeftFavoriteStarVisualContractOk = true;
        if (bLeftPanelCompact)
        {
            if (USizeBox* CompactIcon = RI_MakeIconBox(WidgetTree, NAME_None, TEXT("shape:star-solid"), 13.0f, RICompactUI::GetWarningTextColor()))
            {
                RI_AddVertical(FavoritesListBox, CompactIcon, FMargin(0.f, 2.f, 0.f, 2.f));
            }
        }
        else if (ViewModel.Favorites.Num() == 0)
        {
            RI_AddVertical(FavoritesListBox, RI_MakeIconLabel(WidgetTree, TEXT("shape:star-outline"), TEXT("No favorites"), 11.0f, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetMutedTextColor(), RICompactUI::GetMutedTextColor()));
        }
        else
        {
            for (const FRIFavoriteViewModel& Favorite : ViewModel.Favorites)
            {
                UBorder* RowSurface = RICompactUI::MakeSurfaceCard(
                    WidgetTree,
                    NAME_None,
                    RI_GetDockRowSurfaceColor(),
                    RICompactUI::GetSurfaceCardPadding(true));
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NAME_None);

                UImage* StarIcon = nullptr;
                USizeBox* StarHitBox = nullptr;
                UButton* StarButton = RICompactUI::MakeFavoriteGhostButton(
                    WidgetTree,
                    NAME_None,
                    NAME_None,
                    NAME_None,
                    true,
                    RICompactUI::GetWarningTextColor(),
                    RICompactUI::GetMutedTextColor(),
                    &StarIcon,
                    &StarHitBox);
                bLastLeftFavoriteStarVisualContractOk = bLastLeftFavoriteStarVisualContractOk
                    && RICompactUI::HasFavoriteGhostButtonContract(StarButton, StarHitBox, StarIcon);

                UInspectorDockFavoriteActionProxy* ToggleProxy = NewObject<UInspectorDockFavoriteActionProxy>(this);
                ToggleProxy->Owner = this;
                ToggleProxy->SourceItem = Favorite.SourceItem;
                ToggleProxy->bToggleFavorite = true;
                ActionProxies.Add(ToggleProxy);
                if (StarButton)
                {
                    StarButton->OnClicked.AddDynamic(ToggleProxy, &UInspectorDockFavoriteActionProxy::HandleClicked);
                }
                RI_AddHorizontal(Row, StarButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));

                UButton* TextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NAME_None);
                RICompactUI::ConfigureSwatchButton(TextButton);
                const FString FavoriteLabel = FString::Printf(TEXT("%s  %s"), *Favorite.DisplayName.ToString(), *Favorite.ValueText.ToString());
                UTextBlock* RowText = RICompactUI::MakeEllipsisText(
                    WidgetTree,
                    FavoriteLabel,
                    RICompactUI::GetLabelFontSize(),
                    false,
                    RICompactUI::GetSecondaryTextColor());
                RowText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
                TextButton->AddChild(RowText);
                if (UButtonSlot* TextSlot = Cast<UButtonSlot>(TextButton->GetContentSlot()))
                {
                    TextSlot->SetHorizontalAlignment(HAlign_Fill);
                    TextSlot->SetVerticalAlignment(VAlign_Center);
                    TextSlot->SetPadding(FMargin(0.f));
                }

                UInspectorDockFavoriteActionProxy* NavigateProxy = NewObject<UInspectorDockFavoriteActionProxy>(this);
                NavigateProxy->Owner = this;
                NavigateProxy->SourceItem = Favorite.SourceItem;
                NavigateProxy->bToggleFavorite = false;
                ActionProxies.Add(NavigateProxy);
                TextButton->OnClicked.AddDynamic(NavigateProxy, &UInspectorDockFavoriteActionProxy::HandleClicked);
                RI_AddHorizontal(Row, TextButton, FMargin(0.f), ESlateSizeRule::Fill);
                RowSurface->SetContent(Row);
                RI_AddVertical(FavoritesListBox, RowSurface, FMargin(0.f, 0.f, 0.f, 3.f));
            }
        }
    }
}

void UInspectorDockRootWidget::RefreshAfterComponentFocus(const FString& ComponentName, bool bFocusSucceeded)
{
    CurrentViewModel.ActiveTab = ERIInspectorTab::Actor;
    if (bFocusSucceeded)
    {
        RefreshComponentSelectionPresentation(ComponentName);
        RefreshHostedActorSectionsDeferred(CurrentViewModel);
    }
    RefreshTabPresentation(CurrentViewModel);
    RefreshActionBar(CurrentViewModel);
    if (ActionStatusText && Controller)
    {
        ActionStatusText->SetText(FText::FromString(Controller->GetLastIntentLog()));
    }
}

void UInspectorDockRootWidget::RefreshComponentSelectionPresentation(const FString& ComponentName)
{
    for (FRIComponentNodeViewModel& Component : CurrentViewModel.Components)
    {
        const FString RouteComponentName = Component.ComponentName.IsEmpty() ? Component.DisplayName.ToString() : Component.ComponentName;
        Component.bSelected = RouteComponentName.Equals(ComponentName, ESearchCase::IgnoreCase);
    }

    for (const TPair<FString, TWeakObjectPtr<UBorder>>& RowPair : ComponentRowSurfaces)
    {
        const bool bSelected = RowPair.Key.Equals(ComponentName, ESearchCase::IgnoreCase);
        const FLinearColor RowColor = bSelected ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetSecondaryTextColor();
        if (UBorder* Surface = RowPair.Value.Get())
        {
            Surface->SetBrushColor(bSelected ? RI_GetDockSelectedRowSurfaceColor() : RI_GetDockRowSurfaceColor());
        }
        if (const TWeakObjectPtr<UTextBlock>* TextPtr = ComponentRowTexts.Find(RowPair.Key))
        {
            if (UTextBlock* Text = TextPtr->Get())
            {
                RICompactUI::ApplyTextStyle(Text, RICompactUI::GetLabelFontSize(), bSelected, RowColor);
            }
        }
    }
}

void UInspectorDockRootWidget::RefreshViewportOverlay(const FRIInspectorViewModel&)
{
}

void UInspectorDockRootWidget::RefreshHostedActorSections(const FRIInspectorViewModel& ViewModel)
{
    if (ActorAttributesWidget)
    {
        ActorAttributesWidget->SetOnlyModified(ViewModel.bOnlyModify);
        ActorAttributesWidget->RefreshFromSubsystem();
    }
    if (ActorFunctionsWidget)
    {
        ActorFunctionsWidget->RefreshFromSubsystem();
    }
}

void UInspectorDockRootWidget::RefreshHostedActorSectionsDeferred(const FRIInspectorViewModel& ViewModel)
{
    if (ActorAttributesWidget)
    {
        ActorAttributesWidget->SetOnlyModified(ViewModel.bOnlyModify);
        ActorAttributesWidget->RefreshFromSubsystemDeferred();
    }
    if (ActorFunctionsWidget)
    {
        ActorFunctionsWidget->RefreshFromSubsystemDeferred();
    }
}

void UInspectorDockRootWidget::RefreshChangesTab(const FRIInspectorViewModel& ViewModel)
{
    if (!PatchListBox)
    {
        return;
    }

    PatchListBox->ClearChildren();
    if (ViewModel.StagedPatches.Num() == 0)
    {
        RI_AddVertical(PatchListBox, RI_MakeIconLabel(WidgetTree, TEXT("shape:status"), TEXT("No staged changes. Apply is gated until a patch row exists."), 11.0f, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetMutedTextColor(), RICompactUI::GetMutedTextColor()));
        return;
    }

    for (const FRIPatchViewModel& Patch : ViewModel.StagedPatches)
    {
        UBorder* RowCard = RI_MakeSectionCard(WidgetTree, NAME_None);
        RowCard->SetBrushColor(RI_GetDockRowSurfaceColor());
        UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        RowCard->SetContent(Row);

        UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        if (USizeBox* PatchIcon = RI_MakeIconBox(WidgetTree, NAME_None, TEXT("shape:status"), 12.0f, RICompactUI::GetWarningTextColor()))
        {
            RI_AddHorizontal(Header, PatchIcon, FMargin(0.f, 1.f, RICompactUI::GetInlineGap() + 2.f, 0.f));
        }
        RI_AddHorizontal(Header, RICompactUI::MakeText(
            WidgetTree,
            FString::Printf(TEXT("%s | %s"), *Patch.TargetPath.ToString(), *Patch.PropertyName.ToString()),
            RICompactUI::GetLabelFontSize(),
            true,
            RICompactUI::GetStrongTextColor(),
            true),
            FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f),
            ESlateSizeRule::Fill);
        UButton* RevertButton = MakeDockIconButton(NAME_None, TEXT("Revert"), TEXT(""));
        RevertButton->SetIsEnabled(Patch.bCanRevert);
        UInspectorDockPatchActionProxy* Proxy = NewObject<UInspectorDockPatchActionProxy>(this);
        Proxy->Owner = this;
        Proxy->PatchId = Patch.PatchId;
        ActionProxies.Add(Proxy);
        RevertButton->OnClicked.AddDynamic(Proxy, &UInspectorDockPatchActionProxy::HandleRevertClicked);
        RI_AddHorizontal(Header, RevertButton);
        RI_AddVertical(Row, Header);

        UTextBlock* DiffText = RICompactUI::MakeText(
            WidgetTree,
            FString::Printf(TEXT("Old: %s\nNew: %s"), *Patch.OldValueText.ToString(), *Patch.NewValueText.ToString()),
            RICompactUI::GetMutedFontSize(),
            false,
            RICompactUI::GetSecondaryTextColor(),
            true);
        RI_AddVertical(Row, DiffText, FMargin(0.f, 4.f, 0.f, 0.f));
        RI_AddVertical(PatchListBox, RowCard, FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }
}

void UInspectorDockRootWidget::RefreshTabPresentation(const FRIInspectorViewModel& ViewModel)
{
    if (HeaderText)
    {
        HeaderText->SetText(FText::FromString(FString::Printf(TEXT("Inspector / %s"), *RI_TabLabel(ViewModel.ActiveTab))));
    }

    if (TabSwitcher)
    {
        int32 ActiveIndex = 0;
        switch (ViewModel.ActiveTab)
        {
        case ERIInspectorTab::Changes:
            ActiveIndex = 1;
            break;
        case ERIInspectorTab::Settings:
            ActiveIndex = 2;
            break;
        case ERIInspectorTab::Tools:
            ActiveIndex = 3;
            break;
        case ERIInspectorTab::Actor:
        default:
            ActiveIndex = 0;
            break;
        }
        if (TabSwitcher->GetChildrenCount() > ActiveIndex)
        {
            TabSwitcher->SetActiveWidgetIndex(ActiveIndex);
        }
    }

    SetTabButtonStyle(ActorTabButton, ViewModel.ActiveTab == ERIInspectorTab::Actor);
    SetTabButtonStyle(ChangesTabButton, ViewModel.ActiveTab == ERIInspectorTab::Changes);
    SetTabButtonStyle(SettingsTabButton, ViewModel.ActiveTab == ERIInspectorTab::Settings);
    SetTabButtonStyle(ToolsTabButton, ViewModel.ActiveTab == ERIInspectorTab::Tools);
}

void UInspectorDockRootWidget::RefreshActionBar(const FRIInspectorViewModel& ViewModel)
{
    if (OnlyModifyCheckBox)
    {
        OnlyModifyCheckBox->SetIsChecked(ViewModel.bOnlyModify);
    }
    if (ApplyButton)
    {
        ApplyButton->SetIsEnabled(ViewModel.StagedPatches.Num() > 0);
    }
    if (ResetButton)
    {
        ResetButton->SetIsEnabled(ViewModel.StagedPatches.Num() > 0);
    }
    if (UndoButton)
    {
        UndoButton->SetIsEnabled(ViewModel.bCanUndo);
    }
    if (RedoButton)
    {
        RedoButton->SetIsEnabled(ViewModel.bCanRedo);
    }
    if (ActionStatusText && Controller)
    {
        ActionStatusText->SetText(FText::FromString(Controller->GetLastIntentLog()));
    }
}

void UInspectorDockRootWidget::SetActiveTab(ERIInspectorTab InTab)
{
    if (Controller)
    {
        Controller->SetActiveTab(InTab);
    }
    RefreshFromController(EInspectorRefreshReason::UIStateChanged);
}

void UInspectorDockRootWidget::SetTabButtonStyle(UButton* Button, bool bActive) const
{
    RICompactUI::ConfigureButton(
        Button,
        bActive ? RICompactUI::ERIButtonVisualStyle::TabActive : RICompactUI::ERIButtonVisualStyle::TabInactive,
        true);
}

void UInspectorDockRootWidget::HandleActorTabClicked() { SetActiveTab(ERIInspectorTab::Actor); }
void UInspectorDockRootWidget::HandleChangesTabClicked() { SetActiveTab(ERIInspectorTab::Changes); }
void UInspectorDockRootWidget::HandleSettingsTabClicked() { SetActiveTab(ERIInspectorTab::Settings); }
void UInspectorDockRootWidget::HandleToolsTabClicked() { SetActiveTab(ERIInspectorTab::Tools); }

void UInspectorDockRootWidget::HandleOnlyModifyChanged(bool bIsChecked)
{
    if (Controller)
    {
        Controller->SetOnlyModify(bIsChecked);
    }
    RefreshFromController(EInspectorRefreshReason::UIStateChanged);
}

void UInspectorDockRootWidget::HandleRefreshClicked()
{
    if (Controller)
    {
        Controller->RequestRefresh();
    }
    RefreshFromController(EInspectorRefreshReason::StructureChanged);
}

void UInspectorDockRootWidget::HandleResetClicked()
{
    if (Controller)
    {
        FString Error;
        Controller->RequestReset(Error);
    }
    RefreshFromController(EInspectorRefreshReason::ValuesChanged);
}

void UInspectorDockRootWidget::HandleUndoClicked()
{
    if (Controller)
    {
        Controller->RequestUndo();
    }
    RefreshFromController(EInspectorRefreshReason::UndoRedo);
}

void UInspectorDockRootWidget::HandleRedoClicked()
{
    if (Controller)
    {
        Controller->RequestRedo();
    }
    RefreshFromController(EInspectorRefreshReason::UndoRedo);
}

void UInspectorDockRootWidget::HandleApplyClicked()
{
    if (Controller)
    {
        FRIApplyResult Result;
        Controller->RequestApplyStagedPatches(Result);
    }
    RefreshFromController(EInspectorRefreshReason::ValuesChanged);
}

void UInspectorDockRootWidget::HandleSearchTextChanged(const FText& InText)
{
    if (bSuppressSearchTextChanged)
    {
        return;
    }

    if (Controller)
    {
        Controller->SetSearchText(InText);
    }
    RefreshFromController(EInspectorRefreshReason::UIStateChanged);
}

void UInspectorDockRootWidget::HandleFunctionRunProxyClicked(FName FunctionName)
{
    if (Controller)
    {
        FString Error;
        Controller->RequestRunFunction(FunctionName, Error);
    }
    RefreshFromController(EInspectorRefreshReason::ValuesChanged);
}

void UInspectorDockRootWidget::HandlePatchRevertProxyClicked(FGuid PatchId)
{
    if (Controller)
    {
        FRIApplyResult Result;
        Controller->RequestRevertPatch(PatchId, Result);
    }
    RefreshFromController(EInspectorRefreshReason::ValuesChanged);
}

void UInspectorDockRootWidget::HandleComponentProxyClicked(const FString& RouteToken)
{
    const double StartSeconds = FPlatformTime::Seconds();
    bool bFocusSucceeded = false;
    FString Error;
    if (Controller)
    {
        bFocusSucceeded = Controller->RequestSelectComponentTreeNode(RouteToken, Error);
        CurrentViewModel = Controller->GetCurrentViewModel(ERIViewModelHydrationMode::DockHydrated);
    }
    RefreshActorContext(CurrentViewModel);
    if (bFocusSucceeded)
    {
        RefreshHostedActorSectionsDeferred(CurrentViewModel);
    }
    RefreshTabPresentation(CurrentViewModel);
    RefreshActionBar(CurrentViewModel);
    if (ActionStatusText && Controller)
    {
        ActionStatusText->SetText(FText::FromString(Controller->GetLastIntentLog()));
    }
    LastComponentFocusIntentMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] DockComponentFocusIntent %.2f ms | Route=%s Result=%s Deferred=%d"),
        LastComponentFocusIntentMs,
        *RouteToken,
        bFocusSucceeded ? TEXT("ok") : *Error,
        AreHostedActorSectionsDeferredRefreshPendingForAutomation() ? 1 : 0);
}

void UInspectorDockRootWidget::HandleFavoriteProxyClicked(UObject* SourceItem)
{
    if (Controller)
    {
        FString Error;
        Controller->RequestNavigateToPinnedItem(SourceItem, Error);
        if (ActionStatusText)
        {
            ActionStatusText->SetText(FText::FromString(Controller->GetLastIntentLog()));
        }
    }
    RefreshFromController(EInspectorRefreshReason::StructureChanged);
}

void UInspectorDockRootWidget::HandleFavoriteToggleProxyClicked(UObject* SourceItem)
{
    if (Controller)
    {
        FString Error;
        Controller->RequestToggleFavorite(SourceItem, Error);
        if (ActionStatusText)
        {
            ActionStatusText->SetText(FText::FromString(Controller->GetLastIntentLog()));
        }
    }
    RefreshFromController(EInspectorRefreshReason::UIStateChanged);
}

FString UInspectorDockRootWidget::GetDockLayoutDebugSummary() const
{
    const FVector2D PhysicalViewportSize = RI_GetDockPhysicalViewportSize(const_cast<UInspectorDockRootWidget*>(this));
    const float ViewportScale = LastDockViewportScale > KINDA_SMALL_NUMBER
        ? LastDockViewportScale
        : RI_GetDockViewportScale(const_cast<UInspectorDockRootWidget*>(this));
    const FVector2D LogicalViewportSize = PhysicalViewportSize / FMath::Max(ViewportScale, 0.01f);
    const float CenterPhysicalWidth = RI_GetDockCenterPhysicalWidth(PhysicalViewportSize.X, bLeftPanelCompact);
    const float CenterLogicalWidth = RI_PhysicalToLogical(CenterPhysicalWidth, ViewportScale);
    const bool bCompactAtPreviewWidth = RI_ShouldUseCompactLeftPanel(1080.0f);
    const bool bExpandedAtReferenceWidth = !RI_ShouldUseCompactLeftPanel(1390.0f);
    const bool bCompactAtNarrowWidth = RI_ShouldUseCompactLeftPanel(1000.0f);
    const bool bCompactTextHidden = !bLeftPanelCompact
        || ((ActorNameText == nullptr || ActorNameText->GetVisibility() == ESlateVisibility::Collapsed)
            && (ActorClassText == nullptr || ActorClassText->GetVisibility() == ESlateVisibility::Collapsed)
            && (StagedBannerText == nullptr || StagedBannerText->GetVisibility() == ESlateVisibility::Collapsed)
            && (SearchTextBox == nullptr || SearchTextBox->GetVisibility() == ESlateVisibility::Collapsed)
            && (ComponentTitleWidget == nullptr || ComponentTitleWidget->GetVisibility() == ESlateVisibility::Collapsed)
            && (FavoritesTitleWidget == nullptr || FavoritesTitleWidget->GetVisibility() == ESlateVisibility::Collapsed));
    const int32 AttributeRows = ActorAttributesWidget ? ActorAttributesWidget->GetEntryWidgetCountForAutomation() : INDEX_NONE;
    const int32 HostedFunctionRows = ActorFunctionsWidget ? ActorFunctionsWidget->GetEntryWidgetCountForAutomation() : INDEX_NONE;
    const bool bPanelBorder = RI_DockWidgetExists(WidgetTree, TEXT("RI_DockLeftPanelViewportBorder"))
        && RI_DockWidgetExists(WidgetTree, TEXT("RI_DockRightPanelViewportBorder"));
    const bool bPanelBlur = RI_DockWidgetExists(WidgetTree, TEXT("RI_DockLeftPanelBackgroundBlur"))
        && RI_DockWidgetExists(WidgetTree, TEXT("RI_DockRightPanelBackgroundBlur"));
    const bool bPanelBase = RI_DockWidgetExists(WidgetTree, TEXT("RI_DockLeftPanelBackgroundBase"))
        && RI_DockWidgetExists(WidgetTree, TEXT("RI_DockRightPanelBackgroundBase"));
    const bool bPanelWash = RI_DockWidgetExists(WidgetTree, TEXT("RI_DockLeftPanelBackgroundWash"))
        && RI_DockWidgetExists(WidgetTree, TEXT("RI_DockRightPanelBackgroundWash"));
    const bool bPanelGrid = RI_DockWidgetExists(WidgetTree, TEXT("RI_DockLeftPanelBackgroundGrid"))
        && RI_DockWidgetExists(WidgetTree, TEXT("RI_DockRightPanelBackgroundGrid"));
    const bool bPanelChrome = bPanelBorder && bPanelBlur && bPanelBase && bPanelWash && bPanelGrid;
    return FString::Printf(
        TEXT("DockRoot=1 LeftPanel=%s RightPanel=1 SideWidthLogical=%.0f SideWidthPhysical=%.0f CompactLeftLogical=%.0f CompactLeftPhysical=%.0f PanelGapLogical=%.0f PanelGapPhysical=%.0f CenterWidth=%.0f CenterPhysical=%.0f LogicalWidth=%.0f PhysicalWidth=%.0f ViewportScale=%.2f ReadableScale=%.2f CompactAt1080=%d ExpandedAt1390=%d CompactAt1000=%d CompactTextHidden=%d CenterPassThrough=1 CenterSelectionPill=0 PanelChrome=%d PanelBorder=%d PanelBlur=%d PanelBase=%d PanelGrid=%d PanelWash=%d FavoritesFrame=%d FavoritesScroll=%d FunctionsFrame=%d ActionBar=%d PatchRows=%d FunctionRows=%d AttributeRows=%d AttributesTransform=%d AttributesPending=%d FunctionsPending=%d OpenHydrationPending=%d ViewModelMs=%.2f HostedCreateMs=%.2f LastComponentFocusIntentMs=%.2f ActiveTab=%d"),
        bLeftPanelCompact ? TEXT("Compact") : TEXT("Expanded"),
        LastDockSidePanelLogicalWidth,
        LastDockSidePanelPhysicalWidth,
        LastDockCompactLeftLogicalWidth,
        LastDockCompactLeftPhysicalWidth,
        LastDockPanelGapLogical,
        LastDockPanelGapPhysical,
        CenterLogicalWidth,
        CenterPhysicalWidth,
        LogicalViewportSize.X,
        PhysicalViewportSize.X,
        ViewportScale,
        LastDockReadableScale,
        bCompactAtPreviewWidth ? 1 : 0,
        bExpandedAtReferenceWidth ? 1 : 0,
        bCompactAtNarrowWidth ? 1 : 0,
        bCompactTextHidden ? 1 : 0,
        bPanelChrome ? 1 : 0,
        bPanelBorder ? 1 : 0,
        bPanelBlur ? 1 : 0,
        bPanelBase ? 1 : 0,
        bPanelGrid ? 1 : 0,
        bPanelWash ? 1 : 0,
        FavoritesFrameSizeBox ? 1 : 0,
        FavoritesScrollBox ? 1 : 0,
        ActorFunctionsFrameSizeBox ? 1 : 0,
        ActionBarBox ? 1 : 0,
        CurrentViewModel.StagedPatches.Num(),
        HostedFunctionRows,
        AttributeRows,
        ActorAttributesWidget && ActorAttributesWidget->HasActorTransformBlockForAutomation() ? 1 : 0,
        ActorAttributesWidget && ActorAttributesWidget->IsDeferredRefreshPendingForAutomation() ? 1 : 0,
        ActorFunctionsWidget && ActorFunctionsWidget->IsDeferredRefreshPendingForAutomation() ? 1 : 0,
        bOpenHydrationPending ? 1 : 0,
        LastViewModelRefreshMs,
        LastHostedCreateMs,
        LastComponentFocusIntentMs,
        static_cast<int32>(CurrentViewModel.ActiveTab));
}

bool UInspectorDockRootWidget::AreHostedActorSectionsDeferredRefreshPendingForAutomation() const
{
    return (ActorAttributesWidget && ActorAttributesWidget->IsDeferredRefreshPendingForAutomation())
        || (ActorFunctionsWidget && ActorFunctionsWidget->IsDeferredRefreshPendingForAutomation());
}

bool UInspectorDockRootWidget::FlushHostedActorSectionsDeferredRefreshForAutomation()
{
    bool bAttributesOk = true;
    bool bFunctionsOk = true;
    if (ActorAttributesWidget)
    {
        bAttributesOk = ActorAttributesWidget->FlushDeferredRefreshForAutomation();
    }
    if (ActorFunctionsWidget)
    {
        bFunctionsOk = ActorFunctionsWidget->FlushDeferredRefreshForAutomation();
    }
    return bAttributesOk && bFunctionsOk;
}
