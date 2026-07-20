#pragma once

#include "CoreMinimal.h"
#include "RuntimeInspector.h"
#include "RuntimeInspectorSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ContentWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "UObject/Package.h"

namespace RICompactUI
{
    enum class ERIThemePreset : uint8
    {
        StudioSlate,
        SoftContrast
    };

    enum class ERIButtonVisualStyle : uint8
    {
        Primary,
        Secondary,
        Subtle,
        Danger,
        Header,
        TabActive,
        TabInactive
    };

    enum class ERISectionVisualStyle : uint8
    {
        Standard,
        Emphasis
    };

    enum class ERIInputVisualStyle : uint8
    {
        Standard,
        Strong,
        Muted
    };

    enum class ERIStatusVisualStyle : uint8
    {
        Normal,
        Strong,
        Success,
        Warning,
        Danger
    };

    struct FRIButtonPalette
    {
        FLinearColor Normal;
        FLinearColor Hovered;
        FLinearColor Pressed;
        FLinearColor Disabled;
        FLinearColor Text;
    };

    struct FRISectionPalette
    {
        FLinearColor Background;
        FLinearColor Text;
    };

    struct FRIInputPalette
    {
        FLinearColor Background;
        FLinearColor Hovered;
        FLinearColor Focused;
        FLinearColor ReadOnly;
        FLinearColor Text;
        FLinearColor Hint;
    };

    struct FRIThemeMetrics
    {
        int32 SectionTitleFontSize = 8;
        int32 LabelFontSize = 7;
        int32 ValueFontSize = 7;
        int32 MutedFontSize = 6;
        float ButtonHeight = 22.0f;
        float InputHeight = 26.0f;
        float CompactListHeight = 96.0f;
        float StandardListHeight = 120.0f;
        float CornerRadius = 4.0f;
        float BorderWidth = 1.0f;
        FMargin SectionPadding = FMargin(6.f, 3.f);
        FMargin ButtonPadding = FMargin(0.f);
        FMargin InputPadding = FMargin(6.f, 4.f);
    };

    struct FRIThemePresetTokens
    {
        FRIThemeMetrics Metrics;
        FLinearColor PageBackground;
        FLinearColor FooterBackground;
        FLinearColor ContextStripBackground;
        FLinearColor ContextPrimaryCellBackground;
        FLinearColor ContextSecondaryCellBackground;
        FLinearColor ContextStatusCellBackground;
        FLinearColor SectionSurfaceBackground;
        FLinearColor RowSurfaceBackground;
        FLinearColor CellSurfaceBackground;
        FLinearColor SelectedRowSurfaceBackground;
        FLinearColor StrongText;
        FLinearColor SecondaryText;
        FLinearColor MutedText;
        FLinearColor SuccessText;
        FLinearColor WarningText;
        FLinearColor ErrorText;
        FRIButtonPalette PrimaryButton;
        FRIButtonPalette SecondaryButton;
        FRIButtonPalette SubtleButton;
        FRIButtonPalette DangerButton;
        FRIButtonPalette HeaderButton;
        FRIButtonPalette TabActiveButton;
        FRIButtonPalette TabInactiveButton;
        FRISectionPalette StandardSection;
        FRISectionPalette EmphasisSection;
        FRIInputPalette StandardInput;
        FRIInputPalette StrongInput;
        FRIInputPalette MutedInput;
    };

    inline FLinearColor MakeTokenColor(const TCHAR* Hex, float Alpha = 1.0f)
    {
        FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
        Color.A = Alpha;
        return Color;
    }

    inline ERIThemePreset GetActiveThemePreset()
    {
        const int32 OverrideValue = RI_GetThemePresetOverrideValue();
        switch (OverrideValue)
        {
        case 0:
            return ERIThemePreset::StudioSlate;
        case 1:
            return ERIThemePreset::SoftContrast;
        default:
            break;
        }

        const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
        if (!Settings)
        {
            return ERIThemePreset::StudioSlate;
        }

        switch (Settings->ThemePreset)
        {
        case ERuntimeInspectorThemePreset::SoftContrast:
            return ERIThemePreset::SoftContrast;
        case ERuntimeInspectorThemePreset::StudioSlate:
        default:
            return ERIThemePreset::StudioSlate;
        }
    }

