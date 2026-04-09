#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "Templates/SharedPointer.h"
#include "CoreMinimal.h"
#include "InspectorWorldSubsystem.h"
#include "UObject/WeakObjectPtr.h"

class FRuntimeInspectorInputProcessor : public IInputProcessor
{
public:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    virtual void Tick(
        const float DeltaTime,
        FSlateApplication& SlateApp,
        TSharedRef<ICursor, ESPMode::ThreadSafe> Cursor) override;

    virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
    virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
    virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
};
