#include "RuntimeInspectorInputProcessor.h"
#include "RuntimeInspectorSettings.h"
#include "InspectorWorldSubsystem.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

bool FRuntimeInspectorInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    UInspectorWorldSubsystem* Sub = Subsystem.Get();
    if (!Sub) return false;


    //if (!Sub->IsInspectorOpen()) return false;
    if (!Sub->IsOpen()) return false;
    if (InKeyEvent.IsRepeat()) return true; 

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

bool FRuntimeInspectorInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
    UInspectorWorldSubsystem* Sub = Subsystem.Get();
    if (!Sub) return false;

    if (!Sub->IsOpen()) return false;

    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        return Sub->HandlePanelMouseButtonDown(MouseEvent);
    }

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    if (!Settings || !Settings->bEnableRightMousePick) return false;

    if (MouseEvent.GetEffectingButton() != EKeys::RightMouseButton) return false;

    if (Settings->bRightMousePickRequiresCtrl && !MouseEvent.IsControlDown()) return false;
    if (Settings->bRightMousePickRequiresShift && !MouseEvent.IsShiftDown()) return false;

    Sub->PickActorInView();
    return true; // 吃掉事件，避免弹出右键菜单/传给游戏
}

bool FRuntimeInspectorInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
    UInspectorWorldSubsystem* Sub = Subsystem.Get();
    if (!Sub || !Sub->IsOpen())
    {
        return false;
    }

    return Sub->HandlePanelMouseMove(MouseEvent);
}

bool FRuntimeInspectorInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
    UInspectorWorldSubsystem* Sub = Subsystem.Get();
    if (!Sub || !Sub->IsOpen())
    {
        return false;
    }

    return Sub->HandlePanelMouseButtonUp(MouseEvent);
}
