#include "InspectorTouchScrollBox.h"

UInspectorTouchScrollBox::UInspectorTouchScrollBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ApplyRuntimeInspectorDefaults();
}

void UInspectorTouchScrollBox::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    ApplyRuntimeInspectorDefaults();
}

void UInspectorTouchScrollBox::ApplyRuntimeInspectorDefaults()
{
    if (!bRuntimeInspectorTouchAdapterEnabled)
    {
        return;
    }

    SetIsTouchScrollingEnabled(true);
    SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
}

namespace RIInspectorTouchScroll
{
    void Configure(UScrollBox* ScrollBox)
    {
        if (!ScrollBox)
        {
            return;
        }

        ScrollBox->SetIsTouchScrollingEnabled(true);
        ScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    }

    bool HasTouchSupport(const UScrollBox* ScrollBox)
    {
        return ScrollBox && ScrollBox->GetIsTouchScrollingEnabled();
    }

    FString Describe(const UScrollBox* ScrollBox)
    {
        if (!ScrollBox)
        {
            return TEXT("None");
        }

        return FString::Printf(
            TEXT("%s[class=%s,touch=%d,offset=%.2f]"),
            *ScrollBox->GetName(),
            *ScrollBox->GetClass()->GetName(),
            ScrollBox->GetIsTouchScrollingEnabled() ? 1 : 0,
            ScrollBox->GetScrollOffset());
    }
}