    inline const FRIThemePresetTokens& GetThemePresetTokens()
    {
        static const FRIThemePresetTokens StudioSlate = []()
        {
            FRIThemePresetTokens Tokens;
            Tokens.Metrics = {
                8, 7, 6, 5,
                20.0f, 20.0f, 96.0f, 122.0f,
                5.0f, 1.0f,
                FMargin(5.f, 3.f),
                FMargin(0.f),
                FMargin(6.f, 2.f)
            };
            Tokens.PageBackground = MakeTokenColor(TEXT("07111E"), 0.94f);
            Tokens.FooterBackground = MakeTokenColor(TEXT("091220"), 0.96f);
            Tokens.ContextStripBackground = MakeTokenColor(TEXT("0B1626"), 0.95f);
            Tokens.ContextPrimaryCellBackground = MakeTokenColor(TEXT("18283D"), 0.98f);
            Tokens.ContextSecondaryCellBackground = MakeTokenColor(TEXT("101B2B"), 0.96f);
            Tokens.ContextStatusCellBackground = MakeTokenColor(TEXT("402C10"), 0.98f);
            Tokens.SectionSurfaceBackground = MakeTokenColor(TEXT("142234"), 0.93f);
            Tokens.RowSurfaceBackground = MakeTokenColor(TEXT("0F1B2B"), 0.92f);
            Tokens.CellSurfaceBackground = MakeTokenColor(TEXT("132236"), 0.93f);
            Tokens.SelectedRowSurfaceBackground = MakeTokenColor(TEXT("1A3652"), 0.95f);
            Tokens.StrongText = MakeTokenColor(TEXT("F2F6FA"));
            Tokens.SecondaryText = MakeTokenColor(TEXT("AAB6C5"));
            Tokens.MutedText = MakeTokenColor(TEXT("6E7B8C"));
            Tokens.SuccessText = FLinearColor(0.42f, 0.92f, 0.58f, 1.0f);
            Tokens.WarningText = FLinearColor(0.95f, 0.75f, 0.25f, 1.0f);
            Tokens.ErrorText = FLinearColor(0.95f, 0.35f, 0.35f, 1.0f);
            Tokens.PrimaryButton = {
                MakeTokenColor(TEXT("2F8CFF")),
                MakeTokenColor(TEXT("46C8FF")),
                MakeTokenColor(TEXT("1658B4")),
                MakeTokenColor(TEXT("17202B"), 0.65f),
                FLinearColor(0.98f, 0.99f, 1.0f, 1.0f)
            };
            Tokens.SecondaryButton = {
                MakeTokenColor(TEXT("172538")),
                MakeTokenColor(TEXT("1F344D")),
                MakeTokenColor(TEXT("0F1B2A")),
                MakeTokenColor(TEXT("0F141C"), 0.65f),
                MakeTokenColor(TEXT("E5ECF4"))
            };
            Tokens.SubtleButton = {
                MakeTokenColor(TEXT("121C2A")),
                MakeTokenColor(TEXT("18283D")),
                MakeTokenColor(TEXT("0B1420")),
                MakeTokenColor(TEXT("0B121A"), 0.65f),
                MakeTokenColor(TEXT("DDE6EF"))
            };
            Tokens.DangerButton = {
                FLinearColor(0.55f, 0.19f, 0.22f, 1.0f),
                FLinearColor(0.67f, 0.24f, 0.28f, 1.0f),
                FLinearColor(0.42f, 0.14f, 0.17f, 1.0f),
                FLinearColor(0.23f, 0.18f, 0.19f, 0.65f),
                FLinearColor(1.0f, 0.96f, 0.96f, 1.0f)
            };
            Tokens.HeaderButton = {
                MakeTokenColor(TEXT("0B1320")),
                MakeTokenColor(TEXT("122034")),
                MakeTokenColor(TEXT("080D16")),
                MakeTokenColor(TEXT("080D16"), 0.75f),
                MakeTokenColor(TEXT("EDF2F7"))
            };
            Tokens.TabActiveButton = {
                MakeTokenColor(TEXT("173858"), 0.94f),
                MakeTokenColor(TEXT("214D73"), 0.96f),
                MakeTokenColor(TEXT("102B45"), 0.96f),
                MakeTokenColor(TEXT("0D1420"), 0.75f),
                MakeTokenColor(TEXT("F2F6FA"))
            };
            Tokens.TabInactiveButton = {
                MakeTokenColor(TEXT("090F18"), 0.76f),
                MakeTokenColor(TEXT("121F30"), 0.88f),
                MakeTokenColor(TEXT("080D15"), 0.90f),
                MakeTokenColor(TEXT("09101A"), 0.75f),
                MakeTokenColor(TEXT("AEBBCC"))
            };
            Tokens.StandardSection = {
                MakeTokenColor(TEXT("17283C"), 0.90f),
                MakeTokenColor(TEXT("EDF2F7"))
            };
            Tokens.EmphasisSection = {
                MakeTokenColor(TEXT("1C3854"), 0.92f),
                MakeTokenColor(TEXT("F5F9FC"))
            };
            Tokens.StandardInput = {
                MakeTokenColor(TEXT("132236"), 0.96f),
                MakeTokenColor(TEXT("182A3F")),
                MakeTokenColor(TEXT("1B4A87")),
                MakeTokenColor(TEXT("0E1620"), 0.72f),
                MakeTokenColor(TEXT("EDF2F7")),
                MakeTokenColor(TEXT("8290A3"))
            };
            Tokens.StrongInput = {
                MakeTokenColor(TEXT("16263B"), 0.98f),
                MakeTokenColor(TEXT("1D344F")),
                MakeTokenColor(TEXT("1E5AA3")),
                MakeTokenColor(TEXT("101925"), 0.75f),
                MakeTokenColor(TEXT("F3F8FC")),
                MakeTokenColor(TEXT("8A9FBD"))
            };
            Tokens.MutedInput = {
                MakeTokenColor(TEXT("0F1B2A"), 0.90f),
                MakeTokenColor(TEXT("132236"), 0.95f),
                MakeTokenColor(TEXT("182638"), 0.95f),
                MakeTokenColor(TEXT("0B141E"), 0.70f),
                MakeTokenColor(TEXT("D9E2EC")),
                MakeTokenColor(TEXT("6F7D90"))
            };
            return Tokens;
        }();

        static const FRIThemePresetTokens SoftContrast = []()
        {
            FRIThemePresetTokens Tokens = StudioSlate;
            Tokens.Metrics.CornerRadius = 7.0f;
            Tokens.Metrics.BorderWidth = 1.25f;
            Tokens.PageBackground = MakeTokenColor(TEXT("081424"), 0.94f);
            Tokens.FooterBackground = MakeTokenColor(TEXT("0B1624"), 0.97f);
            Tokens.ContextStripBackground = MakeTokenColor(TEXT("0D1728"), 0.96f);
            Tokens.ContextPrimaryCellBackground = MakeTokenColor(TEXT("182A3F"), 0.99f);
            Tokens.ContextSecondaryCellBackground = MakeTokenColor(TEXT("132236"), 0.96f);
            Tokens.ContextStatusCellBackground = MakeTokenColor(TEXT("4D3514"), 0.99f);
            Tokens.SectionSurfaceBackground = MakeTokenColor(TEXT("17283D"), 0.95f);
            Tokens.RowSurfaceBackground = MakeTokenColor(TEXT("101B2B"), 0.93f);
            Tokens.CellSurfaceBackground = MakeTokenColor(TEXT("142234"), 0.94f);
            Tokens.SelectedRowSurfaceBackground = MakeTokenColor(TEXT("1D3D5C"), 0.96f);
            Tokens.StrongText = FLinearColor(0.98f, 0.98f, 0.99f, 1.0f);
            Tokens.SecondaryText = FLinearColor(0.88f, 0.91f, 0.95f, 1.0f);
            Tokens.MutedText = FLinearColor(0.78f, 0.80f, 0.84f, 1.0f);
            Tokens.SuccessText = FLinearColor(0.52f, 0.88f, 0.68f, 1.0f);
            Tokens.WarningText = FLinearColor(0.96f, 0.79f, 0.40f, 1.0f);
            Tokens.ErrorText = FLinearColor(0.95f, 0.49f, 0.49f, 1.0f);
            Tokens.Metrics.SectionTitleFontSize = 8;
            Tokens.Metrics.LabelFontSize = 7;
            Tokens.Metrics.ValueFontSize = 6;
            Tokens.Metrics.MutedFontSize = 5;
            Tokens.Metrics.ButtonHeight = 22.0f;
            Tokens.Metrics.InputHeight = 22.0f;
            Tokens.Metrics.CompactListHeight = 94.0f;
            Tokens.Metrics.StandardListHeight = 120.0f;
            Tokens.Metrics.SectionPadding = FMargin(5.f, 3.f);
            Tokens.Metrics.InputPadding = FMargin(7.f, 4.f);
            Tokens.PrimaryButton.Normal = MakeTokenColor(TEXT("2F8CFF"));
            Tokens.PrimaryButton.Hovered = MakeTokenColor(TEXT("46C8FF"));
            Tokens.SecondaryButton.Normal = MakeTokenColor(TEXT("1B2B41"));
            Tokens.SecondaryButton.Hovered = MakeTokenColor(TEXT("223952"));
            Tokens.SecondaryButton.Pressed = MakeTokenColor(TEXT("142133"));
            Tokens.SubtleButton.Normal = MakeTokenColor(TEXT("152335"));
            Tokens.SubtleButton.Hovered = MakeTokenColor(TEXT("1C2D43"));
            Tokens.SubtleButton.Pressed = MakeTokenColor(TEXT("101A29"));
            Tokens.HeaderButton.Normal = MakeTokenColor(TEXT("101927"));
            Tokens.HeaderButton.Hovered = MakeTokenColor(TEXT("17283D"));
            Tokens.HeaderButton.Pressed = MakeTokenColor(TEXT("0B121E"));
            Tokens.TabActiveButton.Normal = MakeTokenColor(TEXT("1A456B"), 0.94f);
            Tokens.TabActiveButton.Hovered = MakeTokenColor(TEXT("265A86"), 0.96f);
            Tokens.TabActiveButton.Pressed = MakeTokenColor(TEXT("173858"), 0.96f);
            Tokens.TabInactiveButton.Normal = MakeTokenColor(TEXT("0D1623"), 0.78f);
            Tokens.TabInactiveButton.Hovered = MakeTokenColor(TEXT("172638"), 0.90f);
            Tokens.TabInactiveButton.Pressed = MakeTokenColor(TEXT("0A111C"), 0.92f);
            Tokens.StandardSection.Background = MakeTokenColor(TEXT("1A2D42"), 0.91f);
            Tokens.EmphasisSection.Background = MakeTokenColor(TEXT("213F5B"), 0.93f);
            Tokens.StandardInput.Background = MakeTokenColor(TEXT("17283D"), 0.97f);
            Tokens.StandardInput.Hovered = MakeTokenColor(TEXT("1C2F46"));
            Tokens.StandardInput.Focused = FLinearColor(0.25f, 0.39f, 0.72f, 1.0f);
            Tokens.StrongInput.Background = FLinearColor(0.18f, 0.22f, 0.29f, 0.98f);
            Tokens.StrongInput.Hovered = FLinearColor(0.22f, 0.27f, 0.35f, 1.0f);
            Tokens.StrongInput.Focused = FLinearColor(0.28f, 0.45f, 0.78f, 1.0f);
            return Tokens;
        }();

        switch (GetActiveThemePreset())
        {
        case ERIThemePreset::SoftContrast:
            return SoftContrast;
        case ERIThemePreset::StudioSlate:
        default:
            return StudioSlate;
        }
    }

    inline const FRIThemeMetrics& GetThemeMetrics()
    {
        return GetThemePresetTokens().Metrics;
    }

    inline float& ReadableScaleOverride()
    {
        static float Scale = 1.0f;
        return Scale;
    }

    inline void SetReadableScaleOverride(float InScale)
    {
        ReadableScaleOverride() = FMath::Clamp(InScale, 1.0f, 1.25f);
    }

    inline float GetReadableScaleOverride()
    {
        return ReadableScaleOverride();
    }

    inline int32 ScaleReadableFontSize(int32 BaseSize)
    {
        return FMath::Max(BaseSize, FMath::RoundToInt(static_cast<float>(BaseSize) * GetReadableScaleOverride()));
    }

    inline int32 GetSectionTitleFontSize()
    {
        return ScaleReadableFontSize(GetThemeMetrics().SectionTitleFontSize);
    }

    inline int32 GetLabelFontSize()
    {
        return ScaleReadableFontSize(GetThemeMetrics().LabelFontSize);
    }

    inline int32 GetValueFontSize()
    {
        return ScaleReadableFontSize(GetThemeMetrics().ValueFontSize);
    }

    inline int32 GetMutedFontSize()
    {
        return ScaleReadableFontSize(GetThemeMetrics().MutedFontSize);
    }

    inline float GetButtonHeight()
    {
        return GetThemeMetrics().ButtonHeight * GetReadableScaleOverride();
    }

    inline float GetInputHeight()
    {
        return GetThemeMetrics().InputHeight * GetReadableScaleOverride();
    }

    inline float GetCompactListHeight()
    {
        return GetThemeMetrics().CompactListHeight;
    }

    inline float GetStandardListHeight()
    {
        return GetThemeMetrics().StandardListHeight;
    }

    inline FMargin GetPanelPadding()
    {
        return GetActiveThemePreset() == ERIThemePreset::SoftContrast
            ? FMargin(8.f, 6.f)
            : FMargin(6.f, 5.f);
    }

    inline FMargin GetSurfaceCardPadding(bool bDense = false)
    {
        if (bDense)
        {
            return GetActiveThemePreset() == ERIThemePreset::SoftContrast
                ? FMargin(6.f, 5.f)
                : FMargin(5.f, 4.f);
        }

        return GetActiveThemePreset() == ERIThemePreset::SoftContrast
            ? FMargin(8.f, 6.f)
            : FMargin(6.f, 5.f);
    }

    inline float GetSectionGap()
    {
        return GetActiveThemePreset() == ERIThemePreset::SoftContrast ? 6.f : 5.f;
    }

    inline float GetInlineGap()
    {
        return GetActiveThemePreset() == ERIThemePreset::SoftContrast ? 4.f : 3.f;
    }

    inline FLinearColor GetPageBackgroundColor()
    {
        return GetThemePresetTokens().PageBackground;
    }

    inline FLinearColor GetFooterBackgroundColor()
    {
        return GetThemePresetTokens().FooterBackground;
    }

    inline FLinearColor GetContextStripBackgroundColor()
    {
        return GetThemePresetTokens().ContextStripBackground;
    }

