#include "RuntimeInspectorSettings.h"

URuntimeInspectorSettings::URuntimeInspectorSettings()
{
    ToggleKey = EKeys::O;
    PickKey = EKeys::P;

    bPickKeyRequiresCtrl = false;
    bPickKeyRequiresShift = false;

    bEnableRightMousePick = true;
    bRightMousePickRequiresCtrl = true;   // 默认 Ctrl+RMB，避免抢占游戏右键
    bRightMousePickRequiresShift = false;

    bEnableOutlinePP = true;
    OutlinePPWeight = 1.0f;
    ThemePreset = ERuntimeInspectorThemePreset::StudioSlate;
    ToolsSelfTestsTable = nullptr;
    ToolsWorkflowsTable = nullptr;

    OutlinePostProcessMaterial = TSoftObjectPtr<UMaterialInterface>(
        FSoftObjectPath(TEXT("/RuntimeInspector/Effects/M_RI_OutlinePP.M_RI_OutlinePP"))
    );

    // Security (optional)
    bRequireUnlock = false;
    UnlockCode = TEXT("");
    bAutoLockOnClose = true;
}
