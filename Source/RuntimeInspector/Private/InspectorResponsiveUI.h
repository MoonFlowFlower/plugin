#pragma once

#include "CoreMinimal.h"
#include "RuntimeInspectorSettings.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"

namespace RIResponsiveUI
{
    struct FContext
    {
        FVector2D PhysicalViewportSize = FVector2D::ZeroVector;
        float HostViewportDpi = 1.0f;
        float UserScale = 1.0f;
        float ContentScale = 1.0f;

        float PhysicalToLogical(float PhysicalSize) const
        {
            return PhysicalSize / FMath::Max(HostViewportDpi, 0.01f);
        }

        float ScaledPhysicalToLogical(float BasePhysicalSize) const
        {
            return PhysicalToLogical(BasePhysicalSize * UserScale);
        }

        FVector2D ScaledPhysicalToLogical(const FVector2D& BasePhysicalSize) const
        {
            return FVector2D(
                ScaledPhysicalToLogical(BasePhysicalSize.X),
                ScaledPhysicalToLogical(BasePhysicalSize.Y));
        }
    };

    inline FContext GetContext(UWidget* Widget)
    {
        FContext Result;
        if (Widget)
        {
            Result.PhysicalViewportSize = UWidgetLayoutLibrary::GetViewportSize(Widget);
            const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(Widget);
            Result.HostViewportDpi = ViewportScale > KINDA_SMALL_NUMBER ? ViewportScale : 1.0f;
        }

        const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
        Result.UserScale = Settings ? FMath::Clamp(Settings->UIScale, 0.8f, 1.5f) : 1.0f;
        Result.ContentScale = Result.UserScale / Result.HostViewportDpi;
        return Result;
    }
}
