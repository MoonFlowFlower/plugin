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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "HAL/PlatformTime.h"
#include "InspectorWorldSubsystem.h"

namespace
{
    static constexpr float RI_DockSidePanelWidth = 256.0f;
    static constexpr float RI_DockLeftCompactWidth = 58.0f;
    static constexpr float RI_DockOuterPadding = 0.0f;
    static constexpr float RI_DockPanelGap = 16.0f;
    static constexpr float RI_DockNarrowCollapseThreshold = 1500.0f;
    static constexpr float RI_DockFavoritesFrameHeight = 190.0f;
    static constexpr float RI_DockFunctionsFrameHeight = 220.0f;

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

    static UBorder* RI_MakePanelSurface(UWidgetTree* WidgetTree, const FName& Name)
    {
        UBorder* Border = RICompactUI::MakeSurfaceCard(WidgetTree, Name, RICompactUI::GetPageBackgroundColor(), RICompactUI::GetPanelPadding());
        Border->SetVisibility(ESlateVisibility::Visible);
        return Border;
    }

    static UBorder* RI_MakeSectionCard(UWidgetTree* WidgetTree, const FName& Name)
    {
        return RICompactUI::MakeSurfaceCard(WidgetTree, Name, RICompactUI::GetSectionSurfaceBackgroundColor(), RICompactUI::GetSurfaceCardPadding());
    }

    static FVector2D RI_GetDockLogicalViewportSize(UWidget* Widget)
    {
        FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(Widget);
        const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(Widget);
        if (ViewportScale > KINDA_SMALL_NUMBER)
        {
            ViewportSize /= ViewportScale;
        }

        return ViewportSize;
    }

    static float RI_GetDockCenterWidth(float ViewportWidth, bool bLeftCompact)
    {
        const float LeftWidth = bLeftCompact ? RI_DockLeftCompactWidth : RI_DockSidePanelWidth;
        return ViewportWidth - LeftWidth - RI_DockSidePanelWidth - (RI_DockPanelGap * 2.0f);
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
    RefreshFromController();
}

void UInspectorDockRootWidget::SetController(URuntimeInspectorController* InController)
{
    Controller = InController;
    BuildWidgetTreeIfNeeded();
    BuildHostedPageTabs();
    RefreshFromController();
}

void UInspectorDockRootWidget::BuildWidgetTreeIfNeeded()
{
    if (bWidgetTreeBuilt || !WidgetTree)
    {
        return;
    }

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

    LeftPanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_DockLeftPanelSize"));
    LeftPanelSizeBox->SetWidthOverride(RI_DockSidePanelWidth);
    UBorder* LeftSurface = RI_MakePanelSurface(WidgetTree, TEXT("RI_DockLeftPanelSurface"));
    LeftPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_DockLeftPanel"));
    LeftSurface->SetContent(LeftPanelBox);
    LeftPanelSizeBox->SetContent(LeftSurface);
    RI_AddHorizontal(DockBox, LeftPanelSizeBox, FMargin(0.f, 0.f, RI_DockPanelGap, 0.f));
    BuildLeftPanel(LeftPanelBox);

    UOverlay* CenterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_DockViewportOverlay"));
    CenterOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    RI_AddHorizontal(DockBox, CenterOverlay, FMargin(0.f), ESlateSizeRule::Fill);
    BuildCenterOverlay(CenterOverlay);

    RightPanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_DockRightPanelSize"));
    RightPanelSizeBox->SetWidthOverride(RI_DockSidePanelWidth);
    UBorder* RightSurface = RI_MakePanelSurface(WidgetTree, TEXT("RI_DockRightPanelSurface"));
    RightPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_DockRightInspector"));
    RightSurface->SetContent(RightPanelBox);
    RightPanelSizeBox->SetContent(RightSurface);
    RI_AddHorizontal(DockBox, RightPanelSizeBox, FMargin(RI_DockPanelGap, 0.f, 0.f, 0.f));
    BuildRightInspector(RightPanelBox);

    UOverlay* ModalLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RI_DockModalToastLayer"));
    ModalLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot* ModalSlot = RootOverlay->AddChildToOverlay(ModalLayer))
    {
        ModalSlot->SetHorizontalAlignment(HAlign_Fill);
        ModalSlot->SetVerticalAlignment(VAlign_Fill);
    }

