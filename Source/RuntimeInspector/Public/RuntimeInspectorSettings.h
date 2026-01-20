#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "RuntimeInspectorSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Runtime Inspector"))
class RUNTIMEINSPECTOR_API URuntimeInspectorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    URuntimeInspectorSettings();

    // 键盘：打开/关闭面板
    UPROPERTY(EditAnywhere, config, Category = "Hotkeys")
    FKey ToggleKey;

    // 键盘：拾取（相机正前方 LineTrace）
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

};
