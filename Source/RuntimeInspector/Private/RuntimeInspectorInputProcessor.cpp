#include "RuntimeInspectorInputProcessor.h"
#include "InspectorWorldSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

bool FRuntimeInspectorInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    UInspectorWorldSubsystem* Sub = Subsystem.Get();
    if (!Sub) return false;

    // 只在面板打开时才拦截
    if (!Sub->IsInspectorOpen()) return false;

    // 避免按住键导致重复触发
    if (InKeyEvent.IsRepeat()) return true; // 你也可以 return false，但通常 true 更“锁死”行为

    const bool bCtrl = InKeyEvent.IsControlDown();
    const bool bShift = InKeyEvent.IsShiftDown();
    const FKey Key = InKeyEvent.GetKey();

    if (!bCtrl) return false;

    if (Key == EKeys::Z)
    {
        if (bShift) { Sub->Redo(); }
        else { Sub->Undo(); }
        return true; // handled
    }

    if (Key == EKeys::Y)
    {
        Sub->Redo();
        return true;
    }

    return false;
}

void FRuntimeInspectorInputProcessor::Tick(
    const float DeltaTime,
    FSlateApplication& SlateApp,
    TSharedRef<ICursor, ESPMode::ThreadSafe> Cursor)
{
    // no-op
    
}