    bWidgetTreeBuilt = true;
}

void UInspectorDockRootWidget::BuildLeftPanel(UVerticalBox* OutPanel)
{
    if (!OutPanel || !WidgetTree)
    {
        return;
    }

    UBorder* ActorCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_SelectedActorCard"));
    UVerticalBox* ActorCardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    ActorCard->SetContent(ActorCardBox);
    UHorizontalBox* ActorTopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SelectedActorTopRow"));
    if (USizeBox* ActorIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_SelectedActorIcon"), TEXT("shape:object"), 16.0f, RICompactUI::GetSuccessTextColor()))
    {
        RI_AddHorizontal(ActorTopRow, ActorIcon, FMargin(0.f, 1.f, RICompactUI::GetInlineGap() + 3.f, 0.f));
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
    RI_AddVertical(ActorCardBox, ActorPathText, FMargin(0.f, 6.f, 0.f, 0.f));
    RI_AddVertical(OutPanel, ActorCard, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

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
    RI_AddVertical(OutPanel, StagedBanner, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

    SearchTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RI_SearchBar"));
    RICompactUI::ConfigureEditableTextBox(SearchTextBox, RICompactUI::GetStrongTextColor(), RICompactUI::GetValueFontSize(), RICompactUI::ERIInputVisualStyle::Muted);
    SearchTextBox->SetHintText(FText::FromString(TEXT("Search actor context")));
    SearchTextBox->OnTextChanged.AddDynamic(this, &UInspectorDockRootWidget::HandleSearchTextChanged);
    UBorder* SearchSurface = RI_MakeSectionCard(WidgetTree, TEXT("RI_SearchBarSurface"));
    SearchSurface->SetBrushColor(RICompactUI::GetContextSecondaryCellBackgroundColor());
    UHorizontalBox* SearchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SearchBarRow"));
    if (USizeBox* SearchIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_SearchIcon"), TEXT("search_white_64"), 13.0f, RICompactUI::GetMutedTextColor()))
    {
        RI_AddHorizontal(SearchRow, SearchIcon, FMargin(0.f, 3.f, RICompactUI::GetInlineGap() + 2.f, 0.f));
    }
    RI_AddHorizontal(SearchRow, RICompactUI::WrapValueControl(WidgetTree, SearchTextBox), FMargin(0.f), ESlateSizeRule::Fill);
    SearchSurface->SetContent(SearchRow);
    RI_AddVertical(OutPanel, SearchSurface, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

    RI_AddVertical(OutPanel, RI_MakeIconSectionTitle(WidgetTree, TEXT("shape:components"), TEXT("Components"), RICompactUI::ERISectionVisualStyle::Standard, TEXT("RI_ComponentTreeTitle")));
    UBorder* ComponentCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_ComponentTreeCard"));
    UScrollBox* ComponentScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ComponentTreeScroll"));
    ComponentListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ComponentTree"));
    ComponentScrollBox->AddChild(ComponentListBox);
    ComponentCard->SetContent(ComponentScrollBox);
    RI_AddVertical(OutPanel, ComponentCard, FMargin(0.f, 2.f, 0.f, RICompactUI::GetSectionGap()), ESlateSizeRule::Fill);

    FavoritesFrameSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_FavoritesFrameSize"));
    FavoritesFrameSizeBox->SetHeightOverride(RI_DockFavoritesFrameHeight);
    FavoritesFrameSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
    UVerticalBox* FavoritesFrameBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FavoritesFrame"));
    FavoritesFrameSizeBox->SetContent(FavoritesFrameBox);

    RI_AddVertical(FavoritesFrameBox, RI_MakeIconSectionTitle(WidgetTree, TEXT("star_white_outline_64"), TEXT("Favorites"), RICompactUI::ERISectionVisualStyle::Standard, TEXT("RI_FavoritesTitle")));
    UBorder* FavoritesCard = RI_MakeSectionCard(WidgetTree, TEXT("RI_FavoritesCard"));
    FavoritesCard->SetClipping(EWidgetClipping::ClipToBounds);
    FavoritesScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_FavoritesScroll"));
    FavoritesListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FavoritesPanel"));
    FavoritesScrollBox->AddChild(FavoritesListBox);
    FavoritesCard->SetContent(FavoritesScrollBox);
    RI_AddVertical(FavoritesFrameBox, FavoritesCard, FMargin(0.f, 2.f, 0.f, 0.f), ESlateSizeRule::Fill);
    RI_AddVertical(OutPanel, FavoritesFrameSizeBox);
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
    HeaderSurface->SetBrushColor(RICompactUI::GetFooterBackgroundColor());
    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_RightInspectorHeaderRow"));
    if (USizeBox* HeaderIcon = RI_MakeIconBox(WidgetTree, TEXT("RI_RightInspectorHeaderIcon"), TEXT("shape:object"), 16.0f, RICompactUI::GetSuccessTextColor()))
    {
        RI_AddHorizontal(HeaderRow, HeaderIcon, FMargin(0.f, 1.f, RICompactUI::GetInlineGap() + 3.f, 0.f));
    }
    HeaderText = MakeBoundText(TEXT("RI_RightInspectorHeader"));
    RICompactUI::ApplyTextStyle(HeaderText, RICompactUI::GetSectionTitleFontSize() + 3, true, RICompactUI::GetStrongTextColor());
    RI_AddHorizontal(HeaderRow, HeaderText, FMargin(0.f), ESlateSizeRule::Fill);
    HeaderSurface->SetContent(HeaderRow);
    RI_AddVertical(OutPanel, HeaderSurface, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

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
    RI_AddVertical(OutPanel, TabButtonBox, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));

    TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("RI_TabContent"));
    ActorTabPageBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorTabPage"));
    ChangesTabScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ChangesTabScroll"));
    BuildActorTab(ActorTabPageBox);
    BuildChangesTab(ChangesTabScrollBox);
    TabSwitcher->AddChild(ActorTabPageBox);
    TabSwitcher->AddChild(ChangesTabScrollBox);
    RI_AddVertical(OutPanel, TabSwitcher, FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()), ESlateSizeRule::Fill);

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
    RI_AddHorizontal(OnlyModifyBox, OnlyModifyCheckSize);
    RI_AddHorizontal(OnlyModifyBox, RICompactUI::MakeText(WidgetTree, TEXT("Only Modify"), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetSecondaryTextColor()), FMargin(4.f, 0.f, 0.f, 0.f));
    RI_AddHorizontal(ActionTopRow, OnlyModifyBox, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f), ESlateSizeRule::Fill);

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
    RI_AddHorizontal(ActionTopRow, RefreshButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontal(ActionTopRow, ResetButton);
    RI_AddHorizontal(ActionBottomRow, UndoButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontal(ActionBottomRow, RedoButton, FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
    RI_AddHorizontal(ActionBottomRow, ApplyButton);
    RI_AddVertical(ActionBarBox, ActionTopRow, FMargin(0.f, 0.f, 0.f, 3.f));
    RI_AddVertical(ActionBarBox, ActionBottomRow);
    RI_AddVertical(OutPanel, ActionBarBox);

    ActionStatusText = MakeBoundText(TEXT("RI_ActionStatusText"));
    RICompactUI::ApplyTextStyle(ActionStatusText, RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor());
    RI_AddVertical(OutPanel, ActionStatusText, FMargin(0.f, RICompactUI::GetInlineGap(), 0.f, 0.f));
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
    ActorFunctionsFrameSizeBox->SetHeightOverride(RI_DockFunctionsFrameHeight);
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
    if (!TabSwitcher || !Controller || !WidgetTree)
    {
        return;
    }

    UInspectorWorldSubsystem* InspectorSubsystem = Controller->GetSubsystem();
    if (!InspectorSubsystem)
    {
        return;
    }

    BuildHostedActorSections(InspectorSubsystem);

    if (!FilePageWidget)
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

    if (!SettingsPageWidget)
    {
        SettingsPageWidget = CreateWidget<UInspectorSettingsPageWidget>(GetWorld(), UInspectorSettingsPageWidget::StaticClass());
        if (SettingsPageWidget)
        {
            SettingsPageWidget->SetInspectorSubsystem(InspectorSubsystem);
            TabSwitcher->AddChild(SettingsPageWidget);
        }
    }

    if (!ToolsPageWidget)
    {
        ToolsPageWidget = CreateWidget<UInspectorTestPageWidget>(GetWorld(), UInspectorTestPageWidget::StaticClass());
        if (ToolsPageWidget)
        {
            ToolsPageWidget->SetInspectorSubsystem(InspectorSubsystem);
            TabSwitcher->AddChild(ToolsPageWidget);
        }
    }
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
    BuildWidgetTreeIfNeeded();
    BuildHostedPageTabs();
    if (!Controller)
    {
        return;
    }

    CurrentViewModel = Controller->GetCurrentViewModel();
    RefreshLayoutForViewport();
    RefreshActorContext(CurrentViewModel);
    RefreshViewportOverlay(CurrentViewModel);
    RefreshHostedActorSections(CurrentViewModel);
    RefreshChangesTab(CurrentViewModel);
    RefreshTabPresentation(CurrentViewModel);
    RefreshActionBar(CurrentViewModel);

    if (FilePageWidget)
    {
        FilePageWidget->RefreshFastFromSubsystem();
    }
    if (SettingsPageWidget && CurrentViewModel.ActiveTab == ERIInspectorTab::Settings)
    {
        SettingsPageWidget->RefreshFromSubsystem();
    }
    if (ToolsPageWidget && CurrentViewModel.ActiveTab == ERIInspectorTab::Tools)
    {
        ToolsPageWidget->RefreshFromSubsystem();
    }
}

void UInspectorDockRootWidget::RefreshLayoutForViewport()
{
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
    bLeftPanelCompact = ViewportSize.X > 1.0f && ViewportSize.X < RI_DockNarrowCollapseThreshold;
    if (LeftPanelSizeBox)
    {
        LeftPanelSizeBox->SetWidthOverride(bLeftPanelCompact ? RI_DockLeftCompactWidth : RI_DockSidePanelWidth);
    }
    if (RightPanelSizeBox)
    {
        RightPanelSizeBox->SetWidthOverride(RI_DockSidePanelWidth);
    }
}

void UInspectorDockRootWidget::RefreshActorContext(const FRIInspectorViewModel& ViewModel)
{
    if (ActorNameText)
    {
        ActorNameText->SetText(ViewModel.SelectedActor.ActorDisplayName);
        RICompactUI::ApplyTextStyle(ActorNameText, RICompactUI::GetSectionTitleFontSize() + 2, true, RICompactUI::GetStrongTextColor());
    }
    if (ActorClassText)
    {
        ActorClassText->SetText(ViewModel.SelectedActor.ActorClassName);
        RICompactUI::ApplyTextStyle(ActorClassText, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetSecondaryTextColor());
    }
    if (ActorPathText)
    {
        const bool bHidePathForTightPanel = bLeftPanelCompact || RI_DockSidePanelWidth <= 280.0f;
        ActorPathText->SetVisibility(bHidePathForTightPanel ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        ActorPathText->SetText(bHidePathForTightPanel ? FText::GetEmpty() : ViewModel.SelectedActor.ActorPath);
        RICompactUI::ApplyTextStyle(ActorPathText, RICompactUI::GetMutedFontSize(), false, RICompactUI::GetMutedTextColor());
    }
    if (StagedBannerText)
    {
        const int32 PatchCount = ViewModel.StagedPatches.Num();
        StagedBannerText->SetText(FText::FromString(PatchCount > 0 ? FString::Printf(TEXT("%d staged changes"), PatchCount) : TEXT("No staged patch")));
        RICompactUI::ApplyTextStyle(StagedBannerText, RICompactUI::GetLabelFontSize(), true, PatchCount > 0 ? RICompactUI::GetWarningTextColor() : RICompactUI::GetMutedTextColor());
    }
    if (SearchTextBox && Controller)
    {
        const FText DesiredSearchText = FText::FromString(Controller->GetSearchText());
        if (!SearchTextBox->GetText().EqualTo(DesiredSearchText))
        {
            bSuppressSearchTextChanged = true;
            SearchTextBox->SetText(DesiredSearchText);
            bSuppressSearchTextChanged = false;
        }
    }

    ActionProxies.Reset();
    ComponentRowSurfaces.Reset();
    ComponentRowTexts.Reset();

    if (ComponentListBox)
    {
        ComponentListBox->ClearChildren();
        if (bLeftPanelCompact)
        {
            RI_AddVertical(ComponentListBox, RICompactUI::MakeText(WidgetTree, TEXT("..."), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetMutedTextColor()));
        }
        else if (ViewModel.Components.Num() == 0)
        {
            RI_AddVertical(ComponentListBox, RI_MakeIconLabel(WidgetTree, TEXT("shape:components"), TEXT("No components"), 11.0f, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetMutedTextColor(), RICompactUI::GetMutedTextColor()));
        }
        else
        {
            for (const FRIComponentNodeViewModel& Component : ViewModel.Components)
            {
                const FString Prefix = Component.ParentIndex == INDEX_NONE ? TEXT("") : TEXT("  ");
                UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NAME_None);
                UBorder* RowSurface = RICompactUI::MakeSurfaceCard(
                    WidgetTree,
                    NAME_None,
                    Component.bSelected ? RICompactUI::GetSelectedRowSurfaceBackgroundColor() : RICompactUI::GetRowSurfaceBackgroundColor(),
                    RICompactUI::GetSurfaceCardPadding(true));
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NAME_None);
                const FLinearColor RowColor = Component.bSelected ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetSecondaryTextColor();
                if (USizeBox* RowIcon = RI_MakeIconBox(WidgetTree, NAME_None, TEXT("glyph:>"), 9.0f, RowColor))
                {
                    RI_AddHorizontal(Row, RowIcon, FMargin(0.f, 2.f, RICompactUI::GetInlineGap() + 1.f, 0.f));
                }
                const FString ComponentLabel = FString::Printf(TEXT("%s%s  |  %s"), *Prefix, *Component.DisplayName.ToString(), *Component.ClassName.ToString());
                UTextBlock* RowText = RICompactUI::MakeEllipsisText(
                    WidgetTree,
                    ComponentLabel,
                    RICompactUI::GetLabelFontSize(),
                    Component.bSelected,
                    RowColor);
                RI_AddHorizontal(Row, RowText, FMargin(0.f), ESlateSizeRule::Fill);
                RowSurface->SetContent(Row);
                RowButton->AddChild(RowSurface);
                RICompactUI::ConfigureSwatchButton(RowButton);

                const FString RouteComponentName = Component.ComponentName.IsEmpty() ? Component.DisplayName.ToString() : Component.ComponentName;
                ComponentRowSurfaces.Add(RouteComponentName, RowSurface);
                ComponentRowTexts.Add(RouteComponentName, RowText);

                UInspectorDockComponentActionProxy* Proxy = NewObject<UInspectorDockComponentActionProxy>(this);
                Proxy->Owner = this;
                Proxy->ComponentName = RouteComponentName;
                ActionProxies.Add(Proxy);
                RowButton->OnClicked.AddDynamic(Proxy, &UInspectorDockComponentActionProxy::HandleClicked);
                RI_AddVertical(ComponentListBox, RowButton, FMargin(0.f, 0.f, 0.f, 3.f));
            }
        }
    }

    if (FavoritesListBox)
    {
        FavoritesListBox->ClearChildren();
        if (bLeftPanelCompact)
        {
            RI_AddVertical(FavoritesListBox, RICompactUI::MakeText(WidgetTree, TEXT("*"), RICompactUI::GetLabelFontSize(), true, RICompactUI::GetWarningTextColor()));
        }
        else if (ViewModel.Favorites.Num() == 0)
        {
            RI_AddVertical(FavoritesListBox, RI_MakeIconLabel(WidgetTree, TEXT("star_white_outline_64"), TEXT("No favorites"), 11.0f, RICompactUI::GetLabelFontSize(), false, RICompactUI::GetMutedTextColor(), RICompactUI::GetMutedTextColor()));
        }
        else
        {
            for (const FRIFavoriteViewModel& Favorite : ViewModel.Favorites)
            {
                UBorder* RowSurface = RICompactUI::MakeSurfaceCard(
                    WidgetTree,
                    NAME_None,
                    RICompactUI::GetRowSurfaceBackgroundColor(),
                    RICompactUI::GetSurfaceCardPadding(true));
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NAME_None);

                UButton* StarButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NAME_None);
                USizeBox* StarHitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
                StarHitBox->SetWidthOverride(22.0f);
                StarHitBox->SetHeightOverride(22.0f);
                StarHitBox->SetVisibility(ESlateVisibility::HitTestInvisible);
                if (USizeBox* StarIcon = RI_MakeIconBox(WidgetTree, NAME_None, TEXT("star_white_solid_64"), 12.0f, RICompactUI::GetWarningTextColor()))
                {
                    StarIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
                    StarHitBox->SetContent(StarIcon);
                }
                RICompactUI::CenterSizeBoxContent(StarHitBox);
                StarButton->AddChild(StarHitBox);
                RICompactUI::ConfigureGhostIconButton(StarButton);
                if (UButtonSlot* StarSlot = Cast<UButtonSlot>(StarButton->GetContentSlot()))
                {
                    StarSlot->SetHorizontalAlignment(HAlign_Center);
                    StarSlot->SetVerticalAlignment(VAlign_Center);
                    StarSlot->SetPadding(FMargin(0.f));
                }

                UInspectorDockFavoriteActionProxy* ToggleProxy = NewObject<UInspectorDockFavoriteActionProxy>(this);
                ToggleProxy->Owner = this;
                ToggleProxy->SourceItem = Favorite.SourceItem;
                ToggleProxy->bToggleFavorite = true;
                ActionProxies.Add(ToggleProxy);
                StarButton->OnClicked.AddDynamic(ToggleProxy, &UInspectorDockFavoriteActionProxy::HandleClicked);
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
            Surface->SetBrushColor(bSelected ? RICompactUI::GetSelectedRowSurfaceBackgroundColor() : RICompactUI::GetRowSurfaceBackgroundColor());
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
        RowCard->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());
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
    RefreshFromController();
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
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleRefreshClicked()
{
    if (Controller)
    {
        Controller->RequestRefresh();
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleResetClicked()
{
    if (Controller)
    {
        FString Error;
        Controller->RequestReset(Error);
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleUndoClicked()
{
    if (Controller)
    {
        Controller->RequestUndo();
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleRedoClicked()
{
    if (Controller)
    {
        Controller->RequestRedo();
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleApplyClicked()
{
    if (Controller)
    {
        FRIApplyResult Result;
        Controller->RequestApplyStagedPatches(Result);
    }
    RefreshFromController();
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
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleFunctionRunProxyClicked(FName FunctionName)
{
    if (Controller)
    {
        FString Error;
        Controller->RequestRunFunction(FunctionName, Error);
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandlePatchRevertProxyClicked(FGuid PatchId)
{
    if (Controller)
    {
        FRIApplyResult Result;
        Controller->RequestRevertPatch(PatchId, Result);
    }
    RefreshFromController();
}

void UInspectorDockRootWidget::HandleComponentProxyClicked(const FString& ComponentName)
{
    const double StartSeconds = FPlatformTime::Seconds();
    bool bFocusSucceeded = false;
    FString Error;
    if (Controller)
    {
        bFocusSucceeded = Controller->RequestFocusComponentWithRefreshPolicy(ComponentName, Error, false);
    }
    RefreshAfterComponentFocus(ComponentName, bFocusSucceeded);
    LastComponentFocusIntentMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] DockComponentFocusIntent %.2f ms | Component=%s Result=%s Deferred=%d"),
        LastComponentFocusIntentMs,
        *ComponentName,
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
    RefreshFromController();
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
    RefreshFromController();
}

FString UInspectorDockRootWidget::GetDockLayoutDebugSummary() const
{
    const FVector2D ViewportSize = RI_GetDockLogicalViewportSize(const_cast<UInspectorDockRootWidget*>(this));
    const float CenterWidth = RI_GetDockCenterWidth(ViewportSize.X, bLeftPanelCompact);
    const int32 AttributeRows = ActorAttributesWidget ? ActorAttributesWidget->GetEntryWidgetCountForAutomation() : INDEX_NONE;
    const int32 HostedFunctionRows = ActorFunctionsWidget ? ActorFunctionsWidget->GetEntryWidgetCountForAutomation() : INDEX_NONE;
    return FString::Printf(
        TEXT("DockRoot=1 LeftPanel=%s RightPanel=1 SideWidth=%.0f CenterWidth=%.0f CenterPassThrough=1 CenterSelectionPill=0 FavoritesFrame=%d FavoritesScroll=%d FunctionsFrame=%d ActionBar=%d PatchRows=%d FunctionRows=%d AttributeRows=%d AttributesTransform=%d AttributesPending=%d FunctionsPending=%d LastComponentFocusIntentMs=%.2f ActiveTab=%d"),
        bLeftPanelCompact ? TEXT("Compact") : TEXT("Expanded"),
        RI_DockSidePanelWidth,
        CenterWidth,
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