    inline FLinearColor GetContextPrimaryCellBackgroundColor()
    {
        return GetThemePresetTokens().ContextPrimaryCellBackground;
    }

    inline FLinearColor GetContextSecondaryCellBackgroundColor()
    {
        return GetThemePresetTokens().ContextSecondaryCellBackground;
    }

    inline FLinearColor GetContextStatusCellBackgroundColor()
    {
        return GetThemePresetTokens().ContextStatusCellBackground;
    }

    inline FLinearColor GetSectionSurfaceBackgroundColor()
    {
        return GetThemePresetTokens().SectionSurfaceBackground;
    }

    inline FLinearColor GetRowSurfaceBackgroundColor()
    {
        return GetThemePresetTokens().RowSurfaceBackground;
    }

    inline FLinearColor GetCellSurfaceBackgroundColor()
    {
        return GetThemePresetTokens().CellSurfaceBackground;
    }

    inline FLinearColor GetSelectedRowSurfaceBackgroundColor()
    {
        return GetThemePresetTokens().SelectedRowSurfaceBackground;
    }

    inline FLinearColor GetStrongTextColor()
    {
        return GetThemePresetTokens().StrongText;
    }

    inline FLinearColor GetSecondaryTextColor()
    {
        return GetThemePresetTokens().SecondaryText;
    }

    inline FLinearColor GetMutedTextColor()
    {
        return GetThemePresetTokens().MutedText;
    }

    inline FLinearColor GetSuccessTextColor()
    {
        return GetThemePresetTokens().SuccessText;
    }

    inline FLinearColor GetWarningTextColor()
    {
        return GetThemePresetTokens().WarningText;
    }

    inline FLinearColor GetErrorTextColor()
    {
        return GetThemePresetTokens().ErrorText;
    }

    inline FLinearColor GetAxisAccentColor(int32 AxisIndex)
    {
        switch (AxisIndex)
        {
        case 0:
            return FLinearColor(0.88f, 0.34f, 0.28f, 1.0f);
        case 1:
            return FLinearColor(0.42f, 0.76f, 0.34f, 1.0f);
        case 2:
        default:
            return FLinearColor(0.35f, 0.58f, 0.92f, 1.0f);
        }
    }

    inline FLinearColor GetStatusTextColor(ERIStatusVisualStyle Style)
    {
        switch (Style)
        {
        case ERIStatusVisualStyle::Strong:
            return GetStrongTextColor();
        case ERIStatusVisualStyle::Success:
            return GetSuccessTextColor();
        case ERIStatusVisualStyle::Warning:
            return GetWarningTextColor();
        case ERIStatusVisualStyle::Danger:
            return GetErrorTextColor();
        case ERIStatusVisualStyle::Normal:
        default:
            return GetSecondaryTextColor();
        }
    }

    inline FRIButtonPalette GetButtonPalette(ERIButtonVisualStyle Style)
    {
        const FRIThemePresetTokens& Theme = GetThemePresetTokens();
        switch (Style)
        {
        case ERIButtonVisualStyle::Primary:
            return Theme.PrimaryButton;
        case ERIButtonVisualStyle::Danger:
            return Theme.DangerButton;
        case ERIButtonVisualStyle::Header:
            return Theme.HeaderButton;
        case ERIButtonVisualStyle::TabActive:
            return Theme.TabActiveButton;
        case ERIButtonVisualStyle::TabInactive:
            return Theme.TabInactiveButton;
        case ERIButtonVisualStyle::Subtle:
            return Theme.SubtleButton;
        case ERIButtonVisualStyle::Secondary:
        default:
            return Theme.SecondaryButton;
        }
    }

    inline FLinearColor GetButtonFillColor(ERIButtonVisualStyle Style)
    {
        return GetButtonPalette(Style).Normal;
    }

    inline FLinearColor GetButtonTextColor(ERIButtonVisualStyle Style)
    {
        return GetButtonPalette(Style).Text;
    }

    inline FRISectionPalette GetSectionPalette(ERISectionVisualStyle Style)
    {
        const FRIThemePresetTokens& Theme = GetThemePresetTokens();
        switch (Style)
        {
        case ERISectionVisualStyle::Emphasis:
            return Theme.EmphasisSection;
        case ERISectionVisualStyle::Standard:
        default:
            return Theme.StandardSection;
        }
    }

    inline FRIInputPalette GetInputPalette(ERIInputVisualStyle Style)
    {
        const FRIThemePresetTokens& Theme = GetThemePresetTokens();
        switch (Style)
        {
        case ERIInputVisualStyle::Strong:
            return Theme.StrongInput;
        case ERIInputVisualStyle::Muted:
            return Theme.MutedInput;
        case ERIInputVisualStyle::Standard:
        default:
            return Theme.StandardInput;
        }
    }

    inline void ApplyTextStyle(UTextBlock* TextBlock, int32 Size, bool bBold, const FLinearColor& Color)
    {
        if (!TextBlock)
        {
            return;
        }

        FSlateFontInfo Font = TextBlock->GetFont();
        Font.Size = Size;
        Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
        TextBlock->SetFont(Font);
        TextBlock->SetColorAndOpacity(FSlateColor(Color));
    }

