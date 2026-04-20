#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
#include "InputCoreTypes.h"
#include "RuntimeInspectorSettings.generated.h"

UENUM()
enum class ERuntimeInspectorThemePreset : uint8
{
    StudioSlate UMETA(DisplayName = "Studio Slate"),
    SoftContrast UMETA(DisplayName = "Soft Contrast")
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Runtime Inspector"))
class RUNTIMEINSPECTOR_API URuntimeInspectorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    URuntimeInspectorSettings();

    // 键盘：打开/关闭面板
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    FKey ToggleKey;

    // 键盘：按当前鼠标位置拾取命中的 Actor
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    FKey PickKey;

    // 键盘拾取是否要求 Ctrl/Shift（避免和游戏按键冲突）
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    bool bPickKeyRequiresCtrl;

    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    bool bPickKeyRequiresShift;

    // 鼠标右键拾取
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    bool bEnableRightMousePick;

    // 右键拾取是否要求 Ctrl/Shift（强烈建议默认 Ctrl+RMB）
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys", meta = (EditCondition = "bEnableRightMousePick"))
    bool bRightMousePickRequiresCtrl;

    UPROPERTY(EditAnywhere, config, Category = "Hotkeys", meta = (EditCondition = "bEnableRightMousePick"))
    bool bRightMousePickRequiresShift;

    // 让它出现在 Project Settings -> Plugins 分类下
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

    UPROPERTY(EditAnywhere, config, Category = "Outline")
    bool bEnableOutlinePP = true;

    UPROPERTY(EditAnywhere, config, Category = "Outline")
    float OutlinePPWeight = 1.0f;

    UPROPERTY(EditAnywhere, config, Category = "Outline")
    TSoftObjectPtr<UMaterialInterface> OutlinePostProcessMaterial;

    // Apply throttling (for sliders/drags): debounce property writes to reduce频繁 SetValueFromText.
    UPROPERTY(EditAnywhere, config, Category = "Apply")
    bool bEnableApplyDebounce = true;

    // Suggested 0.05~0.10 seconds.
    UPROPERTY(EditAnywhere, config, Category = "Apply", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.20"))
    float ApplyDebounceSeconds = 0.08f;

    UPROPERTY(EditAnywhere, config, Category = "Appearance")
    ERuntimeInspectorThemePreset ThemePreset = ERuntimeInspectorThemePreset::StudioSlate;

    UPROPERTY(EditAnywhere, config, Category = "Tools")
    TSoftObjectPtr<UDataTable> ToolsSelfTestsTable;

    UPROPERTY(EditAnywhere, config, Category = "Tools")
    TSoftObjectPtr<UDataTable> ToolsWorkflowsTable;

    // ===== Security (optional) =====
    // If enabled, the inspector panel will stay locked until unlocked via console command `ri.Unlock`.
    UPROPERTY(EditAnywhere, config, Category = "Security")
    bool bRequireUnlock = false;

    // If non-empty, `ri.Unlock <code>` must match this string.
    UPROPERTY(EditAnywhere, config, Category = "Security", meta = (EditCondition = "bRequireUnlock"))
    FString UnlockCode;

    // Recommended for packaged builds: auto-lock again when the panel is closed.
    UPROPERTY(EditAnywhere, config, Category = "Security", meta = (EditCondition = "bRequireUnlock"))
    bool bAutoLockOnClose = true;

};
