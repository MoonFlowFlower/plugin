#pragma once

#include "CoreMinimal.h"
#include "RuntimeInspector.h"
#include "RuntimeInspectorSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/ContentWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

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
                8, 7, 7, 6,
                22.0f, 26.0f, 96.0f, 120.0f,
                4.0f, 1.0f,
                FMargin(6.f, 3.f),
                FMargin(0.f),
                FMargin(6.f, 4.f)
            };
            Tokens.PageBackground = FLinearColor(0.01f, 0.01f, 0.01f, 0.80f);
            Tokens.FooterBackground = FLinearColor(0.05f, 0.05f, 0.05f, 0.92f);
            Tokens.ContextStripBackground = FLinearColor(0.05f, 0.06f, 0.08f, 0.90f);
            Tokens.ContextPrimaryCellBackground = FLinearColor(0.10f, 0.12f, 0.16f, 0.96f);
            Tokens.ContextSecondaryCellBackground = FLinearColor(0.08f, 0.09f, 0.12f, 0.92f);
            Tokens.ContextStatusCellBackground = FLinearColor(0.09f, 0.12f, 0.10f, 0.96f);
            Tokens.SectionSurfaceBackground = FLinearColor(0.09f, 0.09f, 0.09f, 0.88f);
            Tokens.RowSurfaceBackground = FLinearColor(0.04f, 0.04f, 0.04f, 0.80f);
            Tokens.CellSurfaceBackground = FLinearColor(0.06f, 0.06f, 0.06f, 0.82f);
            Tokens.SelectedRowSurfaceBackground = FLinearColor(0.09f, 0.14f, 0.18f, 0.88f);
            Tokens.StrongText = FLinearColor(0.96f, 0.96f, 0.96f, 1.0f);
            Tokens.SecondaryText = FLinearColor(0.84f, 0.88f, 0.96f, 1.0f);
            Tokens.MutedText = FLinearColor(0.72f, 0.72f, 0.72f, 1.0f);
            Tokens.SuccessText = FLinearColor(0.42f, 0.92f, 0.58f, 1.0f);
            Tokens.WarningText = FLinearColor(0.95f, 0.75f, 0.25f, 1.0f);
            Tokens.ErrorText = FLinearColor(0.95f, 0.35f, 0.35f, 1.0f);
            Tokens.PrimaryButton = {
                FLinearColor(0.14f, 0.38f, 0.86f, 1.0f),
                FLinearColor(0.18f, 0.46f, 0.96f, 1.0f),
                FLinearColor(0.10f, 0.29f, 0.68f, 1.0f),
                FLinearColor(0.20f, 0.24f, 0.30f, 0.65f),
                FLinearColor(0.98f, 0.99f, 1.0f, 1.0f)
            };
            Tokens.SecondaryButton = {
                FLinearColor(0.28f, 0.29f, 0.33f, 1.0f),
                FLinearColor(0.34f, 0.35f, 0.40f, 1.0f),
                FLinearColor(0.22f, 0.23f, 0.27f, 1.0f),
                FLinearColor(0.20f, 0.21f, 0.24f, 0.65f),
                FLinearColor(0.95f, 0.95f, 0.97f, 1.0f)
            };
            Tokens.SubtleButton = {
                FLinearColor(0.22f, 0.23f, 0.26f, 1.0f),
                FLinearColor(0.27f, 0.28f, 0.32f, 1.0f),
                FLinearColor(0.18f, 0.19f, 0.22f, 1.0f),
                FLinearColor(0.18f, 0.19f, 0.22f, 0.65f),
                FLinearColor(0.93f, 0.93f, 0.95f, 1.0f)
            };
            Tokens.DangerButton = {
                FLinearColor(0.55f, 0.19f, 0.22f, 1.0f),
                FLinearColor(0.67f, 0.24f, 0.28f, 1.0f),
                FLinearColor(0.42f, 0.14f, 0.17f, 1.0f),
                FLinearColor(0.23f, 0.18f, 0.19f, 0.65f),
                FLinearColor(1.0f, 0.96f, 0.96f, 1.0f)
            };
            Tokens.HeaderButton = {
                FLinearColor(0.20f, 0.21f, 0.24f, 1.0f),
                FLinearColor(0.24f, 0.25f, 0.29f, 1.0f),
                FLinearColor(0.16f, 0.17f, 0.20f, 1.0f),
                FLinearColor(0.16f, 0.17f, 0.20f, 0.75f),
                FLinearColor(0.95f, 0.95f, 0.97f, 1.0f)
            };
            Tokens.TabActiveButton = {
                FLinearColor(0.70f, 0.72f, 0.76f, 1.0f),
                FLinearColor(0.76f, 0.78f, 0.82f, 1.0f),
                FLinearColor(0.58f, 0.60f, 0.65f, 1.0f),
                FLinearColor(0.20f, 0.20f, 0.22f, 0.75f),
                FLinearColor(0.10f, 0.11f, 0.13f, 1.0f)
            };
            Tokens.TabInactiveButton = {
                FLinearColor(0.15f, 0.16f, 0.18f, 1.0f),
                FLinearColor(0.19f, 0.20f, 0.23f, 1.0f),
                FLinearColor(0.11f, 0.12f, 0.14f, 1.0f),
                FLinearColor(0.12f, 0.13f, 0.15f, 0.75f),
                FLinearColor(0.92f, 0.92f, 0.94f, 1.0f)
            };
            Tokens.StandardSection = {
                FLinearColor(0.20f, 0.21f, 0.24f, 1.0f),
                FLinearColor(0.95f, 0.95f, 0.97f, 1.0f)
            };
            Tokens.EmphasisSection = {
                FLinearColor(0.17f, 0.19f, 0.24f, 1.0f),
                FLinearColor(0.97f, 0.98f, 1.0f, 1.0f)
            };
            Tokens.StandardInput = {
                FLinearColor(0.18f, 0.19f, 0.22f, 0.96f),
                FLinearColor(0.22f, 0.23f, 0.27f, 1.0f),
                FLinearColor(0.22f, 0.34f, 0.66f, 1.0f),
                FLinearColor(0.15f, 0.16f, 0.18f, 0.72f),
                FLinearColor(0.95f, 0.95f, 0.97f, 1.0f),
                FLinearColor(0.60f, 0.63f, 0.69f, 1.0f)
            };
            Tokens.StrongInput = {
                FLinearColor(0.16f, 0.19f, 0.25f, 0.98f),
                FLinearColor(0.19f, 0.23f, 0.30f, 1.0f),
                FLinearColor(0.20f, 0.32f, 0.62f, 1.0f),
                FLinearColor(0.14f, 0.16f, 0.20f, 0.75f),
                FLinearColor(0.97f, 0.98f, 1.0f, 1.0f),
                FLinearColor(0.62f, 0.68f, 0.78f, 1.0f)
            };
            Tokens.MutedInput = {
                FLinearColor(0.16f, 0.17f, 0.19f, 0.90f),
                FLinearColor(0.18f, 0.19f, 0.21f, 0.95f),
                FLinearColor(0.21f, 0.24f, 0.30f, 0.95f),
                FLinearColor(0.14f, 0.15f, 0.17f, 0.70f),
                FLinearColor(0.90f, 0.91f, 0.93f, 1.0f),
                FLinearColor(0.56f, 0.58f, 0.63f, 1.0f)
            };
            return Tokens;
        }();

        static const FRIThemePresetTokens SoftContrast = []()
        {
            FRIThemePresetTokens Tokens = StudioSlate;
            Tokens.Metrics.CornerRadius = 6.0f;
            Tokens.Metrics.BorderWidth = 1.25f;
            Tokens.PageBackground = FLinearColor(0.03f, 0.03f, 0.04f, 0.84f);
            Tokens.FooterBackground = FLinearColor(0.08f, 0.08f, 0.10f, 0.94f);
            Tokens.ContextStripBackground = FLinearColor(0.07f, 0.08f, 0.10f, 0.92f);
            Tokens.ContextPrimaryCellBackground = FLinearColor(0.12f, 0.14f, 0.18f, 0.98f);
            Tokens.ContextSecondaryCellBackground = FLinearColor(0.10f, 0.11f, 0.14f, 0.94f);
            Tokens.ContextStatusCellBackground = FLinearColor(0.10f, 0.14f, 0.12f, 0.98f);
            Tokens.SectionSurfaceBackground = FLinearColor(0.16f, 0.17f, 0.20f, 0.92f);
            Tokens.RowSurfaceBackground = FLinearColor(0.11f, 0.12f, 0.14f, 0.88f);
            Tokens.CellSurfaceBackground = FLinearColor(0.14f, 0.15f, 0.18f, 0.90f);
            Tokens.SelectedRowSurfaceBackground = FLinearColor(0.16f, 0.22f, 0.28f, 0.94f);
            Tokens.StrongText = FLinearColor(0.98f, 0.98f, 0.99f, 1.0f);
            Tokens.SecondaryText = FLinearColor(0.88f, 0.91f, 0.95f, 1.0f);
            Tokens.MutedText = FLinearColor(0.78f, 0.80f, 0.84f, 1.0f);
            Tokens.SuccessText = FLinearColor(0.52f, 0.88f, 0.68f, 1.0f);
            Tokens.WarningText = FLinearColor(0.96f, 0.79f, 0.40f, 1.0f);
            Tokens.ErrorText = FLinearColor(0.95f, 0.49f, 0.49f, 1.0f);
            Tokens.Metrics.ButtonHeight = 24.0f;
            Tokens.Metrics.InputHeight = 28.0f;
            Tokens.Metrics.CompactListHeight = 104.0f;
            Tokens.Metrics.StandardListHeight = 132.0f;
            Tokens.Metrics.SectionPadding = FMargin(8.f, 4.f);
            Tokens.Metrics.InputPadding = FMargin(8.f, 5.f);
            Tokens.PrimaryButton.Normal = FLinearColor(0.18f, 0.50f, 0.88f, 1.0f);
            Tokens.PrimaryButton.Hovered = FLinearColor(0.25f, 0.58f, 0.95f, 1.0f);
            Tokens.SecondaryButton.Normal = FLinearColor(0.35f, 0.37f, 0.42f, 1.0f);
            Tokens.SecondaryButton.Hovered = FLinearColor(0.41f, 0.43f, 0.49f, 1.0f);
            Tokens.SecondaryButton.Pressed = FLinearColor(0.28f, 0.30f, 0.35f, 1.0f);
            Tokens.SubtleButton.Normal = FLinearColor(0.18f, 0.19f, 0.22f, 1.0f);
            Tokens.SubtleButton.Hovered = FLinearColor(0.22f, 0.23f, 0.27f, 1.0f);
            Tokens.SubtleButton.Pressed = FLinearColor(0.15f, 0.16f, 0.19f, 1.0f);
            Tokens.HeaderButton.Normal = FLinearColor(0.24f, 0.26f, 0.30f, 1.0f);
            Tokens.HeaderButton.Hovered = FLinearColor(0.29f, 0.31f, 0.36f, 1.0f);
            Tokens.HeaderButton.Pressed = FLinearColor(0.20f, 0.22f, 0.26f, 1.0f);
            Tokens.StandardSection.Background = FLinearColor(0.22f, 0.23f, 0.26f, 1.0f);
            Tokens.EmphasisSection.Background = FLinearColor(0.20f, 0.24f, 0.30f, 1.0f);
            Tokens.StandardInput.Background = FLinearColor(0.20f, 0.21f, 0.24f, 0.96f);
            Tokens.StandardInput.Hovered = FLinearColor(0.24f, 0.25f, 0.29f, 1.0f);
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

    inline int32 GetSectionTitleFontSize()
    {
        return GetThemeMetrics().SectionTitleFontSize;
    }

    inline int32 GetLabelFontSize()
    {
        return GetThemeMetrics().LabelFontSize;
    }

    inline int32 GetValueFontSize()
    {
        return GetThemeMetrics().ValueFontSize;
    }

    inline int32 GetMutedFontSize()
    {
        return GetThemeMetrics().MutedFontSize;
    }

    inline float GetButtonHeight()
    {
        return GetThemeMetrics().ButtonHeight;
    }

    inline float GetInputHeight()
    {
        return GetThemeMetrics().InputHeight;
    }

    inline float GetCompactListHeight()
    {
        return GetThemeMetrics().CompactListHeight;
    }

    inline float GetStandardListHeight()
    {
        return GetThemeMetrics().StandardListHeight;
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
        ButtonStyle.NormalPadding = Metrics.ButtonPadding;
        ButtonStyle.PressedPadding = FMargin(Metrics.ButtonPadding.Left, Metrics.ButtonPadding.Top + 1.f, Metrics.ButtonPadding.Right, Metrics.ButtonPadding.Bottom);
        ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
        ButtonStyle.Normal.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ButtonStyle.Hovered.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ButtonStyle.Pressed.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ButtonStyle.Disabled.OutlineSettings.CornerRadii = FVector4(Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius, Metrics.CornerRadius);
        ButtonStyle.Normal.OutlineSettings.Width = Metrics.BorderWidth;
        ButtonStyle.Hovered.OutlineSettings.Width = Metrics.BorderWidth;
        ButtonStyle.Pressed.OutlineSettings.Width = Metrics.BorderWidth;
        ButtonStyle.Disabled.OutlineSettings.Width = Metrics.BorderWidth;
        Button->WidgetStyle = ButtonStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        Button->SetBackgroundColor(FLinearColor::White);
        Button->SetColorAndOpacity(FLinearColor::White);

        if (bApplyTextColorToChildren)
        {
            ApplyTextColorRecursive(Button, Palette.Text);
        }
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
        SizeBox->SetContent(MakeText(WidgetTree, Label, EffectiveFontSize, true, GetButtonTextColor(Style)));
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

    inline USizeBox* WrapFixedHeight(UWidgetTree* WidgetTree, UWidget* Child, float HeightOverride)
    {
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SizeBox->SetHeightOverride(HeightOverride);
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
        ComboBox->WidgetStyle = ComboBoxStyle;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }
}