    inline UTextBlock* MakeText(UWidgetTree* WidgetTree, const FString& InText, int32 Size, bool bBold, const FLinearColor& Color, bool bWrap = false)
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(FText::FromString(InText));
        Text->SetAutoWrapText(bWrap);
        Text->SetClipping(EWidgetClipping::ClipToBounds);
        ApplyTextStyle(Text, Size, bBold, Color);
        return Text;
    }

    inline void ConfigureEllipsisText(UTextBlock* Text, const FString& FullText = FString())
    {
        if (!Text)
        {
            return;
        }

        Text->SetAutoWrapText(false);
        Text->SetClipping(EWidgetClipping::ClipToBounds);
        Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
        Text->SetToolTipText(FullText.IsEmpty() ? FText::GetEmpty() : FText::FromString(FullText));
    }

    inline UTextBlock* MakeEllipsisText(
        UWidgetTree* WidgetTree,
        const FString& InText,
        int32 Size,
        bool bBold,
        const FLinearColor& Color)
    {
        UTextBlock* Text = MakeText(WidgetTree, InText, Size, bBold, Color, false);
        ConfigureEllipsisText(Text, InText);
        return Text;
    }

    inline UTexture2D* GetGeneratedShapeIconTexture(const FString& Shape);

    inline UTexture2D* GetFavoriteIconTexture(bool bFavorited)
    {
        if (UTexture2D* GeneratedTexture = GetGeneratedShapeIconTexture(bFavorited ? TEXT("star-solid") : TEXT("star-outline")))
        {
            return GeneratedTexture;
        }

        static TWeakObjectPtr<UTexture2D> OutlineTexture;
        static TWeakObjectPtr<UTexture2D> SolidTexture;

        TWeakObjectPtr<UTexture2D>& CachedTexture = bFavorited ? SolidTexture : OutlineTexture;
        if (!CachedTexture.IsValid())
        {
            const TCHAR* AssetPath = bFavorited
                ? TEXT("/RuntimeInspector/UI/Assets/star_white_solid_64.star_white_solid_64")
                : TEXT("/RuntimeInspector/UI/Assets/star_white_outline_64.star_white_outline_64");
            CachedTexture = LoadObject<UTexture2D>(nullptr, AssetPath);
        }

        return CachedTexture.Get();
    }

    inline UTexture2D* LoadIconTexture(const TCHAR* AssetName)
    {
        if (!AssetName || AssetName[0] == TEXT('\0'))
        {
            return nullptr;
        }

        static TMap<FString, TWeakObjectPtr<UTexture2D>> CachedTextures;
        const FString IconName(AssetName);
        TWeakObjectPtr<UTexture2D>& CachedTexture = CachedTextures.FindOrAdd(IconName);
        if (!CachedTexture.IsValid())
        {
            const FString AssetPath = FString::Printf(TEXT("/RuntimeInspector/UI/Assets/%s.%s"), *IconName, *IconName);
            CachedTexture = LoadObject<UTexture2D>(nullptr, *AssetPath);
        }

        return CachedTexture.Get();
    }

    inline UImage* MakeIcon(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const TCHAR* AssetName,
        float DesiredSize,
        const FLinearColor& Tint)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
        if (UTexture2D* Texture = LoadIconTexture(AssetName))
        {
            Image->SetBrushFromTexture(Texture, false);
        }
        if (DesiredSize > 0.f)
        {
            FSlateBrush Brush = Image->GetBrush();
            Brush.ImageSize = FVector2D(DesiredSize, DesiredSize);
            Image->SetBrush(Brush);
            Image->SetDesiredSizeOverride(FVector2D(DesiredSize, DesiredSize));
        }
        Image->SetColorAndOpacity(Tint);
        return Image;
    }

    inline bool TryGetGlyphIcon(const TCHAR* IconName, FString& OutGlyph)
    {
        OutGlyph.Reset();
        if (!IconName || IconName[0] == TEXT('\0'))
        {
            return false;
        }

        const FString IconString(IconName);
        static const FString GlyphPrefix(TEXT("glyph:"));
        if (!IconString.StartsWith(GlyphPrefix, ESearchCase::CaseSensitive))
        {
            return false;
        }

        OutGlyph = IconString.Mid(GlyphPrefix.Len());
        return !OutGlyph.IsEmpty();
    }

    inline bool TryGetShapeIcon(const TCHAR* IconName, FString& OutShape)
    {
        OutShape.Reset();
        if (!IconName || IconName[0] == TEXT('\0'))
        {
            return false;
        }

        const FString IconString(IconName);
        static const FString ShapePrefix(TEXT("shape:"));
        if (!IconString.StartsWith(ShapePrefix, ESearchCase::CaseSensitive))
        {
            return false;
        }

        OutShape = IconString.Mid(ShapePrefix.Len());
        return !OutShape.IsEmpty();
    }

    inline UTextBlock* MakeGlyphIcon(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FString& Glyph,
        float DesiredSize,
        const FLinearColor& Tint)
    {
        if (!WidgetTree || Glyph.IsEmpty())
        {
            return nullptr;
        }

        UTextBlock* Icon = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Icon->SetText(FText::FromString(Glyph));
        Icon->SetAutoWrapText(false);
        Icon->SetClipping(EWidgetClipping::ClipToBounds);
        Icon->SetJustification(ETextJustify::Center);
        Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        ApplyTextStyle(Icon, FMath::Max(6, FMath::RoundToInt(DesiredSize)), true, Tint);
        return Icon;
    }

    inline UWidget* MakeIconLine(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FLinearColor& Tint,
        float Width,
        float Height)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UBorder* Line = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(Tint);
        Brush.OutlineSettings.CornerRadii = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
        Line->SetBrush(Brush);
        Line->SetPadding(FMargin(0.f));
        Line->SetVisibility(ESlateVisibility::HitTestInvisible);

        USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LineSize->SetWidthOverride(Width);
        LineSize->SetHeightOverride(Height);
        LineSize->SetContent(Line);
        if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(LineSize->GetContentSlot()))
        {
            SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
            SizeBoxSlot->SetPadding(FMargin(0.f));
        }
        LineSize->SetVisibility(ESlateVisibility::HitTestInvisible);
        return LineSize;
    }

    inline void AddGeneratedIconPart(
        UWidgetTree* WidgetTree,
        UOverlay* Icon,
        const FLinearColor& Tint,
        float Width,
        float Height,
        const FMargin& Padding,
        EHorizontalAlignment HorizontalAlignment,
        EVerticalAlignment VerticalAlignment,
        float AngleDegrees = 0.0f)
    {
        if (!WidgetTree || !Icon)
        {
            return;
        }

        UWidget* Part = MakeIconLine(WidgetTree, NAME_None, Tint, Width, Height);
        if (!Part)
        {
            return;
        }
        if (!FMath::IsNearlyZero(AngleDegrees))
        {
            Part->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            Part->SetRenderTransformAngle(AngleDegrees);
        }

        if (UOverlaySlot* Slot = Icon->AddChildToOverlay(Part))
        {
            Slot->SetPadding(Padding);
            Slot->SetHorizontalAlignment(HorizontalAlignment);
            Slot->SetVerticalAlignment(VerticalAlignment);
        }
    }

    inline void AddGeneratedIconFrameAt(
        UWidgetTree* WidgetTree,
        UOverlay* Icon,
        const FLinearColor& Tint,
        float Left,
        float Top,
        float Size,
        float Stroke)
    {
        AddGeneratedIconPart(WidgetTree, Icon, Tint, Size, Stroke, FMargin(Left, Top, 0.f, 0.f), HAlign_Left, VAlign_Top);
        AddGeneratedIconPart(WidgetTree, Icon, Tint, Size, Stroke, FMargin(Left, Top + Size - Stroke, 0.f, 0.f), HAlign_Left, VAlign_Top);
        AddGeneratedIconPart(WidgetTree, Icon, Tint, Stroke, Size, FMargin(Left, Top, 0.f, 0.f), HAlign_Left, VAlign_Top);
        AddGeneratedIconPart(WidgetTree, Icon, Tint, Stroke, Size, FMargin(Left + Size - Stroke, Top, 0.f, 0.f), HAlign_Left, VAlign_Top);
    }

    inline float GetGeneratedIconDistanceToSegment(const FVector2D& Point, const FVector2D& Start, const FVector2D& End)
    {
        const FVector2D Segment = End - Start;
        const float SegmentLengthSquared = Segment.SizeSquared();
        if (SegmentLengthSquared <= KINDA_SMALL_NUMBER)
        {
            return FVector2D::Distance(Point, Start);
        }

        const float T = FMath::Clamp(FVector2D::DotProduct(Point - Start, Segment) / SegmentLengthSquared, 0.0f, 1.0f);
        return FVector2D::Distance(Point, Start + Segment * T);
    }

    inline void BlendGeneratedIconAlpha(TArray<float>& Alpha, int32 TextureSize, int32 X, int32 Y, float Coverage)
    {
        if (X < 0 || Y < 0 || X >= TextureSize || Y >= TextureSize)
        {
            return;
        }

        const int32 Index = Y * TextureSize + X;
        Alpha[Index] = FMath::Max(Alpha[Index], FMath::Clamp(Coverage, 0.0f, 1.0f));
    }

    inline void DrawGeneratedIconSegment(TArray<float>& Alpha, int32 TextureSize, const FVector2D& Start, const FVector2D& End, float Stroke)
    {
        const float HalfStroke = Stroke * 0.5f;
        const float Feather = 1.35f / static_cast<float>(TextureSize);
        for (int32 Y = 0; Y < TextureSize; ++Y)
        {
            for (int32 X = 0; X < TextureSize; ++X)
            {
                const FVector2D Point(
                    (static_cast<float>(X) + 0.5f) / static_cast<float>(TextureSize),
                    (static_cast<float>(Y) + 0.5f) / static_cast<float>(TextureSize));
                const float Distance = GetGeneratedIconDistanceToSegment(Point, Start, End);
                const float Coverage = (HalfStroke + Feather - Distance) / Feather;
                if (Coverage > 0.0f)
                {
                    BlendGeneratedIconAlpha(Alpha, TextureSize, X, Y, Coverage);
                }
            }
        }
    }

    inline void DrawGeneratedIconCircle(TArray<float>& Alpha, int32 TextureSize, const FVector2D& Center, float Radius, bool bFilled, float Stroke = 0.08f)
    {
        const float Feather = 1.35f / static_cast<float>(TextureSize);
        const float HalfStroke = Stroke * 0.5f;
        for (int32 Y = 0; Y < TextureSize; ++Y)
        {
            for (int32 X = 0; X < TextureSize; ++X)
            {
                const FVector2D Point(
                    (static_cast<float>(X) + 0.5f) / static_cast<float>(TextureSize),
                    (static_cast<float>(Y) + 0.5f) / static_cast<float>(TextureSize));
                const float Distance = FVector2D::Distance(Point, Center);
                const float EdgeDistance = bFilled ? Distance : FMath::Abs(Distance - Radius);
                const float Target = bFilled ? Radius : HalfStroke;
                const float Coverage = (Target + Feather - EdgeDistance) / Feather;
                if (Coverage > 0.0f)
                {
                    BlendGeneratedIconAlpha(Alpha, TextureSize, X, Y, Coverage);
                }
            }
        }
    }

    inline void DrawGeneratedIconRectOutline(TArray<float>& Alpha, int32 TextureSize, const FVector2D& Min, const FVector2D& Max, float Stroke)
    {
        DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(Min.X, Min.Y), FVector2D(Max.X, Min.Y), Stroke);
        DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(Max.X, Min.Y), FVector2D(Max.X, Max.Y), Stroke);
        DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(Max.X, Max.Y), FVector2D(Min.X, Max.Y), Stroke);
        DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(Min.X, Max.Y), FVector2D(Min.X, Min.Y), Stroke);
    }

    inline TArray<FVector2D> MakeGeneratedStarPoints()
    {
        TArray<FVector2D> Points;
        Points.Reserve(10);
        constexpr float Pi = 3.14159265358979323846f;
        for (int32 Index = 0; Index < 10; ++Index)
        {
            const float Angle = (-90.0f + static_cast<float>(Index) * 36.0f) * Pi / 180.0f;
            const float Radius = (Index % 2 == 0) ? 0.34f : 0.15f;
            Points.Add(FVector2D(0.50f + FMath::Cos(Angle) * Radius, 0.52f + FMath::Sin(Angle) * Radius));
        }
        return Points;
    }

    inline bool IsGeneratedPointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Points)
    {
        if (Points.Num() < 3)
        {
            return false;
        }

        bool bInside = false;
        for (int32 Index = 0, Prev = Points.Num() - 1; Index < Points.Num(); Prev = Index++)
        {
            const FVector2D& A = Points[Index];
            const FVector2D& B = Points[Prev];
            const float Denominator = FMath::Abs(B.Y - A.Y) <= KINDA_SMALL_NUMBER ? KINDA_SMALL_NUMBER : (B.Y - A.Y);
            const bool bIntersects = ((A.Y > Point.Y) != (B.Y > Point.Y))
                && (Point.X < (B.X - A.X) * (Point.Y - A.Y) / Denominator + A.X);
            if (bIntersects)
            {
                bInside = !bInside;
            }
        }
        return bInside;
    }

    inline float GetGeneratedPolygonEdgeDistance(const FVector2D& Point, const TArray<FVector2D>& Points)
    {
        float MinDistance = TNumericLimits<float>::Max();
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            const FVector2D& A = Points[Index];
            const FVector2D& B = Points[(Index + 1) % Points.Num()];
            MinDistance = FMath::Min(MinDistance, GetGeneratedIconDistanceToSegment(Point, A, B));
        }
        return MinDistance;
    }

    inline void DrawGeneratedIconClosedPolyline(TArray<float>& Alpha, int32 TextureSize, const TArray<FVector2D>& Points, float Stroke)
    {
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            DrawGeneratedIconSegment(Alpha, TextureSize, Points[Index], Points[(Index + 1) % Points.Num()], Stroke);
        }
    }

    inline void DrawGeneratedIconFilledPolygon(TArray<float>& Alpha, int32 TextureSize, const TArray<FVector2D>& Points)
    {
        const float Feather = 1.35f / static_cast<float>(TextureSize);
        for (int32 Y = 0; Y < TextureSize; ++Y)
        {
            for (int32 X = 0; X < TextureSize; ++X)
            {
                const FVector2D Point(
                    (static_cast<float>(X) + 0.5f) / static_cast<float>(TextureSize),
                    (static_cast<float>(Y) + 0.5f) / static_cast<float>(TextureSize));
                const float Distance = GetGeneratedPolygonEdgeDistance(Point, Points);
                const bool bInside = IsGeneratedPointInPolygon(Point, Points);
                const float Coverage = bInside ? 1.0f : (Feather - Distance) / Feather;
                if (Coverage > 0.0f)
                {
                    BlendGeneratedIconAlpha(Alpha, TextureSize, X, Y, Coverage);
                }
            }
        }
    }

    inline void DrawGeneratedIconShapeAlpha(const FString& ShapeKey, TArray<float>& Alpha, int32 TextureSize)
    {
        const float Stroke = 0.085f;
        if (ShapeKey.Equals(TEXT("object"), ESearchCase::IgnoreCase))
        {
            const float Corner = 0.18f;
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.22f, 0.22f), FVector2D(0.22f + Corner, 0.22f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.22f, 0.22f), FVector2D(0.22f, 0.22f + Corner), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.78f, 0.22f), FVector2D(0.78f - Corner, 0.22f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.78f, 0.22f), FVector2D(0.78f, 0.22f + Corner), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.22f, 0.78f), FVector2D(0.22f + Corner, 0.78f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.22f, 0.78f), FVector2D(0.22f, 0.78f - Corner), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.78f, 0.78f), FVector2D(0.78f - Corner, 0.78f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.78f, 0.78f), FVector2D(0.78f, 0.78f - Corner), Stroke);
        }
        else if (ShapeKey.Equals(TEXT("components"), ESearchCase::IgnoreCase))
        {
            const FVector2D NodeSize(0.13f, 0.13f);
            const TArray<float> Rows = { 0.25f, 0.50f, 0.75f };
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.30f, Rows[0]), FVector2D(0.30f, Rows[2]), Stroke * 0.72f);
            for (float Y : Rows)
            {
                DrawGeneratedIconRectOutline(
                    Alpha,
                    TextureSize,
                    FVector2D(0.20f, Y - NodeSize.Y * 0.5f),
                    FVector2D(0.20f + NodeSize.X, Y + NodeSize.Y * 0.5f),
                    Stroke * 0.72f);
                DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.33f, Y), FVector2D(0.78f, Y), Stroke * 0.72f);
            }
        }
        else if (ShapeKey.Equals(TEXT("status"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.50f, 0.18f), FVector2D(0.82f, 0.78f), Stroke * 0.92f);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.82f, 0.78f), FVector2D(0.18f, 0.78f), Stroke * 0.92f);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.18f, 0.78f), FVector2D(0.50f, 0.18f), Stroke * 0.92f);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.50f, 0.39f), FVector2D(0.50f, 0.57f), Stroke * 0.72f);
            DrawGeneratedIconCircle(Alpha, TextureSize, FVector2D(0.50f, 0.68f), 0.035f, true);
        }
        else if (ShapeKey.Equals(TEXT("search"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconCircle(Alpha, TextureSize, FVector2D(0.44f, 0.43f), 0.20f, false, Stroke * 0.88f);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.59f, 0.58f), FVector2D(0.77f, 0.76f), Stroke * 0.88f);
        }
        else if (ShapeKey.Equals(TEXT("star-outline"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconClosedPolyline(Alpha, TextureSize, MakeGeneratedStarPoints(), Stroke * 0.78f);
        }
        else if (ShapeKey.Equals(TEXT("star-solid"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconFilledPolygon(Alpha, TextureSize, MakeGeneratedStarPoints());
        }
        else if (ShapeKey.Equals(TEXT("chevron-right"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.38f, 0.26f), FVector2D(0.62f, 0.50f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.62f, 0.50f), FVector2D(0.38f, 0.74f), Stroke);
        }
        else if (ShapeKey.Equals(TEXT("chevron-down"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.26f, 0.38f), FVector2D(0.50f, 0.62f), Stroke);
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.50f, 0.62f), FVector2D(0.74f, 0.38f), Stroke);
        }
        else if (ShapeKey.Equals(TEXT("dot"), ESearchCase::IgnoreCase))
        {
            DrawGeneratedIconCircle(Alpha, TextureSize, FVector2D(0.50f, 0.50f), 0.10f, true);
        }
        else if (ShapeKey.Equals(TEXT("blank"), ESearchCase::IgnoreCase))
        {
            return;
        }
        else
        {
            DrawGeneratedIconSegment(Alpha, TextureSize, FVector2D(0.24f, 0.50f), FVector2D(0.76f, 0.50f), Stroke);
        }
    }

    inline UTexture2D* CreateGeneratedShapeIconTexture(const FString& ShapeKey)
    {
        constexpr int32 TextureSize = 128;
        TArray<float> Alpha;
        Alpha.Init(0.0f, TextureSize * TextureSize);
        DrawGeneratedIconShapeAlpha(ShapeKey, Alpha, TextureSize);

        TArray<FColor> Pixels;
        Pixels.SetNumZeroed(TextureSize * TextureSize);
        for (int32 Index = 0; Index < Pixels.Num(); ++Index)
        {
            const uint8 A = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Alpha[Index] * 255.0f), 0, 255));
            Pixels[Index] = FColor(255, 255, 255, A);
        }

        const FString TextureNameString = FString::Printf(TEXT("RI_GeneratedShape_%s"), *ShapeKey.Replace(TEXT("-"), TEXT("_")));
        UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), *TextureNameString));
        if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
        {
            return nullptr;
        }

        Texture->SRGB = true;
        Texture->NeverStream = true;
        Texture->LODGroup = TEXTUREGROUP_UI;
        Texture->Filter = TF_Bilinear;
        FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
        void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
        Texture->UpdateResource();
        Texture->AddToRoot();
        return Texture;
    }

    inline UTexture2D* GetGeneratedShapeIconTexture(const FString& Shape)
    {
        if (Shape.IsEmpty())
        {
            return nullptr;
        }

        static TMap<FString, UTexture2D*> CachedTextures;
        const FString ShapeKey = Shape.ToLower();
        if (UTexture2D** CachedTexture = CachedTextures.Find(ShapeKey))
        {
            if (*CachedTexture)
            {
                return *CachedTexture;
            }
        }

        UTexture2D* Texture = CreateGeneratedShapeIconTexture(ShapeKey);
        CachedTextures.Add(ShapeKey, Texture);
        return Texture;
    }

    inline UWidget* MakeShapeIcon(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FString& Shape,
        float DesiredSize,
        const FLinearColor& Tint)
    {
        if (!WidgetTree || Shape.IsEmpty())
        {
            return nullptr;
        }

        const float Size = FMath::Max(8.0f, DesiredSize);
        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
        if (UTexture2D* Texture = GetGeneratedShapeIconTexture(Shape))
        {
            Image->SetBrushFromTexture(Texture, false);
        }
        FSlateBrush Brush = Image->GetBrush();
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.ImageSize = FVector2D(Size, Size);
        Image->SetBrush(Brush);
        Image->SetDesiredSizeOverride(FVector2D(Size, Size));
        Image->SetColorAndOpacity(Tint);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        return Image;
    }

    inline UWidget* MakeIconWidget(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const TCHAR* IconName,
        float DesiredSize,
        const FLinearColor& Tint)
    {
        if (!IconName || IconName[0] == TEXT('\0'))
        {
            return nullptr;
        }

        FString Shape;
        if (TryGetShapeIcon(IconName, Shape))
        {
            return MakeShapeIcon(WidgetTree, Name, Shape, DesiredSize, Tint);
        }

        FString Glyph;
        if (TryGetGlyphIcon(IconName, Glyph))
        {
            return MakeGlyphIcon(WidgetTree, Name, Glyph, DesiredSize, Tint);
        }

        const FString AssetIconName(IconName);
        if (AssetIconName.Equals(TEXT("search_white_64"), ESearchCase::IgnoreCase))
        {
            return MakeShapeIcon(WidgetTree, Name, TEXT("search"), DesiredSize, Tint);
        }
        if (AssetIconName.Equals(TEXT("star_white_solid_64"), ESearchCase::IgnoreCase))
        {
            return MakeShapeIcon(WidgetTree, Name, TEXT("star-solid"), DesiredSize, Tint);
        }
        if (AssetIconName.Equals(TEXT("star_white_outline_64"), ESearchCase::IgnoreCase))
        {
            return MakeShapeIcon(WidgetTree, Name, TEXT("star-outline"), DesiredSize, Tint);
        }

        return MakeIcon(WidgetTree, Name, IconName, DesiredSize, Tint);
    }

    inline void CenterSizeBoxContent(USizeBox* SizeBox)
    {
        if (!SizeBox)
        {
            return;
        }

        if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SizeBox->GetContentSlot()))
        {
            SizeBoxSlot->SetHorizontalAlignment(HAlign_Center);
            SizeBoxSlot->SetVerticalAlignment(VAlign_Center);
            SizeBoxSlot->SetPadding(FMargin(0.f));
        }
    }

    inline void SetFavoriteIconState(
        UImage* Image,
        bool bFavorited,
        float DesiredSize,
        const FLinearColor& ActiveColor,
        const FLinearColor& InactiveColor)
    {
        if (!Image)
        {
            return;
        }

        if (UTexture2D* Texture = GetFavoriteIconTexture(bFavorited))
        {
            Image->SetBrushFromTexture(Texture, false);
        }

        if (DesiredSize > 0.f)
        {
            FSlateBrush Brush = Image->GetBrush();
            Brush.ImageSize = FVector2D(DesiredSize, DesiredSize);
            Image->SetBrush(Brush);
            Image->SetDesiredSizeOverride(FVector2D(DesiredSize, DesiredSize));
        }

        Image->SetColorAndOpacity(bFavorited ? ActiveColor : InactiveColor);
    }

    inline UImage* MakeFavoriteIcon(
        UWidgetTree* WidgetTree,
        const FName& Name,
        float DesiredSize,
        bool bFavorited,
        const FLinearColor& ActiveColor,
        const FLinearColor& InactiveColor)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
        SetFavoriteIconState(Image, bFavorited, DesiredSize, ActiveColor, InactiveColor);
        return Image;
    }

    inline void ApplyTextColorRecursive(UWidget* Widget, const FLinearColor& Color)
    {
        if (!Widget)
        {
            return;
        }

        if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            const FSlateFontInfo ExistingFont = TextBlock->GetFont();
            ApplyTextStyle(TextBlock, ExistingFont.Size, ExistingFont.TypefaceFontName == FName(TEXT("Bold")), Color);
            return;
        }

        if (UContentWidget* ContentWidget = Cast<UContentWidget>(Widget))
        {
            ApplyTextColorRecursive(ContentWidget->GetContent(), Color);
        }

        if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
        {
            for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
            {
                ApplyTextColorRecursive(PanelWidget->GetChildAt(ChildIndex), Color);
            }
        }
    }

    inline void CenterContentTextRecursive(UWidget* Widget)
    {
        if (!Widget)
        {
            return;
        }

        if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            TextBlock->SetJustification(ETextJustify::Center);
            return;
        }

        if (USizeBox* SizeBox = Cast<USizeBox>(Widget))
        {
            if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(SizeBox->GetContentSlot()))
            {
                SizeBoxSlot->SetHorizontalAlignment(HAlign_Center);
                SizeBoxSlot->SetVerticalAlignment(VAlign_Center);
                SizeBoxSlot->SetPadding(FMargin(0.f));
            }
        }

        if (UBorder* Border = Cast<UBorder>(Widget))
        {
            if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Border->GetContentSlot()))
            {
                BorderSlot->SetHorizontalAlignment(HAlign_Center);
                BorderSlot->SetVerticalAlignment(VAlign_Center);
                BorderSlot->SetPadding(FMargin(0.f));
            }
        }

        if (UContentWidget* ContentWidget = Cast<UContentWidget>(Widget))
        {
            CenterContentTextRecursive(ContentWidget->GetContent());
        }

        if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
        {
            for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
            {
                CenterContentTextRecursive(PanelWidget->GetChildAt(ChildIndex));
            }
        }
    }

    inline void ConfigureButton(UButton* Button, ERIButtonVisualStyle Style, bool bApplyTextColorToChildren);

    inline void SetWidgetEnabledState(UWidget* Widget, bool bEnabled, const FString& DisabledReason = FString(), const FString& EnabledTooltip = FString())
    {
        if (!Widget)
        {
            return;
        }

        Widget->SetIsEnabled(bEnabled);
        if (bEnabled)
        {
            Widget->SetToolTipText(EnabledTooltip.IsEmpty() ? FText::GetEmpty() : FText::FromString(EnabledTooltip));
        }
        else
        {
            Widget->SetToolTipText(DisabledReason.IsEmpty() ? FText::GetEmpty() : FText::FromString(DisabledReason));
        }
    }

    inline void SetButtonAffordance(
        UButton* Button,
        ERIButtonVisualStyle Style,
        bool bEnabled,
        const FString& DisabledReason = FString(),
        const FString& EnabledTooltip = FString(),
        bool bApplyTextColorToChildren = false)
    {
        if (!Button)
        {
            return;
        }

        ConfigureButton(Button, Style, bApplyTextColorToChildren);
        SetWidgetEnabledState(Button, bEnabled, DisabledReason, EnabledTooltip);
    }

    inline void ApplyStatusTone(UTextBlock* TextBlock, ERIStatusVisualStyle Style)
    {
        if (!TextBlock)
        {
            return;
        }

        ApplyTextStyle(TextBlock, TextBlock->GetFont().Size, TextBlock->GetFont().TypefaceFontName == FName(TEXT("Bold")), GetStatusTextColor(Style));
    }

    inline void ConfigureButton(UButton* Button, ERIButtonVisualStyle Style, bool bApplyTextColorToChildren = true)
    {
        if (!Button)
        {
            return;
        }

        const FRIButtonPalette Palette = GetButtonPalette(Style);

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FButtonStyle ButtonStyle = Button->WidgetStyle;
        ButtonStyle.Normal.TintColor = FSlateColor(Palette.Normal);
        ButtonStyle.Hovered.TintColor = FSlateColor(Palette.Hovered);
        ButtonStyle.Pressed.TintColor = FSlateColor(Palette.Pressed);
        ButtonStyle.Disabled.TintColor = FSlateColor(Palette.Disabled);
        const FRIThemeMetrics& Metrics = GetThemeMetrics();
        ButtonStyle.NormalPadding = FMargin(Metrics.ButtonPadding.Left, 0.f, Metrics.ButtonPadding.Right, 0.f);
        ButtonStyle.PressedPadding = FMargin(Metrics.ButtonPadding.Left, 0.f, Metrics.ButtonPadding.Right, 0.f);
        ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
        const bool bTabButton = Style == ERIButtonVisualStyle::TabActive || Style == ERIButtonVisualStyle::TabInactive;
        const float ButtonCornerRadius = bTabButton ? FMath::Max(2.0f, Metrics.CornerRadius - 2.0f) : Metrics.CornerRadius;
        ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius);
        ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius);
        ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius);
        ButtonStyle.Disabled.OutlineSettings.CornerRadii = FVector4(ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius, ButtonCornerRadius);
        const float OutlineWidth = 0.0f;
        ButtonStyle.Normal.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Hovered.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Pressed.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Disabled.OutlineSettings.Width = OutlineWidth;
        ButtonStyle.Normal.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
        ButtonStyle.Hovered.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
        ButtonStyle.Pressed.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
        ButtonStyle.Disabled.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
        Button->WidgetStyle = ButtonStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        Button->SetBackgroundColor(Palette.Normal);
        Button->SetColorAndOpacity(FLinearColor::White);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
        {
            ButtonSlot->SetHorizontalAlignment(HAlign_Center);
            ButtonSlot->SetVerticalAlignment(VAlign_Center);
            ButtonSlot->SetPadding(FMargin(0.f));
        }
        CenterContentTextRecursive(Button);

        if (bApplyTextColorToChildren)
        {
            ApplyTextColorRecursive(Button, Palette.Text);
        }
    }

    inline void ConfigureSwatchButton(UButton* Button)
    {
        if (!Button)
        {
            return;
        }

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FButtonStyle ButtonStyle = Button->WidgetStyle;
        const FSlateColor TransparentTint(FLinearColor::Transparent);
        ButtonStyle.Normal.TintColor = TransparentTint;
        ButtonStyle.Hovered.TintColor = TransparentTint;
        ButtonStyle.Pressed.TintColor = TransparentTint;
        ButtonStyle.Disabled.TintColor = TransparentTint;
        ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        ButtonStyle.Disabled.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
        ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
        ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
        ButtonStyle.Disabled.OutlineSettings.Width = 0.0f;
        ButtonStyle.NormalPadding = FMargin(0.f);
        ButtonStyle.PressedPadding = FMargin(0.f);
        Button->WidgetStyle = ButtonStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        Button->SetBackgroundColor(FLinearColor::Transparent);
        Button->SetColorAndOpacity(FLinearColor::White);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
        {
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
            ButtonSlot->SetVerticalAlignment(VAlign_Fill);
            ButtonSlot->SetPadding(FMargin(0.f));
        }
    }

    inline void ConfigureGhostIconButton(UButton* Button)
    {
        if (!Button)
        {
            return;
        }

        const FRIButtonPalette Palette = GetButtonPalette(ERIButtonVisualStyle::Subtle);
        FLinearColor Hovered = Palette.Hovered;
        Hovered.A *= 0.55f;
        FLinearColor Pressed = Palette.Pressed;
        Pressed.A *= 0.70f;

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FButtonStyle ButtonStyle = Button->WidgetStyle;
        const FSlateColor TransparentTint(FLinearColor::Transparent);
        ButtonStyle.Normal.TintColor = TransparentTint;
        ButtonStyle.Hovered.TintColor = FSlateColor(Hovered);
        ButtonStyle.Pressed.TintColor = FSlateColor(Pressed);
        ButtonStyle.Disabled.TintColor = TransparentTint;
        ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
        const float Radius = FMath::Max(2.0f, GetThemeMetrics().CornerRadius - 2.0f);
        ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        ButtonStyle.Disabled.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
        ButtonStyle.Normal.OutlineSettings.Width = 0.0f;
        ButtonStyle.Hovered.OutlineSettings.Width = 0.0f;
        ButtonStyle.Pressed.OutlineSettings.Width = 0.0f;
        ButtonStyle.Disabled.OutlineSettings.Width = 0.0f;
        ButtonStyle.NormalPadding = FMargin(0.f);
        ButtonStyle.PressedPadding = FMargin(0.f);
        Button->WidgetStyle = ButtonStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        Button->SetBackgroundColor(FLinearColor::Transparent);
        Button->SetColorAndOpacity(FLinearColor::White);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
        {
            ButtonSlot->SetHorizontalAlignment(HAlign_Center);
            ButtonSlot->SetVerticalAlignment(VAlign_Center);
            ButtonSlot->SetPadding(FMargin(0.f));
        }
    }

    inline float GetFavoriteButtonHitSize()
    {
        return 22.0f;
    }

    inline float GetFavoriteIconSize()
    {
        return 12.0f;
    }

    inline UButton* MakeFavoriteGhostButton(
        UWidgetTree* WidgetTree,
        const FName& ButtonName,
        const FName& SizeBoxName,
        const FName& IconName,
        bool bFavorited,
        const FLinearColor& ActiveColor,
        const FLinearColor& InactiveColor,
        UImage** OutIcon,
        USizeBox** OutSizeBox)
    {
        if (OutIcon)
        {
            *OutIcon = nullptr;
        }
        if (OutSizeBox)
        {
            *OutSizeBox = nullptr;
        }
        if (!WidgetTree)
        {
            return nullptr;
        }

        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SizeBoxName);
        SizeBox->SetWidthOverride(GetFavoriteButtonHitSize());
        SizeBox->SetHeightOverride(GetFavoriteButtonHitSize());
        SizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);

        UImage* Icon = MakeFavoriteIcon(
            WidgetTree,
            IconName,
            GetFavoriteIconSize(),
            bFavorited,
            ActiveColor,
            InactiveColor);
        if (Icon)
        {
            Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            SizeBox->SetContent(Icon);
        }

        CenterSizeBoxContent(SizeBox);
        Button->AddChild(SizeBox);
        ConfigureGhostIconButton(Button);
        if (OutIcon)
        {
            *OutIcon = Icon;
        }
        if (OutSizeBox)
        {
            *OutSizeBox = SizeBox;
        }
        return Button;
    }

    inline bool HasFavoriteGhostButtonContract(const UButton* Button, const USizeBox* SizeBox, const UImage* Icon)
    {
        if (!Button || !SizeBox || !Icon)
        {
            return false;
        }

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        const bool bHasGhostButtonStyle =
            FMath::IsNearlyZero(Button->WidgetStyle.Normal.OutlineSettings.Width)
            && FMath::IsNearlyZero(Button->WidgetStyle.Hovered.OutlineSettings.Width)
            && FMath::IsNearlyZero(Button->WidgetStyle.Pressed.OutlineSettings.Width)
            && FMath::IsNearlyZero(Button->WidgetStyle.Disabled.OutlineSettings.Width);
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        return FMath::IsNearlyEqual(SizeBox->GetWidthOverride(), GetFavoriteButtonHitSize())
            && FMath::IsNearlyEqual(SizeBox->GetHeightOverride(), GetFavoriteButtonHitSize())
            && SizeBox->GetVisibility() == ESlateVisibility::HitTestInvisible
            && Icon->GetVisibility() == ESlateVisibility::HitTestInvisible
            && bHasGhostButtonStyle;
    }

    inline void ConfigureCheckBox(UCheckBox* CheckBox)
    {
        if (!CheckBox)
        {
            return;
        }

        const FRIInputPalette Palette = GetInputPalette(ERIInputVisualStyle::Muted);

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FCheckBoxStyle CheckBoxStyle = CheckBox->WidgetStyle;
        CheckBoxStyle.UncheckedImage.TintColor = FSlateColor(Palette.Background);
        CheckBoxStyle.UncheckedHoveredImage.TintColor = FSlateColor(Palette.Hovered);
        CheckBoxStyle.UncheckedPressedImage.TintColor = FSlateColor(Palette.Focused);
        CheckBoxStyle.CheckedImage.TintColor = FSlateColor(GetSuccessTextColor());
        CheckBoxStyle.CheckedHoveredImage.TintColor = FSlateColor(GetSuccessTextColor());
        CheckBoxStyle.CheckedPressedImage.TintColor = FSlateColor(GetSecondaryTextColor());
        CheckBoxStyle.UndeterminedImage.TintColor = FSlateColor(GetWarningTextColor());
        CheckBox->WidgetStyle = CheckBoxStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    inline UButton* MakeLabeledButton(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FString& Label,
        ERIButtonVisualStyle Style,
        float WidthOverride = 0.f,
        float HeightOverride = 0.f,
        int32 FontSize = 0)
    {
        const FRIThemeMetrics& Metrics = GetThemeMetrics();
        const int32 EffectiveFontSize = FontSize > 0 ? FontSize : GetLabelFontSize();
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        if (WidthOverride > 0.f)
        {
            SizeBox->SetWidthOverride(WidthOverride);
        }
        else
        {
            SizeBox->SetMinDesiredWidth(72.f);
        }
        SizeBox->SetHeightOverride(HeightOverride > 0.f ? HeightOverride : Metrics.ButtonHeight);
        UTextBlock* LabelText = MakeText(WidgetTree, Label, EffectiveFontSize, true, GetButtonTextColor(Style));
        LabelText->SetJustification(ETextJustify::Center);
        SizeBox->SetContent(LabelText);
        Button->AddChild(SizeBox);
        ConfigureButton(Button, Style, false);
        return Button;
    }

    inline UButton* MakeCompactActionButton(
        UWidgetTree* WidgetTree,
        const FName& Name,
        const FString& Label,
        const TCHAR* IconAssetName,
        ERIButtonVisualStyle Style,
        float WidthOverride,
        float HeightOverride,
        float IconSize,
        int32 FontSize = 0)
    {
        const int32 EffectiveFontSize = FontSize > 0 ? FontSize : GetValueFontSize();
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SizeBox->SetWidthOverride(WidthOverride);
        SizeBox->SetHeightOverride(HeightOverride > 0.f ? HeightOverride : GetButtonHeight());

        UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        if (UWidget* Icon = MakeIconWidget(WidgetTree, NAME_None, IconAssetName, IconSize, GetButtonTextColor(Style)))
        {
            Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            IconBox->SetWidthOverride(IconSize);
            IconBox->SetHeightOverride(IconSize);
            IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
            IconBox->SetContent(Icon);
            CenterSizeBoxContent(IconBox);
            if (UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox))
            {
                IconSlot->SetPadding(FMargin(0.f, 0.f, 3.f, 0.f));
                IconSlot->SetVerticalAlignment(VAlign_Center);
                IconSlot->SetHorizontalAlignment(HAlign_Center);
            }
        }

        UTextBlock* LabelText = MakeEllipsisText(WidgetTree, Label, EffectiveFontSize, true, GetButtonTextColor(Style));
        LabelText->SetJustification(ETextJustify::Center);
        LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetHorizontalAlignment(HAlign_Fill);
        }

        SizeBox->SetContent(Content);
        Button->AddChild(SizeBox);
        ConfigureButton(Button, Style, false);
        return Button;
    }

    inline UBorder* MakeSectionTitle(
        UWidgetTree* WidgetTree,
        const FString& Label,
        ERISectionVisualStyle Style = ERISectionVisualStyle::Standard,
        const FName& Name = NAME_None)
    {
        const FRIThemeMetrics& Metrics = GetThemeMetrics();
        const FRISectionPalette Palette = GetSectionPalette(Style);
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Border->SetPadding(Metrics.SectionPadding);
        Border->SetBrushColor(Palette.Background);
        Border->SetContent(MakeText(WidgetTree, Label, GetSectionTitleFontSize(), true, Palette.Text));
        return Border;
    }

    inline UBorder* MakeSurfaceCard(
        UWidgetTree* WidgetTree,
        const FName& Name = NAME_None,
        const FLinearColor& Background = FLinearColor::Transparent,
        const FMargin& Padding = FMargin(-1.f))
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Border->SetPadding(Padding.Left >= 0.f ? Padding : GetSurfaceCardPadding());
        Border->SetBrushColor(Background == FLinearColor::Transparent ? GetSectionSurfaceBackgroundColor() : Background);
        return Border;
    }

    inline UWidget* MakeMetricCard(
        UWidgetTree* WidgetTree,
        const FString& Label,
        const FName& ValueName,
        UTextBlock*& OutValueText,
        const FLinearColor& Background,
        int32 ValueFontSize = 0,
        bool bBoldValue = true)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UBorder* Border = MakeSurfaceCard(WidgetTree, NAME_None, Background, GetSurfaceCardPadding(true));
        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Border->SetContent(Box);

        if (UVerticalBoxSlot* LabelSlot = Box->AddChildToVerticalBox(MakeText(WidgetTree, Label, GetMutedFontSize(), true, GetMutedTextColor())))
        {
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }

        OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ValueName);
        OutValueText->SetAutoWrapText(true);
        OutValueText->SetClipping(EWidgetClipping::ClipToBounds);
        ApplyTextStyle(
            OutValueText,
            ValueFontSize > 0 ? ValueFontSize : GetValueFontSize(),
            bBoldValue,
            GetStrongTextColor());
        Box->AddChildToVerticalBox(OutValueText);
        return Border;
    }

    inline UBorder* MakeStackedContentRow(
        UWidgetTree* WidgetTree,
        const FString& Label,
        UWidget* Content,
        const FLinearColor& Background,
        const FLinearColor& LabelColor,
        const FName& Name = NAME_None,
        float ContentGap = 3.0f,
        const FMargin& Padding = FMargin(6.f, 4.f))
    {
        if (!WidgetTree || !Content)
        {
            return nullptr;
        }

        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Border->SetPadding(Padding);
        Border->SetBrushColor(Background);

        UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Border->SetContent(Box);

        if (UVerticalBoxSlot* LabelSlot = Box->AddChildToVerticalBox(MakeText(WidgetTree, Label, GetLabelFontSize(), true, LabelColor, true)))
        {
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ContentGap));
        }

        if (UVerticalBoxSlot* ContentSlot = Box->AddChildToVerticalBox(Content))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Fill);
        }

        return Border;
    }

    inline UBorder* MakeStackedValueRow(
        UWidgetTree* WidgetTree,
        const FString& Label,
        UTextBlock*& OutValueText,
        const FLinearColor& Background,
        const FLinearColor& LabelColor,
        const FLinearColor& ValueColor,
        bool bWrapValue = true,
        const FName& ValueName = NAME_None,
        const FName& BorderName = NAME_None)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ValueName);
        OutValueText->SetAutoWrapText(bWrapValue);
        OutValueText->SetClipping(EWidgetClipping::ClipToBounds);
        ApplyTextStyle(OutValueText, GetValueFontSize(), false, ValueColor);
        return MakeStackedContentRow(WidgetTree, Label, OutValueText, Background, LabelColor, BorderName);
    }

    inline UBorder* MakeStackedValueRow(
        UWidgetTree* WidgetTree,
        const FString& Label,
        TObjectPtr<UTextBlock>& OutValueText,
        const FLinearColor& Background,
        const FLinearColor& LabelColor,
        const FLinearColor& ValueColor,
        bool bWrapValue = true,
        const FName& ValueName = NAME_None,
        const FName& BorderName = NAME_None)
    {
        UTextBlock* RawValueText = OutValueText.Get();
        UBorder* Border = MakeStackedValueRow(WidgetTree, Label, RawValueText, Background, LabelColor, ValueColor, bWrapValue, ValueName, BorderName);
        OutValueText = RawValueText;
        return Border;
    }

    inline USizeBox* WrapFixedHeight(UWidgetTree* WidgetTree, UWidget* Child, float HeightOverride)
    {
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SizeBox->SetHeightOverride(HeightOverride);
        SizeBox->SetContent(Child);
        return SizeBox;
    }

    inline USizeBox* WrapValueControl(
        UWidgetTree* WidgetTree,
        UWidget* Child,
        float MinWidth = 0.f,
        float WidthOverride = 0.f,
        float HeightOverride = 0.f)
    {
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        if (WidthOverride > 0.f)
        {
            SizeBox->SetWidthOverride(WidthOverride);
        }
        else if (MinWidth > 0.f)
        {
            SizeBox->SetMinDesiredWidth(MinWidth);
        }
        SizeBox->SetHeightOverride(HeightOverride > 0.f ? HeightOverride : GetInputHeight());
        SizeBox->SetContent(Child);
        return SizeBox;
    }

    inline USizeBox* WrapMinHeight(UWidgetTree* WidgetTree, UWidget* Child, float MinHeight)
    {
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SizeBox->SetMinDesiredHeight(MinHeight);
        SizeBox->SetContent(Child);
        return SizeBox;
    }

    inline void ConfigureEditableTextBox(
        UEditableTextBox* TextBox,
        const FLinearColor& TextColor,
        int32 FontSize = 0,
        ERIInputVisualStyle Style = ERIInputVisualStyle::Standard)
    {
        if (!TextBox)
        {
            return;
        }

        const FRIInputPalette Palette = GetInputPalette(Style);
        const int32 EffectiveFontSize = FontSize > 0 ? FontSize : GetValueFontSize();

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        FEditableTextBoxStyle EditableStyle = TextBox->WidgetStyle;
        EditableStyle.TextStyle.Font.Size = EffectiveFontSize;
        EditableStyle.TextStyle.Font.TypefaceFontName = FName(TEXT("Regular"));
        EditableStyle.TextStyle.ColorAndOpacity = FSlateColor(TextColor);
        EditableStyle.BackgroundImageNormal.TintColor = FSlateColor(Palette.Background);
        EditableStyle.BackgroundImageHovered.TintColor = FSlateColor(Palette.Hovered);
        EditableStyle.BackgroundImageFocused.TintColor = FSlateColor(Palette.Focused);
        EditableStyle.BackgroundImageReadOnly.TintColor = FSlateColor(Palette.ReadOnly);
        const FRIThemeMetrics& Metrics = GetThemeMetrics();
        EditableStyle.Padding = Metrics.InputPadding;
        EditableStyle.BackgroundImageNormal.DrawAs = ESlateBrushDrawType::RoundedBox;
        EditableStyle.BackgroundImageHovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        EditableStyle.BackgroundImageFocused.DrawAs = ESlateBrushDrawType::RoundedBox;
        EditableStyle.BackgroundImageReadOnly.DrawAs = ESlateBrushDrawType::RoundedBox;
        EditableStyle.BackgroundImageNormal.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        EditableStyle.BackgroundImageHovered.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        EditableStyle.BackgroundImageFocused.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        EditableStyle.BackgroundImageReadOnly.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        EditableStyle.BackgroundImageNormal.OutlineSettings.Width = Metrics.BorderWidth;
        EditableStyle.BackgroundImageHovered.OutlineSettings.Width = Metrics.BorderWidth;
        EditableStyle.BackgroundImageFocused.OutlineSettings.Width = Metrics.BorderWidth;
        EditableStyle.BackgroundImageReadOnly.OutlineSettings.Width = Metrics.BorderWidth;
        TextBox->WidgetStyle = EditableStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        TextBox->SetMinDesiredWidth(120.0f);
        TextBox->SetForegroundColor(TextColor);
        TextBox->SetHintText(FText::GetEmpty());
        TextBox->SetTextOverflowPolicy(ETextOverflowPolicy::Clip);
        TextBox->SetClearKeyboardFocusOnCommit(false);
        TextBox->SetSelectAllTextOnCommit(false);
        TextBox->SetSelectAllTextWhenFocused(false);
    }

    inline UHorizontalBox* MakeAxisValueRow(
        UWidgetTree* WidgetTree,
        const FString& AxisLabel,
        int32 AxisIndex,
        UEditableTextBox*& OutTextBox,
        const TCHAR* BaseName)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            FName(FString::Printf(TEXT("%sRow"), BaseName)));

        UTextBlock* Label = MakeText(
            WidgetTree,
            AxisLabel,
            FMath::Max(5, GetMutedFontSize()),
            true,
            GetAxisAccentColor(AxisIndex),
            false);
        Label->SetJustification(ETextJustify::Center);
        if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 3.f, 0.f));
        }

        OutTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
            UEditableTextBox::StaticClass(),
            FName(FString::Printf(TEXT("%sValue"), BaseName)));
        ConfigureEditableTextBox(OutTextBox, GetStrongTextColor());
        OutTextBox->SetJustification(ETextJustify::Center);
        OutTextBox->SetSelectAllTextWhenFocused(true);
        OutTextBox->SetMinDesiredWidth(68.0f);
        if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutTextBox))
        {
            ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            ValueSlot->SetHorizontalAlignment(HAlign_Fill);
            ValueSlot->SetVerticalAlignment(VAlign_Center);
        }

        return Row;
    }

    inline void ConfigureComboBoxString(
        UComboBoxString* ComboBox,
        const FLinearColor& TextColor,
        float MaxListHeight = 220.0f,
        ERIInputVisualStyle Style = ERIInputVisualStyle::Standard)
    {
        if (!ComboBox)
        {
            return;
        }

        const FRIInputPalette Palette = GetInputPalette(Style);
        const FRIThemeMetrics& Metrics = GetThemeMetrics();
        ComboBox->SetContentPadding(FMargin(Metrics.InputPadding.Left, 3.0f));
        ComboBox->SetMaxListHeight(MaxListHeight);

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        ComboBox->Font.Size = GetValueFontSize();
        ComboBox->Font.TypefaceFontName = FName(TEXT("Regular"));
        ComboBox->ForegroundColor = FSlateColor(TextColor);
        FComboBoxStyle ComboBoxStyle = ComboBox->WidgetStyle;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor = FSlateColor(Palette.Background);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor = FSlateColor(Palette.Hovered);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor = FSlateColor(Palette.Focused);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.TintColor = FSlateColor(Palette.ReadOnly);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Normal.OutlineSettings.Width = Metrics.BorderWidth;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Hovered.OutlineSettings.Width = Metrics.BorderWidth;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Pressed.OutlineSettings.Width = Metrics.BorderWidth;
        ComboBoxStyle.ComboButtonStyle.ButtonStyle.Disabled.OutlineSettings.Width = Metrics.BorderWidth;
        ComboBoxStyle.ComboButtonStyle.DownArrowImage.TintColor = FSlateColor(TextColor);
        ComboBox->WidgetStyle = ComboBoxStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    inline UTextBlock* MakeComboBoxItemText(
        UWidgetTree* WidgetTree,
        const FString& ItemText,
        const FLinearColor& TextColor,
        int32 FontSize = 0)
    {
        return MakeText(
            WidgetTree,
            ItemText,
            FontSize > 0 ? FontSize : GetValueFontSize(),
            false,
            TextColor,
            false);
    }
}
