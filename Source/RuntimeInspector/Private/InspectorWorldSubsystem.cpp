#include "InspectorWorldSubsystem.h"
#include "InspectorGroupItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorDefines.h"
#include "InspectorPropertyItem.h"
#include "InspectorFunctionItem.h"
#include "InspectorFunctionsSectionWidget.h"
#include "InspectorFunctionRowWidget.h"
#include "InspectorGroupsSectionWidget.h"
#include "InspectorMaterialParamRowWidget.h"
#include "InspectorModalBlockerWidget.h"
#include "InspectorPropertiesSectionWidget.h"
#include "InspectorPropertyRowWidget.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorSnapshotItem.h"
#include "InspectorFilePageWidget.h"
#include "InspectorSettingsPageWidget.h"
#include "InspectorTestPageWidget.h"
#include "InspectorCompactWidgetUtils.h"

#include "RuntimeInspectorInputProcessor.h"
#include "RuntimeInspectorSettings.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"


#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ContentWidget.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/ListViewBase.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/TreeView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"

#include "Camera/CameraComponent.h"

#include "Dom/JsonObject.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Scene.h" 

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"

#include "HAL/PlatformFilemanager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"

#include "Kismet/GameplayStatics.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialDomain.h"   


#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/DateTime.h"
#include "Misc/ConfigCacheIni.h"

#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/NoExportTypes.h"
#include "UObject/StructOnScope.h"
#include "Widgets/SWindow.h"

// Forward declarations for helpers defined later in this file
static FString RI_GetMaterialParamValueText(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName);
static FString RI_GetActorDisplayLabel(const AActor* Actor);
static void RI_ApplyLegacyActorHeaderVisibilityFix(UUserWidget* PanelWidget);
static constexpr float RI_MinUsablePanelWidth = 700.0f;
static constexpr float RI_MinUsablePanelHeight = 680.0f;
static constexpr float RI_DefaultPanelViewportWidthFraction = 0.44f;
static constexpr float RI_DefaultPanelViewportHeightFraction = 0.96f;
static constexpr float RI_DefaultPanelWidthMax = 920.0f;
static constexpr float RI_DefaultPanelHeightMin = 680.0f;
static constexpr float RI_DefaultPanelViewportInsetX = 8.0f;

static bool RI_TryGetEditableSearchText(UWidget* Widget, FString& OutText)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutText.Reset();
    if (UEditableTextBox* SearchBox = Cast<UEditableTextBox>(Widget))
    {
        OutText = SearchBox->GetText().ToString();
        return true;
    }
    if (UEditableText* SearchText = Cast<UEditableText>(Widget))
    {
        OutText = SearchText->GetText().ToString();
        return true;
    }
#else
    (void)Widget;
#endif
    return false;
}

static bool RI_TrySetEditableSearchText(UWidget* Widget, const FText& InText)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UEditableTextBox* SearchBox = Cast<UEditableTextBox>(Widget))
    {
        SearchBox->SetText(InText);
        return true;
    }
    if (UEditableText* SearchText = Cast<UEditableText>(Widget))
    {
        SearchText->SetText(InText);
        return true;
    }
#else
    (void)Widget;
    (void)InText;
#endif
    return false;
}

static void RI_BindEditableSearchTextChanged(UWidget* Widget, UInspectorWorldSubsystem* Owner)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Widget || !Owner)
    {
        return;
    }

    if (UEditableTextBox* SearchBox = Cast<UEditableTextBox>(Widget))
    {
        SearchBox->OnTextChanged.RemoveAll(Owner);
        SearchBox->OnTextChanged.AddDynamic(Owner, &UInspectorWorldSubsystem::HandleActorSearchTextChanged);
        return;
    }

    if (UEditableText* SearchText = Cast<UEditableText>(Widget))
    {
        SearchText->OnTextChanged.RemoveAll(Owner);
        SearchText->OnTextChanged.AddDynamic(Owner, &UInspectorWorldSubsystem::HandleActorSearchTextChanged);
    }
#else
    (void)Widget;
    (void)Owner;
#endif
}
static constexpr float RI_DefaultPanelViewportInsetY = 8.0f;
static constexpr float RI_DefaultPanelViewportSafetyMargin = 16.0f;
static const TCHAR* RI_ConfirmDialogClassPath = TEXT("/RuntimeInspector/UI/WBP_ConfirmDialog.WBP_ConfirmDialog_C");
static const FName RI_SelfTestId_ConfirmDialog(TEXT("confirm_dialog_color_input"));
static const FName RI_SelfTestId_SettingsPreview(TEXT("settings_preview"));
static const FName RI_SelfTestId_SettingsSavePersistence(TEXT("settings_save_persistence"));
static const FName RI_SelfTestId_StarLiveEditAndRun(TEXT("star_live_edit_and_run"));
static const FName RI_SelfTestId_StarPreciseNavigation(TEXT("star_precise_navigation"));
static const FName RI_SelfTestId_SettingsHotkey(TEXT("settings_hotkey_rebind"));
static const FName RI_SelfTestId_SettingsPageLayout(TEXT("settings_page_layout"));
static const FName RI_SelfTestId_ThemePresetPreview(TEXT("theme_preset_preview"));
static const FName RI_SelfTestId_PatchPreset(TEXT("patch_preset_roundtrip"));
static const FName RI_SelfTestId_PromotePreview(TEXT("promote_preview"));
static const FName RI_SelfTestId_PromoteConfig(TEXT("promote_config"));
static const FName RI_SelfTestId_PromoteBlueprintApply(TEXT("promote_blueprint_apply"));
static const FName RI_SelfTestId_PromoteMaterialApply(TEXT("promote_material_apply"));
static const FName RI_SelfTestId_AuditReport(TEXT("audit_report"));
static const FName RI_SelfTestId_FilePage(TEXT("file_page_injection"));
static const FName RI_SelfTestId_ContextStrip(TEXT("context_strip"));
static const FName RI_SelfTestId_WorkflowPageView(TEXT("workflow_page_view"));
static const FName RI_SelfTestId_TestPageLayout(TEXT("test_page_layout"));
static const FName RI_SelfTestId_PanelInteraction(TEXT("panel_interaction"));
static const FName RI_SelfTestId_ActorPageStructure(TEXT("actor_page_structure"));
static const FName RI_SelfTestId_FileWorkflow(TEXT("file_workflow"));
static const FName RI_SelfTestId_FilePromote(TEXT("file_promote_workflow"));
static const FName RI_SelfTestId_FileCompare(TEXT("file_compare_view"));
static const FName RI_SelfTestId_FileRoleCompare(TEXT("file_role_compare_view"));
static const FName RI_SelfTestId_FileRemoteSessionCompare(TEXT("file_remote_session_compare_view"));
static const FName RI_SelfTestId_ActorPromoteFile(TEXT("actor_promote_file_workflow"));
static const FName RI_SelfTestId_ActorApplyFile(TEXT("actor_apply_file_workflow"));
static const FName RI_SelfTestId_RuntimeSessionRole(TEXT("runtime_session_role"));
static const FName RI_SelfTestId_RuntimeRoleCompare(TEXT("runtime_role_compare"));
static const FName RI_SelfTestId_RemoteRuntimeFoundation(TEXT("remote_runtime_foundation"));
static const FName RI_SelfTestId_RemoteSessionCompare(TEXT("remote_session_compare"));
static const FName RI_SelfTestId_RemoteSessionTargetSetCompare(TEXT("remote_session_target_set_compare"));
static const FName RI_SelfTestId_RemoteSessionTargetSetCompareMatrix(TEXT("remote_session_target_set_compare_matrix"));
static const FName RI_SelfTestId_RemoteSessionContextUI(TEXT("remote_session_context_ui"));

static float RI_GetViewportLimitedDefaultDimension(float ResolvedDimension, float MinUsableDimension, float ViewportDimension, float ViewportFraction)
{
    if (ViewportDimension <= 1.0f)
    {
        return ResolvedDimension;
    }

    if (ViewportFraction <= 0.0f)
    {
        return ResolvedDimension;
    }

    const float PreferredMaxDimension = FMath::Max(MinUsableDimension, ViewportDimension * ViewportFraction);
    return FMath::Clamp(ResolvedDimension, MinUsableDimension, PreferredMaxDimension);
}

static FVector2D RI_GetLogicalViewportSize(UWorld* World)
{
    if (!World)
    {
        return FVector2D::ZeroVector;
    }

    FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(World);
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(World);
    if (ViewportScale > KINDA_SMALL_NUMBER)
    {
        ViewportSize /= ViewportScale;
    }

    return ViewportSize;
}

static float RI_GetViewportAvailableDimension(float ViewportDimension)
{
    return ViewportDimension > 1.0f
        ? FMath::Max(320.0f, ViewportDimension - RI_DefaultPanelViewportSafetyMargin)
        : 0.0f;
}

static float RI_ResolvePanelDefaultDimension(
    float LogicalViewportDimension,
    float ViewportFraction,
    float PreferredMinDimension,
    float PreferredMaxDimension)
{
    const float AvailableDimension = RI_GetViewportAvailableDimension(LogicalViewportDimension);
    if (AvailableDimension <= 1.0f)
    {
        return 0.0f;
    }

    const float TargetDimension = LogicalViewportDimension * ViewportFraction;
    const float EffectiveMaxDimension = PreferredMaxDimension > 1.0f
        ? FMath::Min(PreferredMaxDimension, AvailableDimension)
        : AvailableDimension;
    const float EffectiveMinDimension = FMath::Min(PreferredMinDimension, EffectiveMaxDimension);
    return FMath::Clamp(TargetDimension, EffectiveMinDimension, EffectiveMaxDimension);
}

static FVector2D RI_GetDefaultPanelCanvasPosition(UWorld* World, const FVector2D& PanelSize)
{
    const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(World);
    if (LogicalViewportSize.X <= 1.0f || LogicalViewportSize.Y <= 1.0f)
    {
        return FVector2D(8.0f, 8.0f);
    }

    const float MaxX = FMath::Max(8.0f, LogicalViewportSize.X - PanelSize.X - 8.0f);
    const float MaxY = FMath::Max(8.0f, LogicalViewportSize.Y - PanelSize.Y - 8.0f);
    return FVector2D(
        FMath::Clamp(LogicalViewportSize.X - PanelSize.X - RI_DefaultPanelViewportInsetX, 8.0f, MaxX),
        FMath::Clamp(RI_DefaultPanelViewportInsetY, 8.0f, MaxY));
}

static FString RI_GetPropertyDisplayNameRuntimeSafe(const FProperty* Prop)
{
    if (!Prop)
    {
        return FString();
    }

#if WITH_EDITOR
    const FString FriendlyName = Prop->GetDisplayNameText().ToString();
    if (!FriendlyName.IsEmpty())
    {
        return FriendlyName;
    }
#endif

    const FString AuthoredName = Prop->GetAuthoredName();
    return AuthoredName.IsEmpty() ? Prop->GetName() : AuthoredName;
}

static FString RI_GetFunctionDisplayNameRuntimeSafe(const UFunction* Function)
{
    if (!Function)
    {
        return TEXT("Function");
    }

#if WITH_EDITOR
    const FString DisplayName = Function->GetDisplayNameText().ToString();
    if (!DisplayName.IsEmpty())
    {
        return DisplayName;
    }
#endif

    return Function->GetName();
}

static bool RI_FunctionHasMetadataRuntimeSafe(const UFunction* Function, const TCHAR* Key)
{
#if WITH_EDITOR
    return Function && Function->HasMetaData(Key);
#else
    return false;
#endif
}

static FString RI_GetFunctionMetadataRuntimeSafe(const UFunction* Function, const TCHAR* Key)
{
#if WITH_EDITOR
    return Function ? Function->GetMetaData(Key) : FString();
#else
    return FString();
#endif
}

static FString RI_GetFunctionTooltipRuntimeSafe(const UFunction* Function)
{
#if WITH_EDITOR
    return Function ? Function->GetToolTipText().ToString() : FString();
#else
    return FString();
#endif
}

static bool RI_IsBlueprintGeneratedClassRuntimeSafe(const UClass* Class)
{
#if WITH_EDITOR
    return Class && Class->ClassGeneratedBy != nullptr;
#else
    return false;
#endif
}

static const FName RI_SelfTestId_RemotePackagedFoundation(TEXT("remote_packaged_foundation"));
static const FName RI_SelfTestId_RemotePackagedPatchPull(TEXT("remote_packaged_patch_pull"));
static const FName RI_SelfTestId_RemotePackagedToSourceClosure(TEXT("remote_packaged_to_source_closure"));
static const FName RI_SelfTestId_FabScreenshotFoundation(TEXT("fab_screenshot_foundation"));
static const FName RI_SelfTestId_FabScreenshotActorPage(TEXT("fab_screenshot_actor_page"));
static const FName RI_SelfTestId_FabScreenshotSettingsPage(TEXT("fab_screenshot_settings_page"));
static const FName RI_SelfTestId_FabScreenshotToolsPage(TEXT("fab_screenshot_tools_page"));
static const FName RI_SelfTestId_FabScreenshotRemoteSession(TEXT("fab_screenshot_remote_session"));
static const FName RI_SelfTestId_FabScreenshotPromoteOrAudit(TEXT("fab_screenshot_promote_or_audit"));
static const FName RI_SelfTestId_WorkflowMatrix(TEXT("workflow_matrix"));
static const FName RI_VerificationProfileId_ColorRuntime(TEXT("color_runtime_edit"));
static const FName RI_VerificationProfileId_SettingsPreview(TEXT("settings_preview_apply"));
static const FName RI_VerificationProfileId_SettingsHotkey(TEXT("settings_hotkey_rebind"));
static const FName RI_VerificationProfileId_SettingsPageLayout(TEXT("settings_page_layout"));
static const FName RI_VerificationProfileId_ThemePresetPreview(TEXT("theme_preset_preview"));
static const FName RI_VerificationProfileId_PromotePreview(TEXT("promote_preview_blueprint"));
static const FName RI_VerificationProfileId_PromoteConfig(TEXT("promote_config_settings"));
static const FName RI_VerificationProfileId_PromoteBlueprintApply(TEXT("promote_blueprint_apply"));
static const FName RI_VerificationProfileId_PromoteMaterialApply(TEXT("promote_material_apply"));
static const FName RI_VerificationProfileId_AuditReport(TEXT("audit_report_current_vs_patch"));
static const FName RI_VerificationProfileId_FilePromote(TEXT("file_promote_config"));
static const FName RI_VerificationProfileId_FileCompare(TEXT("file_compare_view"));
static const FName RI_VerificationProfileId_FileRoleCompare(TEXT("file_role_compare_view"));
static const FName RI_VerificationProfileId_FileRemoteSessionCompare(TEXT("file_remote_session_compare_view"));
static const FName RI_VerificationProfileId_ContextStrip(TEXT("context_strip"));
static const FName RI_VerificationProfileId_WorkflowPageView(TEXT("workflow_page_view"));
static const FName RI_VerificationProfileId_TestPageLayout(TEXT("test_page_layout"));
static const FName RI_VerificationProfileId_ActorPromoteFile(TEXT("actor_promote_file_workflow"));
static const FName RI_VerificationProfileId_ActorApplyFile(TEXT("actor_apply_file_workflow"));
static const FName RI_VerificationProfileId_RuntimeSessionRole(TEXT("runtime_session_role"));
static const FName RI_VerificationProfileId_RuntimeRoleCompare(TEXT("runtime_role_compare"));
static const FName RI_VerificationProfileId_RemoteRuntimeFoundation(TEXT("remote_runtime_foundation"));
static const FName RI_VerificationProfileId_RemoteSessionCompare(TEXT("remote_session_compare"));
static const FName RI_VerificationProfileId_RemoteSessionTargetSetCompare(TEXT("remote_session_target_set_compare"));
static const FName RI_VerificationProfileId_RemoteSessionTargetSetCompareMatrix(TEXT("remote_session_target_set_compare_matrix"));
static const FName RI_VerificationProfileId_RemoteSessionContextUI(TEXT("remote_session_context_ui"));
static const FName RI_VerificationProfileId_RemotePackagedFoundation(TEXT("remote_packaged_foundation"));
static const FName RI_VerificationProfileId_RemotePackagedPatchPull(TEXT("remote_packaged_patch_pull"));
static const FName RI_VerificationProfileId_RemotePackagedToSourceClosure(TEXT("remote_packaged_to_source_closure"));
static const FName RI_VerificationProfileId_FabScreenshotFoundation(TEXT("fab_screenshot_foundation"));
static const FName RI_VerificationProfileId_WorkflowMatrix(TEXT("workflow_matrix"));
static const FName RI_WorkflowId_MainlineSafePatchCore(TEXT("mainline_safe_patch_core"));
static const FName RI_WorkflowId_MainlinePromoteSourceAssets(TEXT("mainline_promote_source_assets"));
static const FName RI_WorkflowId_MainlineFullClosure(TEXT("mainline_full_closure"));
static const FName RI_WorkflowId_MainlineActorPatchRoundtrip(TEXT("mainline_actor_patch_roundtrip"));
static const FName RI_WorkflowId_MainlineActorPromoteFileClosure(TEXT("mainline_actor_promote_file_closure"));
static const FName RI_WorkflowId_MainlineActorApplyFileClosure(TEXT("mainline_actor_apply_file_closure"));
static const FName RI_WorkflowId_MainlineActorEndToEndClosure(TEXT("mainline_actor_end_to_end_closure"));
static const FName RI_WorkflowId_MainlineRemoteActorEndToEndClosure(TEXT("mainline_remote_actor_end_to_end_closure"));
static const FName RI_WorkflowId_MainlineRoleCompareFoundation(TEXT("mainline_role_compare_foundation"));
static const FName RI_WorkflowId_MainlineRemoteRuntimeFoundation(TEXT("mainline_remote_runtime_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionCompareFoundation(TEXT("mainline_remote_session_compare_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionTargetSetCompareFoundation(TEXT("mainline_remote_session_target_set_compare_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionCompareUIFoundation(TEXT("mainline_remote_session_compare_ui_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionCompareScopedUIFoundation(TEXT("mainline_remote_session_compare_scoped_ui_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionCompareMatrixFoundation(TEXT("mainline_remote_session_compare_matrix_foundation"));
static const FName RI_WorkflowId_MainlineRemoteSessionContextUIFoundation(TEXT("mainline_remote_session_context_ui_foundation"));
static const FName RI_WorkflowId_MainlineRemoteWorkflowMatrixFoundation(TEXT("mainline_remote_workflow_matrix_foundation"));
static const FName RI_WorkflowId_MainlineRemotePackagedFoundation(TEXT("mainline_remote_packaged_foundation"));
static const FName RI_WorkflowId_MainlineRemotePackagedPatchPull(TEXT("mainline_remote_packaged_patch_pull"));
static const FName RI_WorkflowId_MainlineRemotePackagedToSourceClosure(TEXT("mainline_remote_packaged_to_source_closure"));
static const FName RI_WorkflowId_FabScreenshotFoundation(TEXT("fab_screenshot_foundation"));
static const FName RI_WorkflowMatrixId_Default(TEXT("mainline_remote_workflow_matrix_default"));
static const FName RI_WorkflowMatrixId_RemotePackagedDefault(TEXT("mainline_remote_packaged_matrix_default"));

static FString RI_WorldTypeLabel(EWorldType::Type WorldType)
{
    switch (WorldType)
    {
    case EWorldType::Editor: return TEXT("Editor");
    case EWorldType::PIE: return TEXT("PIE");
    case EWorldType::Game: return TEXT("Game");
    case EWorldType::GamePreview: return TEXT("GamePreview");
    case EWorldType::EditorPreview: return TEXT("EditorPreview");
    case EWorldType::GameRPC: return TEXT("GameRPC");
    case EWorldType::Inactive: return TEXT("Inactive");
    case EWorldType::None: return TEXT("None");
    default: return TEXT("Unknown");
    }
}

static FString RI_NetModeLabel(ENetMode NetMode)
{
    switch (NetMode)
    {
    case NM_Standalone: return TEXT("Standalone");
    case NM_DedicatedServer: return TEXT("DedicatedServer");
    case NM_ListenServer: return TEXT("ListenServer");
    case NM_Client: return TEXT("Client");
    default: return TEXT("Unknown");
    }
}

static FString RI_NetRoleLabel(ENetRole NetRole)
{
    switch (NetRole)
    {
    case ROLE_None: return TEXT("None");
    case ROLE_SimulatedProxy: return TEXT("SimulatedProxy");
    case ROLE_AutonomousProxy: return TEXT("AutonomousProxy");
    case ROLE_Authority: return TEXT("Authority");
    default: return TEXT("Unknown");
    }
}

static AActor* RI_FindActorByRequest(UWorld* TargetWorld, const FString& RequestedActor)
{
    if (!TargetWorld || RequestedActor.IsEmpty())
    {
        return nullptr;
    }

    const FString RequestedLower = RequestedActor.ToLower();
    for (TActorIterator<AActor> It(TargetWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }

        if (Actor->GetName() == RequestedActor
            || RI_GetActorDisplayLabel(Actor) == RequestedActor
            || Actor->GetPathName() == RequestedActor)
        {
            return Actor;
        }

        if (Actor->GetName().ToLower() == RequestedLower
            || RI_GetActorDisplayLabel(Actor).ToLower() == RequestedLower
            || Actor->GetPathName().ToLower() == RequestedLower)
        {
            return Actor;
        }
    }

    return nullptr;
}

static bool RI_TryMapNetRoleToCompareRole(ENetRole NetRole, bool bHasAuthority, ERIRuntimeCompareRole& OutRole)
{
    switch (NetRole)
    {
    case ROLE_Authority:
        OutRole = ERIRuntimeCompareRole::Authority;
        return true;
    case ROLE_AutonomousProxy:
        OutRole = ERIRuntimeCompareRole::AutonomousProxy;
        return true;
    case ROLE_SimulatedProxy:
        OutRole = ERIRuntimeCompareRole::SimulatedProxy;
        return true;
    default:
        if (bHasAuthority)
        {
            OutRole = ERIRuntimeCompareRole::Authority;
            return true;
        }
        return false;
    }
}

static bool RI_AreEditableSettingsEqual(const FRIEditableSettings& A, const FRIEditableSettings& B)
{
    return A.ToggleKey == B.ToggleKey
        && A.PickKey == B.PickKey
        && A.bPickKeyRequiresCtrl == B.bPickKeyRequiresCtrl
        && A.bPickKeyRequiresShift == B.bPickKeyRequiresShift
        && A.bEnableRightMousePick == B.bEnableRightMousePick
        && A.bRightMousePickRequiresCtrl == B.bRightMousePickRequiresCtrl
        && A.bRightMousePickRequiresShift == B.bRightMousePickRequiresShift
        && A.bEnableOutlinePP == B.bEnableOutlinePP
        && FMath::IsNearlyEqual(A.OutlinePPWeight, B.OutlinePPWeight, KINDA_SMALL_NUMBER)
        && A.bEnableApplyDebounce == B.bEnableApplyDebounce
        && FMath::IsNearlyEqual(A.ApplyDebounceSeconds, B.ApplyDebounceSeconds, KINDA_SMALL_NUMBER)
        && A.bRequireUnlock == B.bRequireUnlock
        && A.bAutoLockOnClose == B.bAutoLockOnClose;
}

static FRIEditableSettings RI_MakeEditableSettings(const URuntimeInspectorSettings* Settings)
{
    FRIEditableSettings Result;
    if (!Settings)
    {
        return Result;
    }

    Result.ToggleKey = Settings->ToggleKey;
    Result.PickKey = Settings->PickKey;
    Result.bPickKeyRequiresCtrl = Settings->bPickKeyRequiresCtrl;
    Result.bPickKeyRequiresShift = Settings->bPickKeyRequiresShift;
    Result.bEnableRightMousePick = Settings->bEnableRightMousePick;
    Result.bRightMousePickRequiresCtrl = Settings->bRightMousePickRequiresCtrl;
    Result.bRightMousePickRequiresShift = Settings->bRightMousePickRequiresShift;
    Result.bEnableOutlinePP = Settings->bEnableOutlinePP;
    Result.OutlinePPWeight = Settings->OutlinePPWeight;
    Result.bEnableApplyDebounce = Settings->bEnableApplyDebounce;
    Result.ApplyDebounceSeconds = Settings->ApplyDebounceSeconds;
    Result.bRequireUnlock = Settings->bRequireUnlock;
    Result.bAutoLockOnClose = Settings->bAutoLockOnClose;
    return Result;
}

static UBlueprint* RI_LoadPreferredSelfTestBlueprint()
{
    static const TCHAR* CandidatePaths[] =
    {
        TEXT("/RuntimeInspector/Test/BP_TestVarsActor.BP_TestVarsActor"),
        TEXT("/RuntimeInspector/BP_TestVarsActor.BP_TestVarsActor")
    };

    for (const TCHAR* CandidatePath : CandidatePaths)
    {
        if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, CandidatePath))
        {
            return Blueprint;
        }
    }

    return nullptr;
}

static UMaterialInstanceConstant* RI_LoadPreferredSelfTestMIC()
{
    static const TCHAR* CandidatePaths[] =
    {
        TEXT("/RuntimeInspector/Test/MI_Test.MI_Test")
    };

    for (const TCHAR* CandidatePath : CandidatePaths)
    {
        if (UMaterialInstanceConstant* MIC = LoadObject<UMaterialInstanceConstant>(nullptr, CandidatePath))
        {
            return MIC;
        }
    }

    return nullptr;
}

static UStaticMesh* RI_LoadPreferredSelfTestMesh()
{
    static const TCHAR* CandidatePaths[] =
    {
        TEXT("/Engine/BasicShapes/Cube.Cube"),
        TEXT("/Engine/BasicShapes/Sphere.Sphere")
    };

    for (const TCHAR* CandidatePath : CandidatePaths)
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, CandidatePath))
        {
            return Mesh;
        }
    }

    return nullptr;
}

static bool RI_TryParseBoolText(const FString& InText, bool& OutValue)
{
    const FString Trimmed = InText.TrimStartAndEnd();
    if (Trimmed.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Trimmed == TEXT("1"))
    {
        OutValue = true;
        return true;
    }

    if (Trimmed.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Trimmed == TEXT("0"))
    {
        OutValue = false;
        return true;
    }

    return false;

}

static int32 RI_GetPanelChildIndex(const UPanelWidget* Parent, const UWidget* Child)
{
    if (!Parent || !Child)
    {
        return INDEX_NONE;
    }

    return Parent->GetChildIndex(const_cast<UWidget*>(Child));
}

static bool RI_IsVerticalSlotRule(const UWidget* Widget, ESlateSizeRule::Type Rule)
{
    const UVerticalBoxSlot* VerticalSlot = Widget ? Cast<UVerticalBoxSlot>(Widget->Slot) : nullptr;
    return VerticalSlot && VerticalSlot->GetSize().SizeRule == Rule;
}

static bool RI_IsRatioNear(float Actual, float Expected, float Tolerance = 0.05f)
{
    return FMath::Abs(Actual - Expected) <= Tolerance;
}

static bool RI_AreSelfTestPrimitiveValuesEquivalent(const FString& A, const FString& B, const FString& Kind)
{
    if (Kind == TEXT("bool"))
    {
        bool AValue = false;
        bool BValue = false;
        return RI_TryParseBoolText(A, AValue) && RI_TryParseBoolText(B, BValue) && AValue == BValue;
    }

    if (Kind == TEXT("float") || Kind == TEXT("int"))
    {
        double AValue = 0.0;
        double BValue = 0.0;
        return FDefaultValueHelper::ParseDouble(A, AValue)
            && FDefaultValueHelper::ParseDouble(B, BValue)
            && FMath::IsNearlyEqual(AValue, BValue, 0.0001);
    }

    return A.Equals(B, ESearchCase::IgnoreCase);
}

static FString RI_FormatLinearColorText(const FLinearColor& Color)
{
    return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Color.R, Color.G, Color.B, Color.A);
}

static FLinearColor RI_MakeDistinctSelfTestColor(const FLinearColor& Original)
{
    const FLinearColor Candidate = (FMath::IsNearlyEqual(Original.R, 0.15f, 0.01f)
        && FMath::IsNearlyEqual(Original.G, 0.75f, 0.01f)
        && FMath::IsNearlyEqual(Original.B, 0.35f, 0.01f))
        ? FLinearColor(0.85f, 0.20f, 0.45f, 1.0f)
        : FLinearColor(0.15f, 0.75f, 0.35f, 1.0f);

    return Candidate;
}

static bool RI_SelectWritablePrimitivePropertyForSelfTest(
    AActor* Actor,
    FName& OutPropertyName,
    FString& OutOriginalText,
    FString& OutPatchedText,
    FString& OutPropertyKind,
    double& OutNumericOriginalValue,
    double& OutNumericTargetValue)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    OutPropertyName = NAME_None;
    OutOriginalText.Reset();
    OutPatchedText.Reset();
    OutPropertyKind.Reset();
    OutNumericOriginalValue = 0.0;
    OutNumericTargetValue = 0.0;

    if (!Actor)
    {
        return false;
    }

    auto TrySelectWritableProperty = [&](FProperty* Property, const FString& Kind) -> bool
    {
        if (!Property || Kind.IsEmpty() || !InspectorPropertyUtils::CanSetFromText(Actor, Property))
        {
            return false;
        }

        const FName CandidateName = Property->GetFName();
        FString CurrentText;
        if (!InspectorPropertyUtils::GetValueAsText(Actor, CandidateName, CurrentText))
        {
            return false;
        }

        FString NewText;
        if (Kind == TEXT("bool"))
        {
            const bool bCurrent = CurrentText.Equals(TEXT("True"), ESearchCase::IgnoreCase) || CurrentText == TEXT("1");
            NewText = bCurrent ? TEXT("False") : TEXT("True");
        }
        else if (Kind == TEXT("float"))
        {
            const double CurrentValue = FCString::Atod(*CurrentText);
            const double CandidateValue = FMath::IsNearlyEqual(CurrentValue, 1.15, 0.001) ? 0.95 : (CurrentValue + 0.15);
            NewText = FString::SanitizeFloat(CandidateValue);
            OutNumericOriginalValue = CurrentValue;
            OutNumericTargetValue = CandidateValue;
        }
        else if (Kind == TEXT("int"))
        {
            const int32 CurrentValue = FCString::Atoi(*CurrentText);
            const int32 CandidateValue = (CurrentValue == 0) ? 1 : 0;
            NewText = FString::FromInt(CandidateValue);
            OutNumericOriginalValue = static_cast<double>(CurrentValue);
            OutNumericTargetValue = static_cast<double>(CandidateValue);
        }
        else
        {
            return false;
        }

        if (CurrentText == NewText)
        {
            return false;
        }

        OutPropertyName = CandidateName;
        OutOriginalText = CurrentText;
        OutPatchedText = NewText;
        OutPropertyKind = Kind;
        return true;
    };

    const TCHAR* PreferredNames[] = {
        TEXT("InitialLifeSpan"),
        TEXT("NetCullDistanceSquared"),
        TEXT("CustomTimeDilation"),
        TEXT("bHidden")
    };

    for (const TCHAR* PreferredName : PreferredNames)
    {
        if (FProperty* Property = Actor->GetClass()->FindPropertyByName(PreferredName))
        {
            const FString Kind = Property->IsA<FBoolProperty>() ? TEXT("bool")
                : ((Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>()) ? TEXT("float")
                : (Property->IsA<FIntProperty>() ? TEXT("int") : TEXT("")));
            if (TrySelectWritableProperty(Property, Kind))
            {
                return true;
            }
        }
    }

    for (TFieldIterator<FProperty> It(Actor->GetClass()); It; ++It)
    {
        FProperty* Property = *It;
        const FString Kind = Property->IsA<FBoolProperty>() ? TEXT("bool")
            : ((Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>()) ? TEXT("float")
            : (Property->IsA<FIntProperty>() ? TEXT("int") : TEXT("")));
        if (TrySelectWritableProperty(Property, Kind))
        {
            return true;
        }
    }

    return false;
#endif

}

static void RI_RefreshPropertyList(UUserWidget* PanelWidget, EInspectorRefreshReason Reason)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    UListViewBase* PropertyList = Cast<UListViewBase>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Properties")));
    if (!PropertyList)
    {
        return;
    }

    switch (Reason)
    {
    case EInspectorRefreshReason::ValuesChanged:
        PropertyList->RequestRefresh();
        break;
    case EInspectorRefreshReason::UIStateChanged:
    case EInspectorRefreshReason::UndoRedo:
    case EInspectorRefreshReason::StructureChanged:
    case EInspectorRefreshReason::TargetInvalid:
    default:
        PropertyList->RegenerateAllEntries();
        break;
    }
#endif

    return;
}

static bool RI_InvokeWidgetFunctionIfPresent(UWidget* Widget, const FName& FunctionName)
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* UserWidget = Cast<UUserWidget>(Widget);
    if (!UserWidget)
    {
        return false;
    }

    if (UFunction* Function = UserWidget->GetClass()->FindFunctionByName(FunctionName))
    {
        if (Function->NumParms == 0)
        {
            UserWidget->ProcessEvent(Function, nullptr);
            return true;
        }
    }
#endif
    return false;
}

static FObjectPropertyBase* RI_FindObjectPropertyByAuthoredName(UClass* InClass, const FName AuthoredName)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!InClass || AuthoredName.IsNone())
    {
        return nullptr;
    }

    const FString Wanted = AuthoredName.ToString();
    for (TFieldIterator<FObjectPropertyBase> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FObjectPropertyBase* Prop = *It;
        if (!Prop)
        {
            continue;
        }

        if (Prop->GetFName() == AuthoredName)
        {
            return Prop;
        }

        const FString PropName = Prop->GetName();
        const FString Authored = Prop->GetAuthoredName();
        if (Authored == Wanted
            || PropName.StartsWith(Wanted, ESearchCase::CaseSensitive)
            || PropName.Contains(Wanted, ESearchCase::CaseSensitive)
            || Authored.StartsWith(Wanted, ESearchCase::CaseSensitive)
            || Authored.Contains(Wanted, ESearchCase::CaseSensitive))
        {
            return Prop;
        }
    }
#else
    (void)InClass;
    (void)AuthoredName;
#endif
    return nullptr;
}

static UObject* RI_ReadObjectPropertyByAuthoredName(UObject* Object, const FName AuthoredName)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Object)
    {
        return nullptr;
    }

    if (FObjectPropertyBase* Prop = RI_FindObjectPropertyByAuthoredName(Object->GetClass(), AuthoredName))
    {
        return Prop->GetObjectPropertyValue_InContainer(Object);
    }
#else
    (void)Object;
    (void)AuthoredName;
#endif
    return nullptr;
}

static bool RI_SetLegacyPropertyRowSwatch(UUserWidget* EntryWidget, const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!EntryWidget || !EntryWidget->WidgetTree)
    {
        return false;
    }

    bool bUpdated = false;

    if (UButton* ColorButton = Cast<UButton>(EntryWidget->WidgetTree->FindWidget(TEXT("BTN_color"))))
    {
        ColorButton->SetBackgroundColor(InColor);
        ColorButton->SetColorAndOpacity(FLinearColor::White);
        ColorButton->InvalidateLayoutAndVolatility();
        bUpdated = true;
    }

    EntryWidget->InvalidateLayoutAndVolatility();
    return bUpdated;
#else
    (void)EntryWidget;
    (void)InColor;
    return false;
#endif
}

static bool RI_SyncLegacyPropertyEntrySwatch(UUserWidget* EntryWidget, UObject* ItemObject, const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!EntryWidget || !ItemObject)
    {
        return false;
    }

    UObject* BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("BoundItem"));
    if (!BoundObject)
    {
        BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("MaterialItem"));
    }
    if (!BoundObject)
    {
        BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("Item"));
    }

    if (BoundObject != ItemObject)
    {
        return false;
    }

    const bool bSwatchSet = RI_SetLegacyPropertyRowSwatch(EntryWidget, InColor);
    RI_InvokeWidgetFunctionIfPresent(EntryWidget, TEXT("RefreshMaterial"));
    RI_InvokeWidgetFunctionIfPresent(EntryWidget, TEXT("RefreshItemShow"));
    if (bSwatchSet)
    {
        RI_SetLegacyPropertyRowSwatch(EntryWidget, InColor);
    }
    return true;
#else
    (void)EntryWidget;
    (void)ItemObject;
    (void)InColor;
    return false;
#endif
}

static bool RI_SyncLegacyPropertyListSwatch(UUserWidget* PanelWidget, UObject* ItemObject, const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree || !ItemObject)
    {
        return false;
    }

    UListViewBase* PropertyList = Cast<UListViewBase>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Properties")));
    if (!PropertyList)
    {
        return false;
    }

    bool bMatched = false;
    const TArray<UUserWidget*> DisplayedEntries = PropertyList->GetDisplayedEntryWidgets();
    for (UUserWidget* EntryWidget : DisplayedEntries)
    {
        bMatched |= RI_SyncLegacyPropertyEntrySwatch(EntryWidget, ItemObject, InColor);
    }

    return bMatched;
#else
    (void)PanelWidget;
    (void)ItemObject;
    (void)InColor;
    return false;
#endif
}

static void RI_RefreshVisiblePropertyEntryWidgets(UUserWidget* PanelWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    UListViewBase* PropertyList = Cast<UListViewBase>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Properties")));
    if (!PropertyList)
    {
        return;
    }

    const TArray<UUserWidget*> DisplayedEntries = PropertyList->GetDisplayedEntryWidgets();
    static const FName RefreshNames[] = {
        TEXT("RefreshMaterial"),
        TEXT("RefreshItem"),
        TEXT("RefreshItemShow")
    };

    for (UUserWidget* EntryWidget : DisplayedEntries)
    {
        if (!EntryWidget)
        {
            continue;
        }

        for (const FName& RefreshName : RefreshNames)
        {
            RI_InvokeWidgetFunctionIfPresent(EntryWidget, RefreshName);
        }
    }
#endif
}

static void RI_RefreshLegacyPropertyPanelWidgets(UUserWidget* PanelWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    RI_InvokeWidgetFunctionIfPresent(PanelWidget, TEXT("RefreshRightItems"));
    RI_InvokeWidgetFunctionIfPresent(PanelWidget, TEXT("RefreshSetListItems"));
    RI_InvokeWidgetFunctionIfPresent(PanelWidget, TEXT("RefreshVisibleRows"));

    UListViewBase* PropertyList = Cast<UListViewBase>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Properties")));
    if (!PropertyList)
    {
        return;
    }

    PropertyList->RegenerateAllEntries();
    PropertyList->RequestRefresh();
    RI_RefreshVisiblePropertyEntryWidgets(PanelWidget);
#endif
}

static void RI_RefreshActorGroupWidgets(UUserWidget* PanelWidget, UInspectorWorldSubsystem* InspectorSubsystem, EInspectorRefreshReason Reason)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    static const FName RefreshActorName(TEXT("RefreshActor"));
    if (UFunction* RefreshActorFunction = PanelWidget->GetClass()->FindFunctionByName(RefreshActorName))
    {
        PanelWidget->ProcessEvent(RefreshActorFunction, nullptr);
    }

    const auto RefreshListLikeWidget = [Reason](UWidget* Widget)
    {
        if (UListViewBase* ListWidget = Cast<UListViewBase>(Widget))
        {
            switch (Reason)
            {
            case EInspectorRefreshReason::ValuesChanged:
                ListWidget->RequestRefresh();
                break;
            case EInspectorRefreshReason::UIStateChanged:
            case EInspectorRefreshReason::UndoRedo:
            case EInspectorRefreshReason::StructureChanged:
            case EInspectorRefreshReason::TargetInvalid:
            default:
                ListWidget->RegenerateAllEntries();
                break;
            }
        }
    };

    if (UTreeView* TreeWidget = Cast<UTreeView>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_TreeGroup"))))
    {
        TArray<UObject*> RootItems;
        if (InspectorSubsystem)
        {
            InspectorSubsystem->GetGroupTreeRootsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), RootItems);
        }
        TreeWidget->SetListItems(RootItems);
        for (UObject* RootItemObject : RootItems)
        {
            if (UInspectorGroupItem* RootItem = Cast<UInspectorGroupItem>(RootItemObject))
            {
                TreeWidget->SetItemExpansion(RootItem, RootItem->bExpanded);
            }
        }
    }

    if (UListView* GroupListWidget = Cast<UListView>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Group"))))
    {
        TArray<UObject*> FlatGroupItems;
        if (InspectorSubsystem)
        {
            InspectorSubsystem->GetGroupItemsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), FlatGroupItems);
        }
        GroupListWidget->SetListItems(FlatGroupItems);
    }

    if (UListView* PinListWidget = Cast<UListView>(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Pin"))))
    {
        TArray<UObject*> PinnedItems;
        if (InspectorSubsystem)
        {
            InspectorSubsystem->GetPinnedItemsForSelected(InspectorSubsystem->GetCurrentActorSearchText(), PinnedItems);
        }
        PinListWidget->SetListItems(PinnedItems);
    }

    RefreshListLikeWidget(PanelWidget->WidgetTree->FindWidget(TEXT("LV_TreeGroup")));
    RefreshListLikeWidget(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Group")));
    RefreshListLikeWidget(PanelWidget->WidgetTree->FindWidget(TEXT("LV_Pin")));
#endif
}

static void RI_EnsureActorGroupTreeMode(UUserWidget* PanelWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    if (FBoolProperty* ShowInTreeProperty = FindFProperty<FBoolProperty>(PanelWidget->GetClass(), TEXT("ShowInTree")))
    {
        ShowInTreeProperty->SetPropertyValue_InContainer(PanelWidget, true);
    }

    if (UWidget* TreeWidget = PanelWidget->WidgetTree->FindWidget(TEXT("LV_TreeGroup")))
    {
        TreeWidget->SetVisibility(ESlateVisibility::Visible);
    }

    if (UWidget* GroupList = PanelWidget->WidgetTree->FindWidget(TEXT("LV_Group")))
    {
        GroupList->SetVisibility(ESlateVisibility::Visible);
    }

    if (UWidget* PinList = PanelWidget->WidgetTree->FindWidget(TEXT("LV_Pin")))
    {
        PinList->SetVisibility(ESlateVisibility::Visible);
    }

    if (UWidget* StarLabel = PanelWidget->WidgetTree->FindWidget(TEXT("Txt_Star")))
    {
        StarLabel->SetVisibility(ESlateVisibility::Visible);
    }
    if (UWidget* StarLabelUpper = PanelWidget->WidgetTree->FindWidget(TEXT("TXT_Star")))
    {
        StarLabelUpper->SetVisibility(ESlateVisibility::Visible);
    }
#endif
}

static bool RI_GroupItemCanExpandForActorPanel(const UInspectorGroupItem* Item)
{
    if (!Item)
    {
        return false;
    }

    if (Item->StableKey == TEXT("ROOT_COMPONENTS"))
    {
        return true;
    }

    if (Item->IsMaterialsRoot())
    {
        return true;
    }

    return Cast<UStaticMeshComponent>(Item->TargetObject) != nullptr && !Item->IsMaterialSlot();
}

static FString RI_FunctionParamTypeLabel(const FProperty* Prop)
{
    if (!Prop)
    {
        return TEXT("Unsupported");
    }

    if (Prop->IsA<FBoolProperty>()) return TEXT("bool");
    if (Prop->IsA<FIntProperty>()) return TEXT("int");
    if (Prop->IsA<FFloatProperty>()) return TEXT("float");
    if (Prop->IsA<FDoubleProperty>()) return TEXT("double");
    if (Prop->IsA<FStrProperty>()) return TEXT("string");
    if (Prop->IsA<FNameProperty>()) return TEXT("name");
    if (Prop->IsA<FEnumProperty>()) return TEXT("enum");
    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (ByteProp->Enum)
        {
            return TEXT("enum");
        }
    }
    return TEXT("Unsupported");
}

static bool RI_FunctionParamHasSupportedType(const FProperty* Prop)
{
    if (!Prop)
    {
        return false;
    }

    if (Prop->IsA<FBoolProperty>()) return true;
    if (Prop->IsA<FIntProperty>()) return true;
    if (Prop->IsA<FFloatProperty>()) return true;
    if (Prop->IsA<FDoubleProperty>()) return true;
    if (Prop->IsA<FStrProperty>()) return true;
    if (Prop->IsA<FNameProperty>()) return true;
    if (Prop->IsA<FEnumProperty>()) return true;
    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        return ByteProp->Enum != nullptr;
    }
    return false;
}

static FString RI_FunctionParamDefaultText(const FProperty* Prop)
{
    if (!Prop)
    {
        return FString();
    }

    if (Prop->IsA<FBoolProperty>())
    {
        return TEXT("False");
    }
    if (Prop->IsA<FIntProperty>())
    {
        return TEXT("0");
    }
    if (Prop->IsA<FFloatProperty>() || Prop->IsA<FDoubleProperty>())
    {
        return TEXT("0");
    }
    if (Prop->IsA<FStrProperty>())
    {
        return FString();
    }
    if (Prop->IsA<FNameProperty>())
    {
        return TEXT("None");
    }
    if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        if (const UEnum* Enum = EnumProp->GetEnum())
        {
            const int32 NumEnums = Enum->NumEnums();
            for (int32 Index = 0; Index < NumEnums; ++Index)
            {
#if WITH_EDITOR
                if (Enum->HasMetaData(TEXT("Hidden"), Index))
                {
                    continue;
                }
#endif
                const FString Candidate = Enum->GetNameStringByIndex(Index);
                if (!Candidate.EndsWith(TEXT("_MAX")))
                {
                    return Candidate;
                }
            }
        }
        return FString();
    }
    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (const UEnum* Enum = ByteProp->Enum)
        {
            const int32 NumEnums = Enum->NumEnums();
            for (int32 Index = 0; Index < NumEnums; ++Index)
            {
#if WITH_EDITOR
                if (Enum->HasMetaData(TEXT("Hidden"), Index))
                {
                    continue;
                }
#endif
                const FString Candidate = Enum->GetNameStringByIndex(Index);
                if (!Candidate.EndsWith(TEXT("_MAX")))
                {
                    return Candidate;
                }
            }
        }
    }
    return FString();
}

static bool RI_BuildFunctionParameterSpec(const FProperty* Prop, FRIFunctionParameterSpec& OutSpec)
{
    OutSpec = FRIFunctionParameterSpec();
    if (!Prop || !Prop->HasAnyPropertyFlags(CPF_Parm) || Prop->HasAnyPropertyFlags(CPF_ReturnParm) || Prop->HasAnyPropertyFlags(CPF_OutParm) || Prop->HasAnyPropertyFlags(CPF_ReferenceParm))
    {
        return false;
    }

    OutSpec.Name = Prop->GetFName();
    OutSpec.DisplayName = RI_GetPropertyDisplayNameRuntimeSafe(Prop);
    OutSpec.TypeLabel = RI_FunctionParamTypeLabel(Prop);
    OutSpec.bIsEnum = OutSpec.TypeLabel.Equals(TEXT("enum"), ESearchCase::IgnoreCase);
    OutSpec.bIsSupported = RI_FunctionParamHasSupportedType(Prop);
    OutSpec.DefaultText = RI_FunctionParamDefaultText(Prop);

    if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        if (const UEnum* Enum = EnumProp->GetEnum())
        {
            const int32 NumEnums = Enum->NumEnums();
            for (int32 Index = 0; Index < NumEnums; ++Index)
            {
#if WITH_EDITOR
                if (Enum->HasMetaData(TEXT("Hidden"), Index))
                {
                    continue;
                }
#endif
                const FString Candidate = Enum->GetNameStringByIndex(Index);
                if (!Candidate.EndsWith(TEXT("_MAX")))
                {
                    OutSpec.EnumOptions.Add(Candidate);
                }
            }
        }
    }
    else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (const UEnum* Enum = ByteProp->Enum)
        {
            const int32 NumEnums = Enum->NumEnums();
            for (int32 Index = 0; Index < NumEnums; ++Index)
            {
#if WITH_EDITOR
                if (Enum->HasMetaData(TEXT("Hidden"), Index))
                {
                    continue;
                }
#endif
                const FString Candidate = Enum->GetNameStringByIndex(Index);
                if (!Candidate.EndsWith(TEXT("_MAX")))
                {
                    OutSpec.EnumOptions.Add(Candidate);
                }
            }
        }
    }

    return OutSpec.bIsSupported;
}

static bool RI_BuildFunctionParameterDefinition(const FProperty* Prop, FRIInspectorFunctionParameterDefinition& OutDefinition)
{
    FRIFunctionParameterSpec Spec;
    if (!RI_BuildFunctionParameterSpec(Prop, Spec))
    {
        return false;
    }

    OutDefinition = FRIInspectorFunctionParameterDefinition();
    OutDefinition.Name = Spec.Name;
    OutDefinition.DisplayName = Spec.DisplayName;
    OutDefinition.TypeLabel = Spec.TypeLabel;
    OutDefinition.ValueType = Spec.bIsEnum ? EInspectorValueType::Enum : EInspectorValueType::Unsupported;
    OutDefinition.DefaultValueText = Spec.DefaultText;
    OutDefinition.bHasDefaultValue = !Spec.DefaultText.IsEmpty();
    OutDefinition.EnumOptions = Spec.EnumOptions;

    if (!Spec.bIsEnum)
    {
        const FString LowerType = Spec.TypeLabel.ToLower();
        if (LowerType == TEXT("bool"))
        {
            OutDefinition.ValueType = EInspectorValueType::Bool;
        }
        else if (LowerType == TEXT("int"))
        {
            OutDefinition.ValueType = EInspectorValueType::Int;
        }
        else if (LowerType == TEXT("float"))
        {
            OutDefinition.ValueType = EInspectorValueType::Float;
        }
        else if (LowerType == TEXT("double"))
        {
            OutDefinition.ValueType = EInspectorValueType::Double;
        }
        else if (LowerType == TEXT("string"))
        {
            OutDefinition.ValueType = EInspectorValueType::String;
        }
        else if (LowerType == TEXT("name"))
        {
            OutDefinition.ValueType = EInspectorValueType::Name;
        }
    }

    return true;
}

static FString RI_FormatFunctionSignature(const TArray<FRIInspectorFunctionParameterDefinition>& Params)
{
    if (Params.Num() == 0)
    {
        return TEXT("void");
    }

    TArray<FString> Parts;
    Parts.Reserve(Params.Num());
    for (const FRIInspectorFunctionParameterDefinition& Param : Params)
    {
        const FString ParamName = Param.DisplayName.IsEmpty() ? Param.Name.ToString() : Param.DisplayName;
        Parts.Add(FString::Printf(TEXT("%s:%s"), *ParamName, *Param.TypeLabel));
    }
    return FString::Join(Parts, TEXT(", "));
}

static FString RI_GetFunctionDisplayName(const UFunction* Function)
{
    if (!Function)
    {
        return TEXT("Function");
    }

    return RI_GetFunctionDisplayNameRuntimeSafe(Function);
}

static bool RI_IsCallableBlueprintFunctionCandidate(const UFunction* Function, TArray<FRIFunctionParameterSpec>& OutParams, FString& OutReason)
{
    OutParams.Reset();
    OutReason.Reset();

    if (!Function)
    {
        OutReason = TEXT("Function invalid");
        return false;
    }

    if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
    {
        OutReason = TEXT("Not BlueprintCallable");
        return false;
    }

    if (Function->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Static | FUNC_Delegate))
    {
        OutReason = TEXT("Unsupported function flag");
        return false;
    }

    if (RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("Latent"))
        || RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("WorldContext")))
    {
        OutReason = TEXT("Latent/world-context functions are excluded");
        return false;
    }

    for (TFieldIterator<FProperty> It(Function); It; ++It)
    {
        FProperty* Prop = *It;
        if (!Prop || !Prop->HasAnyPropertyFlags(CPF_Parm))
        {
            continue;
        }

        if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            OutReason = TEXT("Return values are not supported in v1");
            return false;
        }

        FRIFunctionParameterSpec Spec;
        if (!RI_BuildFunctionParameterSpec(Prop, Spec))
        {
            OutReason = FString::Printf(TEXT("Unsupported parameter type: %s"), *Prop->GetName());
            return false;
        }

        OutParams.Add(MoveTemp(Spec));
    }

    return true;
}

static bool RI_ImportTextToFunctionParam(FProperty* Prop, void* ValuePtr, const FString& InText, FString& OutError)
{
    OutError.Reset();
    if (!Prop || !ValuePtr)
    {
        OutError = TEXT("Invalid parameter destination");
        return false;
    }

    const FString Trimmed = InText.TrimStartAndEnd();
    const TCHAR* Buffer = *Trimmed;
    const TCHAR* Result = Prop->ImportText_Direct(Buffer, ValuePtr, nullptr, PPF_None);
    if (!Result)
    {
        OutError = FString::Printf(TEXT("Failed to parse %s"), *Prop->GetName());
        return false;
    }

    return true;
}

// =======================
// Property Whitelists (MVP)
// - Only used when SearchText is empty.
// - Search mode: show matched supported props even if not in whitelist.
// =======================


DEFINE_LOG_CATEGORY(LogRuntimeInspector);

static TAutoConsoleVariable<int32> CVarRIEnable(
    TEXT("ri.Enable"),
#if UE_BUILD_SHIPPING
    0,
#else
    // Default to enabled in non-shipping builds for convenience.
    // For packaged builds you can still disable in ini: [ConsoleVariables] ri.Enable=0
    1,
#endif
    TEXT("Enable RuntimeInspector. 0=disabled, 1=enabled"),
    ECVF_Default
);

static TAutoConsoleVariable<int32> CVarRIDebugLog(
    TEXT("ri.DebugLog"),
    0,
    TEXT("RuntimeInspector debug logs. 0=off, 1=on"),
    ECVF_Default
);

static FORCEINLINE bool RI_IsDebugLogEnabled()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    return CVarRIDebugLog.GetValueOnGameThread() != 0;
#endif
}
bool UInspectorWorldSubsystem::IsRIEnabled() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    return CVarRIEnable.GetValueOnGameThread() != 0;
#endif
}

FString UInspectorWorldSubsystem::GetRIDisabledReason() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
#else
    return TEXT("Disabled (ri.Enable=0)");
#endif
}

bool UInspectorWorldSubsystem::IsUnlockRequired() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    if (!IsRIEnabled()) return false;
    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    return Settings && Settings->bRequireUnlock;
#endif
}

bool UInspectorWorldSubsystem::IsRIUnlocked() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    return !IsUnlockRequired() || bUnlocked;
#endif
}

FString UInspectorWorldSubsystem::GetRIUnlockHint() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
#else
    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    const FString CodeHint = (Settings && !Settings->UnlockCode.IsEmpty())
        ? TEXT("ri.Unlock <code>")
        : TEXT("ri.Unlock");
    return FString::Printf(TEXT("RuntimeInspector is locked. Run console command: %s"), *CodeHint);
#endif
}

bool UInspectorWorldSubsystem::UnlockRI(const FString& Code, FString& OutError)
{
    OutError.Reset();
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!IsRIEnabled())
    {
        OutError = GetRIDisabledReason();
        return false;
    }

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    if (!Settings || !Settings->bRequireUnlock)
    {
        bUnlocked = true;
        return true;
    }

    const FString Want = Settings->UnlockCode;
    if (!Want.IsEmpty() && Code != Want)
    {
        OutError = TEXT("Unlock code mismatch");
        return false;
    }

    bUnlocked = true;
    PushToast(ERIToastType::Success, TEXT("Unlocked"), 1.2f);
    return true;
#endif
}

void UInspectorWorldSubsystem::LockRI()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bOpen)
    {
        Close();
    }
    bUnlocked = false;
    PushToast(ERIToastType::Info, TEXT("Locked"), 1.0f);
#endif
}

#if RUNTIME_INSPECTOR_ENABLED
static UInspectorWorldSubsystem* RI_GetWorldSubsystem(UWorld* World)
{
    return World ? World->GetSubsystem<UInspectorWorldSubsystem>() : nullptr;
}

static void RI_CmdUnlock(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;

    FString Err;
    const FString Code = (Args.Num() > 0) ? FString::Join(Args, TEXT(" ")) : FString();
    if (!S->UnlockRI(Code, Err))
    {
        if (Err.IsEmpty())
        {
            Err = S->GetRIUnlockHint();
        }
        S->PushToast(ERIToastType::Error, Err, 2.5f);
    }
}

static void RI_CmdLock(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;
    S->LockRI();
}

static void RI_CmdCopyLastImportReport(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;
    FString Copied;
    if (!S->CopyLastImportReportToClipboard(Copied))
    {
        S->PushToast(ERIToastType::Warning, TEXT("No import report"), 2.0f);
    }
}

static void RI_CmdExportLastImportReport(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;

    bool bAsJson = true;
    if (Args.Num() > 0)
    {
        const FString Mode = Args[0].ToLower();
        if (Mode == TEXT("txt") || Mode == TEXT("text"))
        {
            bAsJson = false;
        }
    }

    FString Path, Err;
    if (!S->ExportLastImportReportToFile(bAsJson, Path, Err))
    {
        S->PushToast(ERIToastType::Error, Err.IsEmpty() ? TEXT("Export failed") : Err, 3.0f);
    }
}

static void RI_CmdApplyFabScreenshotState(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;

    const FString ShotName = Args.Num() > 0 ? Args[0] : FString();
    FString Summary;
    FString Error;
    if (!S->ApplyFabScreenshotStateByName(ShotName, Summary, Error))
    {
        const FString FailureText = Error.IsEmpty()
            ? FString::Printf(TEXT("Fab screenshot state failed: %s"), ShotName.IsEmpty() ? TEXT("foundation") : *ShotName)
            : Error;
        S->PushToast(ERIToastType::Error, FailureText, 3.0f);
        return;
    }

    const FString SuccessText = Summary.IsEmpty()
        ? FString::Printf(TEXT("Fab screenshot state applied: %s"), ShotName.IsEmpty() ? TEXT("foundation") : *ShotName)
        : Summary;
    S->PushToast(ERIToastType::Success, SuccessText, 2.0f);
}

static void RI_CmdShowInspectorPage(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;

    const FString PageName = Args.Num() > 0 ? Args[0] : TEXT("Changes");
    FString Error;
    if (!S->SetVisiblePageByName(PageName, Error))
    {
        S->PushToast(ERIToastType::Error, Error.IsEmpty() ? FString::Printf(TEXT("Failed to show page: %s"), *PageName) : Error, 3.0f);
    }
}

static void RI_CmdOpen(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S)
    {
        return;
    }

    S->Open();

    if (Args.Num() > 0)
    {
        FString Error;
        if (!S->SetVisiblePageByName(FString::Join(Args, TEXT(" ")), Error))
        {
            S->PushToast(ERIToastType::Error, Error.IsEmpty() ? TEXT("Failed to set RuntimeInspector page") : Error, 3.0f);
        }
    }
}

static void RI_CmdClose(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S)
    {
        return;
    }

    S->Close();
}

static void RI_CmdFocusComponent(const TArray<FString>& Args, UWorld* World)
{
    UInspectorWorldSubsystem* S = RI_GetWorldSubsystem(World);
    if (!S) return;

    const FString ComponentName = Args.Num() > 0 ? FString::Join(Args, TEXT(" ")) : FString();
    FString Error;
    if (!S->FocusSelectedActorComponentByName(ComponentName, Error))
    {
        S->PushToast(ERIToastType::Error, Error.IsEmpty() ? TEXT("Failed to focus component") : Error, 3.0f);
    }
}

static FAutoConsoleCommandWithWorldAndArgs CCmdRIUnlock(
    TEXT("ri.Unlock"),
    TEXT("Unlock RuntimeInspector. Usage: ri.Unlock [code]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdUnlock)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRILock(
    TEXT("ri.Lock"),
    TEXT("Lock RuntimeInspector (also closes panel)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdLock)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRICopyImportReport(
    TEXT("ri.CopyLastImportReport"),
    TEXT("Copy last snapshot import report to clipboard."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdCopyLastImportReport)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIExportImportReport(
    TEXT("ri.ExportLastImportReport"),
    TEXT("Export last snapshot import report. Usage: ri.ExportLastImportReport [json|txt]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdExportLastImportReport)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIApplyFabScreenshotState(
    TEXT("ri.ApplyFabScreenshotState"),
    TEXT("Apply the RuntimeInspector Fab screenshot presentation state. Usage: ri.ApplyFabScreenshotState [foundation|remote_session|promote_or_audit]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdApplyFabScreenshotState)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIShowInspectorPage(
    TEXT("ri.ShowInspectorPage"),
    TEXT("Show a RuntimeInspector page. Usage: ri.ShowInspectorPage Inspect|Snapshot|Diagnostics"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdShowInspectorPage)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIOpen(
    TEXT("ri.Open"),
    TEXT("Open RuntimeInspector. Usage: ri.Open [Inspect|Snapshot|Diagnostics]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdOpen)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIClose(
    TEXT("ri.Close"),
    TEXT("Close RuntimeInspector."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdClose)
);

static FAutoConsoleCommandWithWorldAndArgs CCmdRIFocusComponent(
    TEXT("ri.FocusComponent"),
    TEXT("Focus a component on the Actor page. Usage: ri.FocusComponent <component name>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RI_CmdFocusComponent)
);
#endif
static const TSet<FName>& GetStaticMeshComponentWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // USceneComponent
        Set.Add(TEXT("Mobility"));                 // Enum
        Set.Add(TEXT("bVisible"));                 // Bool (有些版本可能是 Visible 相关)
        Set.Add(TEXT("bHiddenInGame"));            // Bool

        // UPrimitiveComponent
        Set.Add(TEXT("CastShadow"));               // Bool
        Set.Add(TEXT("bRenderCustomDepth"));       // Bool
        Set.Add(TEXT("CustomDepthStencilValue"));  // Int
        Set.Add(TEXT("BoundsScale"));              // Float
        Set.Add(TEXT("TranslucencySortPriority")); // Int

        // UStaticMeshComponent
        Set.Add(TEXT("ForcedLodModel"));           // Int

        bInit = true;
    }
    return Set;
}

static const TSet<FName>& GetLightComponentWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // UActorComponent / USceneComponent
        Set.Add(TEXT("Mobility"));                 // Enum
        Set.Add(TEXT("bVisible"));                 // Bool
        Set.Add(TEXT("bHiddenInGame"));            // Bool

        // ULightComponent
        Set.Add(TEXT("Intensity"));                // Float
        Set.Add(TEXT("IndirectLightingIntensity"));// Float
        Set.Add(TEXT("bAffectsWorld"));            // Bool
        Set.Add(TEXT("CastShadows"));              // Bool (有些版本叫 CastShadows)
        Set.Add(TEXT("bUseInverseSquaredFalloff"));// Bool (点光/聚光常见)

        // UPointLightComponent / USpotLightComponent 等
        Set.Add(TEXT("AttenuationRadius"));        // Float

        bInit = true;
    }
    return Set;
}

static const TSet<FName>& GetCharacterMovementWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // UCharacterMovementComponent
        Set.Add(TEXT("MaxWalkSpeed"));                 // Float
        Set.Add(TEXT("MaxAcceleration"));              // Float
        Set.Add(TEXT("BrakingDecelerationWalking"));   // Float
        Set.Add(TEXT("JumpZVelocity"));                // Float
        Set.Add(TEXT("GravityScale"));                 // Float
        Set.Add(TEXT("AirControl"));                   // Float

        bInit = true;
    }
    return Set;
}

static const TSet<FName>* GetWhitelistForWhitelistedComponent(const UActorComponent* Comp)
{
    if (!Comp) return nullptr;

    if (Comp->IsA<UStaticMeshComponent>())          return &GetStaticMeshComponentWhitelist();
    if (Comp->IsA<ULightComponent>())               return &GetLightComponentWhitelist();
    if (Comp->IsA<UCharacterMovementComponent>())   return &GetCharacterMovementWhitelist();

    return nullptr;
}

void UInspectorWorldSubsystem::PushToast(ERIToastType Type, const FString& Message, float Duration)
{
#if RUNTIME_INSPECTOR_ENABLED
    // 也可以在这里统一写 UE_LOG，方便排查
	UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("RI Toast: [%d] %s"), (int32)Type, *Message);
    AppendActivityLog(Type, TEXT("RuntimeInspector"), Message);
    OnToast.Broadcast(Type, Message, Duration);
#endif
}

void UInspectorWorldSubsystem::ClearActivityLog()
{
#if RUNTIME_INSPECTOR_ENABLED
    ActivityLogEntries.Reset();
    if (UInspectorTestPageWidget* Page = TestPageWidget.Get())
    {
        Page->RefreshFromSubsystem();
    }
#endif
}

int32 UInspectorWorldSubsystem::GetActivityLogEntryCountForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return ActivityLogEntries.Num();
#else
    return 0;
#endif
}

int32 UInspectorWorldSubsystem::GetActorGroupsEntryCountForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return ActorGroupsEntriesBoxStrong ? ActorGroupsEntriesBoxStrong->GetChildrenCount() : INDEX_NONE;
#else
    return INDEX_NONE;
#endif
}

int32 UInspectorWorldSubsystem::GetActorPinnedEntryCountForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return ActorPinnedEntriesBoxStrong ? ActorPinnedEntriesBoxStrong->GetChildrenCount() : INDEX_NONE;
#else
    return INDEX_NONE;
#endif
}

int32 UInspectorWorldSubsystem::GetActorPropertyRowCountForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (const UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get())
    {
        return SectionWidget->GetEntryWidgetCountForAutomation();
    }
    return INDEX_NONE;
#else
    return INDEX_NONE;
#endif
}

int32 UInspectorWorldSubsystem::GetActorFunctionRowCountForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (const UInspectorFunctionsSectionWidget* SectionWidget = ActorFunctionsSectionWidget.Get())
    {
        return SectionWidget->GetEntryWidgetCountForAutomation();
    }
    return INDEX_NONE;
#else
    return INDEX_NONE;
#endif
}

FString UInspectorWorldSubsystem::GetActorPropertyHostDebugSummaryForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    auto DescribeWidget = [](const UWidget* Widget) -> FString
    {
        if (!Widget)
        {
            return TEXT("None");
        }

        const FVector2D LocalSize = Widget->GetCachedGeometry().GetLocalSize();
        return FString::Printf(
            TEXT("%s[parent=%s,size=%.1fx%.1f,vis=%d]"),
            *Widget->GetName(),
            *GetNameSafe(Widget->GetParent()),
            LocalSize.X,
            LocalSize.Y,
            static_cast<int32>(Widget->GetVisibility()));
    };

    return FString::Printf(
        TEXT("PageStack=%s | SelectionBand=%s | Body=%s | Left=%s | RightHost=%s | Property=%s | Function=%s"),
        *DescribeWidget(ActorWorkbenchPageStackHost.Get()),
        *DescribeWidget(ActorWorkspaceSelectionBand.Get()),
        *DescribeWidget(ActorWorkbenchBodyHost.Get()),
        *DescribeWidget(ActorGroupsSectionHostBox.Get()),
        *DescribeWidget(ActorPropertyFunctionHostBox.Get()),
        *DescribeWidget(ActorPropertiesSectionWidget.Get()),
        *DescribeWidget(ActorFunctionsSectionWidget.Get()));
#else
    return TEXT("RuntimeInspector disabled");
#endif
}

FString UInspectorWorldSubsystem::GetActorPropertyAnchorChainForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    const UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return TEXT("Panel=None");
    }

    auto DescribeWidget = [](const UWidget* Widget) -> FString
    {
        if (!Widget)
        {
            return TEXT("None");
        }

        const FVector2D LocalSize = Widget->GetCachedGeometry().GetLocalSize();
        return FString::Printf(
            TEXT("%s(%s,%.1fx%.1f,vis=%d)"),
            *Widget->GetName(),
            *Widget->GetClass()->GetName(),
            LocalSize.X,
            LocalSize.Y,
            static_cast<int32>(Widget->GetVisibility()));
    };

    TArray<FString> Parts;
    const UWidget* Current = Panel->WidgetTree->FindWidget(TEXT("LV_Properties"));
    while (Current)
    {
        Parts.Add(DescribeWidget(Current));
        Current = Current->GetParent();
    }

    return Parts.Num() > 0 ? FString::Join(Parts, TEXT(" -> ")) : TEXT("LV_Properties=None");
#else
    return TEXT("RuntimeInspector disabled");
#endif
}

FString UInspectorWorldSubsystem::GetInspectBodyChildrenDebugSummaryForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    const UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return TEXT("Panel=None");
    }

    const UPanelWidget* ParentPanel = nullptr;
    if (const UWidget* LegacyBody = Panel->WidgetTree->FindWidget(TEXT("Body")))
    {
        ParentPanel = Cast<UPanelWidget>(LegacyBody->GetParent());
    }

    if (!ParentPanel)
    {
        return TEXT("Parent=None");
    }

    auto DescribeSlot = [](const UWidget* Child) -> FString
    {
        if (const UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Child ? Child->Slot : nullptr))
        {
            return FString::Printf(
                TEXT("slot=V(rule=%d,val=%.2f)"),
                static_cast<int32>(VBoxSlot->GetSize().SizeRule),
                VBoxSlot->GetSize().Value);
        }

        if (const UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Child ? Child->Slot : nullptr))
        {
            return FString::Printf(
                TEXT("slot=H(rule=%d,val=%.2f)"),
                static_cast<int32>(HBoxSlot->GetSize().SizeRule),
                HBoxSlot->GetSize().Value);
        }

        return TEXT("slot=?");
    };

    auto DescribeWidget = [&DescribeSlot](const UWidget* Child) -> FString
    {
        if (!Child)
        {
            return TEXT("None");
        }

        const FVector2D LocalSize = Child->GetCachedGeometry().GetLocalSize();
        return FString::Printf(
            TEXT("%s[%s,%s,%.1fx%.1f,vis=%d]"),
            *Child->GetName(),
            *Child->GetClass()->GetName(),
            *DescribeSlot(Child),
            LocalSize.X,
            LocalSize.Y,
            static_cast<int32>(Child->GetVisibility()));
    };

    TArray<FString> Parts;
    for (int32 Index = 0; Index < ParentPanel->GetChildrenCount(); ++Index)
    {
        const UWidget* Child = ParentPanel->GetChildAt(Index);
        FString Entry = FString::Printf(TEXT("%d:%s"), Index, *DescribeWidget(Child));

        if (const UPanelWidget* ChildPanel = Cast<UPanelWidget>(Child))
        {
            TArray<FString> GrandChildren;
            for (int32 ChildIndex = 0; ChildIndex < ChildPanel->GetChildrenCount(); ++ChildIndex)
            {
                GrandChildren.Add(DescribeWidget(ChildPanel->GetChildAt(ChildIndex)));
            }
            if (GrandChildren.Num() > 0)
            {
                Entry += FString::Printf(TEXT("{%s}"), *FString::Join(GrandChildren, TEXT(" || ")));
            }
        }
        else if (const UContentWidget* ContentWidget = Cast<UContentWidget>(Child))
        {
            if (const UWidget* Content = ContentWidget->GetContent())
            {
                Entry += FString::Printf(TEXT("{content=%s"), *DescribeWidget(Content));
                if (const UPanelWidget* ContentPanel = Cast<UPanelWidget>(Content))
                {
                    TArray<FString> GreatGrandChildren;
                    for (int32 ContentChildIndex = 0; ContentChildIndex < ContentPanel->GetChildrenCount(); ++ContentChildIndex)
                    {
                        GreatGrandChildren.Add(DescribeWidget(ContentPanel->GetChildAt(ContentChildIndex)));
                    }
                    if (GreatGrandChildren.Num() > 0)
                    {
                        Entry += FString::Printf(TEXT("{%s}"), *FString::Join(GreatGrandChildren, TEXT(" || ")));
                    }
                }
                Entry += TEXT("}");
            }
        }

        Parts.Add(Entry);
    }

    return FString::Join(Parts, TEXT(" | "));
#else
    return TEXT("RuntimeInspector disabled");
#endif
}

FString UInspectorWorldSubsystem::GetPanelPresentationDebugSummaryForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    auto DescribeWidget = [](const UWidget* Widget) -> FString
    {
        if (!Widget)
        {
            return TEXT("None");
        }

        const FVector2D LocalSize = Widget->GetCachedGeometry().GetLocalSize();
        return FString::Printf(
            TEXT("%s[parent=%s,size=%.1fx%.1f,vis=%d]"),
            *Widget->GetName(),
            *GetNameSafe(Widget->GetParent()),
            LocalSize.X,
            LocalSize.Y,
            static_cast<int32>(Widget->GetVisibility()));
    };

    const UUserWidget* Panel = PanelWidget.Get();
    const UWidget* RootContent = PanelRootContentWidget.Get();
    const UCanvasPanelSlot* RootCanvasSlot = PanelRootCanvasSlot.Get();
    const USizeBox* SizeBox = PanelSizeBox.Get();
    const FVector2D PanelSize = Panel ? Panel->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
    const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(GetWorld());
    const FVector2D DefaultCanvasPosition = RI_GetDefaultPanelCanvasPosition(GetWorld(), FVector2D(PanelDefaultWidth, PanelDefaultHeight));
    const FVector2D RootSlotSize = RootCanvasSlot ? RootCanvasSlot->GetSize() : FVector2D::ZeroVector;
    const FVector2D RootSlotPosition = RootCanvasSlot ? RootCanvasSlot->GetPosition() : FVector2D::ZeroVector;
    const FVector2D RootSlotAlignment = RootCanvasSlot ? RootCanvasSlot->GetAlignment() : FVector2D::ZeroVector;
    const FAnchors RootAnchors = RootCanvasSlot ? RootCanvasSlot->GetAnchors() : FAnchors();
    const FMargin RootOffsets = RootCanvasSlot ? RootCanvasSlot->GetOffsets() : FMargin();
    const float WidthOverride = SizeBox ? SizeBox->GetWidthOverride() : 0.0f;
    const float HeightOverride = SizeBox ? SizeBox->GetHeightOverride() : 0.0f;

    return FString::Printf(
        TEXT("Panel=%s logical=%.1fx%.1f viewportLogical=%.1fx%.1f defaultCanvasPos=(%.1f,%.1f) translation=(%.1f,%.1f) default=%.1fx%.1f current=%.1fx%.1f Root=%s CanvasSlotSize=%.1fx%.1f CanvasPos=(%.1f,%.1f) CanvasAlign=(%.2f,%.2f) Anchors=(%.2f,%.2f,%.2f,%.2f) Offsets=(%.1f,%.1f,%.1f,%.1f) SizeBox=%s widthOverride=%.1f heightOverride=%.1f"),
        *DescribeWidget(Panel),
        PanelSize.X,
        PanelSize.Y,
        LogicalViewportSize.X,
        LogicalViewportSize.Y,
        DefaultCanvasPosition.X,
        DefaultCanvasPosition.Y,
        PanelTranslation.X,
        PanelTranslation.Y,
        PanelDefaultWidth,
        PanelDefaultHeight,
        PanelWidth,
        PanelHeight,
        *DescribeWidget(RootContent),
        RootSlotSize.X,
        RootSlotSize.Y,
        RootSlotPosition.X,
        RootSlotPosition.Y,
        RootSlotAlignment.X,
        RootSlotAlignment.Y,
        RootAnchors.Minimum.X,
        RootAnchors.Minimum.Y,
        RootAnchors.Maximum.X,
        RootAnchors.Maximum.Y,
        RootOffsets.Left,
        RootOffsets.Top,
        RootOffsets.Right,
        RootOffsets.Bottom,
        *DescribeWidget(SizeBox),
        WidthOverride,
        HeightOverride);
#else
    return TEXT("RuntimeInspector disabled");
#endif
}

FString UInspectorWorldSubsystem::GetPanelHostWindowDebugSummaryForAutomation() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    const UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !FSlateApplication::IsInitialized())
    {
        return TEXT("Panel=None");
    }

    TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget();
    if (!CachedWidget.IsValid())
    {
        return TEXT("PanelWidget=NoCachedWidget");
    }

    TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(CachedWidget.ToSharedRef());
    if (!Window.IsValid())
    {
        return TEXT("Window=None");
    }

    const FSlateRect WindowRect = Window->GetRectInScreen();
    return FString::Printf(
        TEXT("Title=%s | Size=%.1fx%.1f | Pos=(%.1f,%.1f)"),
        *Window->GetTitle().ToString(),
        WindowRect.GetSize().X,
        WindowRect.GetSize().Y,
        WindowRect.Left,
        WindowRect.Top);
#endif
}

FString UInspectorWorldSubsystem::GetLastActivityLogSummaryForAutomation() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (ActivityLogEntries.Num() <= 0)
    {
        return FString();
    }

    const FRIActivityLogEntry& Entry = ActivityLogEntries.Last();
    return FString::Printf(TEXT("%s|%s"), *Entry.Category, *Entry.Message);
#else
    return FString();
#endif
}

void UInspectorWorldSubsystem::AppendActivityLog(ERIToastType Severity, const FString& Category, const FString& Message)
{
#if RUNTIME_INSPECTOR_ENABLED
    const FString TrimmedMessage = Message.TrimStartAndEnd();
    if (TrimmedMessage.IsEmpty())
    {
        return;
    }

    const FString TrimmedCategory = Category.TrimStartAndEnd().IsEmpty() ? TEXT("RuntimeInspector") : Category.TrimStartAndEnd();
    if (ActivityLogEntries.Num() > 0)
    {
        const FRIActivityLogEntry& LastEntry = ActivityLogEntries.Last();
        if (LastEntry.Severity == Severity
            && LastEntry.Category.Equals(TrimmedCategory, ESearchCase::CaseSensitive)
            && LastEntry.Message.Equals(TrimmedMessage, ESearchCase::CaseSensitive))
        {
            return;
        }
    }

    FRIActivityLogEntry Entry;
    Entry.TimestampUtc = FDateTime::UtcNow();
    Entry.Severity = Severity;
    Entry.Category = TrimmedCategory;
    Entry.Message = TrimmedMessage;
    ActivityLogEntries.Add(MoveTemp(Entry));

    constexpr int32 MaxActivityLogEntries = 80;
    if (ActivityLogEntries.Num() > MaxActivityLogEntries)
    {
        ActivityLogEntries.RemoveAt(0, ActivityLogEntries.Num() - MaxActivityLogEntries, EAllowShrinking::No);
    }
#endif
}

static FString RI_ExtractTailAfterLastDot(const FString& PathLike)
{
    int32 Dot = INDEX_NONE;
    if (PathLike.FindLastChar(TEXT('.'), Dot))
    {
        return PathLike.Mid(Dot + 1);
    }
    return PathLike;
}

// 从 Actor 实例名里提取 baseName：
// BP_TestVarsActor_C_UAID_xxx -> BP_TestVarsActor
// BP_TestVarsActor_C_0        -> BP_TestVarsActor
static FString RI_ExtractActorBaseName(const FString& ActorInstanceName)
{
    FString N = ActorInstanceName;

    // 常见：..._C_UAID_... 或 ..._C_0
    int32 CPos = N.Find(TEXT("_C"));
    if (CPos != INDEX_NONE)
    {
        return N.Left(CPos);
    }

    // 兜底：取第一个 '_' 前
    int32 Under = INDEX_NONE;
    if (N.FindChar(TEXT('_'), Under))
    {
        return N.Left(Under);
    }

    return N;
}

// 从 snapshot 里的 class 字符串提取 “类短名”：
// "/RuntimeInspector/Test/BP_TestVarsActor.BP_TestVarsActor_C" -> "BP_TestVarsActor_C"
static FString RI_ExtractShortClassName(const FString& ClassPath)
{
    FString Tail = RI_ExtractTailAfterLastDot(ClassPath);

    // 有些路径可能是 /Script/.. 这种，取最后一个 '/' 后
    int32 Slash = INDEX_NONE;
    if (Tail.FindLastChar(TEXT('/'), Slash))
    {
        Tail = Tail.Mid(Slash + 1);
    }
    return Tail;
}

static bool RI_UClassMatches(const UClass* RuntimeClass, const FString& SnapshotClassPathOrName)
{
    if (!RuntimeClass || SnapshotClassPathOrName.IsEmpty()) return false;

    const FString RuntimeClassPath = RuntimeClass->GetPathName();
    if (RuntimeClassPath == SnapshotClassPathOrName)
    {
        return true;
    }

    const FString WantShort = RI_ExtractShortClassName(SnapshotClassPathOrName);
    if (RuntimeClass->GetName() == WantShort)
    {
        return true;
    }

    return RuntimeClassPath.Contains(WantShort);
}

static bool RI_ClassMatches(AActor* Actor, const FString& SnapshotClassPathOrName)
{
    if (!Actor || SnapshotClassPathOrName.IsEmpty()) return false;
    return RI_UClassMatches(Actor->GetClass(), SnapshotClassPathOrName);
}

static FString RI_PatchTargetKindToString(ERIPatchTargetKind Kind)
{
    switch (Kind)
    {
    case ERIPatchTargetKind::Actor: return TEXT("actor");
    case ERIPatchTargetKind::Component: return TEXT("component");
    case ERIPatchTargetKind::MaterialSlot: return TEXT("materialSlot");
    default: return TEXT("actor");
    }
}

static bool RI_ParsePatchTargetKind(const FString& InValue, ERIPatchTargetKind& OutKind)
{
    if (InValue == TEXT("actor")) { OutKind = ERIPatchTargetKind::Actor; return true; }
    if (InValue == TEXT("component")) { OutKind = ERIPatchTargetKind::Component; return true; }
    if (InValue == TEXT("materialSlot")) { OutKind = ERIPatchTargetKind::MaterialSlot; return true; }
    return false;
}

static FString RI_PatchFieldKindToString(ERIPatchFieldKind Kind)
{
    switch (Kind)
    {
    case ERIPatchFieldKind::Property: return TEXT("property");
    case ERIPatchFieldKind::MaterialScalar: return TEXT("materialScalar");
    case ERIPatchFieldKind::MaterialVector: return TEXT("materialVector");
    default: return TEXT("property");
    }
}

static bool RI_ParsePatchFieldKind(const FString& InValue, ERIPatchFieldKind& OutKind)
{
    if (InValue == TEXT("property")) { OutKind = ERIPatchFieldKind::Property; return true; }
    if (InValue == TEXT("materialScalar")) { OutKind = ERIPatchFieldKind::MaterialScalar; return true; }
    if (InValue == TEXT("materialVector")) { OutKind = ERIPatchFieldKind::MaterialVector; return true; }
    return false;
}

static FString RI_PatchValueKindToString(ERIPatchValueKind Kind)
{
    switch (Kind)
    {
    case ERIPatchValueKind::ImportText:
    default:
        return TEXT("importText");
    }
}

static bool RI_ParsePatchValueKind(const FString& InValue, ERIPatchValueKind& OutKind)
{
    if (InValue == TEXT("importText"))
    {
        OutKind = ERIPatchValueKind::ImportText;
        return true;
    }
    return false;
}

static FString RI_PatchPresetScopeToString(ERIPatchPresetApplicabilityScope Scope)
{
    switch (Scope)
    {
    case ERIPatchPresetApplicabilityScope::CurrentSelection: return TEXT("currentSelection");
    case ERIPatchPresetApplicabilityScope::ActorClass: return TEXT("actorClass");
    case ERIPatchPresetApplicabilityScope::ComponentClass: return TEXT("componentClass");
    default: return TEXT("currentSelection");
    }
}

static bool RI_ParsePatchPresetScope(const FString& InValue, ERIPatchPresetApplicabilityScope& OutScope)
{
    if (InValue == TEXT("currentSelection")) { OutScope = ERIPatchPresetApplicabilityScope::CurrentSelection; return true; }
    if (InValue == TEXT("actorClass")) { OutScope = ERIPatchPresetApplicabilityScope::ActorClass; return true; }
    if (InValue == TEXT("componentClass")) { OutScope = ERIPatchPresetApplicabilityScope::ComponentClass; return true; }
    return false;
}

static bool RI_ParsePropertySnapshotKey(const FString& Key, FString& OutActorPath, FString& OutCompPath, FString& OutClassPath, FString& OutPropName)
{
    TArray<FString> Parts;
    Key.ParseIntoArray(Parts, TEXT("|"), false);
    if (Parts.Num() < 5 || Parts[0] != TEXT("P"))
    {
        return false;
    }

    OutActorPath = Parts[1];
    OutCompPath = Parts[2];
    OutClassPath = Parts[3];
    OutPropName = Parts[4];
    return true;
}

static bool RI_ParseMaterialSnapshotKey(const FString& Key, FString& OutActorPath, FString& OutCompPath, int32& OutSlotIndex, EInspectorMatParamType& OutType, FString& OutParamName)
{
    TArray<FString> Parts;
    Key.ParseIntoArray(Parts, TEXT("|"), false);
    if (Parts.Num() < 6 || Parts[0] != TEXT("M"))
    {
        return false;
    }

    OutActorPath = Parts[1];
    OutCompPath = Parts[2];
    OutSlotIndex = FCString::Atoi(*Parts[3]);
    OutType = static_cast<EInspectorMatParamType>(FCString::Atoi(*Parts[4]));
    OutParamName = Parts[5];
    return true;
}

static FString RI_GetMeshMaterialSlotName(UMeshComponent* MeshComp, int32 SlotIndex)
{
    if (!MeshComp || SlotIndex == INDEX_NONE)
    {
        return FString();
    }

    const TArray<FName> SlotNames = MeshComp->GetMaterialSlotNames();
    if (SlotNames.IsValidIndex(SlotIndex))
    {
        return SlotNames[SlotIndex].ToString();
    }

    return FString();
}

#if RUNTIME_INSPECTOR_ENABLED
static const TCHAR* DefaultPanelPath = TEXT("/RuntimeInspector/UI/WBP_InspectorPanel.WBP_InspectorPanel_C");
#endif

void UInspectorWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

#if RUNTIME_INSPECTOR_ENABLED
    const double StartSeconds = FPlatformTime::Seconds();
    PanelWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(DefaultPanelPath));
    ConfirmDialogWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(RI_ConfirmDialogClassPath));
    PanelWidgetClass.LoadSynchronous();
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] Initialize PanelClassPrewarm %.2f ms | Loaded=%d"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0,
        PanelWidgetClass.Get() ? 1 : 0);
#endif

#if RUNTIME_INSPECTOR_ENABLED
    LoadFavorites();
#endif

    bUnlocked = false;
    LastSavedSettingsSnapshot = GetEditableSettings();
    LastSavedThemePresetSnapshot = GetThemePreset();
    LastAppliedThemePresetFingerprint = static_cast<int32>(RICompactUI::GetActiveThemePreset());
    bSettingsDirty = false;

}

void UInspectorWorldSubsystem::Deinitialize()
{
#if RUNTIME_INSPECTOR_ENABLED
   
    RestoreFabScreenshotPanelTransform();
    RestoreFabScreenshotApplicationScale();
    Close();
    ClearConfirmDialogBinding();
    ReleaseInspectorInputComponent();

    bUnlocked = false;

#endif
   

    Super::Deinitialize();
}

void UInspectorWorldSubsystem::Tick(float DeltaTime)
{
#if RUNTIME_INSPECTOR_ENABLED
    const int32 ActiveThemeFingerprint = static_cast<int32>(RICompactUI::GetActiveThemePreset());
    if (ActiveThemeFingerprint != LastAppliedThemePresetFingerprint)
    {
        LastAppliedThemePresetFingerprint = ActiveThemeFingerprint;
        ScheduleThemePreviewRefresh(GetVisiblePage());
    }

    if (!bInputsBound)
    {
        TryBindInputs();
    }

    if (!bOpen)
    {
        MaybePrecreatePanelWidget();
        MaybePrecreateSecondaryPageWidgets();
    }
    else
    {
        EnsurePanelInteractionInitialized();
    }

    // Closed state should be near-zero overhead.
    if (!bOpen)
    {
        ClearConfirmDialogBinding();
        return;
    }

    ConfirmDialogBindAccum += DeltaTime;
    if (ConfirmDialogBindAccum >= 0.1f)
    {
        ConfirmDialogBindAccum = 0.f;
        RefreshConfirmDialogBinding();
    }

    ConfirmDialogPreviewAccum += DeltaTime;
    if (ConfirmDialogPreviewAccum >= 0.03f)
    {
        ConfirmDialogPreviewAccum = 0.f;
        SyncActiveConfirmDialogColorPreview();
    }

    // Apply debounced property writes.
    FlushPendingPropertyApplies();

    // 0.2s 检查一次够用
    ValidateAccum += DeltaTime;
    if (ValidateAccum >= 0.2f)
    {
        ValidateAccum = 0.f;
        ValidateSelection();
    }
#endif
}

void UInspectorWorldSubsystem::MaybePrecreatePanelWidget()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bPanelWidgetPrecreated || PanelWidget.IsValid() || !IsRIEnabled())
    {
        return;
    }

    if (!GetWorld() || !GetLocalPC())
    {
        return;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    EnsurePanelWidget();
    bPanelWidgetPrecreated = PanelWidget.IsValid();
    if (bPanelWidgetPrecreated)
    {
        UE_LOG(
            LogRuntimeInspector,
            Log,
            TEXT("[RI][Perf] PanelWidgetPrecreate %.2f ms"),
            (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
    }
#endif
}

void UInspectorWorldSubsystem::MaybePrecreateSecondaryPageWidgets()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bPanelWidgetPrecreated || !PanelWidget.IsValid() || !IsRIEnabled())
    {
        return;
    }

    if (!GetWorld() || !GetLocalPC())
    {
        return;
    }

    if (!FilePageWidget.IsValid())
    {
        MaybePrecreateFilePageWidget();
        return;
    }

    if (!SettingsPageWidget.IsValid())
    {
        MaybePrecreateSettingsPageWidget();
    }
#endif
}

void UInspectorWorldSubsystem::MaybePrecreateFilePageWidget()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (FilePageWidget.IsValid() || !PanelWidget.IsValid())
    {
        return;
    }

    UInspectorFilePageWidget* Page = nullptr;
    if (APlayerController* PC = GetLocalPC())
    {
        Page = CreateWidget<UInspectorFilePageWidget>(PC, UInspectorFilePageWidget::StaticClass());
    }
    else if (UWorld* World = GetWorld())
    {
        Page = CreateWidget<UInspectorFilePageWidget>(World, UInspectorFilePageWidget::StaticClass());
    }

    if (!Page)
    {
        return;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    Page->SetInspectorSubsystem(this);
    Page->SetVisibility(ESlateVisibility::Collapsed);
    Page->TakeWidget();
    FilePageWidgetStrong = Page;
    FilePageWidget = Page;

    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] FilePagePrecreate %.2f ms"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
#endif
}

void UInspectorWorldSubsystem::MaybePrecreateSettingsPageWidget()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (SettingsPageWidget.IsValid() || !PanelWidget.IsValid())
    {
        return;
    }

    UInspectorSettingsPageWidget* Page = nullptr;
    if (APlayerController* PC = GetLocalPC())
    {
        Page = CreateWidget<UInspectorSettingsPageWidget>(PC, UInspectorSettingsPageWidget::StaticClass());
    }
    else if (UWorld* World = GetWorld())
    {
        Page = CreateWidget<UInspectorSettingsPageWidget>(World, UInspectorSettingsPageWidget::StaticClass());
    }

    if (!Page)
    {
        return;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    Page->SetInspectorSubsystem(this);
    Page->SetVisibility(ESlateVisibility::Collapsed);
    Page->TakeWidget();
    SettingsPageWidgetStrong = Page;
    SettingsPageWidget = Page;

    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] SettingsPagePrecreate %.2f ms"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
#endif
}

void UInspectorWorldSubsystem::ValidateSelection()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (SelectedActor.IsValid())
    {
        return;
    }

    // 弱引用无效：清空一切，避免 UI 继续引用旧 Items
    if (SelectedActor.Get() != nullptr || ItemPool.Num() > 0)
    {
        SelectedActor = nullptr;
        ClearItemPool();
        RefreshPanel(EInspectorRefreshReason::TargetInvalid);
    }
#endif
}


APlayerController* UInspectorWorldSubsystem::GetLocalPC() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    return World->GetFirstPlayerController();
}

void UInspectorWorldSubsystem::EnsureInspectorInputComponent()
{
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    if (!PC)
    {
        return;
    }

    if (InspectorInputComponent && InspectorInputComponent->GetOwner() != PC)
    {
        ReleaseInspectorInputComponent();
    }

    if (!InspectorInputComponent)
    {
        InspectorInputComponent = NewObject<UInputComponent>(PC, TEXT("RuntimeInspectorDedicatedInputComponent"));
        if (!InspectorInputComponent)
        {
            return;
        }

        InspectorInputComponent->Priority = 10;
        InspectorInputComponent->bBlockInput = false;
        InspectorInputComponent->RegisterComponent();
        PC->PushInputComponent(InspectorInputComponent);
    }
#endif
}

void UInspectorWorldSubsystem::ReleaseInspectorInputComponent()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (APlayerController* PC = GetLocalPC())
    {
        if (InspectorInputComponent)
        {
            PC->PopInputComponent(InspectorInputComponent);
        }
    }

    if (InspectorInputComponent)
    {
        InspectorInputComponent->KeyBindings.Reset();
    }

    InspectorInputComponent = nullptr;
    bInputsBound = false;
#endif
}

void UInspectorWorldSubsystem::RebindInspectorKeys()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureInspectorInputComponent();
    if (!InspectorInputComponent)
    {
        bInputsBound = false;
        return;
    }

    InspectorInputComponent->KeyBindings.Reset();
    InspectorInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &UInspectorWorldSubsystem::OnUndoKeyPressed);
    InspectorInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &UInspectorWorldSubsystem::OnRedoKeyPressed);

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    const FKey ToggleKey = (Settings && Settings->ToggleKey.IsValid()) ? Settings->ToggleKey : EKeys::O;
    const FKey PickKey = (Settings && Settings->PickKey.IsValid()) ? Settings->PickKey : EKeys::P;

    InspectorInputComponent->BindKey(ToggleKey, IE_Pressed, this, &UInspectorWorldSubsystem::Toggle);
    InspectorInputComponent->BindKey(PickKey, IE_Pressed, this, &UInspectorWorldSubsystem::OnPickKeyPressed);

    bInputsBound = true;
#endif
}

void UInspectorWorldSubsystem::TryBindInputs()
{
   
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    
    

    if (!PC) return;
    RebindInspectorKeys();
#endif
}

void UInspectorWorldSubsystem::Toggle()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!IsRIEnabled())
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Toggle ignored: %s"), *GetRIDisabledReason());
        PushToast(ERIToastType::Warning, GetRIDisabledReason(), /*Duration=*/ 3.f);
        return;
    }

    if (bOpen) Close();
    else Open();
#endif
}

void UInspectorWorldSubsystem::Open()
{
#if RUNTIME_INSPECTOR_ENABLED
    const double StartSeconds = FPlatformTime::Seconds();
    if (!IsRIEnabled())
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Open blocked: %s"), *GetRIDisabledReason());
        PushToast(ERIToastType::Warning, GetRIDisabledReason(), /*Duration=*/ 3.f);
        return;
    }

    if (IsUnlockRequired() && !bUnlocked)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Open blocked: locked. %s"), *GetRIUnlockHint());
        PushToast(ERIToastType::Warning, GetRIUnlockHint(), 3.0f);
        return;
    }
    if (bOpen) return;
    bOpen = true;
    bInspectorOpen = true;

    RegisterInputProcessor();

    EnsurePanelWidget();

    if (UUserWidget* W = PanelWidget.Get())
    {
        W->AddToViewport(9999);
        RI_ApplyLegacyActorHeaderVisibilityFix(W);
    }
    EnsurePanelInteractionInitialized();

    bHasCompletedInitialActorPanelRefresh = false;
    if (!ContentSwitcher.IsValid()
        || !ActorTabButton.IsValid()
        || !FileTabButton.IsValid()
        || !SettingsTabButton.IsValid()
        || !TestTabButton.IsValid())
    {
        BindPanelTabButtons();
    }
    EnsureSharedContextStripInjected();
    UpdateSharedContextStrip();
    EnsureActorFunctionsSectionInjected();
    SetContentSwitcherIndex(0);
    ScheduleDeferredOpenActorRefresh(true);

    // 让鼠标可用（也可以后续做成设置项）
    if (APlayerController* PC = GetLocalPC())
    {
        PC->bShowMouseCursor = true;
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }
    EnableOutlinePP(true);
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] Open %.2f ms"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
#endif
}

void UInspectorWorldSubsystem::Close()
{
#if RUNTIME_INSPECTOR_ENABLED
    UnregisterInputProcessor();
    bDraggingPanel = false;
    bResizingPanelVertically = false;

    if (!bOpen) return;
    bOpen = false;
    bInspectorOpen = false;
    UUserWidget* ExistingPanelWidget = PanelWidget.Get();
    if (ExistingPanelWidget)
    {
        ExistingPanelWidget->RemoveFromParent();
    }
    RestoreMountedHostPanelVisibility();
    if (UInspectorFilePageWidget* ExistingFilePage = FilePageWidget.Get())
    {
        ExistingFilePage->CancelDeferredRefresh();
    }
    if (UInspectorSettingsPageWidget* ExistingSettingsPage = SettingsPageWidget.Get())
    {
        ExistingSettingsPage->CancelDeferredRefresh();
    }
    CancelThemePreviewRefresh();
    CancelDeferredOpenActorRefresh();
    RestoreFabScreenshotPanelTransform();
    RestoreFabScreenshotApplicationScale();
    HideSettingsPage();
    ClearConfirmDialogBinding();

    if (APlayerController* PC = GetLocalPC())
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    if (AActor* A = OutlinedActor.Get())
    { SetActorOutline(A, false); }
    OutlinedActor.Reset();
    EnableOutlinePP(false);

    // Optional auto-lock on close
    if (IsUnlockRequired())
    {
        const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
        if (Settings && Settings->bAutoLockOnClose)
        {
            bUnlocked = false;
        }
    }
#endif
}

void UInspectorWorldSubsystem::RefreshSharedContextStrip()
{
#if RUNTIME_INSPECTOR_ENABLED
    UpdateSharedContextStrip();
#endif
}

void UInspectorWorldSubsystem::EnsurePanelWidget()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (PanelWidget.IsValid()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FString PanelPath = PanelWidgetClass.ToSoftObjectPath().ToString();
    UClass* WidgetCls = PanelWidgetClass.LoadSynchronous();
    if (!WidgetCls)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI] Failed to load panel widget class from '%s'. Is the asset cooked/available?"), *PanelPath);
        return;
    }

    UUserWidget* W = nullptr;
    if (APlayerController* PC = GetLocalPC())
    {
        W = CreateWidget<UUserWidget>(PC, WidgetCls);
    }
    if (!W)
    {
        W = CreateWidget<UUserWidget>(World, WidgetCls);
    }
    if (!W)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI] CreateWidget failed for panel class '%s'"), *GetNameSafe(WidgetCls));
        return;
    }
    PanelWidgetStrong = W;
    PanelWidget = W;
    ActorTabButton.Reset();
    FileTabButton.Reset();
    SettingsTabButton.Reset();
    TestTabButton.Reset();
    ContentSwitcher.Reset();
    PanelTitleBarWidget.Reset();
    PanelRootContentWidget.Reset();
    PanelRootCanvasSlot.Reset();
    PanelSizeBox.Reset();
    FileHostPanel.Reset();
    SettingsHostPanel.Reset();
    TestHostPanel.Reset();
    SharedContextStripHostPanel.Reset();
    SharedContextActorText = nullptr;
    SharedContextClassText = nullptr;
    SharedContextSourceText = nullptr;
    SharedContextStagedText = nullptr;
    ActorWorkspaceSelectionBandStrong = nullptr;
    ActorWorkspaceSelectionActorTextStrong = nullptr;
    ActorWorkspaceSelectionSourceTextStrong = nullptr;
    ActorWorkspaceSelectionStateTextStrong = nullptr;
    ActorPropertyFunctionHostBoxStrong = nullptr;
    ActorWorkbenchBodyHostStrong = nullptr;
    ActorWorkbenchPageStackHostStrong = nullptr;
    ActorWorkbenchSidebarHostStrong = nullptr;
    ActorWorkbenchContentHostStrong = nullptr;
    ActorWorkspaceSelectionBand.Reset();
    ActorWorkspaceSelectionActorText.Reset();
    ActorWorkspaceSelectionSourceText.Reset();
    ActorWorkspaceSelectionStateText.Reset();
    ActorPropertyFunctionHostBox.Reset();
    ActorWorkbenchBodyHost.Reset();
    ActorWorkbenchPageStackHost.Reset();
    ActorWorkbenchSidebarHost.Reset();
    ActorWorkbenchContentHost.Reset();
    FilePageWidgetStrong = nullptr;
    SettingsPageWidgetStrong = nullptr;
    TestPageWidgetStrong = nullptr;
    FilePageWidget.Reset();
    SettingsPageWidget.Reset();
    TestPageWidget.Reset();
    HostPanelMountStates.Reset();
    SettingsPageIndex = INDEX_NONE;
    TestPageIndex = INDEX_NONE;
    bPanelInteractionInitialized = false;
#endif
}

void UInspectorWorldSubsystem::CacheInitialPanelHeight()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (PanelDefaultHeight > 0.0f)
    {
        return;
    }

    float ResolvedHeight = 0.0f;
    if (UCanvasPanelSlot* RootCanvasSlot = PanelRootCanvasSlot.Get())
    {
        ResolvedHeight = RootCanvasSlot->GetSize().Y;
    }

    if (ResolvedHeight < RI_MinUsablePanelHeight)
    {
        if (USizeBox* SizeBox = PanelSizeBox.Get())
        {
            ResolvedHeight = SizeBox->GetHeightOverride();
        }
    }

    if (ResolvedHeight < RI_MinUsablePanelHeight)
    {
        if (UWidget* RootContent = PanelRootContentWidget.Get())
        {
            const float CachedHeight = RootContent->GetCachedGeometry().GetLocalSize().Y;
            if (CachedHeight >= RI_MinUsablePanelHeight)
            {
                ResolvedHeight = CachedHeight;
            }
        }
    }

    if (ResolvedHeight < RI_MinUsablePanelHeight)
    {
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            const float CachedHeight = Panel->GetCachedGeometry().GetLocalSize().Y;
            if (CachedHeight >= RI_MinUsablePanelHeight)
            {
                ResolvedHeight = CachedHeight;
            }
        }
    }

    if (ResolvedHeight < RI_MinUsablePanelHeight)
    {
        ResolvedHeight = 760.0f;
    }

    if (UWorld* World = GetWorld())
    {
        const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(World);
        const float ViewportHeightDefault = RI_ResolvePanelDefaultDimension(
            LogicalViewportSize.Y,
            RI_DefaultPanelViewportHeightFraction,
            RI_DefaultPanelHeightMin,
            0.0f);
        if (ViewportHeightDefault > 1.0f)
        {
            ResolvedHeight = ViewportHeightDefault;
        }
    }

    PanelDefaultHeight = ResolvedHeight;
    if (PanelHeight <= 1.0f)
    {
        PanelHeight = PanelDefaultHeight;
    }
#endif
}

void UInspectorWorldSubsystem::CacheInitialPanelWidth()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (PanelDefaultWidth > 0.0f)
    {
        return;
    }

    float ResolvedWidth = 0.0f;
    if (UCanvasPanelSlot* RootCanvasSlot = PanelRootCanvasSlot.Get())
    {
        ResolvedWidth = RootCanvasSlot->GetSize().X;
    }

    if (ResolvedWidth < RI_MinUsablePanelWidth)
    {
        if (USizeBox* SizeBox = PanelSizeBox.Get())
        {
            ResolvedWidth = SizeBox->GetWidthOverride();
        }
    }

    if (ResolvedWidth < RI_MinUsablePanelWidth)
    {
        if (UWidget* RootContent = PanelRootContentWidget.Get())
        {
            const float CachedWidth = RootContent->GetCachedGeometry().GetLocalSize().X;
            if (CachedWidth >= RI_MinUsablePanelWidth)
            {
                ResolvedWidth = CachedWidth;
            }
        }
    }

    if (ResolvedWidth < RI_MinUsablePanelWidth)
    {
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            const float CachedWidth = Panel->GetCachedGeometry().GetLocalSize().X;
            if (CachedWidth >= RI_MinUsablePanelWidth)
            {
                ResolvedWidth = CachedWidth;
            }
        }
    }

    if (ResolvedWidth < RI_MinUsablePanelWidth)
    {
        ResolvedWidth = 820.0f;
    }

    if (UWorld* World = GetWorld())
    {
        const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(World);
        const float ViewportWidthDefault = RI_ResolvePanelDefaultDimension(
            LogicalViewportSize.X,
            RI_DefaultPanelViewportWidthFraction,
            RI_MinUsablePanelWidth,
            RI_DefaultPanelWidthMax);
        if (ViewportWidthDefault > 1.0f)
        {
            ResolvedWidth = ViewportWidthDefault;
        }
    }

    PanelDefaultWidth = ResolvedWidth;
    if (PanelWidth <= 1.0f)
    {
        PanelWidth = PanelDefaultWidth;
    }
#endif
}

void UInspectorWorldSubsystem::EnsurePanelInteractionInitialized()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    if (!PanelTitleBarWidget.IsValid())
    {
        PanelTitleBarWidget = Panel->WidgetTree->FindWidget(TEXT("WindowTitleBarArea"));
    }

    if (!PanelRootContentWidget.IsValid() || !PanelRootCanvasSlot.IsValid())
    {
        if (UPanelWidget* RootPanel = Cast<UPanelWidget>(Panel->WidgetTree->RootWidget))
        {
            const int32 ChildCount = RootPanel->GetChildrenCount();
            UWidget* FallbackCandidate = nullptr;
            UCanvasPanelSlot* FallbackCanvasSlot = nullptr;
            for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
            {
                UWidget* Candidate = RootPanel->GetChildAt(ChildIndex);
                if (!Candidate)
                {
                    continue;
                }

                if (Candidate->GetName().Contains(TEXT("Toast")))
                {
                    continue;
                }

                UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Candidate->Slot);
                if (!CanvasSlot)
                {
                    continue;
                }

                if (!FallbackCandidate)
                {
                    FallbackCandidate = Candidate;
                    FallbackCanvasSlot = CanvasSlot;
                }

                if (Cast<UBorder>(Candidate) || Candidate->GetName().Equals(TEXT("Border")))
                {
                    PanelRootContentWidget = Candidate;
                    PanelRootCanvasSlot = CanvasSlot;
                    break;
                }
            }

            if (!PanelRootContentWidget.IsValid() && FallbackCandidate && FallbackCanvasSlot)
            {
                PanelRootContentWidget = FallbackCandidate;
                PanelRootCanvasSlot = FallbackCanvasSlot;
            }
        }
    }

    if (!PanelSizeBox.IsValid())
    {
        if (USizeBox* NamedSizeBox = Cast<USizeBox>(Panel->WidgetTree->FindWidget(TEXT("SizeBox"))))
        {
            PanelSizeBox = NamedSizeBox;
        }
        else
        {
            TArray<UWidget*> AllWidgets;
            Panel->WidgetTree->GetAllWidgets(AllWidgets);
            for (UWidget* Widget : AllWidgets)
            {
                if (USizeBox* FirstSizeBox = Cast<USizeBox>(Widget))
                {
                    PanelSizeBox = FirstSizeBox;
                    break;
                }
            }
        }
    }

    CacheInitialPanelWidth();
    CacheInitialPanelHeight();
    ApplyPanelInteractionPresentation();
    bPanelInteractionInitialized = PanelTitleBarWidget.IsValid() || PanelSizeBox.IsValid();
#endif
}

void UInspectorWorldSubsystem::ApplyPanelInteractionPresentation()
{
#if RUNTIME_INSPECTOR_ENABLED
    float ResolvedWidth = PanelWidth > 1.0f ? PanelWidth : PanelDefaultWidth;
    float ResolvedHeight = PanelHeight > 1.0f ? PanelHeight : PanelDefaultHeight;
    FVector2D BasePosition(8.0f, 8.0f);

    if (UWorld* World = GetWorld())
    {
        const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(World);
        if (LogicalViewportSize.X > 1.0f)
        {
            ResolvedWidth = FMath::Min(ResolvedWidth, RI_GetViewportAvailableDimension(LogicalViewportSize.X));
        }
        if (LogicalViewportSize.Y > 1.0f)
        {
            ResolvedHeight = FMath::Min(ResolvedHeight, RI_GetViewportAvailableDimension(LogicalViewportSize.Y));
        }

        BasePosition = RI_GetDefaultPanelCanvasPosition(World, FVector2D(ResolvedWidth, ResolvedHeight));
    }

    if (UUserWidget* Panel = PanelWidget.Get())
    {
        FWidgetTransform Transform = Panel->GetRenderTransform();
        Transform.Translation = FVector2D::ZeroVector;
        Panel->SetRenderTransform(Transform);
    }

    if (UCanvasPanelSlot* RootCanvasSlot = PanelRootCanvasSlot.Get())
    {
        CacheInitialPanelWidth();
        CacheInitialPanelHeight();
        RootCanvasSlot->SetAutoSize(false);
        RootCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        RootCanvasSlot->SetAlignment(FVector2D::ZeroVector);
        RootCanvasSlot->SetPosition(BasePosition + PanelTranslation);
        RootCanvasSlot->SetSize(FVector2D(ResolvedWidth, ResolvedHeight));
    }

    if (USizeBox* SizeBox = PanelSizeBox.Get())
    {
        CacheInitialPanelWidth();
        CacheInitialPanelHeight();
        if (!PanelRootCanvasSlot.IsValid())
        {
            SizeBox->SetWidthOverride(ResolvedWidth);
            SizeBox->SetHeightOverride(ResolvedHeight);
        }
    }
#endif
}

bool UInspectorWorldSubsystem::TryGetPanelWindowRect(FSlateRect& OutRect) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !FSlateApplication::IsInitialized())
    {
        return false;
    }

    TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget();
    if (!CachedWidget.IsValid())
    {
        return false;
    }

    TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(CachedWidget.ToSharedRef());
    if (!Window.IsValid())
    {
        return false;
    }

    OutRect = Window->GetRectInScreen();
    return true;
#endif
}

bool UInspectorWorldSubsystem::HandlePanelMouseButtonDown(const FPointerEvent& MouseEvent)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !IsOpen())
    {
        return false;
    }

    return HandlePanelPointerDownAt(MouseEvent.GetScreenSpacePosition());
#endif
}

bool UInspectorWorldSubsystem::HandlePanelMouseMove(const FPointerEvent& MouseEvent)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    return HandlePanelPointerMoveTo(MouseEvent.GetScreenSpacePosition());
#endif
}

bool UInspectorWorldSubsystem::HandlePanelMouseButtonUp(const FPointerEvent& MouseEvent)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return false;
    }

    return HandlePanelPointerUp();
#endif
}

bool UInspectorWorldSubsystem::HandlePanelPointerDownAt(const FVector2D& Cursor)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    EnsurePanelInteractionInitialized();

    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel)
    {
        return false;
    }

    Panel->ForceLayoutPrepass();
    const FGeometry PanelGeometry = Panel->GetCachedGeometry();
    const FVector2D PanelSize = PanelGeometry.GetLocalSize();
    if (PanelSize.X <= 1.0f || PanelSize.Y <= 1.0f)
    {
        return false;
    }

    bool bHasTitleHitRegion = false;
    if (UWidget* TitleBar = PanelTitleBarWidget.Get())
    {
        TitleBar->ForceLayoutPrepass();
        const FGeometry TitleGeometry = TitleBar->GetCachedGeometry();
        if (TitleGeometry.GetLocalSize().X > 1.0f && TitleGeometry.IsUnderLocation(Cursor))
        {
            bHasTitleHitRegion = true;
        }
    }

    if (!bHasTitleHitRegion)
    {
        const FVector2D PanelAbsolutePos = PanelGeometry.GetAbsolutePosition();
        const float TitleStripHeight = FMath::Clamp(PanelSize.Y * 0.085f, 36.0f, 64.0f);
        bHasTitleHitRegion = Cursor.X >= PanelAbsolutePos.X
            && Cursor.X <= (PanelAbsolutePos.X + PanelSize.X)
            && Cursor.Y >= PanelAbsolutePos.Y
            && Cursor.Y <= (PanelAbsolutePos.Y + TitleStripHeight);
    }

    if (bHasTitleHitRegion)
    {
        bDraggingPanel = true;
        bResizingPanelVertically = false;
        PanelInteractionStartCursor = Cursor;
        PanelInteractionStartTranslation = PanelTranslation;
        return true;
    }

    const FVector2D PanelAbsolutePos = PanelGeometry.GetAbsolutePosition();
    const FVector2D PanelBottomLeft = PanelAbsolutePos + FVector2D(0.0f, PanelSize.Y - 14.0f);
    const FVector2D PanelBottomRight = PanelAbsolutePos + FVector2D(PanelSize.X, PanelSize.Y);
    const bool bInBottomResizeStrip = Cursor.X >= PanelBottomLeft.X && Cursor.X <= PanelBottomRight.X
        && Cursor.Y >= PanelBottomLeft.Y && Cursor.Y <= PanelBottomRight.Y;

    if (bInBottomResizeStrip)
    {
        bDraggingPanel = false;
        bResizingPanelVertically = true;
        PanelInteractionStartCursor = Cursor;
        PanelInteractionStartHeight = PanelHeight > 1.0f ? PanelHeight : PanelDefaultHeight;
        return true;
    }

    return false;
#endif
}

bool UInspectorWorldSubsystem::HandlePanelPointerMoveTo(const FVector2D& Cursor)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    if ((!bDraggingPanel && !bResizingPanelVertically) || !IsOpen())
    {
        return false;
    }

    EnsurePanelInteractionInitialized();
    const FVector2D Delta = Cursor - PanelInteractionStartCursor;

    if (bDraggingPanel)
    {
        FVector2D NewTranslation = PanelInteractionStartTranslation + Delta;
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            const FGeometry PanelGeometry = Panel->GetCachedGeometry();
            FSlateRect WindowRect;
            if (PanelGeometry.GetLocalSize().X > 1.0f && PanelGeometry.GetLocalSize().Y > 1.0f && TryGetPanelWindowRect(WindowRect))
            {
                const FVector2D CurrentAbsolute = PanelGeometry.GetAbsolutePosition();
                const FVector2D DesiredAbsolute = CurrentAbsolute + (NewTranslation - PanelTranslation);
                const FVector2D PanelSize = PanelGeometry.GetLocalSize();
                const float ClampedX = FMath::Clamp(DesiredAbsolute.X, WindowRect.Left, WindowRect.Right - PanelSize.X);
                const float ClampedY = FMath::Clamp(DesiredAbsolute.Y, WindowRect.Top, WindowRect.Bottom - PanelSize.Y);
                NewTranslation += FVector2D(ClampedX - DesiredAbsolute.X, ClampedY - DesiredAbsolute.Y);
            }
        }

        PanelTranslation = NewTranslation;
        ApplyPanelInteractionPresentation();
        return true;
    }

    if (bResizingPanelVertically)
    {
        FSlateRect WindowRect;
        const float MaxHeight = TryGetPanelWindowRect(WindowRect) ? FMath::Max(560.0f, WindowRect.GetSize().Y - 32.0f) : 980.0f;
        PanelHeight = FMath::Clamp(PanelInteractionStartHeight + Delta.Y, 560.0f, MaxHeight);
        ApplyPanelInteractionPresentation();
        return true;
    }

    return false;
#endif
}

bool UInspectorWorldSubsystem::HandlePanelPointerUp()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    const bool bHandled = bDraggingPanel || bResizingPanelVertically;
    bDraggingPanel = false;
    bResizingPanelVertically = false;
    return bHandled;
#endif
}

namespace
{
    static bool RI_WidgetContainsTextRecursive(UWidget* Widget, const FString& DesiredText)
    {
        if (!Widget)
        {
            return false;
        }

        if (const UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            if (TextBlock->GetText().ToString().Equals(DesiredText, ESearchCase::IgnoreCase))
            {
                return true;
            }
        }

        if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
        {
            if (UserWidget->WidgetTree && RI_WidgetContainsTextRecursive(UserWidget->WidgetTree->RootWidget, DesiredText))
            {
                return true;
            }
        }

        if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
            {
                if (RI_WidgetContainsTextRecursive(Panel->GetChildAt(Index), DesiredText))
                {
                    return true;
                }
            }
        }

        return false;
    }

    static UTextBlock* RI_FindFirstTextBlockRecursive(UWidget* Widget)
    {
        if (!Widget)
        {
            return nullptr;
        }

        if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            return TextBlock;
        }

        if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
        {
            if (UserWidget->WidgetTree)
            {
                return RI_FindFirstTextBlockRecursive(UserWidget->WidgetTree->RootWidget);
            }
        }

        if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
            {
                if (UTextBlock* TextBlock = RI_FindFirstTextBlockRecursive(Panel->GetChildAt(Index)))
                {
                    return TextBlock;
                }
            }
        }

        return nullptr;
    }

    static void RI_CenterButtonContentRecursive(UWidget* Widget)
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

        if (UButton* Button = Cast<UButton>(Widget))
        {
            if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
            {
                ButtonSlot->SetHorizontalAlignment(HAlign_Center);
                ButtonSlot->SetVerticalAlignment(VAlign_Center);
                ButtonSlot->SetPadding(FMargin(0.f));
            }
        }

        if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
        {
            if (UserWidget->WidgetTree)
            {
                RI_CenterButtonContentRecursive(UserWidget->WidgetTree->RootWidget);
            }
        }

        if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
            {
                RI_CenterButtonContentRecursive(Panel->GetChildAt(Index));
            }
        }
    }

    static USizeBox* RI_FindFirstSizeBoxRecursive(UWidget* Widget)
    {
        if (!Widget)
        {
            return nullptr;
        }

        if (USizeBox* SizeBox = Cast<USizeBox>(Widget))
        {
            return SizeBox;
        }

        if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
        {
            if (UserWidget->WidgetTree)
            {
                return RI_FindFirstSizeBoxRecursive(UserWidget->WidgetTree->RootWidget);
            }
        }

        if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
            {
                if (USizeBox* SizeBox = RI_FindFirstSizeBoxRecursive(Panel->GetChildAt(Index)))
                {
                    return SizeBox;
                }
            }
        }

        return nullptr;
    }

    static bool RI_IsUndesirableDefaultSelectionActor(const AActor* Actor)
    {
        if (!Actor)
        {
            return true;
        }

        const UClass* ActorClass = Actor->GetClass();
        const FString ClassName = ActorClass ? ActorClass->GetName() : FString();

        return ClassName.Contains(TEXT("WorldSettings"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("DefaultPhysicsVolume"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("PlayerStart"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("Brush"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("SkyLight"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("DirectionalLight"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("ExponentialHeightFog"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("SkyAtmosphere"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("VolumetricCloud"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("PostProcessVolume"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("AtmosphericFog"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("GameNetworkManager"), ESearchCase::IgnoreCase);
    }

    static int32 RI_ScoreDefaultSelectionActor(const AActor* Actor)
    {
        if (!Actor || RI_IsUndesirableDefaultSelectionActor(Actor))
        {
            return MIN_int32;
        }

        int32 Score = 0;

        if (const UClass* ActorClass = Actor->GetClass())
        {
            if (RI_IsBlueprintGeneratedClassRuntimeSafe(ActorClass))
            {
                Score += 100;
            }

            const FString ClassPath = ActorClass->GetPathName();
            if (!ClassPath.StartsWith(TEXT("/Script/Engine."), ESearchCase::IgnoreCase))
            {
                Score += 35;
            }
        }

        if (!Actor->IsHidden())
        {
            Score += 15;
        }

        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                continue;
            }

            if (Component->IsA<UPrimitiveComponent>())
            {
                Score += 20;
                break;
            }
        }

        return Score;
    }
    static void RI_ApplyLegacyActorHeaderVisibilityFix_Impl(UUserWidget* PanelWidget)
    {
        if (!PanelWidget || !PanelWidget->WidgetTree)
        {
            return;
        }

        auto CollapseLegacyWidget = [PanelWidget](const TCHAR* WidgetName)
        {
            if (UWidget* Widget = PanelWidget->WidgetTree->FindWidget(WidgetName))
            {
                Widget->SetVisibility(ESlateVisibility::Collapsed);
            }
        };

        auto CollapseLegacyWidgetPair = [PanelWidget](const TCHAR* FirstName, const TCHAR* SecondName)
        {
            UWidget* FirstWidget = PanelWidget->WidgetTree->FindWidget(FirstName);
            UWidget* SecondWidget = PanelWidget->WidgetTree->FindWidget(SecondName);

            UPanelWidget* SharedParent = nullptr;
            if (FirstWidget
                && SecondWidget
                && FirstWidget->Slot
                && SecondWidget->Slot
                && FirstWidget->Slot->Parent
                && FirstWidget->Slot->Parent == SecondWidget->Slot->Parent)
            {
                SharedParent = FirstWidget->Slot->Parent;
            }

            if (SharedParent)
            {
                SharedParent->SetVisibility(ESlateVisibility::Collapsed);
                return;
            }

            if (FirstWidget)
            {
                FirstWidget->SetVisibility(ESlateVisibility::Collapsed);
            }

            if (SecondWidget)
            {
                SecondWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        };

        CollapseLegacyWidgetPair(TEXT("TXT_Log"), TEXT("TXT_Log_1"));
        CollapseLegacyWidgetPair(TEXT("BTN_CopyReport"), TEXT("BTN_ExportReport"));
        CollapseLegacyWidget(TEXT("TXT_SelectedActor"));
        CollapseLegacyWidget(TEXT("TXT_SelectedName"));
        CollapseLegacyWidget(TEXT("TXT_SelectCompName"));
        CollapseLegacyWidget(TEXT("HorizontalBox_379"));
        CollapseLegacyWidget(TEXT("Logger"));
    }
}

static void RI_ApplyLegacyActorHeaderVisibilityFix(UUserWidget* PanelWidget)
{
    RI_ApplyLegacyActorHeaderVisibilityFix_Impl(PanelWidget);
}

static void RI_UpdateActorPropertyHeader(UUserWidget* PanelWidget, UObject* FocusedObject)
{
    if (!PanelWidget || !PanelWidget->WidgetTree)
    {
        return;
    }

    auto MakeObjectDisplayLabel = [](UObject* Object) -> FString
    {
        if (!Object)
        {
            return TEXT("Actor");
        }

        if (const AActor* Actor = Cast<AActor>(Object))
        {
            return RI_GetActorDisplayLabel(Actor);
        }

        if (const UActorComponent* Component = Cast<UActorComponent>(Object))
        {
            return Component->GetName();
        }

        return Object->GetName();
    };

    if (UWidget* HeaderValue = PanelWidget->WidgetTree->FindWidget(TEXT("TXT_SelectedActor")))
    {
        HeaderValue->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* HeaderLabel = PanelWidget->WidgetTree->FindWidget(TEXT("TXT_SelectedName")))
    {
        HeaderLabel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* HeaderParent = PanelWidget->WidgetTree->FindWidget(TEXT("HorizontalBox_379")))
    {
        HeaderParent->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (UWidget* FocusText = PanelWidget->WidgetTree->FindWidget(TEXT("TXT_SelectCompName")))
    {
        FocusText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (UTextBlock* TitleText = Cast<UTextBlock>(PanelWidget->WidgetTree->FindWidget(TEXT("TXT_ToolName"))))
    {
        TitleText->SetText(FText::FromString(TEXT("RuntimeInspector")));
    }

    auto ApplyFillRatio = [](UWidget* Widget, float FillWeight, const FMargin& Padding)
    {
        if (!Widget)
        {
            return;
        }

        if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Widget->Slot))
        {
            FSlateChildSize SizeRule(ESlateSizeRule::Fill);
            SizeRule.Value = FillWeight;
            Slot->SetSize(SizeRule);
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
            Slot->SetPadding(Padding);
        }
    };

    ApplyFillRatio(PanelWidget->WidgetTree->FindWidget(TEXT("Left")), 0.68f, FMargin(0.f, 0.f, 6.f, 0.f));
    ApplyFillRatio(PanelWidget->WidgetTree->FindWidget(TEXT("Right")), 1.32f, FMargin(6.f, 0.f, 0.f, 0.f));
}

UWidgetSwitcher* UInspectorWorldSubsystem::FindContentSwitcher() const
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Widget = PanelWidget.Get();
    if (!Widget || !Widget->WidgetTree)
    {
        return nullptr;
    }

    if (UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Widget->WidgetTree->FindWidget(TEXT("WS_Content"))))
    {
        return Switcher;
    }

    TArray<UWidget*> AllWidgets;
    Widget->WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* Child : AllWidgets)
    {
        if (UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Child))
        {
            return Switcher;
        }
    }
#endif
    return nullptr;
}

UPanelWidget* UInspectorWorldSubsystem::FindFileHostPanel() const
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
    }
    if (!Switcher)
    {
        return nullptr;
    }

    for (int32 ChildIndex = 0; ChildIndex < Switcher->GetChildrenCount(); ++ChildIndex)
    {
        if (UPanelWidget* NamedHost = Cast<UPanelWidget>(Switcher->GetChildAt(ChildIndex)))
        {
            if (NamedHost->GetFName() == TEXT("Data"))
            {
                return NamedHost;
            }
        }
    }

    if (Switcher->GetChildrenCount() > 1)
    {
        return Cast<UPanelWidget>(Switcher->GetChildAt(1));
    }
#endif
    return nullptr;
}

UPanelWidget* UInspectorWorldSubsystem::FindSettingsHostPanel() const
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
    }
    if (!Switcher)
    {
        return nullptr;
    }

    for (int32 ChildIndex = 0; ChildIndex < Switcher->GetChildrenCount(); ++ChildIndex)
    {
        if (UPanelWidget* NamedHost = Cast<UPanelWidget>(Switcher->GetChildAt(ChildIndex)))
        {
            if (NamedHost->GetFName() == TEXT("Settings")
                || NamedHost->GetFName() == TEXT("Setting")
                || NamedHost->GetFName() == TEXT("RI_SettingsHost"))
            {
                return NamedHost;
            }
        }
    }

    if (SettingsPageIndex >= 0 && SettingsPageIndex < Switcher->GetChildrenCount())
    {
        return Cast<UPanelWidget>(Switcher->GetChildAt(SettingsPageIndex));
    }

    if (Switcher->GetChildrenCount() > 2)
    {
        if (UPanelWidget* FallbackHost = Cast<UPanelWidget>(Switcher->GetChildAt(2)))
        {
            if (FallbackHost->GetFName() != TEXT("RI_ToolsHost"))
            {
                return FallbackHost;
            }
        }
    }
#endif
    return nullptr;
}

UPanelWidget* UInspectorWorldSubsystem::FindTestHostPanel() const
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
    }
    if (!Switcher)
    {
        return nullptr;
    }

    for (int32 ChildIndex = 0; ChildIndex < Switcher->GetChildrenCount(); ++ChildIndex)
    {
        if (UPanelWidget* NamedHost = Cast<UPanelWidget>(Switcher->GetChildAt(ChildIndex)))
        {
            if (NamedHost->GetFName() == TEXT("Test")
                || NamedHost->GetFName() == TEXT("Tools")
                || NamedHost->GetFName() == TEXT("RI_ToolsHost"))
            {
                return NamedHost;
            }
        }
    }

    if (TestPageIndex >= 0 && TestPageIndex < Switcher->GetChildrenCount())
    {
        return Cast<UPanelWidget>(Switcher->GetChildAt(TestPageIndex));
    }
#endif
    return nullptr;
}

void UInspectorWorldSubsystem::EnsureLegacySupplementalTabsAndHosts()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Panel || !Panel->WidgetTree || !Switcher)
    {
        return;
    }

    if (!FindTestHostPanel())
    {
        UVerticalBox* ToolsHost = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ToolsHost"));
        ToolsHost->SetVisibility(ESlateVisibility::Visible);
        Switcher->AddChild(ToolsHost);
        if (UWidgetSwitcherSlot* SwitcherSlot = Cast<UWidgetSwitcherSlot>(ToolsHost->Slot))
        {
            SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
            SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    if (!FindSettingsHostPanel())
    {
        UVerticalBox* SettingsHost = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SettingsHost"));
        SettingsHost->SetVisibility(ESlateVisibility::Visible);
        Switcher->AddChild(SettingsHost);
        if (UWidgetSwitcherSlot* SwitcherSlot = Cast<UWidgetSwitcherSlot>(SettingsHost->Slot))
        {
            SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
            SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }

    if (FindPanelTabButtonByTexts({ TEXT("Test"), TEXT("Tools"), TEXT("Diagnostics") }))
    {
        return;
    }

    UButton* AnchorButton = FindPanelTabButtonByTexts({ TEXT("Setting"), TEXT("Settings") });
    if (!AnchorButton)
    {
        AnchorButton = FindPanelTabButtonByTexts({ TEXT("File"), TEXT("Changes"), TEXT("Snapshot") });
    }
    UHorizontalBox* TabContainer = AnchorButton ? Cast<UHorizontalBox>(AnchorButton->GetParent()) : nullptr;
    if (!TabContainer)
    {
        return;
    }

    UButton* ToolsButton = RICompactUI::MakeLabeledButton(
        Panel->WidgetTree,
        TEXT("BTN_ToolsRuntime"),
        TEXT("Tools"),
        RICompactUI::ERIButtonVisualStyle::TabInactive,
        0.f,
        0.f,
        8);
    if (!ToolsButton)
    {
        return;
    }

    if (UHorizontalBoxSlot* Slot = TabContainer->AddChildToHorizontalBox(ToolsButton))
    {
        Slot->SetPadding(FMargin(0.f));
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Fill);
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.0f;
        Slot->SetSize(SizeRule);
    }

    RI_CenterButtonContentRecursive(ToolsButton);
#endif
}

void UInspectorWorldSubsystem::MountPageAsExclusiveChild(UPanelWidget* HostPanel, UWidget* PageWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!HostPanel || !PageWidget)
    {
        return;
    }

    FRIHostPanelMountState* MountState = HostPanelMountStates.FindByPredicate([HostPanel](const FRIHostPanelMountState& Candidate)
    {
        return Candidate.HostPanel.Get() == HostPanel;
    });

    if (!MountState)
    {
        FRIHostPanelMountState NewState;
        NewState.HostPanel = HostPanel;
        HostPanelMountStates.Add(MoveTemp(NewState));
        MountState = &HostPanelMountStates.Last();
    }

    MountState->MountedPageWidget = PageWidget;

    if (PageWidget->GetParent() != HostPanel)
    {
        if (PageWidget->GetParent())
        {
            PageWidget->RemoveFromParent();
        }
        HostPanel->AddChild(PageWidget);
    }

    const int32 ChildCount = HostPanel->GetChildrenCount();
    for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
    {
        UWidget* Child = HostPanel->GetChildAt(ChildIndex);
        if (!Child)
        {
            continue;
        }

        if (Child == PageWidget)
        {
            Child->SetVisibility(ESlateVisibility::Visible);
            continue;
        }

        const bool bAlreadyTracked = MountState->LegacyChildren.ContainsByPredicate([Child](const FRIHostLegacyChildState& Entry)
        {
            return Entry.Widget.Get() == Child;
        });

        if (!bAlreadyTracked)
        {
            FRIHostLegacyChildState Entry;
            Entry.Widget = Child;
            Entry.OriginalVisibility = Child->GetVisibility();
            MountState->LegacyChildren.Add(MoveTemp(Entry));
        }

        Child->SetVisibility(ESlateVisibility::Collapsed);
    }
#endif
}

void UInspectorWorldSubsystem::RestoreMountedHostPanelVisibility()
{
#if RUNTIME_INSPECTOR_ENABLED
    for (FRIHostPanelMountState& MountState : HostPanelMountStates)
    {
        if (UWidget* MountedPage = MountState.MountedPageWidget.Get())
        {
            MountedPage->SetVisibility(ESlateVisibility::Collapsed);
        }

        for (FRIHostLegacyChildState& Entry : MountState.LegacyChildren)
        {
            if (UWidget* Widget = Entry.Widget.Get())
            {
                Widget->SetVisibility(Entry.OriginalVisibility);
            }
        }
    }
    HostPanelMountStates.Reset();
#endif
}

void UInspectorWorldSubsystem::SetContentSwitcherIndex(int32 InIndex)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (InIndex != 0)
    {
        CancelDeferredOpenActorRefresh();
    }

    if (InIndex != 1)
    {
        if (UInspectorFilePageWidget* FilePage = FilePageWidget.Get())
        {
            FilePage->CancelDeferredRefresh();
        }
    }

    if (InIndex != SettingsPageIndex)
    {
        if (UInspectorSettingsPageWidget* SettingsPage = SettingsPageWidget.Get())
        {
            SettingsPage->CancelDeferredRefresh();
        }
    }

    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }
    if (!Switcher)
    {
        return;
    }

    if (InIndex >= 0 && InIndex < Switcher->GetChildrenCount())
    {
        Switcher->SetActiveWidgetIndex(InIndex);
    }

    UpdatePanelTabButtonStyles();
#endif
}

UInspectorWorldSubsystem::ERIVisiblePage UInspectorWorldSubsystem::GetVisiblePage() const
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = const_cast<UInspectorWorldSubsystem*>(this)->FindContentSwitcher();
        const_cast<UInspectorWorldSubsystem*>(this)->ContentSwitcher = Switcher;
    }

    UWidget* ActiveWidget = Switcher ? Switcher->GetActiveWidget() : nullptr;
    UPanelWidget* ActiveHost = Cast<UPanelWidget>(ActiveWidget);

    UPanelWidget* ResolvedFileHost = FileHostPanel.Get();
    if (!ResolvedFileHost)
    {
        ResolvedFileHost = const_cast<UInspectorWorldSubsystem*>(this)->FindFileHostPanel();
        const_cast<UInspectorWorldSubsystem*>(this)->FileHostPanel = ResolvedFileHost;
    }
    if (ActiveHost && ActiveHost == ResolvedFileHost)
    {
        return ERIVisiblePage::Changes;
    }

    UPanelWidget* ResolvedSettingsHost = SettingsHostPanel.Get();
    if (!ResolvedSettingsHost)
    {
        ResolvedSettingsHost = const_cast<UInspectorWorldSubsystem*>(this)->FindSettingsHostPanel();
        const_cast<UInspectorWorldSubsystem*>(this)->SettingsHostPanel = ResolvedSettingsHost;
    }
    if (ActiveHost && ActiveHost == ResolvedSettingsHost)
    {
        return ERIVisiblePage::Settings;
    }

    UPanelWidget* ResolvedTestHost = TestHostPanel.Get();
    if (!ResolvedTestHost)
    {
        ResolvedTestHost = const_cast<UInspectorWorldSubsystem*>(this)->FindTestHostPanel();
        const_cast<UInspectorWorldSubsystem*>(this)->TestHostPanel = ResolvedTestHost;
    }
    if (ActiveHost && ActiveHost == ResolvedTestHost)
    {
        return ERIVisiblePage::Tools;
    }

    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : 0;
    if (ActiveIndex == 1)
    {
        return ERIVisiblePage::Changes;
    }
    if (ActiveIndex == SettingsPageIndex)
    {
        return ERIVisiblePage::Settings;
    }
    if (ActiveIndex == TestPageIndex)
    {
        return ERIVisiblePage::Tools;
    }
#endif
    return ERIVisiblePage::Actor;
}

void UInspectorWorldSubsystem::ScheduleThemePreviewRefresh(ERIVisiblePage RestorePage)
{
#if RUNTIME_INSPECTOR_ENABLED
    PendingThemePreviewPage = RestorePage;

    UWorld* World = GetWorld();
    if (!World || !bOpen)
    {
        return;
    }

    bThemePreviewRefreshScheduled = true;
    World->GetTimerManager().ClearTimer(ThemePreviewRefreshTimerHandle);
    World->GetTimerManager().SetTimer(
        ThemePreviewRefreshTimerHandle,
        this,
        &UInspectorWorldSubsystem::HandleThemePreviewRefreshTimerElapsed,
        0.0f,
        false);
#endif
}

void UInspectorWorldSubsystem::CancelThemePreviewRefresh()
{
#if RUNTIME_INSPECTOR_ENABLED
    bThemePreviewRefreshScheduled = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ThemePreviewRefreshTimerHandle);
    }
#endif
}

void UInspectorWorldSubsystem::HandleThemePreviewRefreshTimerElapsed()
{
#if RUNTIME_INSPECTOR_ENABLED
    CancelThemePreviewRefresh();
    if (!bOpen)
    {
        return;
    }

    const ERIVisiblePage RestorePage = PendingThemePreviewPage;
    Close();
    Open();

    switch (RestorePage)
    {
    case ERIVisiblePage::Changes:
        ShowFilePage();
        break;
    case ERIVisiblePage::Settings:
        ShowSettingsPage();
        break;
    case ERIVisiblePage::Tools:
        ShowTestPage();
        break;
    case ERIVisiblePage::Actor:
    default:
        HandleActorTabClicked();
        break;
    }
#endif
}

void UInspectorWorldSubsystem::ScheduleDeferredOpenActorRefresh(bool bCaptureBaseline)
{
#if RUNTIME_INSPECTOR_ENABLED
    UWorld* World = GetWorld();
    if (!World || !bOpen)
    {
        return;
    }

    bDeferredOpenActorRefreshScheduled = true;
    bDeferredOpenActorRefreshNeedsBaseline = bDeferredOpenActorRefreshNeedsBaseline || bCaptureBaseline;
    World->GetTimerManager().ClearTimer(DeferredOpenActorRefreshTimerHandle);
    World->GetTimerManager().SetTimer(
        DeferredOpenActorRefreshTimerHandle,
        this,
        &UInspectorWorldSubsystem::HandleDeferredOpenActorRefreshTimerElapsed,
        0.01f,
        false);
#endif
}

void UInspectorWorldSubsystem::CancelDeferredOpenActorRefresh()
{
#if RUNTIME_INSPECTOR_ENABLED
    bDeferredOpenActorRefreshScheduled = false;
    bDeferredOpenActorRefreshNeedsBaseline = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeferredOpenActorRefreshTimerHandle);
    }
#endif
}

void UInspectorWorldSubsystem::HandleDeferredOpenActorRefreshTimerElapsed()
{
#if RUNTIME_INSPECTOR_ENABLED
    const double StartSeconds = FPlatformTime::Seconds();
    const bool bCaptureBaseline = bDeferredOpenActorRefreshNeedsBaseline;
    CancelDeferredOpenActorRefresh();

    if (!bOpen)
    {
        return;
    }

    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }

    if (Switcher && Switcher->GetActiveWidgetIndex() != 0)
    {
        return;
    }

    if (!SelectedActor.IsValid())
    {
        AActor* DefaultActor = nullptr;

        if (APlayerController* PC = GetLocalPC())
        {
            if (UWorld* World = GetWorld())
            {
                FVector CameraLocation = FVector::ZeroVector;
                FRotator CameraRotation = FRotator::ZeroRotator;
                PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

                const FVector TraceStart = CameraLocation;
                const FVector TraceEnd = TraceStart + CameraRotation.Vector() * 100000.0f;

                FHitResult Hit;
                FCollisionQueryParams Params(SCENE_QUERY_STAT(RuntimeInspectorOpenFocusTrace), true);
                Params.bReturnPhysicalMaterial = false;

                if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
                {
                    DefaultActor = Hit.GetActor();
                }
            }

            if (!DefaultActor)
            {
                DefaultActor = PC->GetPawn();
            }
        }

        if (!DefaultActor)
        {
            DefaultActor = ResolvePreferredFabScreenshotActor();
        }

        if (!DefaultActor)
        {
            if (UWorld* World = GetWorld())
            {
                int32 BestScore = MIN_int32;
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    AActor* Candidate = *It;
                    const int32 Score = RI_ScoreDefaultSelectionActor(Candidate);
                    if (Score > BestScore)
                    {
                        DefaultActor = Candidate;
                        BestScore = Score;
                    }
                }
            }
        }

        if (DefaultActor)
        {
            SetSelectedActor(DefaultActor);
        }
    }

    if (bCaptureBaseline)
    {
        CaptureBaselineForSelection(/*bIncludeMaterialParams=*/ true);
    }

    RefreshPanel(EInspectorRefreshReason::StructureChanged);
    bHasCompletedInitialActorPanelRefresh = true;
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] DeferredOpenActorRefresh %.2f ms | Baseline=%d"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0,
        bCaptureBaseline ? 1 : 0);
#endif
}

UButton* UInspectorWorldSubsystem::FindPanelTabButtonByText(const FString& DesiredText) const
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Widget = PanelWidget.Get();
    if (!Widget || !Widget->WidgetTree)
    {
        return nullptr;
    }

    TArray<UWidget*> AllWidgets;
    Widget->WidgetTree->GetAllWidgets(AllWidgets);
    for (UWidget* Child : AllWidgets)
    {
        if (UButton* Button = Cast<UButton>(Child))
        {
            if (RI_WidgetContainsTextRecursive(Button, DesiredText))
            {
                return Button;
            }
        }
    }
#endif
    return nullptr;
}

UButton* UInspectorWorldSubsystem::FindPanelTabButtonByTexts(std::initializer_list<const TCHAR*> DesiredTexts) const
{
#if RUNTIME_INSPECTOR_ENABLED
    for (const TCHAR* DesiredText : DesiredTexts)
    {
        if (!DesiredText)
        {
            continue;
        }

        if (UButton* Button = FindPanelTabButtonByText(DesiredText))
        {
            return Button;
        }
    }
#endif
    return nullptr;
}

void UInspectorWorldSubsystem::SetPanelTabButtonLabel(UButton* Button, const FString& NewLabel) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Button)
    {
        return;
    }

    if (UTextBlock* TextBlock = RI_FindFirstTextBlockRecursive(Button))
    {
        TextBlock->SetText(FText::FromString(NewLabel));
        TextBlock->SetJustification(ETextJustify::Center);
    }

    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Center);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(0.f));
    }

    RI_CenterButtonContentRecursive(Button);
#else
    (void)Button;
    (void)NewLabel;
#endif
}

void UInspectorWorldSubsystem::BindPanelTabButtons()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!PanelWidget.IsValid())
    {
        return;
    }

    ContentSwitcher = FindContentSwitcher();
    EnsureLegacySupplementalTabsAndHosts();
    ContentSwitcher = FindContentSwitcher();
    FileHostPanel = FindFileHostPanel();
    SettingsHostPanel = FindSettingsHostPanel();
    TestHostPanel = FindTestHostPanel();
    ActorTabButton = FindPanelTabButtonByTexts({ TEXT("Actor"), TEXT("Inspect") });
    FileTabButton = FindPanelTabButtonByTexts({ TEXT("File"), TEXT("Changes"), TEXT("Snapshot") });
    SettingsTabButton = FindPanelTabButtonByTexts({ TEXT("Setting"), TEXT("Settings") });
    TestTabButton = FindPanelTabButtonByTexts({ TEXT("Test"), TEXT("Tools"), TEXT("Diagnostics") });

    SetPanelTabButtonLabel(ActorTabButton.Get(), TEXT("Actor"));
    SetPanelTabButtonLabel(FileTabButton.Get(), TEXT("Changes"));
    SetPanelTabButtonLabel(SettingsTabButton.Get(), TEXT("Settings"));
    SetPanelTabButtonLabel(TestTabButton.Get(), TEXT("Tools"));

    TArray<UWidget*> PanelWidgets;
    PanelWidget->WidgetTree->GetAllWidgets(PanelWidgets);
    for (UWidget* Widget : PanelWidgets)
    {
        if (UButton* Button = Cast<UButton>(Widget))
        {
            RI_CenterButtonContentRecursive(Button);
        }
    }

    if (UButton* Button = ActorTabButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleActorTabClicked);
    }
    if (UButton* Button = FileTabButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleFileTabClicked);
    }
    if (UButton* Button = SettingsTabButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleSettingsTabClicked);
        Button->SetVisibility(ESlateVisibility::Visible);
    }
    if (UButton* Button = TestTabButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleTestTabClicked);
    }

    BindActorSearchBox();
    UpdatePanelTabButtonStyles();
    EnsureSharedContextStripInjected();
    UpdateSharedContextStrip();
#endif
}

void UInspectorWorldSubsystem::UpdatePanelTabButtonStyles()
{
#if RUNTIME_INSPECTOR_ENABLED
    const ERIVisiblePage ActivePage = GetVisiblePage();
    auto ApplyTabStyle = [ActivePage](UButton* Button, ERIVisiblePage ButtonPage)
    {
        if (!Button)
        {
            return;
        }

        const bool bActive = ActivePage == ButtonPage;
        RICompactUI::ConfigureButton(
            Button,
            bActive
                ? RICompactUI::ERIButtonVisualStyle::TabActive
                : RICompactUI::ERIButtonVisualStyle::TabInactive);

        Button->SetRenderOpacity(bActive ? 1.0f : 0.82f);

        if (UTextBlock* TextBlock = RI_FindFirstTextBlockRecursive(Button))
        {
            FSlateFontInfo Font = TextBlock->GetFont();
            Font.Size = bActive ? 8 : 7;
            TextBlock->SetFont(Font);
        }

        if (USizeBox* SizeBox = RI_FindFirstSizeBoxRecursive(Button))
        {
            SizeBox->SetMinDesiredWidth(92.0f);
            SizeBox->SetHeightOverride(24.0f);
        }

        if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Button->Slot))
        {
            Slot->SetPadding(FMargin(0.f));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    };

    ApplyTabStyle(ActorTabButton.Get(), ERIVisiblePage::Actor);
    ApplyTabStyle(FileTabButton.Get(), ERIVisiblePage::Changes);
    ApplyTabStyle(SettingsTabButton.Get(), ERIVisiblePage::Settings);
    ApplyTabStyle(TestTabButton.Get(), ERIVisiblePage::Tools);
#endif
}

void UInspectorWorldSubsystem::EnsureSharedContextStripInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Widget = PanelWidget.Get();
    if (!Widget || !Widget->WidgetTree)
    {
        return;
    }

    if (SharedContextActorText && SharedContextClassText && SharedContextSourceText && SharedContextStagedText)
    {
        return;
    }

    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }
    if (!Switcher)
    {
        return;
    }

    UPanelWidget* HostPanel = Cast<UPanelWidget>(Switcher->GetParent());
    if (!HostPanel)
    {
        return;
    }

    UBorder* ExistingBorder = Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStrip")));
    if (ExistingBorder)
    {
        if (UHorizontalBox* ExistingRow = Cast<UHorizontalBox>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStripRow"))))
        {
            if (!SharedContextActorCell.IsValid())
            {
                SharedContextActorCell = Cast<UWidget>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextActorCell")));
            }
            if (!SharedContextActorText)
            {
                SharedContextActorText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextActorValue")));
            }
            if (!SharedContextClassText)
            {
                SharedContextClassText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextClassValue")));
            }
            if (!SharedContextSourceText)
            {
                SharedContextSourceText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextSourceValue")));
            }
            if (!SharedContextStagedText)
            {
                SharedContextStagedText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStagedValue")));
            }
        }
        ExistingBorder->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    UVerticalBox* VerticalHostPanel = Cast<UVerticalBox>(HostPanel);
    UVerticalBox* StripHostContainer = VerticalHostPanel;
    if (!StripHostContainer)
    {
        StripHostContainer = Cast<UVerticalBox>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStripHost")));
        if (!StripHostContainer)
        {
            StripHostContainer = Widget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_SharedContextStripHost"));
        }

        if (StripHostContainer->GetParent() != HostPanel)
        {
            const int32 SwitcherIndexInHost = HostPanel->GetChildIndex(Switcher);
            HostPanel->RemoveChild(Switcher);
            HostPanel->InsertChildAt(SwitcherIndexInHost == INDEX_NONE ? HostPanel->GetChildrenCount() : SwitcherIndexInHost, StripHostContainer);

            if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(StripHostContainer->Slot))
            {
                OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
                OverlaySlot->SetVerticalAlignment(VAlign_Fill);
                OverlaySlot->SetPadding(FMargin(0.f));
            }
            else if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(StripHostContainer->Slot))
            {
                VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
                VerticalSlot->SetVerticalAlignment(VAlign_Fill);
                VerticalSlot->SetPadding(FMargin(0.f));
            }
        }

        if (Switcher->GetParent() != StripHostContainer)
        {
            if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(Switcher->GetParent()))
            {
                ExistingParent->RemoveChild(Switcher);
            }

            if (UVerticalBoxSlot* SwitcherSlot = StripHostContainer->AddChildToVerticalBox(Switcher))
            {
                SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
                SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
                SwitcherSlot->SetPadding(FMargin(0.f));
            }
        }
    }

    SharedContextStripHostPanel = StripHostContainer ? static_cast<UPanelWidget*>(StripHostContainer) : HostPanel;

    UBorder* StripBorder = Widget->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_SharedContextStrip"));
    StripBorder->SetPadding(RICompactUI::GetSurfaceCardPadding());
    StripBorder->SetBrushColor(RICompactUI::GetContextStripBackgroundColor());

    UHorizontalBox* Row = Widget->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_SharedContextStripRow"));
    StripBorder->SetContent(Row);

    auto AddCell = [Widget, Row](
        const TCHAR* Label,
        const FName CellName,
        const FName ValueName,
        TObjectPtr<UTextBlock>& OutValueText,
        float FillWeight,
        int32 ValueFontSize,
        bool bBoldValue,
        const FLinearColor& CellColor)
    {
        UBorder* Cell = CellName.IsNone()
            ? Widget->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass())
            : Widget->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), CellName);
        Cell->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
        Cell->SetBrushColor(CellColor);

        UVerticalBox* Box = Widget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Cell->SetContent(Box);

        Box->AddChildToVerticalBox(RICompactUI::MakeText(Widget->WidgetTree, Label, RICompactUI::GetMutedFontSize(), true, RICompactUI::GetMutedTextColor()));
        OutValueText = Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ValueName);
        OutValueText->SetAutoWrapText(true);
        OutValueText->SetClipping(EWidgetClipping::ClipToBounds);
        RICompactUI::ApplyTextStyle(OutValueText, ValueFontSize, bBoldValue, RICompactUI::GetStrongTextColor());
        Box->AddChildToVerticalBox(OutValueText);

        if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Cell))
        {
            FSlateChildSize SizeRule(ESlateSizeRule::Fill);
            SizeRule.Value = FillWeight;
            Slot->SetSize(SizeRule);
            Slot->SetPadding(FMargin(0.f, 0.f, RICompactUI::GetInlineGap(), 0.f));
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
    };

    AddCell(
        TEXT("Selected Actor"),
        TEXT("RI_SharedContextActorCell"),
        TEXT("RI_SharedContextActorValue"),
        SharedContextActorText,
        1.35f,
        10,
        true,
        RICompactUI::GetContextPrimaryCellBackgroundColor());
    AddCell(
        TEXT("Actor Class"),
        NAME_None,
        TEXT("RI_SharedContextClassValue"),
        SharedContextClassText,
        0.96f,
        8,
        false,
        RICompactUI::GetContextSecondaryCellBackgroundColor());
    AddCell(
        TEXT("Source Asset"),
        NAME_None,
        TEXT("RI_SharedContextSourceValue"),
        SharedContextSourceText,
        1.16f,
        8,
        false,
        RICompactUI::GetContextSecondaryCellBackgroundColor());
    AddCell(
        TEXT("Staged State"),
        NAME_None,
        TEXT("RI_SharedContextStagedValue"),
        SharedContextStagedText,
        0.92f,
        9,
        true,
        RICompactUI::GetContextStatusCellBackgroundColor());

    const int32 SwitcherIndex = StripHostContainer ? StripHostContainer->GetChildIndex(Switcher) : INDEX_NONE;
    const int32 InsertIndex = (StripHostContainer && SwitcherIndex != INDEX_NONE) ? SwitcherIndex : (StripHostContainer ? StripHostContainer->GetChildrenCount() : 0);
    if (StripHostContainer)
    {
        StripHostContainer->InsertChildAt(InsertIndex, StripBorder);
    }

    if (UPanelSlot* PanelSlot = StripBorder->Slot)
    {
        if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(PanelSlot))
        {
            VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
            VerticalSlot->SetVerticalAlignment(VAlign_Top);
            VerticalSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
        }
    }

    StripBorder->SetVisibility(ESlateVisibility::Collapsed);
#endif
}

void UInspectorWorldSubsystem::UpdateSharedContextStrip()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureSharedContextStripInjected();

    const AActor* Selected = SelectedActor.Get();
    const FString ActorLabel = Selected ? RI_GetActorDisplayLabel(Selected) : TEXT("No selected actor");
    const FString ActorClass = (Selected && Selected->GetClass()) ? Selected->GetClass()->GetName() : TEXT("No actor class");
    const FString SourcePath = (Selected && Selected->GetClass())
        ? Selected->GetClass()->GetPathName()
        : TEXT("No source asset");
    const FString StagedState = HasStagedPatch()
        ? FString::Printf(TEXT("Staged (%d ops)"), GetStagedPatch().Operations.Num())
        : TEXT("No staged patch");
    const bool bHideActorCell = false;

    auto ApplyValue = [](UTextBlock* TextWidget, const FString& InValue, bool bMuted, int32 FontSize, bool bBold, const FLinearColor& StrongColor)
    {
        if (!TextWidget)
        {
            return;
        }

        TextWidget->SetText(FText::FromString(InValue));
        RICompactUI::ApplyTextStyle(
            TextWidget,
            FontSize,
            bBold,
            bMuted ? RICompactUI::GetMutedTextColor() : StrongColor);
    };

    ApplyValue(SharedContextActorText, ActorLabel, Selected == nullptr, 9, true, RICompactUI::GetStrongTextColor());
    ApplyValue(SharedContextClassText, ActorClass, Selected == nullptr, 7, false, RICompactUI::GetSecondaryTextColor());
    ApplyValue(SharedContextSourceText, SourcePath, Selected == nullptr, 7, false, RICompactUI::GetSecondaryTextColor());
    ApplyValue(
        SharedContextStagedText,
        StagedState,
        !HasStagedPatch(),
        8,
        true,
        HasStagedPatch() ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetStrongTextColor());

    if (SharedContextActorCell.IsValid())
    {
        SharedContextActorCell->SetVisibility(bHideActorCell ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
#endif
}

namespace
{
    static const TCHAR* RI_GetVisiblePageDisplayLabel(UInspectorWorldSubsystem::ERIVisiblePage Page)
    {
        switch (Page)
        {
        case UInspectorWorldSubsystem::ERIVisiblePage::Changes:
            return TEXT("Changes");
        case UInspectorWorldSubsystem::ERIVisiblePage::Settings:
            return TEXT("Settings");
        case UInspectorWorldSubsystem::ERIVisiblePage::Tools:
            return TEXT("Tools");
        case UInspectorWorldSubsystem::ERIVisiblePage::Actor:
        default:
            return TEXT("Actor");
        }
    }

    static UWidget* RI_FindDirectChildUnderHost(UWidget* AnchorWidget, UPanelWidget* HostPanel)
    {
        if (!AnchorWidget || !HostPanel)
        {
            return nullptr;
        }

        UWidget* Current = AnchorWidget;
        while (Current)
        {
            UPanelWidget* ParentPanel = Current->GetParent();
            if (ParentPanel == HostPanel)
            {
                return Current;
            }

            Current = ParentPanel;
        }

        return nullptr;
    }

    static bool RI_FindFunctionHostPanel(UUserWidget* PanelWidget, UWidget*& OutAnchorWidget, UPanelWidget*& OutHostPanel)
    {
        OutAnchorWidget = nullptr;
        OutHostPanel = nullptr;

        if (!PanelWidget || !PanelWidget->WidgetTree)
        {
            return false;
        }

        UWidget* PropertyList = PanelWidget->WidgetTree->FindWidget(TEXT("LV_Properties"));
        if (!PropertyList)
        {
            return false;
        }

        UWidget* Current = PropertyList;
        while (Current)
        {
            UPanelWidget* ParentPanel = Current->GetParent();
            if (!ParentPanel)
            {
                break;
            }

            if (Cast<UVerticalBox>(ParentPanel))
            {
                OutAnchorWidget = Current;
                OutHostPanel = ParentPanel;
                return true;
            }

            Current = ParentPanel;
        }

        OutAnchorWidget = PropertyList;
        OutHostPanel = Cast<UPanelWidget>(PropertyList->GetParent());
        return OutHostPanel != nullptr;
    }

    static void RI_EnsureInspectBodyLayout(UUserWidget* PanelWidget)
    {
        if (!PanelWidget || !PanelWidget->WidgetTree)
        {
            return;
        }

        if (UWidget* BodyWidget = PanelWidget->WidgetTree->FindWidget(TEXT("Body")))
        {
            if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(BodyWidget->Slot))
            {
                BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                BodySlot->SetHorizontalAlignment(HAlign_Fill);
                BodySlot->SetVerticalAlignment(VAlign_Fill);
            }
        }

        if (UWidget* RightWidget = PanelWidget->WidgetTree->FindWidget(TEXT("Right")))
        {
            if (UHorizontalBoxSlot* RightSlot = Cast<UHorizontalBoxSlot>(RightWidget->Slot))
            {
                RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                RightSlot->SetHorizontalAlignment(HAlign_Fill);
                RightSlot->SetVerticalAlignment(VAlign_Fill);
            }
        }
    }

    static bool RI_TryGetSupportedFunctionValueType(const FProperty* Property, EInspectorValueType& OutValueType, FString& OutTypeLabel, TArray<FString>& OutEnumOptions)
    {
        OutValueType = EInspectorValueType::Unsupported;
        OutTypeLabel = TEXT("Unsupported");
        OutEnumOptions.Reset();

        if (!Property)
        {
            return false;
        }

        if (Property->IsA<FBoolProperty>())
        {
            OutValueType = EInspectorValueType::Bool;
            OutTypeLabel = TEXT("bool");
            return true;
        }
        if (Property->IsA<FIntProperty>())
        {
            OutValueType = EInspectorValueType::Int;
            OutTypeLabel = TEXT("int32");
            return true;
        }
        if (Property->IsA<FFloatProperty>())
        {
            OutValueType = EInspectorValueType::Float;
            OutTypeLabel = TEXT("float");
            return true;
        }
        if (Property->IsA<FDoubleProperty>())
        {
            OutValueType = EInspectorValueType::Double;
            OutTypeLabel = TEXT("double");
            return true;
        }
        if (Property->IsA<FStrProperty>())
        {
            OutValueType = EInspectorValueType::String;
            OutTypeLabel = TEXT("FString");
            return true;
        }
        if (Property->IsA<FNameProperty>())
        {
            OutValueType = EInspectorValueType::Name;
            OutTypeLabel = TEXT("FName");
            return true;
        }

        const UEnum* Enum = nullptr;
        if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            Enum = EnumProperty->GetEnum();
        }
        else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            Enum = ByteProperty->Enum;
        }

        if (!Enum)
        {
            return false;
        }

        OutValueType = EInspectorValueType::Enum;
        OutTypeLabel = Enum->GetName();

        for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
        {
#if WITH_EDITOR
            if (Enum->HasMetaData(TEXT("Hidden"), Index))
            {
                continue;
            }
#endif
            const FString Option = Enum->GetNameStringByIndex(Index);
            if (!Option.EndsWith(TEXT("_MAX")))
            {
                OutEnumOptions.Add(Option);
            }
        }
        return OutEnumOptions.Num() > 0;
    }

    static bool RI_BuildFunctionParameterDefinitions(UFunction* Function, TArray<FRIInspectorFunctionParameterDefinition>& OutDefinitions)
    {
        OutDefinitions.Reset();
        if (!Function)
        {
            return false;
        }

        if (RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("Latent"))
            || RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("WorldContext"))
            || RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("BlueprintInternalUseOnly")))
        {
            return false;
        }

        for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
        {
            FProperty* Property = *It;
            if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm))
            {
                continue;
            }

            if (Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
            {
                return false;
            }
            if (Property->HasAnyPropertyFlags(CPF_ReferenceParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
            {
                return false;
            }

            FRIInspectorFunctionParameterDefinition Definition;
            Definition.Name = Property->GetFName();
            Definition.DisplayName = Property->GetAuthoredName();

            if (!RI_TryGetSupportedFunctionValueType(Property, Definition.ValueType, Definition.TypeLabel, Definition.EnumOptions))
            {
                return false;
            }

            const FString DefaultMetaKey = FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName());
            const FString DefaultValue = RI_GetFunctionMetadataRuntimeSafe(Function, *DefaultMetaKey);
            Definition.bHasDefaultValue = !DefaultValue.IsEmpty();
            Definition.DefaultValueText = DefaultValue;
            OutDefinitions.Add(MoveTemp(Definition));
        }

        return true;
    }

    static FString RI_BuildFunctionSignature(UFunction* Function, const TArray<FRIInspectorFunctionParameterDefinition>& Parameters)
    {
        if (!Function)
        {
            return FString();
        }

        TArray<FString> Parts;
        Parts.Reserve(Parameters.Num());
        for (const FRIInspectorFunctionParameterDefinition& Parameter : Parameters)
        {
            FString Part = FString::Printf(TEXT("%s %s"), *Parameter.TypeLabel, *Parameter.Name.ToString());
            if (Parameter.bHasDefaultValue)
            {
                Part += FString::Printf(TEXT(" = %s"), *Parameter.DefaultValueText);
            }
            Parts.Add(MoveTemp(Part));
        }

        return FString::Printf(TEXT("%s(%s)"), *Function->GetName(), *FString::Join(Parts, TEXT(", ")));
    }

    static bool RI_SetEnumValueFromText(FProperty* Property, void* ValuePtr, const FString& InText, FString& OutError)
    {
        OutError.Reset();

        if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            UEnum* Enum = EnumProperty->GetEnum();
            if (!Enum)
            {
                OutError = TEXT("Enum metadata missing");
                return false;
            }

            int64 Value = Enum->GetValueByNameString(InText);
            if (Value == INDEX_NONE)
            {
                Value = Enum->GetValueByName(FName(*InText));
            }
            if (Value == INDEX_NONE)
            {
                OutError = FString::Printf(TEXT("Invalid enum value '%s'"), *InText);
                return false;
            }

            EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, Value);
            return true;
        }

        if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            UEnum* Enum = ByteProperty->Enum;
            if (!Enum)
            {
                OutError = TEXT("Enum metadata missing");
                return false;
            }

            int64 Value = Enum->GetValueByNameString(InText);
            if (Value == INDEX_NONE)
            {
                Value = Enum->GetValueByName(FName(*InText));
            }
            if (Value == INDEX_NONE)
            {
                OutError = FString::Printf(TEXT("Invalid enum value '%s'"), *InText);
                return false;
            }

            ByteProperty->SetIntPropertyValue(ValuePtr, Value);
            return true;
        }

        OutError = TEXT("Unsupported enum parameter");
        return false;
    }

    static bool RI_AssignFunctionArgument(FProperty* Property, void* ValuePtr, const FString& InText, FString& OutError)
    {
        OutError.Reset();
        if (!Property || !ValuePtr)
        {
            OutError = TEXT("Invalid function argument");
            return false;
        }

        if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            const bool bValue = InText.Equals(TEXT("true"), ESearchCase::IgnoreCase) || InText == TEXT("1");
            BoolProperty->SetPropertyValue(ValuePtr, bValue);
            return true;
        }
        if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
        {
            int32 Value = 0;
            if (!LexTryParseString(Value, *InText))
            {
                OutError = FString::Printf(TEXT("Invalid int32 value '%s'"), *InText);
                return false;
            }
            IntProperty->SetPropertyValue(ValuePtr, Value);
            return true;
        }
        if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
        {
            float Value = 0.f;
            if (!LexTryParseString(Value, *InText))
            {
                OutError = FString::Printf(TEXT("Invalid float value '%s'"), *InText);
                return false;
            }
            FloatProperty->SetPropertyValue(ValuePtr, Value);
            return true;
        }
        if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
        {
            double Value = 0.0;
            if (!LexTryParseString(Value, *InText))
            {
                OutError = FString::Printf(TEXT("Invalid double value '%s'"), *InText);
                return false;
            }
            DoubleProperty->SetPropertyValue(ValuePtr, Value);
            return true;
        }
        if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
        {
            StrProperty->SetPropertyValue(ValuePtr, InText);
            return true;
        }
        if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
        {
            NameProperty->SetPropertyValue(ValuePtr, FName(*InText));
            return true;
        }
        if (Property->IsA<FEnumProperty>() || (CastField<FByteProperty>(Property) && CastField<FByteProperty>(Property)->Enum))
        {
            return RI_SetEnumValueFromText(Property, ValuePtr, InText, OutError);
        }

        OutError = FString::Printf(TEXT("Unsupported parameter type '%s'"), *Property->GetCPPType());
        return false;
    }

    static bool RI_IsUserAuthoredFunctionOwnerClass(const UClass* TargetClass, const UClass* OwnerClass)
    {
        if (!TargetClass || !OwnerClass || !TargetClass->IsChildOf(OwnerClass))
        {
            return false;
        }
        return true;
    }
}

void UInspectorWorldSubsystem::CacheActorPageSearchTextFromPanel()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    if (UWidget* SearchWidget = Panel->WidgetTree->FindWidget(TEXT("ETB_Search")))
    {
        FString SearchText;
        if (RI_TryGetEditableSearchText(SearchWidget, SearchText))
        {
            CurrentActorSearchText = SearchText;
        }
    }
#endif
}

void UInspectorWorldSubsystem::BindActorSearchBox()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    RI_BindEditableSearchTextChanged(Panel->WidgetTree->FindWidget(TEXT("ETB_Search")), this);
#endif
}

void UInspectorWorldSubsystem::HandleActorSearchTextChanged(const FText& InText)
{
#if RUNTIME_INSPECTOR_ENABLED
    CurrentActorSearchText = InText.ToString();
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
#else
    (void)InText;
#endif
}

UObject* UInspectorWorldSubsystem::GetFocusedInspectObject() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (SelectedInspectObject.Get() != nullptr)
    {
        return SelectedInspectObject.Get();
    }
    return SelectedActor.Get();
#else
    return nullptr;
#endif
}

void UInspectorWorldSubsystem::EnsureActorFunctionsSectionInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    return;
#endif
}

void UInspectorWorldSubsystem::EnsureActorWorkbenchBodyInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    UWidget* LegacyBody = Panel->WidgetTree->FindWidget(TEXT("Body"));
    UPanelWidget* ParentPanel = LegacyBody ? Cast<UPanelWidget>(LegacyBody->GetParent()) : nullptr;
    if (!LegacyBody || !ParentPanel)
    {
        return;
    }

    UHorizontalBox* LegacyBodyBox = Cast<UHorizontalBox>(LegacyBody);
    UVerticalBox* LegacyLeft = Cast<UVerticalBox>(Panel->WidgetTree->FindWidget(TEXT("Left")));
    UVerticalBox* LegacyRight = Cast<UVerticalBox>(Panel->WidgetTree->FindWidget(TEXT("Right")));
    UVerticalBox* LegacyPageStack = Cast<UVerticalBox>(ParentPanel);
    if (!LegacyBodyBox || !LegacyLeft || !LegacyRight || !LegacyPageStack)
    {
        return;
    }

    if (UHorizontalBox* PreviousCustomHost = ActorWorkbenchBodyHost.Get())
    {
        if (PreviousCustomHost != LegacyBodyBox)
        {
            if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(PreviousCustomHost->GetParent()))
            {
                ExistingParent->RemoveChild(PreviousCustomHost);
            }
            PreviousCustomHost->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    ActorWorkbenchBodyHostStrong = LegacyBodyBox;
    ActorWorkbenchPageStackHostStrong = LegacyPageStack;
    ActorWorkbenchSidebarHostStrong = LegacyLeft;
    ActorWorkbenchContentHostStrong = LegacyRight;
    ActorWorkbenchBodyHost = LegacyBodyBox;
    ActorWorkbenchPageStackHost = LegacyPageStack;
    ActorWorkbenchSidebarHost = LegacyLeft;
    ActorWorkbenchContentHost = LegacyRight;

    LegacyBody->SetVisibility(ESlateVisibility::Visible);
    if (UHorizontalBoxSlot* BodySlot = Cast<UHorizontalBoxSlot>(LegacyLeft->Slot))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 0.68f;
        BodySlot->SetSize(SizeRule);
        BodySlot->SetHorizontalAlignment(HAlign_Fill);
        BodySlot->SetVerticalAlignment(VAlign_Fill);
        BodySlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }
    if (UHorizontalBoxSlot* BodySlot = Cast<UHorizontalBoxSlot>(LegacyRight->Slot))
    {
        FSlateChildSize SizeRule(ESlateSizeRule::Fill);
        SizeRule.Value = 1.32f;
        BodySlot->SetSize(SizeRule);
        BodySlot->SetHorizontalAlignment(HAlign_Fill);
        BodySlot->SetVerticalAlignment(VAlign_Fill);
        BodySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    }
    LegacyBody->InvalidateLayoutAndVolatility();
    LegacyBody->ForceLayoutPrepass();
    ParentPanel->InvalidateLayoutAndVolatility();
    ParentPanel->ForceLayoutPrepass();

    for (UWidget* Ancestor = ParentPanel; Ancestor; Ancestor = Ancestor->GetParent())
    {
        Ancestor->InvalidateLayoutAndVolatility();
        Ancestor->ForceLayoutPrepass();
    }
#endif
}

void UInspectorWorldSubsystem::EnsureActorWorkspaceSelectionBandInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureActorWorkbenchBodyInjected();
    UUserWidget* Panel = PanelWidget.Get();
    UVerticalBox* PageStackHost = ActorWorkbenchPageStackHost.Get();
    UHorizontalBox* BodyHost = ActorWorkbenchBodyHost.Get();
    if (!Panel || !Panel->WidgetTree || !PageStackHost || !BodyHost)
    {
        return;
    }

    UBorder* SelectionBand = ActorWorkspaceSelectionBand.Get();
    if (!SelectionBand)
    {
        SelectionBand = RICompactUI::MakeSurfaceCard(
            Panel->WidgetTree,
            TEXT("RI_ActorWorkspaceSelectionBand"),
            RICompactUI::GetContextStripBackgroundColor(),
            FMargin(14.f, 6.f));

        UVerticalBox* BandRoot = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorWorkspaceSelectionBandRoot"));
        SelectionBand->SetContent(BandRoot);

        if (UVerticalBoxSlot* LabelSlot = BandRoot->AddChildToVerticalBox(
            RICompactUI::MakeText(Panel->WidgetTree, TEXT("Selection"), RICompactUI::GetMutedFontSize(), true, RICompactUI::GetMutedTextColor())))
        {
            LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
        }

        UHorizontalBox* SummaryRow = Panel->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RI_ActorWorkspaceSelectionSummary"));
        BandRoot->AddChildToVerticalBox(SummaryRow);

        auto AddSummaryText = [SummaryRow](UTextBlock* Text, float FillWeight, const FMargin& Padding)
        {
            if (!SummaryRow || !Text)
            {
                return;
            }

            if (UHorizontalBoxSlot* Slot = SummaryRow->AddChildToHorizontalBox(Text))
            {
                FSlateChildSize SizeRule(ESlateSizeRule::Fill);
                SizeRule.Value = FillWeight;
                Slot->SetSize(SizeRule);
                Slot->SetHorizontalAlignment(HAlign_Fill);
                Slot->SetVerticalAlignment(VAlign_Center);
                Slot->SetPadding(Padding);
            }
        };

        UTextBlock* ActorText = RICompactUI::MakeText(
            Panel->WidgetTree,
            TEXT("No selected actor"),
            RICompactUI::GetValueFontSize(),
            true,
            RICompactUI::GetStrongTextColor(),
            true);
        UTextBlock* SourceText = RICompactUI::MakeText(
            Panel->WidgetTree,
            TEXT("No source asset"),
            RICompactUI::GetValueFontSize(),
            false,
            RICompactUI::GetSecondaryTextColor(),
            true);
        UTextBlock* StateText = RICompactUI::MakeText(
            Panel->WidgetTree,
            TEXT("Live only"),
            RICompactUI::GetValueFontSize(),
            true,
            RICompactUI::GetStrongTextColor(),
            false);

        AddSummaryText(ActorText, 0.90f, FMargin(0.f, 0.f, 14.f, 0.f));
        AddSummaryText(SourceText, 1.40f, FMargin(0.f, 0.f, 14.f, 0.f));
        AddSummaryText(StateText, 0.46f, FMargin(0.f));

        ActorWorkspaceSelectionActorTextStrong = ActorText;
        ActorWorkspaceSelectionActorText = ActorText;
        ActorWorkspaceSelectionSourceTextStrong = SourceText;
        ActorWorkspaceSelectionSourceText = ActorWorkspaceSelectionSourceTextStrong;
        ActorWorkspaceSelectionStateTextStrong = StateText;
        ActorWorkspaceSelectionStateText = ActorWorkspaceSelectionStateTextStrong;

        ActorWorkspaceSelectionBandStrong = SelectionBand;
        ActorWorkspaceSelectionBand = SelectionBand;
    }

    if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(SelectionBand->GetParent()))
    {
        if (ExistingParent != PageStackHost)
        {
            ExistingParent->RemoveChild(SelectionBand);
        }
    }

    const int32 BodyIndex = PageStackHost->GetChildIndex(BodyHost);
    const int32 CurrentIndex = PageStackHost->GetChildIndex(SelectionBand);
    if (SelectionBand->GetParent() != PageStackHost || CurrentIndex == INDEX_NONE || (BodyIndex != INDEX_NONE && CurrentIndex >= BodyIndex))
    {
        if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(SelectionBand->GetParent()))
        {
            ExistingParent->RemoveChild(SelectionBand);
        }

        const int32 DesiredIndex = BodyIndex == INDEX_NONE ? 0 : BodyIndex;
        PageStackHost->InsertChildAt(DesiredIndex, SelectionBand);
    }

    if (UVerticalBoxSlot* BandSlot = Cast<UVerticalBoxSlot>(SelectionBand->Slot))
    {
        BandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BandSlot->SetHorizontalAlignment(HAlign_Fill);
        BandSlot->SetVerticalAlignment(VAlign_Top);
        BandSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    UpdateActorWorkspaceSelectionBand();
#endif
}

void UInspectorWorldSubsystem::UpdateActorWorkspaceSelectionBand()
{
#if RUNTIME_INSPECTOR_ENABLED
    const AActor* Selected = SelectedActor.Get();
    const FString ActorLabel = Selected ? RI_GetActorDisplayLabel(Selected) : TEXT("No selected actor");
    const FString SourcePath = (Selected && Selected->GetClass())
        ? Selected->GetClass()->GetPathName()
        : TEXT("No source asset");
    const FString StagedState = HasStagedPatch()
        ? FString::Printf(TEXT("Staged (%d ops)"), GetStagedPatch().Operations.Num())
        : TEXT("Live only");

    if (UTextBlock* Text = ActorWorkspaceSelectionActorText.Get())
    {
        Text->SetText(FText::FromString(ActorLabel));
        RICompactUI::ApplyTextStyle(Text, RICompactUI::GetValueFontSize(), true, Selected ? RICompactUI::GetStrongTextColor() : RICompactUI::GetMutedTextColor());
    }
    if (UTextBlock* Text = ActorWorkspaceSelectionSourceText.Get())
    {
        Text->SetText(FText::FromString(SourcePath));
        RICompactUI::ApplyTextStyle(Text, RICompactUI::GetValueFontSize(), false, Selected ? RICompactUI::GetSecondaryTextColor() : RICompactUI::GetMutedTextColor());
    }
    if (UTextBlock* Text = ActorWorkspaceSelectionStateText.Get())
    {
        Text->SetText(FText::FromString(StagedState));
        RICompactUI::ApplyTextStyle(
            Text,
            RICompactUI::GetValueFontSize(),
            true,
            HasStagedPatch() ? RICompactUI::GetSuccessTextColor() : RICompactUI::GetStrongTextColor());
    }
#endif
}

void UInspectorWorldSubsystem::EnsureActorGroupsSectionInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureActorWorkbenchBodyInjected();
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    USizeBox* HostBox = ActorGroupsSectionHostBox.Get();
    if (!HostBox)
    {
        HostBox = Panel->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorGroupsHost"));
        HostBox->SetMinDesiredWidth(248.0f);
        HostBox->ClearHeightOverride();

        UVerticalBox* RootBox = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorGroupsRoot"));
        HostBox->SetContent(RootBox);

        UBorder* ComponentBorder = Panel->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorGroupsComponentBorder"));
        ComponentBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
        ComponentBorder->SetBrushColor(RICompactUI::GetSectionSurfaceBackgroundColor());

        UVerticalBox* ComponentBox = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorGroupsComponentBox"));
        ComponentBorder->SetContent(ComponentBox);

        if (UVerticalBoxSlot* HeaderSlot = ComponentBox->AddChildToVerticalBox(
            RICompactUI::MakeSectionTitle(Panel->WidgetTree, TEXT("Component"), RICompactUI::ERISectionVisualStyle::Emphasis)))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
        }

        UScrollBox* GroupsScroll = Panel->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorGroupsScroll"));
        UVerticalBox* EntriesBox = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorGroupsEntries"));
        GroupsScroll->AddChild(EntriesBox);

        USizeBox* ComponentBody = Panel->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorGroupsBody"));
        ComponentBody->SetMinDesiredHeight(220.0f);
        ComponentBody->ClearHeightOverride();
        ComponentBody->SetContent(GroupsScroll);
        if (UVerticalBoxSlot* BodySlot = ComponentBox->AddChildToVerticalBox(ComponentBody))
        {
            FSlateChildSize BodySize(ESlateSizeRule::Fill);
            BodySize.Value = 1.0f;
            BodySlot->SetSize(BodySize);
            BodySlot->SetHorizontalAlignment(HAlign_Fill);
            BodySlot->SetVerticalAlignment(VAlign_Fill);
        }

        if (UVerticalBoxSlot* ComponentSlot = RootBox->AddChildToVerticalBox(ComponentBorder))
        {
            FSlateChildSize SectionSize(ESlateSizeRule::Fill);
            SectionSize.Value = 0.64f;
            ComponentSlot->SetSize(SectionSize);
            ComponentSlot->SetHorizontalAlignment(HAlign_Fill);
            ComponentSlot->SetVerticalAlignment(VAlign_Fill);
            ComponentSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
        }

        UBorder* PinnedBorder = Panel->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_ActorPinnedBorder"));
        PinnedBorder->SetPadding(RICompactUI::GetSurfaceCardPadding(true));
        PinnedBorder->SetBrushColor(RICompactUI::GetSectionSurfaceBackgroundColor());

        UVerticalBox* PinnedBox = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPinnedBox"));
        PinnedBorder->SetContent(PinnedBox);

        if (UVerticalBoxSlot* HeaderSlot = PinnedBox->AddChildToVerticalBox(
            RICompactUI::MakeSectionTitle(Panel->WidgetTree, TEXT("Star"), RICompactUI::ERISectionVisualStyle::Emphasis)))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
        }

        UScrollBox* PinnedScroll = Panel->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RI_ActorPinnedScroll"));
        UVerticalBox* PinnedEntries = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPinnedEntries"));
        PinnedScroll->AddChild(PinnedEntries);
        USizeBox* PinnedBody = Panel->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RI_ActorPinnedBody"));
        PinnedBody->SetMinDesiredHeight(132.0f);
        PinnedBody->ClearHeightOverride();
        PinnedBody->SetContent(PinnedScroll);
        if (UVerticalBoxSlot* BodySlot = PinnedBox->AddChildToVerticalBox(PinnedBody))
        {
            FSlateChildSize BodySize(ESlateSizeRule::Fill);
            BodySize.Value = 1.0f;
            BodySlot->SetSize(BodySize);
            BodySlot->SetHorizontalAlignment(HAlign_Fill);
            BodySlot->SetVerticalAlignment(VAlign_Fill);
        }

        if (UVerticalBoxSlot* PinnedSlot = RootBox->AddChildToVerticalBox(PinnedBorder))
        {
            FSlateChildSize SectionSize(ESlateSizeRule::Fill);
            SectionSize.Value = 0.36f;
            PinnedSlot->SetSize(SectionSize);
            PinnedSlot->SetHorizontalAlignment(HAlign_Fill);
            PinnedSlot->SetVerticalAlignment(VAlign_Fill);
        }

        ActorGroupsSectionHostBoxStrong = HostBox;
        ActorGroupsSectionHostBox = HostBox;
        ActorGroupsScrollBoxStrong = GroupsScroll;
        ActorGroupsEntriesBoxStrong = EntriesBox;
        ActorPinnedEntriesBoxStrong = PinnedEntries;
    }

    if (UVerticalBox* SidebarHost = ActorWorkbenchSidebarHost.Get())
    {
        if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(HostBox->GetParent()))
        {
            if (ExistingParent != SidebarHost)
            {
                ExistingParent->RemoveChild(HostBox);
            }
        }

        if (HostBox->GetParent() != SidebarHost)
        {
            SidebarHost->AddChildToVerticalBox(HostBox);
        }

        if (UVerticalBoxSlot* SidebarSlot = Cast<UVerticalBoxSlot>(HostBox->Slot))
        {
            FSlateChildSize SidebarSize(ESlateSizeRule::Fill);
            SidebarSize.Value = 1.0f;
            SidebarSlot->SetSize(SidebarSize);
            SidebarSlot->SetHorizontalAlignment(HAlign_Fill);
            SidebarSlot->SetVerticalAlignment(VAlign_Fill);
            SidebarSlot->SetPadding(FMargin(0.f));
        }

        HostBox->SetVisibility(ESlateVisibility::Visible);
        SidebarHost->InvalidateLayoutAndVolatility();
        SidebarHost->ForceLayoutPrepass();
    }
    else
    {
        UWidgetSwitcher* GroupSwitcher = Cast<UWidgetSwitcher>(Panel->WidgetTree->FindWidget(TEXT("WS_Group")));
        if (!GroupSwitcher)
        {
            return;
        }

        if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(HostBox->GetParent()))
        {
            if (ExistingParent != GroupSwitcher)
            {
                ExistingParent->RemoveChild(HostBox);
            }
        }

        if (HostBox->GetParent() != GroupSwitcher)
        {
            GroupSwitcher->AddChild(HostBox);
        }

        HostBox->SetVisibility(ESlateVisibility::Visible);
        GroupSwitcher->SetActiveWidget(HostBox);
        if (UWidgetSwitcherSlot* SwitcherSlot = Cast<UWidgetSwitcherSlot>(HostBox->Slot))
        {
            SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
            SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
            SwitcherSlot->SetPadding(FMargin(0.f));
        }
        GroupSwitcher->ForceLayoutPrepass();
    }

    if (UWidget* GroupList = Panel->WidgetTree->FindWidget(TEXT("LV_Group")))
    {
        GroupList->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* TreeList = Panel->WidgetTree->FindWidget(TEXT("LV_TreeGroup")))
    {
        TreeList->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* PinList = Panel->WidgetTree->FindWidget(TEXT("LV_Pin")))
    {
        PinList->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* StarLabel = Panel->WidgetTree->FindWidget(TEXT("Txt_Star")))
    {
        StarLabel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* StarLabelUpper = Panel->WidgetTree->FindWidget(TEXT("TXT_Star")))
    {
        StarLabelUpper->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* LegacyComponentHeader = Panel->WidgetTree->FindWidget(TEXT("TextBlock_134")))
    {
        LegacyComponentHeader->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* LegacyGroupsBorder = Panel->WidgetTree->FindWidget(TEXT("Border_1081")))
    {
        LegacyGroupsBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UWidget* LegacyPinnedBorder = Panel->WidgetTree->FindWidget(TEXT("Border_946")))
    {
        LegacyPinnedBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
#endif
}

void UInspectorWorldSubsystem::RefreshActorGroupsSection()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    EnsureActorGroupsSectionInjected();

    UVerticalBox* EntriesBox = ActorGroupsEntriesBoxStrong;
    UVerticalBox* PinnedEntriesBox = ActorPinnedEntriesBoxStrong;
    USizeBox* HostBox = ActorGroupsSectionHostBox.Get();
    UScrollBox* GroupsScroll = ActorGroupsScrollBoxStrong;
    if (!EntriesBox || !PinnedEntriesBox || !HostBox)
    {
        return;
    }

    EntriesBox->ClearChildren();
    PinnedEntriesBox->ClearChildren();
    ActorGroupsClickProxies.Reset();
    ActorPinnedClickProxies.Reset();

    const FString SearchText = GetCurrentActorSearchText();

    TArray<UObject*> RootObjects;
    GetGroupTreeRootsForSelected(SearchText, RootObjects);

    auto AddEmptyStateCard = [Panel](UVerticalBox* TargetBox, const FString& Message)
    {
        if (!Panel || !Panel->WidgetTree || !TargetBox)
        {
            return;
        }

        UBorder* EmptyBorder = Panel->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        EmptyBorder->SetPadding(FMargin(8.f, 10.f));
        EmptyBorder->SetBrushColor(RICompactUI::GetRowSurfaceBackgroundColor());
        EmptyBorder->SetContent(
            RICompactUI::MakeText(
                Panel->WidgetTree,
                Message,
                RICompactUI::GetMutedFontSize(),
                false,
                RICompactUI::GetMutedTextColor(),
                true));

        if (UVerticalBoxSlot* EmptySlot = TargetBox->AddChildToVerticalBox(EmptyBorder))
        {
            EmptySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }
    };

    TArray<UInspectorGroupItem*> FlatItems;
    TFunction<void(UInspectorGroupItem*)> AppendItemRecursive = [&](UInspectorGroupItem* Item)
    {
        if (!Item)
        {
            return;
        }

        if (Item->StableKey != TEXT("PINNED_ROOT"))
        {
            FlatItems.Add(Item);
        }

        if (!Item->bExpanded)
        {
            return;
        }

        TArray<UObject*> ChildObjects;
        GetGroupTreeChildrenForItem(Item, SearchText, ChildObjects);
        for (UObject* ChildObject : ChildObjects)
        {
            if (UInspectorGroupItem* ChildItem = Cast<UInspectorGroupItem>(ChildObject))
            {
                AppendItemRecursive(ChildItem);
            }
        }
    };

    for (UObject* RootObject : RootObjects)
    {
        if (UInspectorGroupItem* RootItem = Cast<UInspectorGroupItem>(RootObject))
        {
            if (RootItem->StableKey == TEXT("PINNED_ROOT"))
            {
                continue;
            }
            AppendItemRecursive(RootItem);
        }
    }

    if (FlatItems.Num() == 0)
    {
        AddEmptyStateCard(EntriesBox, TEXT("No components available."));
    }
    else
    {
        for (UInspectorGroupItem* Item : FlatItems)
        {
            if (!Item)
            {
                continue;
            }

            UButton* RowButton = Panel->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
            UHorizontalBox* RowBox = Panel->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            if (!RowButton || !RowBox)
            {
                continue;
            }

            RowButton->SetClickMethod(EButtonClickMethod::MouseDown);
            RowButton->SetBackgroundColor(RICompactUI::GetRowSurfaceBackgroundColor());
            RowButton->AddChild(RowBox);

            const FString Expander = RI_GroupItemCanExpandForActorPanel(Item) ? (Item->bExpanded ? TEXT("v") : TEXT(">")) : TEXT(" ");
            if (UHorizontalBoxSlot* ExpanderSlot = RowBox->AddChildToHorizontalBox(
                RICompactUI::MakeText(Panel->WidgetTree, Expander, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetMutedTextColor(), false)))
            {
                ExpanderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                ExpanderSlot->SetVerticalAlignment(VAlign_Center);
                ExpanderSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
            }

            if (UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(
                RICompactUI::MakeText(Panel->WidgetTree, Item->DisplayName, RICompactUI::GetLabelFontSize(), true, RICompactUI::GetStrongTextColor(), true)))
            {
                NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                NameSlot->SetVerticalAlignment(VAlign_Center);
            }

            if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RowBox->Slot))
            {
                const float Indent = 6.0f + static_cast<float>(FMath::Max(0, Item->Depth)) * 12.0f;
                ButtonSlot->SetPadding(FMargin(Indent, 4.f, 6.f, 4.f));
                ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
                ButtonSlot->SetVerticalAlignment(VAlign_Fill);
            }

            UInspectorGroupButtonProxy* Proxy = NewObject<UInspectorGroupButtonProxy>(this);
            Proxy->Initialize(this, Item);
            RowButton->OnClicked.AddDynamic(Proxy, &UInspectorGroupButtonProxy::HandleClicked);
            ActorGroupsClickProxies.Add(Proxy);

            if (UVerticalBoxSlot* EntrySlot = EntriesBox->AddChildToVerticalBox(RowButton))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
            }
        }
    }

    TArray<UObject*> PinnedItems;
    GetPinnedItemsForSelected(SearchText, PinnedItems);
    if (PinnedItems.Num() == 0)
    {
        AddEmptyStateCard(PinnedEntriesBox, TEXT("No starred items yet."));
    }
    else
    {
        for (UObject* ItemObject : PinnedItems)
        {
            UWidget* RowWidget = nullptr;
            if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
            {
                UInspectorPropertyRowWidget* PropertyRow = Panel->WidgetTree->ConstructWidget<UInspectorPropertyRowWidget>(
                    UInspectorPropertyRowWidget::StaticClass());
                if (PropertyRow)
                {
                    PropertyRow->SetInspectorSubsystem(this);
                    PropertyRow->SetPropertyItem(PropertyItem);
                    PropertyRow->SetAllowNavigation(true);
                    RowWidget = PropertyRow;
                }
            }
            else if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
            {
                UInspectorMaterialParamRowWidget* MaterialRow = Panel->WidgetTree->ConstructWidget<UInspectorMaterialParamRowWidget>(
                    UInspectorMaterialParamRowWidget::StaticClass());
                if (MaterialRow)
                {
                    MaterialRow->SetInspectorSubsystem(this);
                    MaterialRow->SetMaterialItem(MaterialItem);
                    MaterialRow->SetAllowNavigation(true);
                    RowWidget = MaterialRow;
                }
            }
            else if (UInspectorFunctionItem* FunctionItem = Cast<UInspectorFunctionItem>(ItemObject))
            {
                UInspectorFunctionRowWidget* FunctionRow = Panel->WidgetTree->ConstructWidget<UInspectorFunctionRowWidget>(
                    UInspectorFunctionRowWidget::StaticClass());
                if (FunctionRow)
                {
                    FunctionRow->SetInspectorSubsystem(this);
                    FunctionRow->SetFunctionItem(FunctionItem);
                    FunctionRow->SetAllowNavigation(true);
                    RowWidget = FunctionRow;
                }
            }

            if (!RowWidget)
            {
                continue;
            }

            if (UVerticalBoxSlot* EntrySlot = PinnedEntriesBox->AddChildToVerticalBox(RowWidget))
            {
                EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
                EntrySlot->SetHorizontalAlignment(HAlign_Fill);
            }
        }
    }

    EntriesBox->InvalidateLayoutAndVolatility();
    PinnedEntriesBox->InvalidateLayoutAndVolatility();
    if (GroupsScroll)
    {
        GroupsScroll->SetScrollOffset(0.0f);
        if (EntriesBox->GetChildrenCount() > 0)
        {
            GroupsScroll->ScrollWidgetIntoView(EntriesBox->GetChildAt(0), true, EDescendantScrollDestination::TopOrLeft, 0.0f);
        }
        GroupsScroll->InvalidateLayoutAndVolatility();
    }
    if (HostBox)
    {
        HostBox->InvalidateLayoutAndVolatility();
        HostBox->ForceLayoutPrepass();
    }
#endif
}

void UInspectorWorldSubsystem::EnsureActorPropertiesSectionInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureActorWorkbenchBodyInjected();
    EnsureActorWorkspaceSelectionBandInjected();
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get();
    if (!SectionWidget)
    {
        if (APlayerController* PC = GetLocalPC())
        {
            SectionWidget = CreateWidget<UInspectorPropertiesSectionWidget>(PC, UInspectorPropertiesSectionWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            SectionWidget = CreateWidget<UInspectorPropertiesSectionWidget>(World, UInspectorPropertiesSectionWidget::StaticClass());
        }

        if (!SectionWidget)
        {
            return;
        }

        ActorPropertiesSectionWidgetStrong = SectionWidget;
        ActorPropertiesSectionWidget = SectionWidget;
    }

    UInspectorFunctionsSectionWidget* FunctionWidget = ActorFunctionsSectionWidget.Get();
    if (!FunctionWidget)
    {
        if (APlayerController* PC = GetLocalPC())
        {
            FunctionWidget = CreateWidget<UInspectorFunctionsSectionWidget>(PC, UInspectorFunctionsSectionWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            FunctionWidget = CreateWidget<UInspectorFunctionsSectionWidget>(World, UInspectorFunctionsSectionWidget::StaticClass());
        }

        if (!FunctionWidget)
        {
            return;
        }

        ActorFunctionsSectionWidgetStrong = FunctionWidget;
        ActorFunctionsSectionWidget = FunctionWidget;
    }

    UVerticalBox* HostBox = ActorPropertyFunctionHostBox.Get();
    if (!HostBox)
    {
        HostBox = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_ActorPropertyFunctionHost"));
        ActorPropertyFunctionHostBoxStrong = HostBox;
        ActorPropertyFunctionHostBox = HostBox;
    }

    SectionWidget->SetInspectorSubsystem(this);
    FunctionWidget->SetInspectorSubsystem(this);
    SectionWidget->TakeWidget();
    FunctionWidget->TakeWidget();

    auto AttachChildToHost = [HostBox, SectionWidget](UWidget* ChildWidget)
    {
        if (!HostBox || !ChildWidget)
        {
            return;
        }

        if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(ChildWidget->GetParent()))
        {
            if (ExistingParent != HostBox)
            {
                ExistingParent->RemoveChild(ChildWidget);
            }
        }

        if (ChildWidget->GetParent() != HostBox)
        {
            HostBox->AddChildToVerticalBox(ChildWidget);
        }

        if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(ChildWidget->Slot))
        {
            FSlateChildSize SizeRule(ESlateSizeRule::Fill);
            SizeRule.Value = ChildWidget == SectionWidget ? 0.44f : 0.56f;
            VerticalSlot->SetSize(SizeRule);
            VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
            VerticalSlot->SetVerticalAlignment(VAlign_Fill);
            VerticalSlot->SetPadding(ChildWidget == SectionWidget ? FMargin(0.f, 0.f, 0.f, 8.f) : FMargin(0.f));
        }
    };

    AttachChildToHost(SectionWidget);
    AttachChildToHost(FunctionWidget);

    if (UVerticalBox* ContentHost = ActorWorkbenchContentHost.Get())
    {
        EnsureActorWorkspaceSelectionBandInjected();
        if (UWidget* LegacyPropertyHeader = Panel->WidgetTree->FindWidget(TEXT("TextBlock")))
        {
            LegacyPropertyHeader->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (UWidget* LegacyFocusText = Panel->WidgetTree->FindWidget(TEXT("TXT_SelectCompName")))
        {
            LegacyFocusText->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (UWidget* LegacyPropertiesBorder = Panel->WidgetTree->FindWidget(TEXT("Border_1218")))
        {
            LegacyPropertiesBorder->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(HostBox->GetParent()))
        {
            if (ExistingParent != ContentHost)
            {
                ExistingParent->RemoveChild(HostBox);
            }
        }

        if (HostBox->GetParent() != ContentHost)
        {
            ContentHost->AddChildToVerticalBox(HostBox);
        }

        if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(HostBox->Slot))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Fill);
            VBoxSlot->SetPadding(FMargin(0.f));
        }

        ContentHost->InvalidateLayoutAndVolatility();
        ContentHost->ForceLayoutPrepass();
    }
    else
    {
        UWidget* AnchorWidget = Panel->WidgetTree->FindWidget(TEXT("LV_Properties"));
        UPanelWidget* HostPanel = nullptr;
        if (!RI_FindFunctionHostPanel(Panel, AnchorWidget, HostPanel))
        {
            return;
        }

        UWidget* DirectAnchorChild = RI_FindDirectChildUnderHost(AnchorWidget, HostPanel);
        if (DirectAnchorChild)
        {
            DirectAnchorChild->SetVisibility(ESlateVisibility::Collapsed);
        }
        else if (AnchorWidget)
        {
            AnchorWidget->SetVisibility(ESlateVisibility::Collapsed);
        }

        const bool bNeedsReparent = HostBox->GetParent() != HostPanel;
        const int32 DesiredIndex = DirectAnchorChild ? HostPanel->GetChildIndex(DirectAnchorChild) : HostPanel->GetChildrenCount();
        const int32 CurrentIndex = HostPanel->GetChildIndex(HostBox);

        if (bNeedsReparent || (CurrentIndex != INDEX_NONE && CurrentIndex != DesiredIndex))
        {
            if (UPanelWidget* ExistingParent = Cast<UPanelWidget>(HostBox->GetParent()))
            {
                ExistingParent->RemoveChild(HostBox);
            }

            HostPanel->InsertChildAt(DesiredIndex, HostBox);
            if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(HostBox->Slot))
            {
                VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
            }
        }
    }

    HostBox->SetVisibility(ESlateVisibility::Visible);
    SectionWidget->SetVisibility(ESlateVisibility::Visible);
    FunctionWidget->SetVisibility(ESlateVisibility::Visible);
    HostBox->InvalidateLayoutAndVolatility();
    HostBox->ForceLayoutPrepass();
    SectionWidget->InvalidateLayoutAndVolatility();
    FunctionWidget->InvalidateLayoutAndVolatility();
#endif
}

void UInspectorWorldSubsystem::RefreshActorPropertiesSection()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get())
    {
        bool bOnlyModified = false;
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            if (Panel->WidgetTree)
            {
                if (UCheckBox* OnlyModifyToggle = Cast<UCheckBox>(Panel->WidgetTree->FindWidget(TEXT("Toggle_OnModify"))))
                {
                    bOnlyModified = OnlyModifyToggle->IsChecked();
                }
            }
        }

        SectionWidget->SetOnlyModified(bOnlyModified);
        SectionWidget->RefreshFromSubsystem();
    }
#endif
}

void UInspectorWorldSubsystem::RefreshActorFunctionsSection()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UInspectorFunctionsSectionWidget* SectionWidget = ActorFunctionsSectionWidget.Get())
    {
        SectionWidget->RefreshFromSubsystem();
    }
#endif
}

void UInspectorWorldSubsystem::GetFunctionItemsForSelected(const FString& SearchText, TArray<UInspectorFunctionItem*>& OutItems)
{
    OutItems.Reset();

#if RUNTIME_INSPECTOR_ENABLED
    if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
    {
        return;
    }

    auto AppendFunctionsForTarget = [this, &SearchText, &OutItems](UObject* TargetObject)
    {
        if (!TargetObject)
        {
            return;
        }

        UClass* TargetClass = TargetObject->GetClass();
        if (!TargetClass)
        {
            return;
        }

        TSet<FName> AddedFunctions;
        const FString OwnerLabel = TargetObject == SelectedActor.Get()
            ? TEXT("Actor")
            : TargetObject->GetName();

        for (TFieldIterator<UFunction> It(TargetClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
        {
            UFunction* Function = *It;
            if (!Function || AddedFunctions.Contains(Function->GetFName()))
            {
                continue;
            }

            if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) || Function->HasAnyFunctionFlags(FUNC_Static | FUNC_Delegate | FUNC_MulticastDelegate | FUNC_BlueprintPure))
            {
                continue;
            }

            const UClass* OwnerClass = Function->GetOwnerClass();
            if (!RI_IsUserAuthoredFunctionOwnerClass(TargetClass, OwnerClass))
            {
                continue;
            }

            const FString FunctionName = Function->GetName();
            if (FunctionName.StartsWith(TEXT("Receive"))
                || FunctionName.StartsWith(TEXT("K2_"))
                || FunctionName.StartsWith(TEXT("ExecuteUbergraph"))
                || RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("DeprecatedFunction"))
                || RI_FunctionHasMetadataRuntimeSafe(Function, TEXT("BlueprintInternalUseOnly")))
            {
                continue;
            }

            TArray<FRIInspectorFunctionParameterDefinition> ParameterDefinitions;
            if (!RI_BuildFunctionParameterDefinitions(Function, ParameterDefinitions))
            {
                continue;
            }

            const FString SearchHaystack = RI_GetFunctionDisplayNameRuntimeSafe(Function) + TEXT(" ") + FunctionName;
            if (!SearchText.IsEmpty() && !SearchHaystack.Contains(SearchText, ESearchCase::IgnoreCase))
            {
                continue;
            }

            UInspectorFunctionItem* Item = GetOrCreateFunctionItem(TargetObject, Function->GetFName());
            if (!Item)
            {
                continue;
            }

            Item->SetDisplayMetadata(
                RI_GetFunctionDisplayNameRuntimeSafe(Function),
                OwnerLabel,
                RI_BuildFunctionSignature(Function, ParameterDefinitions),
                RI_GetFunctionTooltipRuntimeSafe(Function));
            Item->SetParameterDefinitions(ParameterDefinitions);
            OutItems.Add(Item);
            AddedFunctions.Add(Function->GetFName());
        }
    };

    UObject* TargetObject = GetFocusedInspectObject();
    AppendFunctionsForTarget(TargetObject);

    if (OutItems.Num() == 0 && TargetObject && TargetObject != SelectedActor.Get())
    {
        AppendFunctionsForTarget(SelectedActor.Get());
    }

    OutItems.Sort([](const UInspectorFunctionItem& Left, const UInspectorFunctionItem& Right)
    {
        return Left.GetDisplayName() < Right.GetDisplayName();
    });
#endif
}

bool UInspectorWorldSubsystem::InvokeFunctionItem(UInspectorFunctionItem* Item, const TArray<FString>& InArgTexts, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!Item)
    {
        OutError = TEXT("Function item invalid");
        return false;
    }

    UObject* TargetObject = Item->GetTargetObject();
    if (!TargetObject)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UFunction* Function = TargetObject->FindFunction(Item->GetFunctionFName());
    if (!Function)
    {
        OutError = FString::Printf(TEXT("Function '%s' not found"), *Item->GetFunctionName());
        return false;
    }

    TArray<FProperty*> InputProperties;
    for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        FProperty* Property = *It;
        if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm))
        {
            continue;
        }
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
        {
            OutError = TEXT("Function has unsupported return or out parameters");
            return false;
        }
        InputProperties.Add(Property);
    }

    if (InputProperties.Num() != InArgTexts.Num())
    {
        OutError = FString::Printf(TEXT("Expected %d arguments, got %d"), InputProperties.Num(), InArgTexts.Num());
        return false;
    }

    FStructOnScope ParamScope(Function);
    uint8* ParamBuffer = ParamScope.GetStructMemory();
    Function->InitializeStruct(ParamBuffer);

    for (int32 ArgIndex = 0; ArgIndex < InputProperties.Num(); ++ArgIndex)
    {
        FProperty* Property = InputProperties[ArgIndex];
        void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ParamBuffer);
        if (!RI_AssignFunctionArgument(Property, ValuePtr, InArgTexts[ArgIndex], OutError))
        {
            OutError = FString::Printf(TEXT("%s (%s)"), *OutError, *Property->GetName());
            return false;
        }
    }

    TargetObject->ProcessEvent(Function, ParamBuffer);

    const FString TargetLabel = TargetObject->GetName();
    const FString Summary = FString::Printf(TEXT("%s.%s"), *TargetLabel, *Function->GetName());
    AppendActivityLog(ERIToastType::Success, TEXT("Function"), Summary);
    PushToast(ERIToastType::Success, FString::Printf(TEXT("Ran %s"), *Function->GetName()), 1.2f);
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    return true;
#endif
}

void UInspectorWorldSubsystem::EnsureSettingsPageInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }
    if (!Switcher)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to find WS_Content widget switcher for settings page."));
        return;
    }

    UPanelWidget* HostPanel = SettingsHostPanel.Get();
    if (!HostPanel)
    {
        HostPanel = FindSettingsHostPanel();
        SettingsHostPanel = HostPanel;
    }
    if (!HostPanel)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to find Settings host panel inside WS_Content."));
        return;
    }

    SettingsPageIndex = Switcher->GetChildIndex(Cast<UWidget>(HostPanel));
    if (SettingsPageIndex == INDEX_NONE)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Settings host panel is not a direct child of WS_Content."));
        return;
    }

    UInspectorSettingsPageWidget* Page = SettingsPageWidget.Get();
    if (!Page)
    {
        if (APlayerController* PC = GetLocalPC())
        {
            Page = CreateWidget<UInspectorSettingsPageWidget>(PC, UInspectorSettingsPageWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            Page = CreateWidget<UInspectorSettingsPageWidget>(World, UInspectorSettingsPageWidget::StaticClass());
        }
        if (!Page)
        {
            UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to create the settings page widget."));
            return;
        }
        SettingsPageWidgetStrong = Page;
        SettingsPageWidget = Page;
    }

    Page->SetInspectorSubsystem(this);
    Page->SetVisibility(ESlateVisibility::Visible);

    MountPageAsExclusiveChild(HostPanel, Page);

    if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Page->Slot))
    {
        VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
        VerticalSlot->SetVerticalAlignment(VAlign_Fill);
        VerticalSlot->SetPadding(FMargin(0.f));
    }
#endif
}

void UInspectorWorldSubsystem::EnsureFilePageInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel || !Panel->WidgetTree)
    {
        return;
    }

    UPanelWidget* HostPanel = FileHostPanel.Get();
    if (!HostPanel)
    {
        HostPanel = FindFileHostPanel();
        FileHostPanel = HostPanel;
    }
    if (!HostPanel)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to find Data host panel inside WS_Content."));
        return;
    }

    UInspectorFilePageWidget* Page = FilePageWidget.Get();
    if (!Page)
    {
        if (APlayerController* PC = GetLocalPC())
        {
            Page = CreateWidget<UInspectorFilePageWidget>(PC, UInspectorFilePageWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            Page = CreateWidget<UInspectorFilePageWidget>(World, UInspectorFilePageWidget::StaticClass());
        }
        if (!Page)
        {
            UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to create the file page widget."));
            return;
        }
        FilePageWidgetStrong = Page;
        FilePageWidget = Page;
    }

    Page->SetInspectorSubsystem(this);
    Page->SetVisibility(ESlateVisibility::Visible);

    MountPageAsExclusiveChild(HostPanel, Page);

    if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Page->Slot))
    {
        VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
        VerticalSlot->SetVerticalAlignment(VAlign_Fill);
        VerticalSlot->SetPadding(FMargin(0.f));
    }
#endif
}

void UInspectorWorldSubsystem::ShowSettingsPage()
{
#if RUNTIME_INSPECTOR_ENABLED
    const double StartSeconds = FPlatformTime::Seconds();
    EnsureSettingsPageInjected();
    if (UInspectorSettingsPageWidget* Page = SettingsPageWidget.Get())
    {
        Page->TakeWidget();
        Page->SetVisibility(ESlateVisibility::Visible);
        SetContentSwitcherIndex(SettingsPageIndex);
        Page->ScheduleDeferredRefresh();
    }
    UpdateSharedContextStrip();
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] ShowSettingsPage %.2f ms"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
#endif
}

void UInspectorWorldSubsystem::EnsureTestPageInjected()
{
#if RUNTIME_INSPECTOR_ENABLED
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }
    if (!Switcher)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to find WS_Content widget switcher for test page."));
        return;
    }

    UPanelWidget* HostPanel = TestHostPanel.Get();
    if (!HostPanel)
    {
        HostPanel = FindTestHostPanel();
        TestHostPanel = HostPanel;
    }
    if (!HostPanel)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to find Test host panel inside WS_Content."));
        return;
    }

    TestPageIndex = Switcher->GetChildIndex(Cast<UWidget>(HostPanel));
    if (TestPageIndex == INDEX_NONE)
    {
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Test host panel is not a direct child of WS_Content."));
        return;
    }

    UInspectorTestPageWidget* Page = TestPageWidget.Get();
    if (!Page)
    {
        if (APlayerController* PC = GetLocalPC())
        {
            Page = CreateWidget<UInspectorTestPageWidget>(PC, UInspectorTestPageWidget::StaticClass());
        }
        else if (UWorld* World = GetWorld())
        {
            Page = CreateWidget<UInspectorTestPageWidget>(World, UInspectorTestPageWidget::StaticClass());
        }
        if (!Page)
        {
            UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] Failed to create the test page widget."));
            return;
        }
        TestPageWidgetStrong = Page;
        TestPageWidget = Page;
    }

    Page->SetInspectorSubsystem(this);
    Page->SetVisibility(ESlateVisibility::Visible);

    MountPageAsExclusiveChild(HostPanel, Page);

    if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Page->Slot))
    {
        VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
        VerticalSlot->SetVerticalAlignment(VAlign_Fill);
        VerticalSlot->SetPadding(FMargin(0.f));
    }
#endif
}

void UInspectorWorldSubsystem::ShowFilePage()
{
#if RUNTIME_INSPECTOR_ENABLED
    const double StartSeconds = FPlatformTime::Seconds();
    EnsureFilePageInjected();
    SetContentSwitcherIndex(1);
    if (UInspectorFilePageWidget* Page = FilePageWidget.Get())
    {
        Page->TakeWidget();
        Page->RefreshFastFromSubsystem();
        Page->ScheduleDeferredRefresh(false, true);
    }
    UpdateSharedContextStrip();
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] ShowFilePage %.2f ms"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
#endif
}

void UInspectorWorldSubsystem::ShowTestPage()
{
#if RUNTIME_INSPECTOR_ENABLED
    EnsureTestPageInjected();
    if (UInspectorTestPageWidget* Page = TestPageWidget.Get())
    {
        Page->RefreshFromSubsystem();
        Page->SetVisibility(ESlateVisibility::Visible);
        SetContentSwitcherIndex(TestPageIndex);
    }
    UpdateSharedContextStrip();
#endif
}

void UInspectorWorldSubsystem::HideSettingsPage()
{
#if RUNTIME_INSPECTOR_ENABLED
    SetContentSwitcherIndex(0);
#endif
}

void UInspectorWorldSubsystem::HandleActorTabClicked()
{
    RestoreMountedHostPanelVisibility();
    SetContentSwitcherIndex(0);

#if RUNTIME_INSPECTOR_ENABLED
    auto HideMountedPage = [](UUserWidget* PageWidget)
    {
        if (!PageWidget)
        {
            return;
        }

        PageWidget->SetVisibility(ESlateVisibility::Collapsed);
        if (PageWidget->GetParent())
        {
            PageWidget->RemoveFromParent();
        }
    };

    HideMountedPage(FilePageWidget.Get());
    HideMountedPage(SettingsPageWidget.Get());
    HideMountedPage(TestPageWidget.Get());

    RefreshActorGroupsSection();
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();

    if (!bHasCompletedInitialActorPanelRefresh || !SelectedActor.IsValid())
    {
        ScheduleDeferredOpenActorRefresh(!SelectedActor.IsValid());
    }
#endif
}

void UInspectorWorldSubsystem::HandleFileTabClicked()
{
    ShowFilePage();
}

void UInspectorWorldSubsystem::HandleSettingsTabClicked()
{
    ShowSettingsPage();
}

void UInspectorWorldSubsystem::HandleTestTabClicked()
{
    ShowTestPage();
}

FRIEditableSettings UInspectorWorldSubsystem::GetEditableSettings() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return RI_MakeEditableSettings(GetDefault<URuntimeInspectorSettings>());
#else
    return FRIEditableSettings();
#endif
}

ERuntimeInspectorThemePreset UInspectorWorldSubsystem::GetThemePreset() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>())
    {
        return Settings->ThemePreset;
    }
#endif
    return ERuntimeInspectorThemePreset::StudioSlate;
}

FRISettingsDiagnostics UInspectorWorldSubsystem::GetSettingsDiagnostics() const
{
    FRISettingsDiagnostics Result;
#if RUNTIME_INSPECTOR_ENABLED
    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    Result.bRuntimeEnabled = IsRIEnabled();
    Result.DisabledReason = GetRIDisabledReason();
    Result.bUnlockRequired = Settings ? Settings->bRequireUnlock : false;
    Result.bUnlocked = bUnlocked;
    Result.bHasUnlockCode = Settings && !Settings->UnlockCode.TrimStartAndEnd().IsEmpty();
    Result.bOutlineMaterialAssigned = Settings && !Settings->OutlinePostProcessMaterial.IsNull();
    Result.OutlineMaterialPath = Settings ? Settings->OutlinePostProcessMaterial.ToSoftObjectPath().ToString() : FString();
    Result.bCustomDepthStencilReady = IsCustomDepthStencilReady();
#endif
    return Result;
}

FRIRuntimeActorRoleSummary UInspectorWorldSubsystem::BuildActorRoleSummary(AActor* InActor) const
{
    FRIRuntimeActorRoleSummary Result;
#if RUNTIME_INSPECTOR_ENABLED
    if (!InActor)
    {
        Result.Summary = TEXT("No selected actor");
        return Result;
    }

    Result.bHasActor = true;
    Result.ActorPath = InActor->GetPathName();
    Result.ActorClass = InActor->GetClass() ? InActor->GetClass()->GetPathName() : FString();
    Result.bHasAuthority = InActor->HasAuthority();
    Result.LocalRoleLabel = RI_NetRoleLabel(InActor->GetLocalRole());
    Result.RemoteRoleLabel = RI_NetRoleLabel(InActor->GetRemoteRole());
    Result.bReplicates = InActor->GetIsReplicated();
    Result.bReplicateMovement = InActor->IsReplicatingMovement();
    Result.OwnerPath = InActor->GetOwner() ? InActor->GetOwner()->GetPathName() : TEXT("No owner");
    Result.Summary = FString::Printf(
        TEXT("%s | Local=%s Remote=%s | Replicates=%s | Authority=%s"),
        *InActor->GetName(),
        *Result.LocalRoleLabel,
        *Result.RemoteRoleLabel,
        Result.bReplicates ? TEXT("yes") : TEXT("no"),
        Result.bHasAuthority ? TEXT("yes") : TEXT("no"));
#else
    Result.Summary = TEXT("RuntimeInspector disabled");
#endif
    return Result;
}

FRIRuntimeSessionSummary UInspectorWorldSubsystem::GetRuntimeSessionSummary() const
{
    FRIRuntimeSessionSummary Result;
#if RUNTIME_INSPECTOR_ENABLED
    const UWorld* World = GetWorld();
    if (!World)
    {
        Result.Summary = TEXT("World unavailable");
        return Result;
    }

    Result.bSessionAvailable = true;
    Result.bIsGameWorld = World->IsGameWorld();
    Result.bIsPIEWorld = World->WorldType == EWorldType::PIE;
    Result.WorldTypeLabel = RI_WorldTypeLabel(World->WorldType);
    Result.NetModeLabel = RI_NetModeLabel(World->GetNetMode());
    Result.MapName = World->GetMapName();

    if (APlayerController* PC = GetLocalPC())
    {
        Result.bHasLocalPlayerController = true;
        Result.LocalPlayerControllerPath = PC->GetPathName();
    }

    Result.Summary = FString::Printf(
        TEXT("%s | Net=%s | LocalPC=%s"),
        *Result.WorldTypeLabel,
        *Result.NetModeLabel,
        Result.bHasLocalPlayerController ? TEXT("yes") : TEXT("no"));
#else
    Result.Summary = TEXT("RuntimeInspector disabled");
#endif
    return Result;
}

FRIRuntimeActorRoleSummary UInspectorWorldSubsystem::GetSelectedActorRoleSummary() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return BuildActorRoleSummary(SelectedActor.Get());
#else
    return FRIRuntimeActorRoleSummary();
#endif
}

bool UInspectorWorldSubsystem::CompareRuntimeRoles(FRIRuntimeRoleCompareReport& OutReport, FString& OutError)
{
    OutReport = FRIRuntimeRoleCompareReport();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    LastRuntimeRoleCompareReport = OutReport;
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutError = TEXT("PIE with local player required");
        LastRuntimeRoleCompareReport = OutReport;
        return false;
    }

    if (!HasStagedPatch())
    {
        OutError = TEXT("No staged patch");
        LastRuntimeRoleCompareReport = OutReport;
        return false;
    }

    const FRIPatchBundle Bundle = GetStagedPatch();
    if (Bundle.Operations.Num() <= 0)
    {
        OutError = TEXT("Staged patch has no operations");
        LastRuntimeRoleCompareReport = OutReport;
        return false;
    }

    AActor* ReferenceActor = nullptr;
    for (const FRIPatchOperation& Operation : Bundle.Operations)
    {
        ReferenceActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
        if (ReferenceActor)
        {
            break;
        }
    }

    if (!ReferenceActor)
    {
        ReferenceActor = SelectedActor.Get();
    }

    if (!ReferenceActor)
    {
        OutError = TEXT("No runtime actor available for role compare");
        LastRuntimeRoleCompareReport = OutReport;
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const auto RestoreSelection = [&]()
    {
        if (SelectedActor.Get() != PreviousSelectedActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }
    };

    if (SelectedActor.Get() != ReferenceActor)
    {
        SetSelectedActor(ReferenceActor);
    }

    OutReport.GeneratedAtUtc = FDateTime::UtcNow().ToIso8601();
    OutReport.ActorPath = ReferenceActor->GetPathName();
    OutReport.ActorClass = ReferenceActor->GetClass() ? ReferenceActor->GetClass()->GetPathName() : FString();
    OutReport.BundleId = Bundle.BundleId;
    OutReport.OperationCount = Bundle.Operations.Num();

    ERIRuntimeCompareRole AvailableRole = ERIRuntimeCompareRole::Authority;
    const bool bHasAvailableRole = RI_TryMapNetRoleToCompareRole(ReferenceActor->GetLocalRole(), ReferenceActor->HasAuthority(), AvailableRole);
    OutReport.AvailableRoleLabel = bHasAvailableRole ? RI_RuntimeCompareRoleLabel(AvailableRole) : TEXT("None");

    TArray<FString> DetailLines;
    const TArray<ERIRuntimeCompareRole>& Roles = RI_GetRuntimeCompareRoles();
    for (const FRIPatchOperation& Operation : Bundle.Operations)
    {
        FRIRuntimeRoleCompareLine Line;
        Line.Target = Operation.Target;
        Line.Field = Operation.Field;
        Line.CompareKey = FString::Printf(TEXT("%s|%s|%s"),
            *Operation.Target.ActorPath,
            *Operation.Target.ComponentPath,
            *Operation.Field.FieldPath);

        TArray<FString> RoleParts;
        FString FirstObservedValue;
        bool bFirstObservedValueSet = false;
        bool bAvailableValueMismatch = false;

        for (ERIRuntimeCompareRole Role : Roles)
        {
            FRIRuntimeRoleFieldState State;
            State.Role = Role;
            State.RoleLabel = RI_RuntimeCompareRoleLabel(Role);

            if (bHasAvailableRole && Role == AvailableRole)
            {
                State.bRoleAvailable = true;

                FString RuntimeValue;
                FString ReadError;
                if (TryReadRuntimePatchOperationValue(Operation, RuntimeValue, ReadError))
                {
                    State.bTargetFound = true;
                    State.bHasValue = true;
                    State.ValueText = RuntimeValue;
                    State.Message = TEXT("Observed in current session");
                    ++Line.AvailableRoleCount;

                    if (!bFirstObservedValueSet)
                    {
                        FirstObservedValue = RuntimeValue;
                        bFirstObservedValueSet = true;
                    }
                    else if (FirstObservedValue != RuntimeValue)
                    {
                        bAvailableValueMismatch = true;
                    }
                }
                else
                {
                    State.Message = ReadError.IsEmpty() ? TEXT("Target not found in current session") : ReadError;
                    ++Line.MissingRoleCount;
                }
            }
            else
            {
                State.Message = TEXT("Role unavailable in current session");
                ++Line.MissingRoleCount;
            }

            RoleParts.Add(FString::Printf(
                TEXT("%s=%s"),
                *State.RoleLabel,
                State.bHasValue ? *State.ValueText : *State.Message));
            Line.RoleStates.Add(MoveTemp(State));
        }

        const bool bMissingAvailableTarget = Line.RoleStates.ContainsByPredicate([](const FRIRuntimeRoleFieldState& State)
        {
            return State.bRoleAvailable && !State.bTargetFound;
        });

        Line.bHasMismatch = Line.MissingRoleCount > 0 || bAvailableValueMismatch || bMissingAvailableTarget || Line.AvailableRoleCount <= 0;
        Line.Summary = FString::Printf(
            TEXT("%s | %s"),
            Operation.Field.DisplayName.IsEmpty() ? *Operation.Field.FieldPath : *Operation.Field.DisplayName,
            *FString::Join(RoleParts, TEXT(" | ")));

        ++OutReport.ComparedLineCount;
        if (Line.bHasMismatch)
        {
            ++OutReport.MismatchCount;
        }
        OutReport.MissingRoleCount += Line.MissingRoleCount;
        DetailLines.Add(FString::Printf(TEXT("[Field] %s"), *Line.Summary));
        OutReport.Lines.Add(MoveTemp(Line));
    }

    FRIVerificationRunResult VerificationResult;
    RunVerificationProfile(RI_VerificationProfileId_RuntimeSessionRole, VerificationResult);

    FRIRuntimeRoleVerificationLine VerificationLine;
    VerificationLine.ProfileId = RI_VerificationProfileId_RuntimeSessionRole;
    VerificationLine.DisplayName = TEXT("Runtime Session Role");

    TArray<FString> VerificationParts;
    for (ERIRuntimeCompareRole Role : Roles)
    {
        FRIRuntimeRoleVerificationState State;
        State.Role = Role;
        State.RoleLabel = RI_RuntimeCompareRoleLabel(Role);

        if (bHasAvailableRole && Role == AvailableRole)
        {
            State.bRoleAvailable = true;
            State.bExecuted = true;
            State.bPassed = VerificationResult.bPassed;
            State.bBlocked = VerificationResult.bBlocked;
            State.Summary = VerificationResult.Summary.IsEmpty() ? TEXT("Executed") : VerificationResult.Summary;
        }
        else
        {
            State.bBlocked = true;
            State.Summary = TEXT("Role unavailable in current session");
        }

        VerificationParts.Add(FString::Printf(
            TEXT("%s=%s"),
            *State.RoleLabel,
            State.bRoleAvailable
                ? (State.bPassed ? TEXT("pass") : (State.bBlocked ? TEXT("blocked") : TEXT("fail")))
                : TEXT("missing")));
        VerificationLine.RoleStates.Add(MoveTemp(State));
    }

    VerificationLine.bHasMismatch = VerificationLine.RoleStates.ContainsByPredicate([](const FRIRuntimeRoleVerificationState& State)
    {
        return !State.bRoleAvailable || !State.bPassed || State.bBlocked;
    });
    VerificationLine.Summary = FString::Printf(TEXT("%s | %s"), *VerificationLine.DisplayName, *FString::Join(VerificationParts, TEXT(" | ")));
    if (VerificationLine.bHasMismatch)
    {
        ++OutReport.VerificationMismatchCount;
    }
    DetailLines.Add(FString::Printf(TEXT("[Verify] %s"), *VerificationLine.Summary));
    OutReport.VerificationLines.Add(MoveTemp(VerificationLine));

    OutReport.Summary = FString::Printf(
        TEXT("RuntimeRoleCompare=ok | Actor=%s AvailableRole=%s Lines=%d Mismatch=%d MissingRoles=%d Verification=%d"),
        *OutReport.ActorPath,
        OutReport.AvailableRoleLabel.IsEmpty() ? TEXT("None") : *OutReport.AvailableRoleLabel,
        OutReport.ComparedLineCount,
        OutReport.MismatchCount,
        OutReport.MissingRoleCount,
        OutReport.VerificationMismatchCount);
    OutReport.Details = FString::Join(DetailLines, TEXT("\n"));
    LastRuntimeRoleCompareReport = OutReport;

    RestoreSelection();
    return true;
#endif
}

bool UInspectorWorldSubsystem::ValidateHotkeyCandidateInternal(FKey InKey, FString& OutError) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!InKey.IsValid())
    {
        OutError = TEXT("Key is invalid.");
        return false;
    }
    if (InKey == EKeys::Escape)
    {
        OutError = TEXT("Escape is reserved for cancel.");
        return false;
    }
    if (InKey.IsModifierKey())
    {
        OutError = TEXT("Modifier keys are not supported here.");
        return false;
    }
    if (InKey.IsMouseButton())
    {
        OutError = TEXT("Mouse buttons are not allowed for Toggle/Pick.");
        return false;
    }
    if (InKey.IsGamepadKey())
    {
        OutError = TEXT("Gamepad keys are not allowed for Toggle/Pick.");
        return false;
    }
    if (InKey.IsAxis1D() || InKey.IsAxis2D() || InKey.IsAxis3D() || InKey.IsAnalog())
    {
        OutError = TEXT("Axis or analog inputs are not supported.");
        return false;
    }
    if (!InKey.IsBindableInBlueprints())
    {
        OutError = TEXT("This key is not a normal bindable keyboard key.");
        return false;
    }
#endif
    OutError.Reset();
    return true;
}

bool UInspectorWorldSubsystem::ValidateHotkeyCandidate(FKey InKey, FString& OutError) const
{
    return ValidateHotkeyCandidateInternal(InKey, OutError);
}

bool UInspectorWorldSubsystem::PreviewApplySettings(const FRIEditableSettings& InSettings, FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED
    FString LocalError;
    if (!ValidateHotkeyCandidateInternal(InSettings.ToggleKey, LocalError))
    {
        OutError = FString::Printf(TEXT("Toggle Key: %s"), *LocalError);
        return false;
    }
    if (!ValidateHotkeyCandidateInternal(InSettings.PickKey, LocalError))
    {
        OutError = FString::Printf(TEXT("Pick Key: %s"), *LocalError);
        return false;
    }
    if (InSettings.ToggleKey == InSettings.PickKey)
    {
        OutError = TEXT("Toggle Key and Pick Key must be different.");
        return false;
    }

    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutError = TEXT("RuntimeInspector settings object is unavailable.");
        return false;
    }

    Settings->ToggleKey = InSettings.ToggleKey;
    Settings->PickKey = InSettings.PickKey;
    Settings->bPickKeyRequiresCtrl = InSettings.bPickKeyRequiresCtrl;
    Settings->bPickKeyRequiresShift = InSettings.bPickKeyRequiresShift;
    Settings->bEnableRightMousePick = InSettings.bEnableRightMousePick;
    Settings->bRightMousePickRequiresCtrl = InSettings.bRightMousePickRequiresCtrl;
    Settings->bRightMousePickRequiresShift = InSettings.bRightMousePickRequiresShift;
    Settings->bEnableOutlinePP = InSettings.bEnableOutlinePP;
    Settings->OutlinePPWeight = FMath::Max(0.0f, InSettings.OutlinePPWeight);
    Settings->bEnableApplyDebounce = InSettings.bEnableApplyDebounce;
    Settings->ApplyDebounceSeconds = FMath::Clamp(InSettings.ApplyDebounceSeconds, 0.0f, 0.20f);
    Settings->bRequireUnlock = InSettings.bRequireUnlock;
    Settings->bAutoLockOnClose = InSettings.bAutoLockOnClose;

    RebindInspectorKeys();
    RefreshOutlineRuntimeSettings();

    UpdateSettingsDirtyFlag();
    OutError.Reset();
    return true;
#else
    OutError = TEXT("RuntimeInspector is disabled.");
    return false;
#endif
}

bool UInspectorWorldSubsystem::PreviewApplyThemePreset(ERuntimeInspectorThemePreset InPreset, FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED
    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutError = TEXT("RuntimeInspector settings object is unavailable.");
        return false;
    }

    if (Settings->ThemePreset != InPreset)
    {
        Settings->ThemePreset = InPreset;
        LastAppliedThemePresetFingerprint = static_cast<int32>(RICompactUI::GetActiveThemePreset());
        ScheduleThemePreviewRefresh(GetVisiblePage());
    }

    UpdateSettingsDirtyFlag();
    OutError.Reset();
    return true;
#else
    OutError = TEXT("RuntimeInspector is disabled.");
    return false;
#endif
}

bool UInspectorWorldSubsystem::SaveSettings(FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED
    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutError = TEXT("RuntimeInspector settings object is unavailable.");
        return false;
    }

    const FRIEditableSettings PendingSettings = GetEditableSettings();
    const ERuntimeInspectorThemePreset PendingThemePreset = GetThemePreset();
    const FString ConfigFilename = Settings->GetDefaultConfigFilename();

    Settings->SaveConfig(CPF_Config, *ConfigFilename);
    if (GConfig)
    {
        GConfig->Flush(false, ConfigFilename);
    }

    Settings->LoadConfig(nullptr, *ConfigFilename);
    RebindInspectorKeys();
    RefreshOutlineRuntimeSettings();

    const FRIEditableSettings ReloadedSettings = GetEditableSettings();
    const ERuntimeInspectorThemePreset ReloadedThemePreset = GetThemePreset();
    const bool bSettingsMatch = RI_AreEditableSettingsEqual(ReloadedSettings, PendingSettings);
    const bool bThemeMatches = ReloadedThemePreset == PendingThemePreset;
    if (!bSettingsMatch || !bThemeMatches)
    {
        UpdateSettingsDirtyFlag();
        OutError = FString::Printf(
            TEXT("Settings persistence mismatch | OutlineBefore=%.3f OutlineAfter=%.3f ThemeBefore=%d ThemeAfter=%d"),
            PendingSettings.OutlinePPWeight,
            ReloadedSettings.OutlinePPWeight,
            static_cast<int32>(PendingThemePreset),
            static_cast<int32>(ReloadedThemePreset));
        return false;
    }

    LastSavedSettingsSnapshot = ReloadedSettings;
    LastSavedThemePresetSnapshot = ReloadedThemePreset;
    UpdateSettingsDirtyFlag();
    OutError.Reset();
    return true;
#else
    OutError = TEXT("RuntimeInspector is disabled.");
    return false;
#endif
}

void UInspectorWorldSubsystem::ReloadSettingsFromConfig()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>())
    {
        const ERuntimeInspectorThemePreset PreviousThemePreset = Settings->ThemePreset;
        Settings->LoadConfig();
        RebindInspectorKeys();
        RefreshOutlineRuntimeSettings();
        LastSavedSettingsSnapshot = GetEditableSettings();
        LastSavedThemePresetSnapshot = GetThemePreset();
        UpdateSettingsDirtyFlag();
        LastAppliedThemePresetFingerprint = static_cast<int32>(RICompactUI::GetActiveThemePreset());
        if (PreviousThemePreset != Settings->ThemePreset)
        {
            ScheduleThemePreviewRefresh(GetVisiblePage());
        }
    }
#endif
}

void UInspectorWorldSubsystem::UpdateSettingsDirtyFlag()
{
#if RUNTIME_INSPECTOR_ENABLED
    bSettingsDirty = !RI_AreEditableSettingsEqual(GetEditableSettings(), LastSavedSettingsSnapshot)
        || GetThemePreset() != LastSavedThemePresetSnapshot;
#endif
}

bool UInspectorWorldSubsystem::IsCustomDepthStencilReady() const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth")))
    {
        return CVar->GetInt() >= 3;
    }
#endif
    return false;
}

void UInspectorWorldSubsystem::RefreshOutlineRuntimeSettings()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (AActor* Actor = OutlinedActor.Get())
    {
        SetActorOutline(Actor, false);
    }

    EnableOutlinePP(false);

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    if (bOpen && Settings && Settings->bEnableOutlinePP)
    {
        EnableOutlinePP(true);
        if (AActor* Actor = OutlinedActor.Get())
        {
            SetActorOutline(Actor, true, 1);
        }
    }
#endif
}

void UInspectorWorldSubsystem::ClearConfirmDialogBinding()
{
#if RUNTIME_INSPECTOR_ENABLED
    DeactivateConfirmDialogModalState();

    if (UButton* Button = ActiveConfirmDialogYesButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
    }
    if (UButton* Button = ActiveConfirmDialogNoButton.Get())
    {
        Button->OnClicked.RemoveAll(this);
    }
    if (UEditableTextBox* TextBox = ActiveConfirmDialogInputR.Get())
    {
        TextBox->OnTextChanged.RemoveAll(this);
        TextBox->OnTextCommitted.RemoveAll(this);
    }
    if (UEditableTextBox* TextBox = ActiveConfirmDialogInputG.Get())
    {
        TextBox->OnTextChanged.RemoveAll(this);
        TextBox->OnTextCommitted.RemoveAll(this);
    }
    if (UEditableTextBox* TextBox = ActiveConfirmDialogInputB.Get())
    {
        TextBox->OnTextChanged.RemoveAll(this);
        TextBox->OnTextCommitted.RemoveAll(this);
    }
    if (UEditableTextBox* TextBox = ActiveConfirmDialogInputA.Get())
    {
        TextBox->OnTextChanged.RemoveAll(this);
        TextBox->OnTextCommitted.RemoveAll(this);
    }
    if (UEditableTextBox* TextBox = ActiveConfirmDialogInputHex.Get())
    {
        TextBox->OnTextChanged.RemoveAll(this);
        TextBox->OnTextCommitted.RemoveAll(this);
    }

    if (ActiveColorEditItem.IsValid())
    {
        FinalizeActiveColorEdit(!bActiveColorEditCanceled);
    }

    ActiveConfirmDialogWidget.Reset();
    ActiveConfirmDialogModalBlockerWidget.Reset();
    ActiveConfirmDialogYesButton.Reset();
    ActiveConfirmDialogNoButton.Reset();
    ActiveConfirmDialogInputR.Reset();
    ActiveConfirmDialogInputG.Reset();
    ActiveConfirmDialogInputB.Reset();
    ActiveConfirmDialogInputA.Reset();
    ActiveConfirmDialogInputHex.Reset();
    ActiveColorEditItem.Reset();
    bActiveColorEditPreviewDirty = false;
    bActiveColorEditCanceled = false;
    bHasActiveColorEditOriginalColor = false;
    bHasActiveColorEditLastPreviewColor = false;
    ActiveColorEditOriginalColor = FLinearColor::Black;
    ActiveColorEditLastPreviewColor = FLinearColor::Black;
    bUpdatingConfirmDialogText = false;
#endif
}

bool UInspectorWorldSubsystem::TryBindActiveConfirmDialog(UUserWidget* DialogWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!DialogWidget)
    {
        return false;
    }

    UEditableTextBox* InputR = Cast<UEditableTextBox>(DialogWidget->GetWidgetFromName(TEXT("InputTXT_R")));
    UEditableTextBox* InputG = Cast<UEditableTextBox>(DialogWidget->GetWidgetFromName(TEXT("InputTXT_G")));
    UEditableTextBox* InputB = Cast<UEditableTextBox>(DialogWidget->GetWidgetFromName(TEXT("InputTXT_B")));
    UEditableTextBox* InputA = Cast<UEditableTextBox>(DialogWidget->GetWidgetFromName(TEXT("InputTXT_A")));
    UEditableTextBox* InputHex = Cast<UEditableTextBox>(DialogWidget->GetWidgetFromName(TEXT("InputTXT_SRGB")));
    UButton* YesButton = Cast<UButton>(DialogWidget->GetWidgetFromName(TEXT("BTN_Yes")));
    UButton* NoButton = Cast<UButton>(DialogWidget->GetWidgetFromName(TEXT("BTN_No")));

    if (!InputR || !InputG || !InputB || !InputA || !InputHex)
    {
        return false;
    }

    ClearConfirmDialogBinding();

    ActiveConfirmDialogWidget = DialogWidget;
    ActiveConfirmDialogYesButton = YesButton;
    ActiveConfirmDialogNoButton = NoButton;
    ActiveConfirmDialogInputR = InputR;
    ActiveConfirmDialogInputG = InputG;
    ActiveConfirmDialogInputB = InputB;
    ActiveConfirmDialogInputA = InputA;
    ActiveConfirmDialogInputHex = InputHex;

    InputR->OnTextChanged.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogRChanged);
    InputR->OnTextCommitted.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogRCommitted);

    InputG->OnTextChanged.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogGChanged);
    InputG->OnTextCommitted.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogGCommitted);

    InputB->OnTextChanged.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogBChanged);
    InputB->OnTextCommitted.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogBCommitted);

    InputA->OnTextChanged.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogAChanged);
    InputA->OnTextCommitted.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogACommitted);

    InputHex->OnTextChanged.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogHexChanged);
    InputHex->OnTextCommitted.AddDynamic(this, &UInspectorWorldSubsystem::HandleConfirmDialogHexCommitted);

    if (YesButton)
    {
        YesButton->OnClicked.RemoveAll(this);
        YesButton->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleActiveConfirmDialogAccepted);
    }
    if (NoButton)
    {
        NoButton->OnClicked.RemoveAll(this);
        NoButton->OnClicked.AddDynamic(this, &UInspectorWorldSubsystem::HandleActiveConfirmDialogCanceled);
    }

    TryActivateConfirmDialogColorPage(DialogWidget);
    ActivateConfirmDialogModalState(DialogWidget);
    return true;
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::ActivateConfirmDialogModalState(UUserWidget* DialogWidget)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!DialogWidget)
    {
        return;
    }

    APlayerController* PC = GetLocalPC();
    UWorld* World = GetWorld();
    if (!PC || !World)
    {
        return;
    }

    if (UInspectorModalBlockerWidget* ExistingBlocker = ActiveConfirmDialogModalBlockerWidget.Get())
    {
        ExistingBlocker->RemoveFromParent();
    }

    UInspectorModalBlockerWidget* BlockerWidget = CreateWidget<UInspectorModalBlockerWidget>(PC, UInspectorModalBlockerWidget::StaticClass());
    if (BlockerWidget)
    {
        BlockerWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
        BlockerWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
        BlockerWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
        BlockerWidget->AddToViewport(10000);
        ActiveConfirmDialogModalBlockerWidget = BlockerWidget;
    }

    if (UUserWidget* InspectorPanel = PanelWidget.Get())
    {
        InspectorPanel->SetIsEnabled(false);
    }

    PC->bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetWidgetToFocus(DialogWidget->TakeWidget());
    PC->SetInputMode(Mode);
#endif
}

void UInspectorWorldSubsystem::DeactivateConfirmDialogModalState()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UInspectorModalBlockerWidget* BlockerWidget = ActiveConfirmDialogModalBlockerWidget.Get())
    {
        BlockerWidget->RemoveFromParent();
    }

    if (UUserWidget* InspectorPanel = PanelWidget.Get())
    {
        InspectorPanel->SetIsEnabled(true);
    }

    if (APlayerController* PC = GetLocalPC())
    {
        PC->bShowMouseCursor = bOpen;
        if (bOpen)
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            if (UUserWidget* InspectorPanel = PanelWidget.Get())
            {
                Mode.SetWidgetToFocus(InspectorPanel->TakeWidget());
            }
            PC->SetInputMode(Mode);
        }
        else
        {
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
#endif
}

bool UInspectorWorldSubsystem::TryActivateConfirmDialogColorPage(UUserWidget* DialogWidget) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!DialogWidget)
    {
        return false;
    }

    UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(DialogWidget->GetWidgetFromName(TEXT("WidgetSwitcher_167")));
    UWidget* ColorCanvas = DialogWidget->GetWidgetFromName(TEXT("Canvas_Color"));
    if (!Switcher || !ColorCanvas)
    {
        return false;
    }

    UWidget* SwitcherChild = ColorCanvas;
    while (SwitcherChild && SwitcherChild->GetParent() && SwitcherChild->GetParent() != Switcher)
    {
        SwitcherChild = SwitcherChild->GetParent();
    }

    if (!SwitcherChild || SwitcherChild->GetParent() != Switcher)
    {
        return false;
    }

    Switcher->SetActiveWidget(SwitcherChild);
    return Switcher->GetActiveWidget() == SwitcherChild;
#else
    (void)DialogWidget;
    return false;
#endif
}

bool UInspectorWorldSubsystem::IsConfirmDialogColorPageActive(UUserWidget* DialogWidget) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!DialogWidget)
    {
        return false;
    }

    UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(DialogWidget->GetWidgetFromName(TEXT("WidgetSwitcher_167")));
    UWidget* ColorCanvas = DialogWidget->GetWidgetFromName(TEXT("Canvas_Color"));
    if (!Switcher || !ColorCanvas)
    {
        return false;
    }

    UWidget* SwitcherChild = ColorCanvas;
    while (SwitcherChild && SwitcherChild->GetParent() && SwitcherChild->GetParent() != Switcher)
    {
        SwitcherChild = SwitcherChild->GetParent();
    }

    return SwitcherChild && SwitcherChild->GetParent() == Switcher && Switcher->GetActiveWidget() == SwitcherChild;
#else
    (void)DialogWidget;
    return false;
#endif
}

void UInspectorWorldSubsystem::RefreshConfirmDialogBinding()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!ActiveConfirmDialogWidget.IsValid() && !ActiveColorEditItem.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ClearConfirmDialogBinding();
        return;
    }

    UClass* ConfirmDialogClass = ConfirmDialogWidgetClass.LoadSynchronous();
    if (!ConfirmDialogClass)
    {
        return;
    }

    TArray<UUserWidget*> FoundDialogs;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundDialogs, ConfirmDialogClass, false);

    UUserWidget* DialogToBind = nullptr;
    for (UUserWidget* Candidate : FoundDialogs)
    {
        if (Candidate && Candidate->IsInViewport())
        {
            DialogToBind = Candidate;
            break;
        }
    }

    if (!DialogToBind)
    {
        ClearConfirmDialogBinding();
        return;
    }

    if (ActiveConfirmDialogWidget.Get() == DialogToBind)
    {
        return;
    }

    TryBindActiveConfirmDialog(DialogToBind);
#endif
}

namespace
{
static FStructProperty* RI_FindStructPropertyByAuthoredName(UClass* InClass, const FName AuthoredName)
{
    if (!InClass || AuthoredName.IsNone())
    {
        return nullptr;
    }

    for (TFieldIterator<FStructProperty> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FStructProperty* Prop = *It;
        if (!Prop)
        {
            continue;
        }

        if (Prop->GetFName() == AuthoredName)
        {
            return Prop;
        }

        const FString PropName = Prop->GetName();
        const FString Authored = Prop->GetAuthoredName();
        const FString Wanted = AuthoredName.ToString();

        if (Authored == Wanted)
        {
            return Prop;
        }

        if (PropName.StartsWith(Wanted, ESearchCase::CaseSensitive) ||
            PropName.Contains(Wanted, ESearchCase::CaseSensitive) ||
            Authored.StartsWith(Wanted, ESearchCase::CaseSensitive) ||
            Authored.Contains(Wanted, ESearchCase::CaseSensitive))
        {
            return Prop;
        }
    }

    return nullptr;
}
}

bool UInspectorWorldSubsystem::TryGetActiveConfirmDialogColor(FLinearColor& OutColor) const
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get();
    if (!DialogWidget)
    {
        return false;
    }

    const FStructProperty* ColorProp = RI_FindStructPropertyByAuthoredName(DialogWidget->GetClass(), TEXT("CurrentColor"));
    if (!ColorProp)
    {
        return false;
    }

    if (ColorProp->Struct == TBaseStructure<FLinearColor>::Get())
    {
        const FLinearColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<FLinearColor>(DialogWidget);
        if (!ColorPtr)
        {
            return false;
        }

        OutColor = *ColorPtr;
        return true;
    }

    if (ColorProp->Struct == TBaseStructure<FColor>::Get())
    {
        const FColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<FColor>(DialogWidget);
        if (!ColorPtr)
        {
            return false;
        }

        OutColor = FLinearColor::FromSRGBColor(*ColorPtr);
        return true;
    }
#endif
    return false;
}

bool UInspectorWorldSubsystem::TrySetActiveConfirmDialogColor(const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get();
    if (!DialogWidget)
    {
        return false;
    }

    FStructProperty* ColorProp = RI_FindStructPropertyByAuthoredName(DialogWidget->GetClass(), TEXT("CurrentColor"));
    if (!ColorProp)
    {
        return false;
    }

    if (ColorProp->Struct == TBaseStructure<FLinearColor>::Get())
    {
        FLinearColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<FLinearColor>(DialogWidget);
        if (!ColorPtr)
        {
            return false;
        }

        ColorProp->CopyCompleteValue(ColorPtr, &InColor);
        return true;
    }

    if (ColorProp->Struct == TBaseStructure<FColor>::Get())
    {
        FColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<FColor>(DialogWidget);
        if (!ColorPtr)
        {
            return false;
        }

        const FColor SRGBColor = InColor.ToFColorSRGB();
        ColorProp->CopyCompleteValue(ColorPtr, &SRGBColor);
        return true;
    }
#endif
    return false;
}

bool UInspectorWorldSubsystem::ApplyActiveConfirmDialogColor(const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get();
    if (!DialogWidget)
    {
        return false;
    }

    UFunction* InitDataFn = DialogWidget->FindFunction(TEXT("InitData"));
    if (!InitDataFn)
    {
        return false;
    }

    struct FRIInitDataParams
    {
        FString Title;
        FString Content;
        FString Type;
    };

    FRIInitDataParams Params;
    Params.Title = TEXT("Color");
    Params.Content = InColor.ToString();
    Params.Type = TEXT("Color");

    TGuardValue<bool> GuardUpdatingText(bUpdatingConfirmDialogText, true);
    DialogWidget->ProcessEvent(InitDataFn, &Params);
    TryActivateConfirmDialogColorPage(DialogWidget);
    return true;
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::TryBuildActiveConfirmDialogColorFromInputs(FLinearColor& OutColor) const
{
#if RUNTIME_INSPECTOR_ENABLED
    const UEditableTextBox* InputR = ActiveConfirmDialogInputR.Get();
    const UEditableTextBox* InputG = ActiveConfirmDialogInputG.Get();
    const UEditableTextBox* InputB = ActiveConfirmDialogInputB.Get();
    const UEditableTextBox* InputA = ActiveConfirmDialogInputA.Get();
    if (!InputR || !InputG || !InputB || !InputA)
    {
        return false;
    }

    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    float A = 1.0f;

    TryParseConfirmDialogUnitFloat(InputR->GetText(), R, R);
    TryParseConfirmDialogUnitFloat(InputG->GetText(), G, G);
    TryParseConfirmDialogUnitFloat(InputB->GetText(), B, B);
    TryParseConfirmDialogUnitFloat(InputA->GetText(), A, A);

    OutColor = FLinearColor(R, G, B, A);
    return true;
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::RefreshActiveConfirmDialogColor()
{
#if RUNTIME_INSPECTOR_ENABLED
    UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get();
    if (!DialogWidget)
    {
        return;
    }

    if (UFunction* RefreshFn = DialogWidget->FindFunction(TEXT("RefreshColor")))
    {
        TGuardValue<bool> GuardUpdatingText(bUpdatingConfirmDialogText, true);
        DialogWidget->ProcessEvent(RefreshFn, nullptr);
    }
#endif
}

bool UInspectorWorldSubsystem::TryParseConfirmDialogUnitFloat(const FText& InText, float CurrentValue, float& OutValue) const
{
    FString ValueString = InText.ToString();
    ValueString.TrimStartAndEndInline();

    if (ValueString.IsEmpty())
    {
        OutValue = CurrentValue;
        return false;
    }

    double ParsedValue = 0.0;
    if (!FDefaultValueHelper::ParseDouble(ValueString, ParsedValue))
    {
        OutValue = CurrentValue;
        return false;
    }

    OutValue = FMath::Clamp(static_cast<float>(ParsedValue), 0.0f, 1.0f);
    return true;
}

bool UInspectorWorldSubsystem::TryParseConfirmDialogHexColor(const FText& InText, FLinearColor& OutColor) const
{
    FString HexString = InText.ToString();
    HexString.TrimStartAndEndInline();
    HexString.RemoveFromStart(TEXT("#"));

    if (!(HexString.Len() == 6 || HexString.Len() == 8))
    {
        return false;
    }

    for (TCHAR Char : HexString)
    {
        if (!FChar::IsHexDigit(Char))
        {
            return false;
        }
    }

    OutColor = FLinearColor::FromSRGBColor(FColor::FromHex(HexString));
    return true;
}

bool UInspectorWorldSubsystem::TryGetInspectorItemColor(UObject* ItemObject, FLinearColor& OutColor) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    OutColor = FLinearColor::Black;

    if (const UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        if (PropertyItem->GetValueType() == EInspectorValueType::LinearColor)
        {
            return PropertyItem->GetLinearColor(OutColor);
        }

        if (PropertyItem->GetValueType() == EInspectorValueType::Color)
        {
            FColor SRGBColor = FColor::Black;
            if (!PropertyItem->GetColor(SRGBColor))
            {
                return false;
            }

            OutColor = FLinearColor::FromSRGBColor(SRGBColor);
            return true;
        }

        return false;
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        FString Error;
        return MaterialItem->GetVector(OutColor, Error);
    }

    return false;
#endif
}

bool UInspectorWorldSubsystem::ApplyInspectorItemColor(UObject* ItemObject, const FLinearColor& InColor, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    return ApplyInspectorItemColorInternal(ItemObject, InColor, OutError, false);
#endif
}

bool UInspectorWorldSubsystem::ApplyInspectorItemColorInternal(UObject* ItemObject, const FLinearColor& InColor, FString& OutError, bool bSuppressHistory)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    TGuardValue<bool> GuardPreviewHistory(bApplyingColorDialogPreview, bSuppressHistory);

    const auto RefreshVisibleItemDisplay = [this, ItemObject]()
    {
        FLinearColor UpdatedColor = FLinearColor::Black;
        const bool bHasUpdatedColor = TryGetInspectorItemColor(ItemObject, UpdatedColor);

        if (UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get())
        {
            SectionWidget->RefreshItemDisplay(ItemObject);
        }

        if (UUserWidget* Panel = PanelWidget.Get())
        {
            RI_RefreshPropertyList(Panel, EInspectorRefreshReason::ValuesChanged);
            RefreshPanel(EInspectorRefreshReason::ValuesChanged);
            RI_RefreshLegacyPropertyPanelWidgets(Panel);
            if (bHasUpdatedColor)
            {
                RI_SyncLegacyPropertyListSwatch(Panel, ItemObject, UpdatedColor);
            }
        }
    };

    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        if (PropertyItem->GetValueType() == EInspectorValueType::LinearColor)
        {
            const bool bApplied = PropertyItem->SetLinearColor(InColor, OutError);
            if (bApplied)
            {
                RefreshVisibleItemDisplay();
            }
            return bApplied;
        }

        if (PropertyItem->GetValueType() == EInspectorValueType::Color)
        {
            const bool bApplied = PropertyItem->SetColor(InColor.ToFColorSRGB(), OutError);
            if (bApplied)
            {
                RefreshVisibleItemDisplay();
            }
            return bApplied;
        }

        OutError = TEXT("Item is not a color property");
        return false;
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        const bool bApplied = MaterialItem->SetVector(InColor, OutError);
        if (bApplied)
        {
            RefreshVisibleItemDisplay();
        }
        return bApplied;
    }

    OutError = TEXT("Unsupported color item");
    return false;
#endif
}

bool UInspectorWorldSubsystem::OpenColorEditorForItemInternal(UObject* ItemObject)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    FLinearColor InitialColor = FLinearColor::Black;
    if (!TryGetInspectorItemColor(ItemObject, InitialColor))
    {
        return false;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = GetLocalPC();
    UClass* ConfirmDialogClass = ConfirmDialogWidgetClass.LoadSynchronous();
    if (!World || !PC || !ConfirmDialogClass)
    {
        return false;
    }

    if (UUserWidget* ExistingDialog = ActiveConfirmDialogWidget.Get())
    {
        ExistingDialog->RemoveFromParent();
        ClearConfirmDialogBinding();
    }

    UUserWidget* DialogWidget = CreateWidget<UUserWidget>(PC, ConfirmDialogClass);
    if (!DialogWidget)
    {
        return false;
    }

    DialogWidget->AddToViewport(10001);

    UFunction* InitDataFn = DialogWidget->FindFunction(TEXT("InitData"));
    if (!InitDataFn)
    {
        DialogWidget->RemoveFromParent();
        return false;
    }

    const FString HexText = InitialColor.ToFColorSRGB().ToHex();

    struct FRIInitDataParams
    {
        FString Title;
        FString Content;
        FString Type;
    };

    FRIInitDataParams Params;
    Params.Title = TEXT("Color");
    Params.Content = HexText;
    Params.Type = TEXT("Color");
    DialogWidget->ProcessEvent(InitDataFn, &Params);
    TryActivateConfirmDialogColorPage(DialogWidget);

    if (!TryBindActiveConfirmDialog(DialogWidget))
    {
        DialogWidget->RemoveFromParent();
        return false;
    }

    ActiveColorEditItem = ItemObject;
    bActiveColorEditPreviewDirty = false;
    bActiveColorEditCanceled = false;
    bHasActiveColorEditOriginalColor = true;
    bHasActiveColorEditLastPreviewColor = true;
    ActiveColorEditOriginalColor = InitialColor;
    ActiveColorEditLastPreviewColor = InitialColor;
    ApplyActiveConfirmDialogColor(InitialColor);
    RefreshActiveConfirmDialogColor();
    return true;
#endif
}

void UInspectorWorldSubsystem::ApplyActiveColorEditItemIfNeeded(const FLinearColor& InColor)
{
#if RUNTIME_INSPECTOR_ENABLED
    UObject* ItemObject = ActiveColorEditItem.Get();
    if (!ItemObject)
    {
        return;
    }

    FString Error;
    if (!ApplyInspectorItemColorInternal(ItemObject, InColor, Error, true))
    {
        return;
    }

    bActiveColorEditPreviewDirty = bHasActiveColorEditOriginalColor
        && !InColor.Equals(ActiveColorEditOriginalColor, KINDA_SMALL_NUMBER);
    bHasActiveColorEditLastPreviewColor = true;
    ActiveColorEditLastPreviewColor = InColor;
#endif
}

void UInspectorWorldSubsystem::SyncActiveConfirmDialogColorPreview()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!ActiveColorEditItem.IsValid() || !ActiveConfirmDialogWidget.IsValid())
    {
        return;
    }

    FLinearColor CurrentColor = FLinearColor::Black;
    if (!TryGetActiveConfirmDialogColor(CurrentColor))
    {
        return;
    }

    if (bHasActiveColorEditLastPreviewColor && CurrentColor.Equals(ActiveColorEditLastPreviewColor, KINDA_SMALL_NUMBER))
    {
        return;
    }

    ApplyActiveColorEditItemIfNeeded(CurrentColor);
#endif
}

void UInspectorWorldSubsystem::FinalizeActiveColorEdit(bool bAccept)
{
#if RUNTIME_INSPECTOR_ENABLED
    UObject* ItemObject = ActiveColorEditItem.Get();
    if (!ItemObject)
    {
        return;
    }

    if (!bHasActiveColorEditOriginalColor)
    {
        return;
    }

    const bool bHasPreviewColor = bHasActiveColorEditLastPreviewColor;
    const FLinearColor PreviewColor = bHasPreviewColor ? ActiveColorEditLastPreviewColor : ActiveColorEditOriginalColor;
    const bool bHasFinalDelta = !PreviewColor.Equals(ActiveColorEditOriginalColor, KINDA_SMALL_NUMBER);

    FString Error;
    if (!bAccept || !bHasFinalDelta)
    {
        ApplyInspectorItemColorInternal(ItemObject, ActiveColorEditOriginalColor, Error, true);
        return;
    }

    ApplyInspectorItemColorInternal(ItemObject, ActiveColorEditOriginalColor, Error, true);
    Error.Reset();
    ApplyInspectorItemColorInternal(ItemObject, PreviewColor, Error, false);
#endif
}

void UInspectorWorldSubsystem::HandleActiveConfirmDialogAccepted()
{
#if RUNTIME_INSPECTOR_ENABLED
    bActiveColorEditCanceled = false;
    if (UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get())
    {
        DialogWidget->RemoveFromParent();
    }
    ClearConfirmDialogBinding();
#endif
}

void UInspectorWorldSubsystem::HandleActiveConfirmDialogCanceled()
{
#if RUNTIME_INSPECTOR_ENABLED
    bActiveColorEditCanceled = true;
    if (UUserWidget* DialogWidget = ActiveConfirmDialogWidget.Get())
    {
        DialogWidget->RemoveFromParent();
    }
    ClearConfirmDialogBinding();
#endif
}

void UInspectorWorldSubsystem::ApplyActiveConfirmDialogChannels()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bUpdatingConfirmDialogText)
    {
        return;
    }

    FLinearColor CurrentColor;
    if (!TryGetActiveConfirmDialogColor(CurrentColor))
    {
        return;
    }

    const UEditableTextBox* InputR = ActiveConfirmDialogInputR.Get();
    const UEditableTextBox* InputG = ActiveConfirmDialogInputG.Get();
    const UEditableTextBox* InputB = ActiveConfirmDialogInputB.Get();
    const UEditableTextBox* InputA = ActiveConfirmDialogInputA.Get();

    if (!InputR || !InputG || !InputB || !InputA)
    {
        return;
    }

    float NewR = CurrentColor.R;
    float NewG = CurrentColor.G;
    float NewB = CurrentColor.B;
    float NewA = CurrentColor.A;
    bool bChanged = false;

    bChanged |= TryParseConfirmDialogUnitFloat(InputR->GetText(), CurrentColor.R, NewR);
    bChanged |= TryParseConfirmDialogUnitFloat(InputG->GetText(), CurrentColor.G, NewG);
    bChanged |= TryParseConfirmDialogUnitFloat(InputB->GetText(), CurrentColor.B, NewB);
    bChanged |= TryParseConfirmDialogUnitFloat(InputA->GetText(), CurrentColor.A, NewA);

    if (!bChanged)
    {
        return;
    }

    if (TrySetActiveConfirmDialogColor(FLinearColor(NewR, NewG, NewB, NewA)))
    {
        RefreshActiveConfirmDialogColor();
        ApplyActiveColorEditItemIfNeeded(FLinearColor(NewR, NewG, NewB, NewA));
    }
#endif
}

void UInspectorWorldSubsystem::HandleConfirmDialogNumericTextChanged(UEditableTextBox* SourceTextBox, int32 ChannelIndex, const FText& InText)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bUpdatingConfirmDialogText || !SourceTextBox)
    {
        return;
    }

    FLinearColor CurrentColor;
    if (!TryBuildActiveConfirmDialogColorFromInputs(CurrentColor))
    {
        return;
    }

    float ParsedValue = 0.0f;
    float CurrentValue = 0.0f;
    switch (ChannelIndex)
    {
    case 0: CurrentValue = CurrentColor.R; break;
    case 1: CurrentValue = CurrentColor.G; break;
    case 2: CurrentValue = CurrentColor.B; break;
    case 3: CurrentValue = CurrentColor.A; break;
    default: return;
    }

    if (!TryParseConfirmDialogUnitFloat(InText, CurrentValue, ParsedValue))
    {
        return;
    }

    switch (ChannelIndex)
    {
    case 0: CurrentColor.R = ParsedValue; break;
    case 1: CurrentColor.G = ParsedValue; break;
    case 2: CurrentColor.B = ParsedValue; break;
    case 3: CurrentColor.A = ParsedValue; break;
    default: return;
    }

    ApplyActiveConfirmDialogColor(CurrentColor);
    ApplyActiveColorEditItemIfNeeded(CurrentColor);
#endif
}

void UInspectorWorldSubsystem::HandleConfirmDialogHexTextChanged(const FText& InText)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bUpdatingConfirmDialogText)
    {
        return;
    }

    FLinearColor ParsedColor;
    if (!TryParseConfirmDialogHexColor(InText, ParsedColor))
    {
        return;
    }

    ApplyActiveConfirmDialogColor(ParsedColor);
    ApplyActiveColorEditItemIfNeeded(ParsedColor);
#endif
}

void UInspectorWorldSubsystem::HandleConfirmDialogRChanged(const FText& InText)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputR.Get(), 0, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogGChanged(const FText& InText)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputG.Get(), 1, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogBChanged(const FText& InText)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputB.Get(), 2, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogAChanged(const FText& InText)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputA.Get(), 3, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogHexChanged(const FText& InText)
{
    HandleConfirmDialogHexTextChanged(InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogRCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputR.Get(), 0, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogGCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputG.Get(), 1, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogBCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputB.Get(), 2, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogACommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    HandleConfirmDialogNumericTextChanged(ActiveConfirmDialogInputA.Get(), 3, InText);
}

void UInspectorWorldSubsystem::HandleConfirmDialogHexCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    HandleConfirmDialogHexTextChanged(InText);
}

TArray<FRISelfTestDefinition> UInspectorWorldSubsystem::GetAvailableSelfTests() const
{
    TArray<FRISelfTestDefinition> Results;
#if RUNTIME_INSPECTOR_ENABLED
    const bool bPIEAvailable = IsSelfTestPIEAvailable();

    auto AddDefinition = [&Results, bPIEAvailable](
        FName Id,
        const TCHAR* DisplayName,
        const TCHAR* Category,
        const TCHAR* Description,
        bool bRequiresPIE,
        bool bMutatesRuntime)
    {
        FRISelfTestDefinition Definition;
        Definition.Id = Id;
        Definition.DisplayName = DisplayName;
        Definition.Category = Category;
        Definition.Description = Description;
        Definition.bRequiresPIE = bRequiresPIE;
        Definition.bMutatesRuntime = bMutatesRuntime;
        Definition.bEnabled = !bRequiresPIE || bPIEAvailable;
        Results.Add(Definition);
    };

    AddDefinition(
        RI_SelfTestId_ConfirmDialog,
        TEXT("Confirm Dialog Color Input"),
        TEXT("UI"),
        TEXT("Verifies RGBA and Hex inputs update the runtime color dialog correctly."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_SettingsPreview,
        TEXT("Settings Preview"),
        TEXT("Settings"),
        TEXT("Verifies preview-apply and reload flow for runtime settings."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_SettingsSavePersistence,
        TEXT("Settings Save Persistence"),
        TEXT("Settings"),
        TEXT("Verifies saving settings survives config reload and keeps the saved outline value."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_StarLiveEditAndRun,
        TEXT("Star Live Edit And Run"),
        TEXT("Actor"),
        TEXT("Verifies starred property/material rows stay editable in the Star pane and starred functions can execute from the same pane."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_StarPreciseNavigation,
        TEXT("Star Precise Navigation"),
        TEXT("Actor"),
        TEXT("Verifies starred property, material, and function rows navigate back to the exact target context."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_SettingsHotkey,
        TEXT("Settings Hotkey Rebind"),
        TEXT("Settings"),
        TEXT("Verifies toggle-key rebinding swaps input bindings and restores the saved key."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_SettingsPageLayout,
        TEXT("Settings Page Layout"),
        TEXT("Settings"),
        TEXT("Verifies the Settings tab uses its own page-level scroll root, footer controls, and status rows without showing legacy host content."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_ThemePresetPreview,
        TEXT("Theme Preset Preview"),
        TEXT("Settings"),
        TEXT("Verifies switching the active theme preset rebuilds the open panel, keeps the Snapshot controls visible, and restores the original preset."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_PatchPreset,
        TEXT("Patch Preset Roundtrip"),
        TEXT("Patch"),
        TEXT("Captures a staged patch, saves a preset, reloads it, reapplies it, and rolls it back."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_PromotePreview,
        TEXT("Promote Preview"),
        TEXT("Promote"),
        TEXT("Builds a dry-run Blueprint source promote preview from a staged runtime patch."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_PromoteConfig,
        TEXT("Promote Config"),
        TEXT("Promote"),
        TEXT("Promotes a config-backed RuntimeInspector setting to source, then restores the original value."),
        false,
        true);

    AddDefinition(
        RI_SelfTestId_PromoteBlueprintApply,
        TEXT("Promote Blueprint Apply"),
        TEXT("Promote"),
        TEXT("Promotes a runtime-edited Blueprint actor property to Blueprint source, verifies the asset default changed, then restores it."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_PromoteMaterialApply,
        TEXT("Promote Material Apply"),
        TEXT("Promote"),
        TEXT("Promotes a runtime MID vector parameter to its source MIC, verifies the asset changed, then restores it."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_AuditReport,
        TEXT("Audit Report"),
        TEXT("Audit"),
        TEXT("Builds baseline/current and current/patch audit reports from a staged runtime patch and exports them."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_FilePage,
        TEXT("File Page Injection"),
        TEXT("File"),
        TEXT("Verifies the File tab injects the managed RuntimeInspector file workflow widget into the existing Data page host."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_ContextStrip,
        TEXT("Context Strip"),
        TEXT("UI"),
        TEXT("Verifies the shared context strip renders actor, class, source, and staged-state summaries for both empty and selected states."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_WorkflowPageView,
        TEXT("Workflow Page View"),
        TEXT("Workflow"),
        TEXT("Verifies the Test tab renders workflow rows, selection UI, and the nested actor end-to-end workflow entry inside the existing host."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_TestPageLayout,
        TEXT("Test Page Layout"),
        TEXT("Workflow"),
        TEXT("Verifies the Test tab uses a page-level scroll root and still renders remote session, workflow, test, and report sections."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_PanelInteraction,
        TEXT("Panel Interaction"),
        TEXT("UI"),
        TEXT("Verifies the RuntimeInspector panel can be dragged from the title bar and vertically resized from the bottom edge."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_ActorPageStructure,
        TEXT("Actor Page Structure"),
        TEXT("UI"),
        TEXT("Verifies the Inspect page shows the component tree, starred property list, component-focus property routing, and color swatch rows."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FileWorkflow,
        TEXT("File Workflow"),
        TEXT("File"),
        TEXT("Verifies stage/export/preset/audit/clear actions drive the shared File report pipeline correctly."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_FilePromote,
        TEXT("File Promote Workflow"),
        TEXT("File"),
        TEXT("Verifies File-page promote preview/apply actions drive shared reports and restore config-backed source state."),
        false,
        true);

    AddDefinition(
        RI_SelfTestId_FileCompare,
        TEXT("File Compare View"),
        TEXT("File"),
        TEXT("Verifies the File page renders baseline/current and patch/source compare rows with explicit compare-pair semantics."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_FileRoleCompare,
        TEXT("File Role Compare View"),
        TEXT("File"),
        TEXT("Verifies the File page renders runtime role compare summary, preview, and role rows from a staged patch."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_FileRemoteSessionCompare,
        TEXT("File Remote Session Compare View"),
        TEXT("File"),
        TEXT("Verifies the File page renders remote session target-set compare summary, preview, and compare rows from the multi-session API."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_ActorPromoteFile,
        TEXT("Actor Promote File Workflow"),
        TEXT("Actor"),
        TEXT("Verifies the selected actor can be edited, staged, promoted to source, audited, cleared, and restored through the File workflow."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_ActorApplyFile,
        TEXT("Actor Apply File Workflow"),
        TEXT("Actor"),
        TEXT("Verifies the selected actor can be edited, staged, audited, exported, cleared, and restored through the File workflow."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_RuntimeSessionRole,
        TEXT("Runtime Session Role"),
        TEXT("Runtime"),
        TEXT("Verifies PIE session summary and selected-actor role summary resolve into stable runtime diagnostics."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_RuntimeRoleCompare,
        TEXT("Runtime Role Compare"),
        TEXT("Runtime"),
        TEXT("Verifies staged patch fields produce a role-aware compare report with explicit missing-role and verification states."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_RemoteRuntimeFoundation,
        TEXT("Remote Runtime Foundation"),
        TEXT("Remote"),
        TEXT("Verifies runtime sessions can be listed, explicitly connected, and enumerated into actor targets through the shared remote-runtime API."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_RemoteSessionCompare,
        TEXT("Remote Session Compare"),
        TEXT("Remote"),
        TEXT("Verifies editor and PIE runtime sessions can both be discovered and compared against the same actor query."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_RemoteSessionTargetSetCompare,
        TEXT("Remote Session Target Set Compare"),
        TEXT("Remote"),
        TEXT("Verifies editor and PIE runtime sessions can compare filtered target inventories and report shared, missing, and mismatched targets."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_RemoteSessionTargetSetCompareMatrix,
        TEXT("Remote Session Target Set Compare Matrix"),
        TEXT("Remote"),
        TEXT("Runs a curated white-listed batch of remote session target-set compare requests and verifies every matrix entry completes with the expected session/filter echo."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_RemoteSessionContextUI,
        TEXT("Remote Session Context UI"),
        TEXT("Remote"),
        TEXT("Verifies File and Test pages render the same shared remote session, target query, and workflow context from the subsystem authority state."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_RemotePackagedFoundation,
        TEXT("Remote Packaged Foundation"),
        TEXT("Remote"),
        TEXT("Verifies an external packaged runtime session can be discovered on loopback, connected, and enumerated into actor targets."),
        false,
        false);

    AddDefinition(
        RI_SelfTestId_RemotePackagedPatchPull,
        TEXT("Remote Packaged Patch Pull"),
        TEXT("Remote"),
        TEXT("Applies a runtime property change inside an external packaged session, pulls the patch bundle back into the editor, and restores the runtime value."),
        false,
        true);

    AddDefinition(
        RI_SelfTestId_RemotePackagedToSourceClosure,
        TEXT("Remote Packaged To Source Closure"),
        TEXT("Remote"),
        TEXT("Applies a packaged runtime change, pulls the patch into the editor, stages and audits it, runs promote preview/apply, then restores editor source state."),
        false,
        true);

    AddDefinition(
        RI_SelfTestId_FabScreenshotFoundation,
        TEXT("Fab Screenshot Foundation"),
        TEXT("Presentation"),
        TEXT("Applies the clean RuntimeInspector screenshot state: SoftContrast theme, Snapshot page active, advanced sections collapsed, and BP_TestVarsActor-preferred selection."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FabScreenshotActorPage,
        TEXT("Fab Screenshot Actor Page"),
        TEXT("Presentation"),
        TEXT("Applies the clean RuntimeInspector screenshot state and switches the runtime UI to the Inspect page for media capture."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FabScreenshotSettingsPage,
        TEXT("Fab Screenshot Settings Page"),
        TEXT("Presentation"),
        TEXT("Applies the clean RuntimeInspector screenshot state and switches the runtime UI to the Snapshot page for media capture."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FabScreenshotToolsPage,
        TEXT("Fab Screenshot Tools Page"),
        TEXT("Presentation"),
        TEXT("Applies the clean RuntimeInspector screenshot state and switches the runtime UI to the Diagnostics page for media capture."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FabScreenshotRemoteSession,
        TEXT("Fab Screenshot Remote Session"),
        TEXT("Presentation"),
        TEXT("Applies the remote-session compare screenshot state and verifies the runtime UI is ready for media capture."),
        true,
        false);

    AddDefinition(
        RI_SelfTestId_FabScreenshotPromoteOrAudit,
        TEXT("Fab Screenshot Promote Or Audit"),
        TEXT("Presentation"),
        TEXT("Applies the promote-or-audit screenshot state and verifies the runtime UI is ready for media capture."),
        true,
        true);

    AddDefinition(
        RI_SelfTestId_WorkflowMatrix,
        TEXT("Workflow Matrix"),
        TEXT("Workflow"),
        TEXT("Runs a curated white-listed batch of remote workflows and verifies actor-aware and remote-runtime orchestration entries all complete successfully."),
        true,
        true);
#endif
    return Results;
}

bool UInspectorWorldSubsystem::IsSelfTestPIEAvailable() const
{
#if RUNTIME_INSPECTOR_ENABLED
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld() && GetLocalPC() != nullptr;
#else
    return false;
#endif
}

FString UInspectorWorldSubsystem::BuildSelfTestSummary(const FString& FullReport) const
{
    const FString Trimmed = FullReport.TrimStartAndEnd();
    int32 PipeIndex = INDEX_NONE;
    if (Trimmed.FindChar(TEXT('|'), PipeIndex))
    {
        return Trimmed.Left(PipeIndex).TrimEnd();
    }
    return Trimmed;
}

FRISelfTestResult UInspectorWorldSubsystem::MakeSelfTestResult(const FRISelfTestDefinition& Definition, bool bPassed, const FString& FullReport, int32 DurationMs) const
{
    FRISelfTestResult Result;
    Result.Id = Definition.Id;
    Result.DisplayName = Definition.DisplayName;
    Result.bPassed = bPassed;
    Result.FullReport = FullReport;
    Result.Summary = BuildSelfTestSummary(FullReport);
    Result.StartedAt = FDateTime::UtcNow().ToIso8601();
    Result.DurationMs = DurationMs;
    return Result;
}

bool UInspectorWorldSubsystem::ExecuteSelfTestByIdInternal(FName TestId, FString& OutReport, bool& bOutPassed)
{
    OutReport.Reset();
    bOutPassed = false;

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (TestId == RI_SelfTestId_ConfirmDialog)
    {
        bOutPassed = RunConfirmDialogColorInputSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_SettingsPreview)
    {
        bOutPassed = RunSettingsPreviewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_SettingsSavePersistence)
    {
        bOutPassed = RunSettingsSavePersistenceSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_StarLiveEditAndRun)
    {
        bOutPassed = RunStarLiveEditAndRunSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_StarPreciseNavigation)
    {
        bOutPassed = RunStarPreciseNavigationSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_SettingsHotkey)
    {
        bOutPassed = RunSettingsHotkeyRebindSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_SettingsPageLayout)
    {
        bOutPassed = RunSettingsPageLayoutSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_ThemePresetPreview)
    {
        bOutPassed = RunThemePresetPreviewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PatchPreset)
    {
        bOutPassed = RunPatchPresetRoundtripSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PromotePreview)
    {
        bOutPassed = RunPromotePreviewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PromoteConfig)
    {
        bOutPassed = RunPromoteConfigSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PromoteBlueprintApply)
    {
        bOutPassed = RunPromoteBlueprintApplySelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PromoteMaterialApply)
    {
        bOutPassed = RunPromoteMaterialApplySelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_AuditReport)
    {
        bOutPassed = RunAuditReportSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FilePage)
    {
        bOutPassed = RunFilePageInjectionSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_ContextStrip)
    {
        bOutPassed = RunContextStripSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_WorkflowPageView)
    {
        bOutPassed = RunWorkflowPageViewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_TestPageLayout)
    {
        bOutPassed = RunTestPageLayoutSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_PanelInteraction)
    {
        bOutPassed = RunPanelInteractionSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_ActorPageStructure)
    {
        bOutPassed = RunActorPageStructureSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FileWorkflow)
    {
        bOutPassed = RunFileWorkflowSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FilePromote)
    {
        bOutPassed = RunFilePromoteWorkflowSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FileCompare)
    {
        bOutPassed = RunFileCompareViewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FileRoleCompare)
    {
        bOutPassed = RunFileRoleCompareViewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FileRemoteSessionCompare)
    {
        bOutPassed = RunFileRemoteSessionCompareViewSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_ActorPromoteFile)
    {
        bOutPassed = RunActorPromoteFileWorkflowSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_ActorApplyFile)
    {
        bOutPassed = RunActorApplyFileWorkflowSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RuntimeSessionRole)
    {
        bOutPassed = RunRuntimeSessionRoleSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RuntimeRoleCompare)
    {
        bOutPassed = RunRuntimeRoleCompareSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemoteRuntimeFoundation)
    {
        bOutPassed = RunRemoteRuntimeFoundationSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemoteSessionCompare)
    {
        bOutPassed = RunRemoteSessionCompareSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemoteSessionTargetSetCompare)
    {
        bOutPassed = RunRemoteSessionTargetSetCompareSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemoteSessionTargetSetCompareMatrix)
    {
        bOutPassed = RunRemoteSessionTargetSetCompareMatrixSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemoteSessionContextUI)
    {
        bOutPassed = RunRemoteSessionContextUISelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemotePackagedFoundation)
    {
        bOutPassed = RunRemotePackagedFoundationSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemotePackagedPatchPull)
    {
        bOutPassed = RunRemotePackagedPatchPullSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_RemotePackagedToSourceClosure)
    {
        bOutPassed = RunRemotePackagedToSourceClosureSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotFoundation)
    {
        bOutPassed = RunFabScreenshotFoundationSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotActorPage)
    {
        bOutPassed = RunFabScreenshotActorPageSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotSettingsPage)
    {
        bOutPassed = RunFabScreenshotSettingsPageSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotToolsPage)
    {
        bOutPassed = RunFabScreenshotToolsPageSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotRemoteSession)
    {
        bOutPassed = RunFabScreenshotRemoteSessionSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_FabScreenshotPromoteOrAudit)
    {
        bOutPassed = RunFabScreenshotPromoteOrAuditSelfTest(OutReport);
        return true;
    }

    if (TestId == RI_SelfTestId_WorkflowMatrix)
    {
        bOutPassed = RunWorkflowMatrixSelfTest(OutReport);
        return true;
    }

    OutReport = FString::Printf(TEXT("Unknown self test id: %s"), *TestId.ToString());
    return false;
#endif
}

bool UInspectorWorldSubsystem::RunSelfTestById(FName TestId, FRISelfTestResult& OutResult)
{
    OutResult = FRISelfTestResult();

#if !RUNTIME_INSPECTOR_ENABLED
    OutResult.Id = TestId;
    OutResult.DisplayName = TestId.ToString();
    OutResult.FullReport = TEXT("RuntimeInspector disabled");
    OutResult.Summary = OutResult.FullReport;
    return false;
#else
    const TArray<FRISelfTestDefinition> Definitions = GetAvailableSelfTests();
    const FRISelfTestDefinition* Definition = Definitions.FindByPredicate([TestId](const FRISelfTestDefinition& Candidate)
    {
        return Candidate.Id == TestId;
    });

    if (!Definition)
    {
        OutResult.Id = TestId;
        OutResult.DisplayName = TestId.ToString();
        OutResult.FullReport = FString::Printf(TEXT("Unknown self test id: %s"), *TestId.ToString());
        OutResult.Summary = OutResult.FullReport;
        OutResult.StartedAt = FDateTime::UtcNow().ToIso8601();
        OutResult.DurationMs = 0;
        return false;
    }

    if (Definition->bRequiresPIE && !IsSelfTestPIEAvailable())
    {
        OutResult = MakeSelfTestResult(*Definition, false, TEXT("Blocked: PIE with a local player controller is required."), 0);
        OutResult.StartedAt = FDateTime::UtcNow().ToIso8601();
        LastSelfTestResults.RemoveAll([TestId](const FRISelfTestResult& Existing) { return Existing.Id == TestId; });
        LastSelfTestResults.Add(OutResult);
        return false;
    }

    const FDateTime StartedAt = FDateTime::UtcNow();
    const double StartSeconds = FPlatformTime::Seconds();

    FString Report;
    bool bPassed = false;
    ExecuteSelfTestByIdInternal(TestId, Report, bPassed);

    const int32 DurationMs = FMath::Max(0, FMath::RoundToInt(static_cast<float>((FPlatformTime::Seconds() - StartSeconds) * 1000.0)));
    OutResult = MakeSelfTestResult(*Definition, bPassed, Report, DurationMs);
    OutResult.StartedAt = StartedAt.ToIso8601();

    LastSelfTestResults.RemoveAll([TestId](const FRISelfTestResult& Existing) { return Existing.Id == TestId; });
    LastSelfTestResults.Add(OutResult);
    return bPassed;
#endif
}

void UInspectorWorldSubsystem::RunAllSelfTests(TArray<FRISelfTestResult>& OutResults)
{
    OutResults.Reset();
    LastSelfTestResults.Reset();

#if RUNTIME_INSPECTOR_ENABLED
    const TArray<FRISelfTestDefinition> Definitions = GetAvailableSelfTests();
    for (const FRISelfTestDefinition& Definition : Definitions)
    {
        FRISelfTestResult Result;
        RunSelfTestById(Definition.Id, Result);
        OutResults.Add(Result);
    }
#endif
}

TArray<FRIVerificationProfile> UInspectorWorldSubsystem::GetAvailableVerificationProfiles() const
{
    TArray<FRIVerificationProfile> Results;
#if RUNTIME_INSPECTOR_ENABLED
    auto AddProfile = [&Results](FName ProfileId, const TCHAR* DisplayName, const TCHAR* PatchCategory, std::initializer_list<FName> TestIds, bool bRequiresPIE = true)
    {
        FRIVerificationProfile Profile;
        Profile.ProfileId = ProfileId;
        Profile.DisplayName = DisplayName;
        Profile.PatchCategory = PatchCategory;
        Profile.RequiredSelfTestIds.Append(TestIds.begin(), static_cast<int32>(TestIds.size()));
        Profile.bRequiresPIE = bRequiresPIE;
        Profile.bMutatesRuntime = true;
        Results.Add(MoveTemp(Profile));
    };

    AddProfile(
        RI_VerificationProfileId_ColorRuntime,
        TEXT("Color Runtime Edit"),
        TEXT("UI"),
        { RI_SelfTestId_ConfirmDialog });

    AddProfile(
        RI_VerificationProfileId_SettingsPreview,
        TEXT("Settings Preview Apply"),
        TEXT("Settings"),
        { RI_SelfTestId_SettingsPreview });

    AddProfile(
        RI_VerificationProfileId_SettingsHotkey,
        TEXT("Settings Hotkey Rebind"),
        TEXT("Settings"),
        { RI_SelfTestId_SettingsHotkey });

    AddProfile(
        RI_VerificationProfileId_SettingsPageLayout,
        TEXT("Settings Page Layout"),
        TEXT("Settings"),
        { RI_SelfTestId_SettingsPageLayout });

    AddProfile(
        RI_VerificationProfileId_ThemePresetPreview,
        TEXT("Theme Preset Preview"),
        TEXT("Settings"),
        { RI_SelfTestId_ThemePresetPreview });

    AddProfile(
        RI_VerificationProfileId_PromotePreview,
        TEXT("Promote Preview"),
        TEXT("Promote"),
        { RI_SelfTestId_PromotePreview });

    AddProfile(
        RI_VerificationProfileId_PromoteConfig,
        TEXT("Promote Config"),
        TEXT("Promote"),
        { RI_SelfTestId_PromoteConfig },
        false);

    AddProfile(
        RI_VerificationProfileId_PromoteBlueprintApply,
        TEXT("Promote Blueprint Apply"),
        TEXT("Promote"),
        { RI_SelfTestId_PromoteBlueprintApply });

    AddProfile(
        RI_VerificationProfileId_PromoteMaterialApply,
        TEXT("Promote Material Apply"),
        TEXT("Promote"),
        { RI_SelfTestId_PromoteMaterialApply });

    AddProfile(
        RI_VerificationProfileId_AuditReport,
        TEXT("Audit Report"),
        TEXT("Audit"),
        { RI_SelfTestId_AuditReport });

    AddProfile(
        RI_VerificationProfileId_FilePromote,
        TEXT("File Promote Workflow"),
        TEXT("File"),
        { RI_SelfTestId_FilePromote },
        false);

    AddProfile(
        RI_VerificationProfileId_FileCompare,
        TEXT("File Compare View"),
        TEXT("File"),
        { RI_SelfTestId_FileCompare });

    AddProfile(
        RI_VerificationProfileId_FileRoleCompare,
        TEXT("File Role Compare View"),
        TEXT("File"),
        { RI_SelfTestId_FileRoleCompare });

    AddProfile(
        RI_VerificationProfileId_FileRemoteSessionCompare,
        TEXT("File Remote Session Compare View"),
        TEXT("File"),
        { RI_SelfTestId_FileRemoteSessionCompare });

    AddProfile(
        RI_VerificationProfileId_ContextStrip,
        TEXT("Context Strip"),
        TEXT("UI"),
        { RI_SelfTestId_ContextStrip });

    AddProfile(
        RI_VerificationProfileId_WorkflowPageView,
        TEXT("Workflow Page View"),
        TEXT("Workflow"),
        { RI_SelfTestId_WorkflowPageView });

    AddProfile(
        RI_VerificationProfileId_TestPageLayout,
        TEXT("Test Page Layout"),
        TEXT("Workflow"),
        { RI_SelfTestId_TestPageLayout });

    AddProfile(
        RI_VerificationProfileId_ActorPromoteFile,
        TEXT("Actor Promote File Workflow"),
        TEXT("Actor"),
        { RI_SelfTestId_ActorPromoteFile });

    AddProfile(
        RI_VerificationProfileId_ActorApplyFile,
        TEXT("Actor Apply File Workflow"),
        TEXT("Actor"),
        { RI_SelfTestId_ActorApplyFile });

    AddProfile(
        RI_VerificationProfileId_RuntimeSessionRole,
        TEXT("Runtime Session Role"),
        TEXT("Runtime"),
        { RI_SelfTestId_RuntimeSessionRole });

    AddProfile(
        RI_VerificationProfileId_RuntimeRoleCompare,
        TEXT("Runtime Role Compare"),
        TEXT("Runtime"),
        { RI_SelfTestId_RuntimeRoleCompare });

    AddProfile(
        RI_VerificationProfileId_RemoteRuntimeFoundation,
        TEXT("Remote Runtime Foundation"),
        TEXT("Remote"),
        { RI_SelfTestId_RemoteRuntimeFoundation });

    AddProfile(
        RI_VerificationProfileId_RemoteSessionCompare,
        TEXT("Remote Session Compare"),
        TEXT("Remote"),
        { RI_SelfTestId_RemoteSessionCompare });

    AddProfile(
        RI_VerificationProfileId_RemoteSessionTargetSetCompare,
        TEXT("Remote Session Target Set Compare"),
        TEXT("Remote"),
        { RI_SelfTestId_RemoteSessionTargetSetCompare });

    AddProfile(
        RI_VerificationProfileId_RemoteSessionTargetSetCompareMatrix,
        TEXT("Remote Session Target Set Compare Matrix"),
        TEXT("Remote"),
        { RI_SelfTestId_RemoteSessionTargetSetCompareMatrix });

    AddProfile(
        RI_VerificationProfileId_RemoteSessionContextUI,
        TEXT("Remote Session Context UI"),
        TEXT("Remote"),
        { RI_SelfTestId_RemoteSessionContextUI });

    AddProfile(
        RI_VerificationProfileId_RemotePackagedFoundation,
        TEXT("Remote Packaged Foundation"),
        TEXT("Remote"),
        { RI_SelfTestId_RemotePackagedFoundation },
        false);

    AddProfile(
        RI_VerificationProfileId_RemotePackagedPatchPull,
        TEXT("Remote Packaged Patch Pull"),
        TEXT("Remote"),
        { RI_SelfTestId_RemotePackagedPatchPull },
        false);

    AddProfile(
        RI_VerificationProfileId_RemotePackagedToSourceClosure,
        TEXT("Remote Packaged To Source Closure"),
        TEXT("Remote"),
        { RI_SelfTestId_RemotePackagedToSourceClosure },
        false);

    AddProfile(
        RI_VerificationProfileId_FabScreenshotFoundation,
        TEXT("Fab Screenshot Foundation"),
        TEXT("Presentation"),
        { RI_SelfTestId_FabScreenshotFoundation });

    AddProfile(
        RI_VerificationProfileId_WorkflowMatrix,
        TEXT("Workflow Matrix"),
        TEXT("Workflow"),
        { RI_SelfTestId_WorkflowMatrix });
#endif
    return Results;
}

bool UInspectorWorldSubsystem::RunVerificationProfile(FName ProfileId, FRIVerificationRunResult& OutResult)
{
    OutResult = FRIVerificationRunResult();

#if !RUNTIME_INSPECTOR_ENABLED
    OutResult.ProfileId = ProfileId;
    OutResult.DisplayName = ProfileId.ToString();
    OutResult.Summary = TEXT("RuntimeInspector disabled");
    OutResult.FullReport = OutResult.Summary;
    OutResult.bPassed = false;
    OutResult.bBlocked = true;
    LastVerificationRunResult = OutResult;
    return false;
#else
    const TArray<FRIVerificationProfile> Profiles = GetAvailableVerificationProfiles();
    const FRIVerificationProfile* Profile = Profiles.FindByPredicate([ProfileId](const FRIVerificationProfile& Candidate)
    {
        return Candidate.ProfileId == ProfileId;
    });

    if (!Profile)
    {
        OutResult.ProfileId = ProfileId;
        OutResult.DisplayName = ProfileId.ToString();
        OutResult.Summary = FString::Printf(TEXT("Unknown verification profile: %s"), *ProfileId.ToString());
        OutResult.FullReport = OutResult.Summary;
        OutResult.bPassed = false;
        OutResult.bBlocked = false;
        LastVerificationRunResult = OutResult;
        return false;
    }

    OutResult.ProfileId = Profile->ProfileId;
    OutResult.DisplayName = Profile->DisplayName;
    OutResult.PatchCategory = Profile->PatchCategory;

    if (Profile->bRequiresPIE && !IsSelfTestPIEAvailable())
    {
        OutResult.bPassed = false;
        OutResult.bBlocked = true;
        OutResult.Summary = TEXT("Blocked: PIE with a local player controller is required.");
        OutResult.FullReport = OutResult.Summary;
        LastVerificationRunResult = OutResult;
        return false;
    }

    TArray<FString> ReportSections;
    bool bAnyFailed = false;
    bool bAnyBlocked = false;
    for (const FName& TestId : Profile->RequiredSelfTestIds)
    {
        FRISelfTestResult TestResult;
        RunSelfTestById(TestId, TestResult);
        OutResult.TestResults.Add(TestResult);

        const bool bBlocked = TestResult.FullReport.StartsWith(TEXT("Blocked:"), ESearchCase::CaseSensitive)
            || TestResult.Summary.StartsWith(TEXT("Blocked:"), ESearchCase::CaseSensitive);
        bAnyBlocked = bAnyBlocked || bBlocked;
        bAnyFailed = bAnyFailed || (!TestResult.bPassed && !bBlocked);
        ReportSections.Add(FString::Printf(TEXT("[%s] %s"), *TestResult.DisplayName, *TestResult.FullReport));
    }

    OutResult.bBlocked = bAnyBlocked;
    OutResult.bPassed = !bAnyBlocked && !bAnyFailed && OutResult.TestResults.Num() > 0;
    OutResult.Summary = FString::Printf(
        TEXT("%s=%s Tests=%d"),
        *OutResult.DisplayName,
        OutResult.bBlocked ? TEXT("BLOCKED") : (OutResult.bPassed ? TEXT("PASS") : TEXT("FAIL")),
        OutResult.TestResults.Num());
    OutResult.FullReport = FString::Join(ReportSections, TEXT("\n"));
    LastVerificationRunResult = OutResult;
    return OutResult.bPassed;
#endif
}

TArray<FRIWorkflowDefinition> UInspectorWorldSubsystem::GetAvailableWorkflows() const
{
    TArray<FRIWorkflowDefinition> Results;
#if RUNTIME_INSPECTOR_ENABLED
    auto AddWorkflow = [&Results](
        FName WorkflowId,
        const TCHAR* DisplayName,
        const TCHAR* Description,
        std::initializer_list<FName> VerificationProfileIds,
        std::initializer_list<FName> SelfTestIds,
        const TCHAR* Category,
        bool bRequiresPIE = true,
        bool bRequiresSelectedActor = false,
        bool bMutatesRuntime = false,
        bool bMutatesSource = false,
        std::initializer_list<const TCHAR*> Tags = {},
        std::initializer_list<FName> ChildWorkflowIds = {})
    {
        FRIWorkflowDefinition Workflow;
        Workflow.WorkflowId = WorkflowId;
        Workflow.DisplayName = DisplayName;
        Workflow.Description = Description;
        Workflow.Category = Category;
        Workflow.bRequiresPIE = bRequiresPIE;
        Workflow.bRequiresSelectedActor = bRequiresSelectedActor;
        Workflow.bMutatesRuntime = bMutatesRuntime;
        Workflow.bMutatesSource = bMutatesSource;
        for (const TCHAR* Tag : Tags)
        {
            Workflow.Tags.Add(Tag);
        }
        Workflow.ChildWorkflowIds.Append(ChildWorkflowIds.begin(), static_cast<int32>(ChildWorkflowIds.size()));
        Workflow.VerificationProfileIds.Append(VerificationProfileIds.begin(), static_cast<int32>(VerificationProfileIds.size()));
        Workflow.SelfTestIds.Append(SelfTestIds.begin(), static_cast<int32>(SelfTestIds.size()));
        Results.Add(MoveTemp(Workflow));
    };

    AddWorkflow(
        RI_WorkflowId_MainlineSafePatchCore,
        TEXT("Mainline Safe Patch Core"),
        TEXT("Runs the stable patch, settings, and file compare workflow inside PIE."),
        { RI_VerificationProfileId_RuntimeSessionRole, RI_VerificationProfileId_SettingsPreview, RI_VerificationProfileId_SettingsHotkey, RI_VerificationProfileId_FileCompare },
        { RI_SelfTestId_FilePage, RI_SelfTestId_FileWorkflow },
        TEXT("Mainline"),
        true,
        false,
        true,
        false,
        { TEXT("safe-patch"), TEXT("settings"), TEXT("file") });

    AddWorkflow(
        RI_WorkflowId_MainlinePromoteSourceAssets,
        TEXT("Mainline Promote Source Assets"),
        TEXT("Runs config, Blueprint, material, and File-page promote workflows with automatic restore."),
        { RI_VerificationProfileId_PromoteConfig, RI_VerificationProfileId_PromoteBlueprintApply, RI_VerificationProfileId_PromoteMaterialApply, RI_VerificationProfileId_FilePromote },
        {},
        TEXT("Mainline"),
        true,
        false,
        true,
        true,
        { TEXT("promote"), TEXT("source"), TEXT("assets") });

    AddWorkflow(
        RI_WorkflowId_MainlineActorPatchRoundtrip,
        TEXT("Mainline Actor Patch Roundtrip"),
        TEXT("Runs selected-actor patch preset, audit, and promote preview steps on a stable target actor."),
        { RI_VerificationProfileId_AuditReport, RI_VerificationProfileId_PromotePreview },
        { RI_SelfTestId_PatchPreset },
        TEXT("Actor"),
        true,
        true,
        true,
        false,
        { TEXT("actor"), TEXT("patch"), TEXT("roundtrip") });

    AddWorkflow(
        RI_WorkflowId_MainlineActorPromoteFileClosure,
        TEXT("Mainline Actor Promote File Closure"),
        TEXT("Runs selected-actor apply, stage, promote preview/apply, applied audit, clear, and restore steps through the File workflow."),
        { RI_VerificationProfileId_RuntimeSessionRole, RI_VerificationProfileId_ActorPromoteFile },
        { RI_SelfTestId_FilePage },
        TEXT("Actor"),
        true,
        true,
        true,
        true,
        { TEXT("actor"), TEXT("promote"), TEXT("file") });

    AddWorkflow(
        RI_WorkflowId_MainlineActorApplyFileClosure,
        TEXT("Mainline Actor Apply File Closure"),
        TEXT("Runs selected-actor apply, stage, compare, export, clear, and restore steps through the File workflow."),
        { RI_VerificationProfileId_RuntimeSessionRole, RI_VerificationProfileId_ActorApplyFile },
        { RI_SelfTestId_FilePage },
        TEXT("Actor"),
        true,
        true,
        true,
        false,
        { TEXT("actor"), TEXT("apply"), TEXT("file") });

    AddWorkflow(
        RI_WorkflowId_MainlineActorEndToEndClosure,
        TEXT("Mainline Actor End-To-End"),
        TEXT("Runs selected-actor patch roundtrip, file apply/compare, and file promote/apply/restore as one curated actor-aware closure."),
        {},
        {},
        TEXT("Mainline"),
        true,
        true,
        true,
        true,
        { TEXT("actor"), TEXT("end-to-end"), TEXT("promote"), TEXT("file") },
        { RI_WorkflowId_MainlineActorPatchRoundtrip, RI_WorkflowId_MainlineActorApplyFileClosure, RI_WorkflowId_MainlineActorPromoteFileClosure });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteActorEndToEndClosure,
        TEXT("Mainline Remote Actor End-To-End"),
        TEXT("Runs the remote-runtime foundation, remote compare matrix, role compare, and selected-actor end-to-end closure as one white-listed remote automation batch."),
        {},
        {},
        TEXT("Mainline"),
        true,
        true,
        true,
        true,
        { TEXT("remote"), TEXT("actor"), TEXT("end-to-end"), TEXT("batch"), TEXT("white-listed"), TEXT("orchestration") },
        { RI_WorkflowId_MainlineRemoteRuntimeFoundation, RI_WorkflowId_MainlineRemoteSessionCompareMatrixFoundation, RI_WorkflowId_MainlineRoleCompareFoundation, RI_WorkflowId_MainlineActorEndToEndClosure });

    AddWorkflow(
        RI_WorkflowId_MainlineRoleCompareFoundation,
        TEXT("Mainline Role Compare Foundation"),
        TEXT("Stages a runtime patch and proves role-aware compare output, missing-role reporting, verification states, and File-page role compare rendering."),
        { RI_VerificationProfileId_RuntimeRoleCompare, RI_VerificationProfileId_FileRoleCompare },
        {},
        TEXT("Phase3"),
        true,
        false,
        true,
        false,
        { TEXT("runtime"), TEXT("role-aware"), TEXT("compare") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteRuntimeFoundation,
        TEXT("Mainline Remote Runtime Foundation"),
        TEXT("Verifies the local PIE runtime can be discovered as an explicit runtime session, connected, and enumerated into remote-runtime targets."),
        { RI_VerificationProfileId_RemoteRuntimeFoundation },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("runtime"), TEXT("session"), TEXT("target") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemotePackagedFoundation,
        TEXT("Mainline Remote Packaged Foundation"),
        TEXT("Verifies an external packaged runtime can be discovered on loopback, connected, and enumerated into actor targets from the editor authority side."),
        { RI_VerificationProfileId_RemotePackagedFoundation },
        {},
        TEXT("Phase3"),
        false,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("packaged"), TEXT("loopback"), TEXT("foundation") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemotePackagedPatchPull,
        TEXT("Mainline Remote Packaged Patch Pull"),
        TEXT("Applies a property mutation in an external packaged runtime, pulls the patch bundle into the editor, stages it, and restores the packaged runtime value."),
        { RI_VerificationProfileId_RemotePackagedPatchPull },
        {},
        TEXT("Phase3"),
        false,
        false,
        true,
        false,
        { TEXT("remote"), TEXT("packaged"), TEXT("patch"), TEXT("pull"), TEXT("loopback") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemotePackagedToSourceClosure,
        TEXT("Mainline Remote Packaged To Source Closure"),
        TEXT("Runs the packaged runtime apply, patch pull, local stage, patch-vs-source audit, promote preview/apply, and source restore closure without requiring PIE."),
        { RI_VerificationProfileId_RemotePackagedToSourceClosure },
        {},
        TEXT("Phase3"),
        false,
        false,
        true,
        true,
        { TEXT("remote"), TEXT("packaged"), TEXT("source"), TEXT("closure"), TEXT("loopback") });

    AddWorkflow(
        RI_WorkflowId_FabScreenshotFoundation,
        TEXT("Fab Screenshot Foundation"),
        TEXT("Applies the clean RuntimeInspector presentation state used for Fab screenshots and first-screen capture."),
        { RI_VerificationProfileId_FabScreenshotFoundation },
        {},
        TEXT("Presentation"),
        true,
        false,
        false,
        false,
        { TEXT("fab"), TEXT("presentation"), TEXT("screenshot"), TEXT("changes"), TEXT("softcontrast") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionCompareFoundation,
        TEXT("Mainline Remote Session Compare"),
        TEXT("Verifies editor and PIE sessions can both be discovered and compared against the same actor query through the remote-runtime API."),
        { RI_VerificationProfileId_RemoteSessionCompare },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("compare"), TEXT("target") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionTargetSetCompareFoundation,
        TEXT("Mainline Remote Session Target Set Compare"),
        TEXT("Verifies editor and PIE sessions can compare filtered target inventories and report shared, missing, and mismatched targets through the remote-runtime API."),
        { RI_VerificationProfileId_RemoteSessionTargetSetCompare },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("target-set"), TEXT("compare"), TEXT("supports-session-override"), TEXT("supports-name-filter"), TEXT("supports-class-filter") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionCompareUIFoundation,
        TEXT("Mainline Remote Session Compare UI"),
        TEXT("Builds remote session target-set compare output and proves the File page renders session summary, preview, and compare rows from the shared cache."),
        { RI_VerificationProfileId_FileRemoteSessionCompare },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("target-set"), TEXT("file"), TEXT("supports-session-override"), TEXT("supports-name-filter"), TEXT("supports-class-filter") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionContextUIFoundation,
        TEXT("Mainline Remote Session Context UI"),
        TEXT("Verifies File and Test pages consume the same shared remote session, target query, and workflow context from the subsystem authority state."),
        { RI_VerificationProfileId_RemoteSessionContextUI },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("context"), TEXT("ui"), TEXT("file"), TEXT("test"), TEXT("packaged") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionCompareScopedUIFoundation,
        TEXT("Mainline Remote Session Compare Scoped UI"),
        TEXT("Runs the File-page remote session compare closure with explicit session pair and BP_TestVarsActor filters so orchestration can prove scoped compare output."),
        { RI_VerificationProfileId_FileRemoteSessionCompare, RI_VerificationProfileId_RemoteSessionTargetSetCompare },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("target-set"), TEXT("file"), TEXT("scoped"), TEXT("filter"), TEXT("bp-testvarsactor"), TEXT("supports-session-override"), TEXT("supports-name-filter"), TEXT("supports-class-filter") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteSessionCompareMatrixFoundation,
        TEXT("Mainline Remote Session Compare Matrix"),
        TEXT("Runs a curated white-listed matrix of remote session target-set compare requests so automation can validate full-inventory and BP_TestVarsActor-scoped session pairs as one remote closure."),
        { RI_VerificationProfileId_RemoteSessionTargetSetCompareMatrix },
        {},
        TEXT("Phase3"),
        true,
        false,
        false,
        false,
        { TEXT("remote"), TEXT("session"), TEXT("target-set"), TEXT("matrix"), TEXT("white-listed"), TEXT("supports-batch-orchestration") });

    AddWorkflow(
        RI_WorkflowId_MainlineRemoteWorkflowMatrixFoundation,
        TEXT("Mainline Remote Workflow Matrix"),
        TEXT("Runs a curated white-listed matrix of remote workflows so automation can validate remote foundations, compare matrices, and actor-aware apply/file closure as one stable batch."),
        { RI_VerificationProfileId_WorkflowMatrix },
        {},
        TEXT("Phase3"),
        true,
        false,
        true,
        false,
        { TEXT("remote"), TEXT("workflow"), TEXT("matrix"), TEXT("batch"), TEXT("white-listed"), TEXT("supports-batch-orchestration") });

    AddWorkflow(
        RI_WorkflowMatrixId_RemotePackagedDefault,
        TEXT("Mainline Remote Packaged Matrix"),
        TEXT("Runs the curated white-listed packaged runtime batch: session foundation, patch pull, and editor-side promote closure."),
        {},
        {},
        TEXT("Phase3"),
        false,
        false,
        true,
        true,
        { TEXT("remote"), TEXT("packaged"), TEXT("matrix"), TEXT("batch"), TEXT("white-listed"), TEXT("loopback") },
        { RI_WorkflowId_MainlineRemotePackagedFoundation, RI_WorkflowId_MainlineRemotePackagedPatchPull, RI_WorkflowId_MainlineRemotePackagedToSourceClosure });

    AddWorkflow(
        RI_WorkflowId_MainlineFullClosure,
        TEXT("Mainline Full Closure"),
        TEXT("Runs the curated RuntimeInspector closure suite: UI, settings, patch, compare, and promote."),
        { RI_VerificationProfileId_ColorRuntime, RI_VerificationProfileId_RuntimeSessionRole, RI_VerificationProfileId_RuntimeRoleCompare, RI_VerificationProfileId_RemoteRuntimeFoundation, RI_VerificationProfileId_RemoteSessionCompare, RI_VerificationProfileId_RemoteSessionTargetSetCompare, RI_VerificationProfileId_RemoteSessionContextUI, RI_VerificationProfileId_SettingsPreview, RI_VerificationProfileId_SettingsHotkey, RI_VerificationProfileId_SettingsPageLayout, RI_VerificationProfileId_ThemePresetPreview, RI_VerificationProfileId_ContextStrip, RI_VerificationProfileId_PromoteConfig, RI_VerificationProfileId_PromoteBlueprintApply, RI_VerificationProfileId_PromoteMaterialApply, RI_VerificationProfileId_FileCompare, RI_VerificationProfileId_FileRoleCompare, RI_VerificationProfileId_FileRemoteSessionCompare, RI_VerificationProfileId_FilePromote, RI_VerificationProfileId_WorkflowPageView, RI_VerificationProfileId_TestPageLayout },
        { RI_SelfTestId_FilePage, RI_SelfTestId_FileWorkflow },
        TEXT("Mainline"),
        true,
        false,
        true,
        true,
        { TEXT("full"), TEXT("closure"), TEXT("file"), TEXT("promote") });
#endif
    return Results;
}

bool UInspectorWorldSubsystem::RunWorkflowById(FName WorkflowId, FRIWorkflowRunResult& OutResult)
{
    OutResult = FRIWorkflowRunResult();

#if !RUNTIME_INSPECTOR_ENABLED
    OutResult.WorkflowId = WorkflowId;
    OutResult.DisplayName = WorkflowId.ToString();
    OutResult.Summary = TEXT("RuntimeInspector disabled");
    OutResult.FullReport = OutResult.Summary;
    OutResult.bPassed = false;
    OutResult.bBlocked = true;
    LastWorkflowRunResult = OutResult;
    return false;
#else
    const TArray<FRIWorkflowDefinition> Workflows = GetAvailableWorkflows();
    TSet<FName> ActiveWorkflowIds;
    TFunction<bool(FName, FRIWorkflowRunResult&)> ExecuteWorkflow = [&](FName InWorkflowId, FRIWorkflowRunResult& LocalResult) -> bool
    {
        LocalResult = FRIWorkflowRunResult();

        const FRIWorkflowDefinition* Workflow = Workflows.FindByPredicate([InWorkflowId](const FRIWorkflowDefinition& Candidate)
        {
            return Candidate.WorkflowId == InWorkflowId;
        });

        if (!Workflow)
        {
            LocalResult.WorkflowId = InWorkflowId;
            LocalResult.DisplayName = InWorkflowId.ToString();
            LocalResult.Summary = FString::Printf(TEXT("Unknown workflow: %s"), *InWorkflowId.ToString());
            LocalResult.FullReport = LocalResult.Summary;
            return false;
        }

        LocalResult.WorkflowId = Workflow->WorkflowId;
        LocalResult.DisplayName = Workflow->DisplayName;
        LocalResult.Description = Workflow->Description;
        LocalResult.Category = Workflow->Category;
        LocalResult.Tags = Workflow->Tags;
        LocalResult.bMutatedRuntime = Workflow->bMutatesRuntime;
        LocalResult.bMutatedSource = Workflow->bMutatesSource;
        LocalResult.SelectedActorPath = SelectedActor.IsValid() ? SelectedActor->GetPathName() : FString();

        if (ActiveWorkflowIds.Contains(InWorkflowId))
        {
            LocalResult.bPassed = false;
            LocalResult.bBlocked = true;
            LocalResult.Summary = FString::Printf(TEXT("Blocked: recursive workflow dependency detected for %s"), *InWorkflowId.ToString());
            LocalResult.FullReport = LocalResult.Summary;
            return false;
        }

        if (Workflow->bRequiresPIE && !IsSelfTestPIEAvailable())
        {
            LocalResult.bPassed = false;
            LocalResult.bBlocked = true;
            LocalResult.Summary = TEXT("Blocked: PIE with a local player controller is required.");
            LocalResult.FullReport = LocalResult.Summary;
            return false;
        }

        if (Workflow->bRequiresSelectedActor && !SelectedActor.IsValid())
        {
            LocalResult.bPassed = false;
            LocalResult.bBlocked = true;
            LocalResult.Summary = TEXT("Blocked: selected actor is required.");
            LocalResult.FullReport = LocalResult.Summary;
            return false;
        }

        const FRIRuntimeSessionTargetSetCompareRequest PreviousRemoteSessionCompareRequest = ActiveRemoteSessionTargetSetCompareRequest;
        if (InWorkflowId == RI_WorkflowId_MainlineRemoteSessionCompareScopedUIFoundation)
        {
            const bool bHasExplicitOverride =
                !PreviousRemoteSessionCompareRequest.LeftSessionId.TrimStartAndEnd().IsEmpty()
                || !PreviousRemoteSessionCompareRequest.RightSessionId.TrimStartAndEnd().IsEmpty()
                || !PreviousRemoteSessionCompareRequest.NameFilter.TrimStartAndEnd().IsEmpty()
                || !PreviousRemoteSessionCompareRequest.ClassFilter.TrimStartAndEnd().IsEmpty();
            if (!bHasExplicitOverride)
            {
                FRIRuntimeSessionTargetSetCompareRequest ScopedRequest;
                ScopedRequest.LeftSessionId = TEXT("local_editor_current");
                ScopedRequest.RightSessionId = TEXT("local_pie_current");
                ScopedRequest.NameFilter = TEXT("BP_TestVarsActor");
                ScopedRequest.ClassFilter = TEXT("BP_TestVarsActor");
                ActiveRemoteSessionTargetSetCompareRequest = ScopedRequest;
            }
        }

        ActiveWorkflowIds.Add(InWorkflowId);

        TArray<FString> ReportSections;
        bool bAnyFailed = false;
        bool bAnyBlocked = false;

        for (const FName& ChildWorkflowId : Workflow->ChildWorkflowIds)
        {
            FRIWorkflowRunResult ChildResult;
            ExecuteWorkflow(ChildWorkflowId, ChildResult);

            LocalResult.ExecutedChildWorkflowIds.Add(ChildWorkflowId);
            LocalResult.ExecutedChildWorkflowSummaries.Add(ChildResult.Summary);
            LocalResult.bMutatedRuntime = LocalResult.bMutatedRuntime || ChildResult.bMutatedRuntime;
            LocalResult.bMutatedSource = LocalResult.bMutatedSource || ChildResult.bMutatedSource;

            const bool bStepPassed = ChildResult.bPassed && !ChildResult.bBlocked;
            LocalResult.PassedStepCount += bStepPassed ? 1 : 0;
            LocalResult.FailedStepCount += bStepPassed ? 0 : 1;
            bAnyBlocked = bAnyBlocked || ChildResult.bBlocked;
            bAnyFailed = bAnyFailed || !bStepPassed;
            ReportSections.Add(FString::Printf(TEXT("[Workflow] %s :: %s"), *ChildResult.DisplayName, *ChildResult.FullReport));
        }

        for (const FName& ProfileId : Workflow->VerificationProfileIds)
        {
            FRIVerificationRunResult VerificationResult;
            RunVerificationProfile(ProfileId, VerificationResult);
            LocalResult.VerificationResults.Add(VerificationResult);

            const bool bStepPassed = VerificationResult.bPassed && !VerificationResult.bBlocked;
            LocalResult.PassedStepCount += bStepPassed ? 1 : 0;
            LocalResult.FailedStepCount += bStepPassed ? 0 : 1;
            bAnyBlocked = bAnyBlocked || VerificationResult.bBlocked;
            bAnyFailed = bAnyFailed || !bStepPassed;
            ReportSections.Add(FString::Printf(TEXT("[Profile] %s :: %s"), *VerificationResult.DisplayName, *VerificationResult.FullReport));
        }

        for (const FName& TestId : Workflow->SelfTestIds)
        {
            FRISelfTestResult TestResult;
            RunSelfTestById(TestId, TestResult);
            LocalResult.SelfTestResults.Add(TestResult);

            const bool bBlocked = TestResult.FullReport.StartsWith(TEXT("Blocked:"), ESearchCase::CaseSensitive)
                || TestResult.Summary.StartsWith(TEXT("Blocked:"), ESearchCase::CaseSensitive);
            const bool bStepPassed = TestResult.bPassed && !bBlocked;
            LocalResult.PassedStepCount += bStepPassed ? 1 : 0;
            LocalResult.FailedStepCount += bStepPassed ? 0 : 1;
            bAnyBlocked = bAnyBlocked || bBlocked;
            bAnyFailed = bAnyFailed || !bStepPassed;
            ReportSections.Add(FString::Printf(TEXT("[SelfTest] %s :: %s"), *TestResult.DisplayName, *TestResult.FullReport));
        }

        ActiveWorkflowIds.Remove(InWorkflowId);
        ActiveRemoteSessionTargetSetCompareRequest = PreviousRemoteSessionCompareRequest;

        const int32 TotalStepCount = LocalResult.ExecutedChildWorkflowIds.Num() + LocalResult.VerificationResults.Num() + LocalResult.SelfTestResults.Num();
        LocalResult.bBlocked = bAnyBlocked;
        LocalResult.bPassed = !bAnyBlocked && !bAnyFailed && TotalStepCount > 0;
        LocalResult.Summary = FString::Printf(
            TEXT("%s=%s | Passed=%d Failed=%d"),
            *LocalResult.WorkflowId.ToString(),
            LocalResult.bBlocked ? TEXT("BLOCKED") : (LocalResult.bPassed ? TEXT("PASS") : TEXT("FAIL")),
            LocalResult.PassedStepCount,
            LocalResult.FailedStepCount);
        LocalResult.FullReport = FString::Join(ReportSections, TEXT("\n"));
        return LocalResult.bPassed;
    };

    ExecuteWorkflow(WorkflowId, OutResult);
    LastWorkflowRunResult = OutResult;
    return OutResult.bPassed;
#endif
}

TArray<FRIWorkflowMatrixDefinition> UInspectorWorldSubsystem::GetAvailableWorkflowMatrices() const
{
    TArray<FRIWorkflowMatrixDefinition> Results;
#if RUNTIME_INSPECTOR_ENABLED
    FRIWorkflowMatrixDefinition DefaultMatrix;
    DefaultMatrix.MatrixId = RI_WorkflowMatrixId_Default;
    DefaultMatrix.DisplayName = TEXT("Mainline Remote Workflow Matrix");
    DefaultMatrix.Description = TEXT("Runs the curated remote workflow batch: remote runtime foundation, remote session compare matrix, and actor-aware apply/file closure.");
    DefaultMatrix.bRequiresPIE = true;

    FRIWorkflowMatrixEntry RemoteRuntimeEntry;
    RemoteRuntimeEntry.EntryId = TEXT("remote_runtime_foundation");
    RemoteRuntimeEntry.DisplayName = TEXT("Remote Runtime Foundation");
    RemoteRuntimeEntry.Description = TEXT("Verifies remote session discovery and connection for the active PIE runtime.");
    RemoteRuntimeEntry.WorkflowId = RI_WorkflowId_MainlineRemoteRuntimeFoundation;
    DefaultMatrix.Entries.Add(MoveTemp(RemoteRuntimeEntry));

    FRIWorkflowMatrixEntry RemoteCompareMatrixEntry;
    RemoteCompareMatrixEntry.EntryId = TEXT("remote_session_compare_matrix");
    RemoteCompareMatrixEntry.DisplayName = TEXT("Remote Session Compare Matrix");
    RemoteCompareMatrixEntry.Description = TEXT("Runs the white-listed remote session target-set compare matrix.");
    RemoteCompareMatrixEntry.WorkflowId = RI_WorkflowId_MainlineRemoteSessionCompareMatrixFoundation;
    DefaultMatrix.Entries.Add(MoveTemp(RemoteCompareMatrixEntry));

    FRIWorkflowMatrixEntry ActorApplyEntry;
    ActorApplyEntry.EntryId = TEXT("actor_apply_file_closure");
    ActorApplyEntry.DisplayName = TEXT("Actor Apply File Closure");
    ActorApplyEntry.Description = TEXT("Runs the actor-aware apply/file closure against BP_TestVarsActor as the stable mutating step of the remote batch.");
    ActorApplyEntry.WorkflowId = RI_WorkflowId_MainlineActorApplyFileClosure;
    ActorApplyEntry.ActorQuery = TEXT("BP_TestVarsActor");
    DefaultMatrix.Entries.Add(MoveTemp(ActorApplyEntry));

    Results.Add(MoveTemp(DefaultMatrix));

    FRIWorkflowMatrixDefinition PackagedMatrix;
    PackagedMatrix.MatrixId = RI_WorkflowMatrixId_RemotePackagedDefault;
    PackagedMatrix.DisplayName = TEXT("Mainline Remote Packaged Matrix");
    PackagedMatrix.Description = TEXT("Runs the curated packaged loopback batch: packaged session foundation, packaged patch pull, and packaged-to-source closure.");
    PackagedMatrix.bRequiresPIE = false;

    FRIWorkflowMatrixEntry PackagedFoundationEntry;
    PackagedFoundationEntry.EntryId = TEXT("remote_packaged_foundation");
    PackagedFoundationEntry.DisplayName = TEXT("Remote Packaged Foundation");
    PackagedFoundationEntry.Description = TEXT("Discovers and connects to the loopback packaged runtime session.");
    PackagedFoundationEntry.WorkflowId = RI_WorkflowId_MainlineRemotePackagedFoundation;
    PackagedMatrix.Entries.Add(MoveTemp(PackagedFoundationEntry));

    FRIWorkflowMatrixEntry PackagedPullEntry;
    PackagedPullEntry.EntryId = TEXT("remote_packaged_patch_pull");
    PackagedPullEntry.DisplayName = TEXT("Remote Packaged Patch Pull");
    PackagedPullEntry.Description = TEXT("Mutates a packaged runtime target, pulls the patch bundle, and stages it in the editor.");
    PackagedPullEntry.WorkflowId = RI_WorkflowId_MainlineRemotePackagedPatchPull;
    PackagedMatrix.Entries.Add(MoveTemp(PackagedPullEntry));

    FRIWorkflowMatrixEntry PackagedClosureEntry;
    PackagedClosureEntry.EntryId = TEXT("remote_packaged_to_source_closure");
    PackagedClosureEntry.DisplayName = TEXT("Remote Packaged To Source Closure");
    PackagedClosureEntry.Description = TEXT("Runs the packaged runtime to editor source closure for the staged BP_TestVarsActor change.");
    PackagedClosureEntry.WorkflowId = RI_WorkflowId_MainlineRemotePackagedToSourceClosure;
    PackagedMatrix.Entries.Add(MoveTemp(PackagedClosureEntry));

    Results.Add(MoveTemp(PackagedMatrix));
#endif
    return Results;
}

bool UInspectorWorldSubsystem::RunWorkflowMatrixById(FName MatrixId, FRIWorkflowMatrixRunResult& OutResult)
{
    OutResult = FRIWorkflowMatrixRunResult();

#if !RUNTIME_INSPECTOR_ENABLED
    OutResult.MatrixId = MatrixId;
    OutResult.DisplayName = MatrixId.ToString();
    OutResult.bPassed = false;
    OutResult.bBlocked = true;
    OutResult.Summary = TEXT("RuntimeInspector disabled");
    OutResult.FullReport = OutResult.Summary;
    LastWorkflowMatrixRunResult = OutResult;
    return false;
#else
    const TArray<FRIWorkflowMatrixDefinition> Definitions = GetAvailableWorkflowMatrices();
    const FRIWorkflowMatrixDefinition* Definition = Definitions.FindByPredicate([MatrixId](const FRIWorkflowMatrixDefinition& Candidate)
    {
        return Candidate.MatrixId == MatrixId;
    });

    if (!Definition)
    {
        OutResult.MatrixId = MatrixId;
        OutResult.DisplayName = MatrixId.ToString();
        OutResult.bPassed = false;
        OutResult.Summary = FString::Printf(TEXT("Unknown workflow matrix: %s"), *MatrixId.ToString());
        OutResult.FullReport = OutResult.Summary;
        LastWorkflowMatrixRunResult = OutResult;
        return false;
    }

    OutResult.MatrixId = Definition->MatrixId;
    OutResult.DisplayName = Definition->DisplayName;
    if (Definition->bRequiresPIE && !IsSelfTestPIEAvailable())
    {
        OutResult.bPassed = false;
        OutResult.bBlocked = true;
        OutResult.Summary = TEXT("Blocked: PIE with a local player controller is required.");
        OutResult.FullReport = OutResult.Summary;
        LastWorkflowMatrixRunResult = OutResult;
        return false;
    }

    const TArray<FRIWorkflowDefinition> Workflows = GetAvailableWorkflows();
    UWorld* World = GetWorld();
    AActor* MatrixPreviousSelectedActor = SelectedActor.Get();
    const FRIRuntimeSessionTargetSetCompareRequest MatrixPreviousRemoteSessionCompareRequest = ActiveRemoteSessionTargetSetCompareRequest;
    TArray<FString> ReportSections;

    for (const FRIWorkflowMatrixEntry& Entry : Definition->Entries)
    {
        FRIWorkflowMatrixEntryRunResult EntryResult;
        EntryResult.EntryId = Entry.EntryId;
        EntryResult.DisplayName = Entry.DisplayName;
        EntryResult.WorkflowId = Entry.WorkflowId;
        EntryResult.RequestedActor = Entry.ActorQuery;
        EntryResult.RemoteSessionCompareRequest = Entry.RemoteSessionCompareRequest;

        const FRIWorkflowDefinition* Workflow = Workflows.FindByPredicate([&Entry](const FRIWorkflowDefinition& Candidate)
        {
            return Candidate.WorkflowId == Entry.WorkflowId;
        });

        if (!Workflow)
        {
            EntryResult.bPassed = false;
            EntryResult.bBlocked = false;
            EntryResult.Summary = FString::Printf(TEXT("%s=FAIL | Unknown workflow=%s"), *Entry.EntryId.ToString(), *Entry.WorkflowId.ToString());
            EntryResult.FullReport = EntryResult.Summary;
        }
        else
        {
            AActor* EntryPreviousSelectedActor = SelectedActor.Get();
            const FRIRuntimeSessionTargetSetCompareRequest EntryPreviousRemoteSessionCompareRequest = ActiveRemoteSessionTargetSetCompareRequest;
            bool bEntryBlocked = false;

            if (!Entry.ActorQuery.TrimStartAndEnd().IsEmpty())
            {
                if (AActor* RequestedActor = RI_FindActorByRequest(World, Entry.ActorQuery))
                {
                    SetSelectedActor(RequestedActor);
                    EntryResult.RequestedActor = RequestedActor->GetPathName();
                }
                else
                {
                    bEntryBlocked = true;
                    EntryResult.RequestedActor = Entry.ActorQuery;
                    EntryResult.Summary = FString::Printf(TEXT("%s=BLOCKED | Actor not found=%s"), *Entry.EntryId.ToString(), *Entry.ActorQuery);
                    EntryResult.FullReport = EntryResult.Summary;
                }
            }

            const bool bHasRemoteSessionCompareOverride =
                !Entry.RemoteSessionCompareRequest.LeftSessionId.TrimStartAndEnd().IsEmpty()
                || !Entry.RemoteSessionCompareRequest.RightSessionId.TrimStartAndEnd().IsEmpty()
                || !Entry.RemoteSessionCompareRequest.NameFilter.TrimStartAndEnd().IsEmpty()
                || !Entry.RemoteSessionCompareRequest.ClassFilter.TrimStartAndEnd().IsEmpty();
            if (!bEntryBlocked && bHasRemoteSessionCompareOverride)
            {
                SetActiveRemoteSessionTargetSetCompareRequest(Entry.RemoteSessionCompareRequest);
            }

            if (!bEntryBlocked && Workflow->bRequiresSelectedActor && !SelectedActor.IsValid())
            {
                bEntryBlocked = true;
                EntryResult.Summary = FString::Printf(TEXT("%s=BLOCKED | Workflow requires selected actor"), *Entry.EntryId.ToString());
                EntryResult.FullReport = EntryResult.Summary;
            }

            if (!bEntryBlocked)
            {
                RunWorkflowById(Entry.WorkflowId, EntryResult.WorkflowRunResult);
                EntryResult.bPassed = EntryResult.WorkflowRunResult.bPassed;
                EntryResult.bBlocked = EntryResult.WorkflowRunResult.bBlocked;
                EntryResult.Summary = FString::Printf(
                    TEXT("%s=%s | Workflow=%s | RequestedActor=%s | Passed=%d Failed=%d"),
                    *Entry.EntryId.ToString(),
                    EntryResult.WorkflowRunResult.bBlocked ? TEXT("BLOCKED") : (EntryResult.WorkflowRunResult.bPassed ? TEXT("PASS") : TEXT("FAIL")),
                    *Entry.WorkflowId.ToString(),
                    EntryResult.RequestedActor.IsEmpty() ? TEXT("-") : *EntryResult.RequestedActor,
                    EntryResult.WorkflowRunResult.PassedStepCount,
                    EntryResult.WorkflowRunResult.FailedStepCount);
                EntryResult.FullReport = FString::Printf(
                    TEXT("%s | WorkflowSummary=%s"),
                    *EntryResult.Summary,
                    EntryResult.WorkflowRunResult.Summary.IsEmpty() ? TEXT("-") : *EntryResult.WorkflowRunResult.Summary);
            }

            if (SelectedActor.Get() != EntryPreviousSelectedActor)
            {
                SetSelectedActor(EntryPreviousSelectedActor);
            }
            SetActiveRemoteSessionTargetSetCompareRequest(EntryPreviousRemoteSessionCompareRequest);
        }

        const bool bEntryPassed = EntryResult.bPassed && !EntryResult.bBlocked;
        OutResult.PassedEntryCount += bEntryPassed ? 1 : 0;
        OutResult.FailedEntryCount += bEntryPassed ? 0 : 1;
        OutResult.bBlocked = OutResult.bBlocked || EntryResult.bBlocked;
        ReportSections.Add(EntryResult.FullReport);
        OutResult.EntryResults.Add(MoveTemp(EntryResult));
    }

    if (SelectedActor.Get() != MatrixPreviousSelectedActor)
    {
        SetSelectedActor(MatrixPreviousSelectedActor);
    }
    SetActiveRemoteSessionTargetSetCompareRequest(MatrixPreviousRemoteSessionCompareRequest);

    OutResult.bPassed = !OutResult.bBlocked && OutResult.FailedEntryCount == 0 && OutResult.EntryResults.Num() > 0;
    OutResult.Summary = FString::Printf(
        TEXT("%s=%s | Passed=%d Failed=%d"),
        *OutResult.MatrixId.ToString(),
        OutResult.bBlocked ? TEXT("BLOCKED") : (OutResult.bPassed ? TEXT("PASS") : TEXT("FAIL")),
        OutResult.PassedEntryCount,
        OutResult.FailedEntryCount);
    OutResult.FullReport = FString::Join(ReportSections, TEXT("\n"));
    LastWorkflowMatrixRunResult = OutResult;
    return OutResult.bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunConfirmDialogColorInputSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    LastConfirmDialogSelfTestReport = OutReport;
    bLastConfirmDialogSelfTestPassed = false;
    return false;
#else
    OutReport.Reset();
    LastConfirmDialogSelfTestReport.Reset();
    bLastConfirmDialogSelfTestPassed = false;

    UWorld* World = GetWorld();
    APlayerController* PC = GetLocalPC();
    UClass* ConfirmDialogClass = ConfirmDialogWidgetClass.LoadSynchronous();
    if (!World || !PC || !ConfirmDialogClass)
    {
        OutReport = FString::Printf(
            TEXT("Missing prerequisite. World=%s PC=%s ConfirmDialogClass=%s"),
            World ? TEXT("ok") : TEXT("null"),
            PC ? TEXT("ok") : TEXT("null"),
            ConfirmDialogClass ? TEXT("ok") : TEXT("null"));
        LastConfirmDialogSelfTestReport = OutReport;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] %s"), *OutReport);
        return false;
    }

    const bool bWasOpen = bOpen;
    const ERIVisiblePage PreviousVisiblePage = GetVisiblePage();
    if (!bOpen)
    {
        Open();
    }

    UUserWidget* TestWidget = CreateWidget<UUserWidget>(PC, ConfirmDialogClass);
    if (!TestWidget)
    {
        OutReport = TEXT("CreateWidget failed for WBP_ConfirmDialog");
        LastConfirmDialogSelfTestReport = OutReport;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] %s"), *OutReport);
        if (!bWasOpen)
        {
            Close();
        }
        return false;
    }

    TestWidget->AddToViewport(10001);

    bool bOverallSuccess = false;
    auto Cleanup = [&]()
    {
        if (UUserWidget* ActiveDialog = ActiveConfirmDialogWidget.Get())
        {
            ActiveDialog->RemoveFromParent();
        }

        ClearConfirmDialogBinding();
        if (TestWidget)
        {
            TestWidget->RemoveFromParent();
        }

        if (bWasOpen)
        {
            FString RestorePageError;
            SetVisiblePageByName(RI_GetVisiblePageDisplayLabel(PreviousVisiblePage), RestorePageError);
            RefreshPanel(EInspectorRefreshReason::StructureChanged);
            RefreshConfirmDialogBinding();
        }
        else
        {
            Close();
        }
    };

    UFunction* InitDataFn = TestWidget->FindFunction(TEXT("InitData"));
    if (!InitDataFn)
    {
        OutReport = TEXT("InitData not found on WBP_ConfirmDialog");
        LastConfirmDialogSelfTestReport = OutReport;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] %s"), *OutReport);
        Cleanup();
        return false;
    }

    struct FRIInitDataParams
    {
        FString Title;
        FString Content;
        FString Type;
    };

    FRIInitDataParams InitDataParams;
    InitDataParams.Title = TEXT("Color");
    InitDataParams.Content = TEXT("FF0000FF");
    InitDataParams.Type = TEXT("Color");
    TestWidget->ProcessEvent(InitDataFn, &InitDataParams);
    const bool bDirectColorPageOk = TryActivateConfirmDialogColorPage(TestWidget) && IsConfirmDialogColorPageActive(TestWidget);

    if (!TryBindActiveConfirmDialog(TestWidget))
    {
        OutReport = TEXT("Failed to bind active confirm dialog inputs");
        LastConfirmDialogSelfTestReport = OutReport;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] %s"), *OutReport);
        Cleanup();
        return false;
    }

    UEditableTextBox* InputR = ActiveConfirmDialogInputR.Get();
    UEditableTextBox* InputG = ActiveConfirmDialogInputG.Get();
    UEditableTextBox* InputB = ActiveConfirmDialogInputB.Get();
    UEditableTextBox* InputA = ActiveConfirmDialogInputA.Get();
    UEditableTextBox* InputHex = ActiveConfirmDialogInputHex.Get();
    if (!InputR || !InputG || !InputB || !InputA || !InputHex)
    {
        OutReport = TEXT("Failed to resolve one or more confirm dialog text boxes");
        LastConfirmDialogSelfTestReport = OutReport;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI] %s"), *OutReport);
        Cleanup();
        return false;
    }

    auto ParseDisplayedFloat = [this](const UEditableTextBox* TextBox, float DefaultValue)
    {
        float ParsedValue = DefaultValue;
        if (!TextBox)
        {
            return ParsedValue;
        }

        TryParseConfirmDialogUnitFloat(TextBox->GetText(), DefaultValue, ParsedValue);
        return ParsedValue;
    };
    const auto ColorNear = [](const FLinearColor& A, const FLinearColor& B, float Tolerance)
    {
        return FMath::IsNearlyEqual(A.R, B.R, Tolerance)
            && FMath::IsNearlyEqual(A.G, B.G, Tolerance)
            && FMath::IsNearlyEqual(A.B, B.B, Tolerance)
            && FMath::IsNearlyEqual(A.A, B.A, Tolerance);
    };

    const bool bInitialApply = ApplyActiveConfirmDialogColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
    const float InitialR = ParseDisplayedFloat(InputR, -1.0f);
    const float InitialG = ParseDisplayedFloat(InputG, -1.0f);
    const float InitialB = ParseDisplayedFloat(InputB, -1.0f);
    const float InitialA = ParseDisplayedFloat(InputA, -1.0f);

    InputR->SetText(FText::FromString(TEXT("0.25")));
    HandleConfirmDialogRChanged(InputR->GetText());
    const float AfterRValue = ParseDisplayedFloat(InputR, -1.0f);
    const FString HexAfterR = InputHex->GetText().ToString();

    InputHex->SetText(FText::FromString(TEXT("00FF00FF")));
    HandleConfirmDialogHexChanged(InputHex->GetText());
    const float AfterHexR = ParseDisplayedFloat(InputR, -1.0f);
    const FString GAfterHex = InputG->GetText().ToString();
    const float AfterHexG = ParseDisplayedFloat(InputG, -1.0f);
    const float AfterHexB = ParseDisplayedFloat(InputB, -1.0f);
    const float AfterHexA = ParseDisplayedFloat(InputA, -1.0f);
    const FString HexAfterHex = InputHex->GetText().ToString();

    const bool bPassR =
        bInitialApply &&
        FMath::IsNearlyEqual(InitialR, 1.0f, 0.02f) &&
        InitialG <= 0.02f &&
        InitialB <= 0.02f &&
        FMath::IsNearlyEqual(InitialA, 1.0f, 0.02f) &&
        FMath::IsNearlyEqual(AfterRValue, 0.25f, 0.02f);

    const bool bPassHex =
        AfterHexR <= 0.05f &&
        AfterHexG >= 0.95f &&
        AfterHexB <= 0.05f &&
        FMath::IsNearlyEqual(AfterHexA, 1.0f, 0.02f);

    TestWidget->RemoveFromParent();
    TestWidget = nullptr;
    ClearConfirmDialogBinding();

    bool bMaterialDialogOpened = false;
    bool bMaterialColorPageOk = false;
    bool bMaterialModalOk = false;
    bool bMaterialModalClearedOk = false;
    bool bMaterialPanelEnabledOk = false;
    bool bMaterialSwatchOk = false;
    bool bMaterialPreviewOk = false;
    bool bMaterialApplyOk = false;
    bool bInjectedMaterialRowFound = false;
    bool bInjectedMaterialSwatchVisible = false;
    bool bLegacyMaterialEntryFound = false;
    bool bLegacyMaterialColorButtonFound = false;
    FLinearColor InjectedMaterialSwatchColor = FLinearColor::Black;
    FLinearColor LegacyMaterialButtonColor = FLinearColor::Black;
    FString MaterialVectorName = TEXT("None");
    FString MaterialHexApplied = TEXT("None");

    FString FoundationSummary;
    FString FoundationError;
    if (ApplyFabScreenshotFoundationState(FoundationSummary, FoundationError))
    {
        FString ShowActorPageError;
        SetVisiblePageByName(TEXT("Actor"), ShowActorPageError);
        if (bDeferredOpenActorRefreshScheduled)
        {
            HandleDeferredOpenActorRefreshTimerElapsed();
        }

        SetContentSwitcherIndex(0);
        RefreshPanel(EInspectorRefreshReason::StructureChanged);
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            for (int32 Attempt = 0; Attempt < 2; ++Attempt)
            {
                if (FSlateApplication::IsInitialized())
                {
                    FSlateApplication::Get().Tick(ESlateTickType::All);
                }

                Panel->TakeWidget();
                Panel->ForceLayoutPrepass();
                if (TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget())
                {
                    CachedWidget->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
                }
            }
        }

        AActor* TestActor = SelectedActor.Get();
        if (TestActor)
        {
            TArray<UActorComponent*> Components;
            TestActor->GetComponents(Components);

            UInspectorMaterialParamItem* TestMaterialItem = nullptr;
            FLinearColor OriginalMaterialColor = FLinearColor::Black;
            for (UActorComponent* Component : Components)
            {
                UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
                if (!MeshComponent)
                {
                    continue;
                }

                for (int32 SlotIndex = 0; SlotIndex < MeshComponent->GetNumMaterials(); ++SlotIndex)
                {
                    UMaterialInterface* Material = MeshComponent->GetMaterial(SlotIndex);
                    if (!Material)
                    {
                        continue;
                    }

                    TArray<FMaterialParameterInfo> VectorInfos;
                    TArray<FGuid> ParameterIds;
                    Material->GetAllVectorParameterInfo(VectorInfos, ParameterIds);
                    if (VectorInfos.Num() == 0)
                    {
                        continue;
                    }

                    UInspectorMaterialParamItem* Candidate = GetOrCreateMaterialItem(MeshComponent, SlotIndex, VectorInfos[0].Name, EInspectorMatParamType::Vector);
                    if (!Candidate)
                    {
                        continue;
                    }

                    FString MaterialError;
                    FLinearColor CandidateColor = FLinearColor::Black;
                    if (!Candidate->GetVector(CandidateColor, MaterialError))
                    {
                        continue;
                    }

                    TestMaterialItem = Candidate;
                    OriginalMaterialColor = CandidateColor;
                    MaterialVectorName = Candidate->GetPropertyName();
                    break;
                }

                if (TestMaterialItem)
                {
                    break;
                }
            }

            if (TestMaterialItem)
            {
                if (UMeshComponent* ViewMeshComponent = TestMaterialItem->GetMeshComponent())
                {
                    SetPropertyView_MaterialOnly(ViewMeshComponent, TestMaterialItem->GetSlotIndex());
                    if (UUserWidget* Panel = PanelWidget.Get())
                    {
                        Panel->TakeWidget();
                        Panel->ForceLayoutPrepass();
                        if (TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget())
                        {
                            CachedWidget->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
                        }
                    }
                }

                const FLinearColor UpdatedMaterialColor = RI_MakeDistinctSelfTestColor(OriginalMaterialColor);
                MaterialHexApplied = UpdatedMaterialColor.ToFColorSRGB().ToHex();
                bMaterialDialogOpened = OpenColorEditorForAnyItem(TestMaterialItem);

                if (bMaterialDialogOpened)
                {
                    UUserWidget* MaterialDialog = ActiveConfirmDialogWidget.Get();
                    bMaterialColorPageOk = TryActivateConfirmDialogColorPage(MaterialDialog) && IsConfirmDialogColorPageActive(MaterialDialog);
                    bMaterialModalOk = ActiveConfirmDialogModalBlockerWidget.IsValid();

                    if (TrySetActiveConfirmDialogColor(UpdatedMaterialColor))
                    {
                        SyncActiveConfirmDialogColorPreview();
                    }

                    auto DoesDisplayedSwatchMatch = [this, TestMaterialItem, UpdatedMaterialColor, ColorNear](UListViewBase* PropertyList) -> bool
                    {
                        if (!PropertyList || !TestMaterialItem)
                        {
                            return false;
                        }

                        const FString TargetLabel = TestMaterialItem->GetPropertyName();
                        const TArray<UUserWidget*> DisplayedEntries = PropertyList->GetDisplayedEntryWidgets();
                        for (UUserWidget* EntryWidget : DisplayedEntries)
                        {
                            if (UInspectorMaterialParamRowWidget* MaterialRow = Cast<UInspectorMaterialParamRowWidget>(EntryWidget))
                            {
                                if (!MaterialRow->IsDisplayingItem(TestMaterialItem))
                                {
                                    continue;
                                }

                                FLinearColor SwatchColor = FLinearColor::Black;
                                return MaterialRow->TryGetDisplayedColorSwatchForAutomation(SwatchColor)
                                    && ColorNear(SwatchColor, UpdatedMaterialColor, 0.02f);
                            }

                            TFunction<bool(UWidget*)> HasTargetLabel = [&](UWidget* Widget) -> bool
                            {
                                if (!Widget)
                                {
                                    return false;
                                }

                                if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
                                {
                                    return TextBlock->GetText().ToString().Contains(TargetLabel);
                                }

                                if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
                                {
                                    return UserWidget->WidgetTree && HasTargetLabel(UserWidget->WidgetTree->RootWidget);
                                }

                                if (const UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
                                {
                                    for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
                                    {
                                        if (HasTargetLabel(PanelWidget->GetChildAt(ChildIndex)))
                                        {
                                            return true;
                                        }
                                    }
                                }

                                return false;
                            };

                            TFunction<bool(UWidget*)> HasMatchingSwatch = [&](UWidget* Widget) -> bool
                            {
                                if (!Widget)
                                {
                                    return false;
                                }

                                if (const UBorder* BorderWidget = Cast<UBorder>(Widget))
                                {
                                    if (ColorNear(BorderWidget->GetBrushColor(), UpdatedMaterialColor, 0.02f))
                                    {
                                        return true;
                                    }
                                }

                                if (const UButton* ButtonWidget = Cast<UButton>(Widget))
                                {
                                    if (ColorNear(ButtonWidget->GetBackgroundColor(), UpdatedMaterialColor, 0.02f))
                                    {
                                        return true;
                                    }
                                }

                                if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
                                {
                                    return UserWidget->WidgetTree && HasMatchingSwatch(UserWidget->WidgetTree->RootWidget);
                                }

                                if (const UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
                                {
                                    for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
                                    {
                                        if (HasMatchingSwatch(PanelWidget->GetChildAt(ChildIndex)))
                                        {
                                            return true;
                                        }
                                    }
                                }

                                return false;
                            };

                            if (HasTargetLabel(EntryWidget) && HasMatchingSwatch(EntryWidget))
                            {
                                return true;
                            }
                        }

                        return false;
                    };

                    auto CaptureLegacySwatchDetails = [TestMaterialItem, &bLegacyMaterialEntryFound, &bLegacyMaterialColorButtonFound, &LegacyMaterialButtonColor](UListViewBase* PropertyList)
                    {
                        if (!PropertyList || !TestMaterialItem)
                        {
                            return;
                        }

                        const TArray<UUserWidget*> DisplayedEntries = PropertyList->GetDisplayedEntryWidgets();
                        for (UUserWidget* EntryWidget : DisplayedEntries)
                        {
                            if (!EntryWidget)
                            {
                                continue;
                            }

                            UObject* BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("BoundItem"));
                            if (!BoundObject)
                            {
                                BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("MaterialItem"));
                            }
                            if (!BoundObject)
                            {
                                BoundObject = RI_ReadObjectPropertyByAuthoredName(EntryWidget, TEXT("Item"));
                            }

                            if (BoundObject != TestMaterialItem)
                            {
                                continue;
                            }

                            bLegacyMaterialEntryFound = true;
                            if (EntryWidget->WidgetTree)
                            {
                                if (UButton* ColorButton = Cast<UButton>(EntryWidget->WidgetTree->FindWidget(TEXT("BTN_color"))))
                                {
                                    bLegacyMaterialColorButtonFound = true;
                                    LegacyMaterialButtonColor = ColorButton->GetBackgroundColor();
                                }
                            }
                            break;
                        }
                    };

                    if (UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get())
                    {
                        if (UInspectorMaterialParamRowWidget* MaterialRow = SectionWidget->FindMaterialRowForAutomation(TestMaterialItem))
                        {
                            bInjectedMaterialRowFound = true;
                            bInjectedMaterialSwatchVisible = MaterialRow->TryGetDisplayedColorSwatchForAutomation(InjectedMaterialSwatchColor);
                            bMaterialSwatchOk = bInjectedMaterialSwatchVisible
                                && ColorNear(InjectedMaterialSwatchColor, UpdatedMaterialColor, 0.02f);
                        }
                    }

                    if (!bMaterialSwatchOk)
                    {
                        if (UUserWidget* Panel = PanelWidget.Get())
                        {
                            if (Panel->WidgetTree)
                            {
                                if (UListViewBase* PropertyList = Cast<UListViewBase>(Panel->WidgetTree->FindWidget(TEXT("LV_Properties"))))
                                {
                                    PropertyList->RequestRefresh();
                                    CaptureLegacySwatchDetails(PropertyList);
                                    bMaterialSwatchOk = DoesDisplayedSwatchMatch(PropertyList);
                                }
                            }
                        }
                    }

                    FLinearColor PreviewColor = FLinearColor::Black;
                    FString MaterialError;
                    bMaterialPreviewOk = TestMaterialItem->GetVector(PreviewColor, MaterialError)
                        && ColorNear(PreviewColor, UpdatedMaterialColor, 0.02f);

                    if (UEditableTextBox* MaterialHexInput = ActiveConfirmDialogInputHex.Get())
                    {
                        MaterialHexInput->SetText(FText::FromString(MaterialHexApplied));
                        HandleConfirmDialogHexChanged(MaterialHexInput->GetText());
                    }

                    FLinearColor AppliedColor = FLinearColor::Black;
                    bMaterialApplyOk = TestMaterialItem->GetVector(AppliedColor, MaterialError) && ColorNear(AppliedColor, UpdatedMaterialColor, 0.02f);
                    HandleActiveConfirmDialogAccepted();
                    bMaterialModalClearedOk = !ActiveConfirmDialogModalBlockerWidget.IsValid();
                    bMaterialPanelEnabledOk = !PanelWidget.IsValid() || PanelWidget->GetIsEnabled();

                    FString RestoreError;
                    TestMaterialItem->SetVector(OriginalMaterialColor, RestoreError);
                }
            }
        }
    }

    bOverallSuccess = bPassR
        && bPassHex
        && bDirectColorPageOk
        && bMaterialDialogOpened
        && bMaterialColorPageOk
        && bMaterialModalOk
        && bMaterialModalClearedOk
        && bMaterialPanelEnabledOk
        && bMaterialSwatchOk
        && bMaterialPreviewOk
        && bMaterialApplyOk;

    OutReport = FString::Printf(
        TEXT("ConfirmDialogColorInputSelfTest=%s | DirectPage=%d | InitialUI=(%.3f, %.3f, %.3f, %.3f) | AfterRUI=%.3f HexAfterR=%s | AfterHexUI=(%.3f, %.3f, %.3f, %.3f) GAfterHex=%s HexAfterHex=%s | MaterialDialog=%d Page=%d Modal=%d ModalCleared=%d PanelEnabled=%d Swatch=%d Preview=%d Apply=%d InjectedRow=%d InjectedSwatch=%d InjectedColor=(%.3f,%.3f,%.3f,%.3f) LegacyEntry=%d LegacyButton=%d LegacyColor=(%.3f,%.3f,%.3f,%.3f) Item=%s Hex=%s"),
        bOverallSuccess ? TEXT("PASS") : TEXT("FAIL"),
        bDirectColorPageOk ? 1 : 0,
        InitialR, InitialG, InitialB, InitialA,
        AfterRValue,
        *HexAfterR,
        AfterHexR, AfterHexG, AfterHexB, AfterHexA,
        *GAfterHex,
        *HexAfterHex,
        bMaterialDialogOpened ? 1 : 0,
        bMaterialColorPageOk ? 1 : 0,
        bMaterialModalOk ? 1 : 0,
        bMaterialModalClearedOk ? 1 : 0,
        bMaterialPanelEnabledOk ? 1 : 0,
        bMaterialSwatchOk ? 1 : 0,
        bMaterialPreviewOk ? 1 : 0,
        bMaterialApplyOk ? 1 : 0,
        bInjectedMaterialRowFound ? 1 : 0,
        bInjectedMaterialSwatchVisible ? 1 : 0,
        InjectedMaterialSwatchColor.R,
        InjectedMaterialSwatchColor.G,
        InjectedMaterialSwatchColor.B,
        InjectedMaterialSwatchColor.A,
        bLegacyMaterialEntryFound ? 1 : 0,
        bLegacyMaterialColorButtonFound ? 1 : 0,
        LegacyMaterialButtonColor.R,
        LegacyMaterialButtonColor.G,
        LegacyMaterialButtonColor.B,
        LegacyMaterialButtonColor.A,
        *MaterialVectorName,
        *MaterialHexApplied);

    Cleanup();
    LastConfirmDialogSelfTestReport = OutReport;
    bLastConfirmDialogSelfTestPassed = bOverallSuccess;
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI] %s"), *OutReport);
    return bOverallSuccess;
#endif
}

FString UInspectorWorldSubsystem::RunConfirmDialogColorInputSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunConfirmDialogColorInputSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunSettingsPreviewSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    const FRIEditableSettings OriginalSettings = GetEditableSettings();

    FRIEditableSettings PreviewSettings = OriginalSettings;
    PreviewSettings.OutlinePPWeight = FMath::Clamp(OriginalSettings.OutlinePPWeight + 0.15f, 0.05f, 5.0f);
    if (FMath::IsNearlyEqual(PreviewSettings.OutlinePPWeight, OriginalSettings.OutlinePPWeight, KINDA_SMALL_NUMBER))
    {
        PreviewSettings.OutlinePPWeight = FMath::Clamp(OriginalSettings.OutlinePPWeight + 0.35f, 0.05f, 5.0f);
    }

    FString Error;
    if (!PreviewApplySettings(PreviewSettings, Error))
    {
        OutReport = FString::Printf(TEXT("SettingsPreviewSelfTest=FAIL | PreviewError=%s"), *Error);
        return false;
    }

    const FRIEditableSettings AfterPreview = GetEditableSettings();
    const bool bPreviewApplied = FMath::IsNearlyEqual(AfterPreview.OutlinePPWeight, PreviewSettings.OutlinePPWeight, 0.001f);

    ReloadSettingsFromConfig();
    const FRIEditableSettings AfterReload = GetEditableSettings();
    const bool bReloadRestored = FMath::IsNearlyEqual(AfterReload.OutlinePPWeight, LastSavedSettingsSnapshot.OutlinePPWeight, 0.001f);

    const bool bPassed = bPreviewApplied && bReloadRestored;
    OutReport = FString::Printf(
        TEXT("SettingsPreviewSelfTest=%s | Original=%.3f PreviewTarget=%.3f AfterPreview=%.3f AfterReload=%.3f"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        OriginalSettings.OutlinePPWeight,
        PreviewSettings.OutlinePPWeight,
        AfterPreview.OutlinePPWeight,
        AfterReload.OutlinePPWeight);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunSettingsPreviewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunSettingsPreviewSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunSettingsSavePersistenceSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    const FRIEditableSettings OriginalSettings = GetEditableSettings();
    FRIEditableSettings CandidateSettings = OriginalSettings;
    CandidateSettings.OutlinePPWeight = FMath::Clamp(OriginalSettings.OutlinePPWeight + 0.25f, 0.05f, 5.0f);
    if (FMath::IsNearlyEqual(CandidateSettings.OutlinePPWeight, OriginalSettings.OutlinePPWeight, KINDA_SMALL_NUMBER))
    {
        CandidateSettings.OutlinePPWeight = FMath::Clamp(OriginalSettings.OutlinePPWeight + 0.5f, 0.05f, 5.0f);
    }

    FString Error;
    if (!PreviewApplySettings(CandidateSettings, Error))
    {
        OutReport = FString::Printf(TEXT("SettingsSavePersistenceSelfTest=FAIL | PreviewError=%s"), *Error);
        return false;
    }

    const FRIEditableSettings AfterPreview = GetEditableSettings();
    const bool bPreviewApplied = FMath::IsNearlyEqual(AfterPreview.OutlinePPWeight, CandidateSettings.OutlinePPWeight, 0.001f);
    if (!bPreviewApplied)
    {
        ReloadSettingsFromConfig();
        OutReport = FString::Printf(TEXT("SettingsSavePersistenceSelfTest=FAIL | PreviewDidNotApply Before=%.3f Preview=%.3f Current=%.3f"),
            OriginalSettings.OutlinePPWeight,
            CandidateSettings.OutlinePPWeight,
            AfterPreview.OutlinePPWeight);
        return false;
    }

    if (!SaveSettings(Error))
    {
        ReloadSettingsFromConfig();
        OutReport = FString::Printf(TEXT("SettingsSavePersistenceSelfTest=FAIL | SaveError=%s"), *Error);
        return false;
    }

    const FRIEditableSettings AfterSaveReload = GetEditableSettings();
    const bool bPersisted = FMath::IsNearlyEqual(AfterSaveReload.OutlinePPWeight, CandidateSettings.OutlinePPWeight, 0.001f);

    FRIEditableSettings RestoreSettings = OriginalSettings;
    PreviewApplySettings(RestoreSettings, Error);
    SaveSettings(Error);
    ReloadSettingsFromConfig();

    OutReport = FString::Printf(
        TEXT("SettingsSavePersistenceSelfTest=%s | Before=%.3f Saved=%.3f Reloaded=%.3f"),
        bPersisted ? TEXT("PASS") : TEXT("FAIL"),
        OriginalSettings.OutlinePPWeight,
        CandidateSettings.OutlinePPWeight,
        AfterSaveReload.OutlinePPWeight);
    return bPersisted;
#endif
}

FString UInspectorWorldSubsystem::RunSettingsSavePersistenceSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunSettingsSavePersistenceSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunStarLiveEditAndRunSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    FString FoundationSummary;
    FString Error;
    if (!ApplyFabScreenshotFoundationState(FoundationSummary, Error))
    {
        OutReport = FString::Printf(TEXT("StarLiveEditAndRunSelfTest=FAIL | FoundationError=%s"), *Error);
        return false;
    }

    if (!SetVisiblePageByName(TEXT("Actor"), Error))
    {
        OutReport = FString::Printf(TEXT("StarLiveEditAndRunSelfTest=FAIL | ShowActorError=%s"), *Error);
        return false;
    }

    AActor* PreferredActor = ResolvePreferredFabScreenshotActor();
    if (!PreferredActor)
    {
        OutReport = TEXT("StarLiveEditAndRunSelfTest=FAIL | Preferred actor unavailable");
        return false;
    }

    SetSelectedActor(PreferredActor);
    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();
    RefreshActorGroupsSection();

    UInspectorPropertyItem* TestPropertyItem = nullptr;
    UObject* TestPropertyTargetObject = nullptr;
    FString PropertyOriginalText;
    FString PropertyPatchedText;
    FString PropertyKind;
    {
        auto TrySelectVisibleComponentProperty = [&]() -> bool
        {
            TArray<UActorComponent*> Components;
            PreferredActor->GetComponents(Components);
            for (UActorComponent* Component : Components)
            {
                if (!Component)
                {
                    continue;
                }

                FString FocusError;
                if (!FocusSelectedActorComponentByName(Component->GetName(), FocusError))
                {
                    continue;
                }

                TArray<UObject*> ComponentItems;
                GetPropertyItemsForSelectedEx(TEXT(""), false, ComponentItems);
                for (UObject* ItemObject : ComponentItems)
                {
                    UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject);
                    if (!PropertyItem || !PropertyItem->IsEditable())
                    {
                        continue;
                    }

                    const EInspectorValueType ValueType = PropertyItem->GetValueType();
                    const bool bIsFloat = ValueType == EInspectorValueType::Float || ValueType == EInspectorValueType::Double;
                    const bool bIsInt = ValueType == EInspectorValueType::Int;
                    if (!bIsFloat && !bIsInt)
                    {
                        continue;
                    }

                    FString CurrentText = PropertyItem->GetValueText();
                    FString PatchedText;
                    FString Kind;
                    if (bIsFloat)
                    {
                        const double CurrentValue = FCString::Atod(*CurrentText);
                        const double CandidateValue = FMath::IsNearlyEqual(CurrentValue, 1.15, 0.001) ? 0.95 : (CurrentValue + 0.15);
                        PatchedText = FString::SanitizeFloat(CandidateValue);
                        Kind = TEXT("float");
                    }
                    else
                    {
                        const int32 CurrentValue = FCString::Atoi(*CurrentText);
                        PatchedText = FString::FromInt((CurrentValue == 0) ? 1 : 0);
                        Kind = TEXT("int");
                    }

                    if (CurrentText == PatchedText)
                    {
                        continue;
                    }

                    TestPropertyItem = PropertyItem;
                    TestPropertyTargetObject = PropertyItem->GetTargetObject();
                    PropertyOriginalText = CurrentText;
                    PropertyPatchedText = PatchedText;
                    PropertyKind = Kind;
                    return true;
                }
            }

            return false;
        };

        if (!TrySelectVisibleComponentProperty())
        {
            OutReport = TEXT("StarLiveEditAndRunSelfTest=FAIL | No writable visible component property found");
            return false;
        }
    }

    if (!TestPropertyItem)
    {
        OutReport = TEXT("StarLiveEditAndRunSelfTest=FAIL | Failed to resolve starred property item");
        return false;
    }

    UStaticMeshComponent* TestMeshComponent = nullptr;
    UInspectorMaterialParamItem* TestMaterialScalarItem = nullptr;
    float OriginalScalarValue = 0.0f;
    FString OriginalScalarError;
    for (UActorComponent* Component : PreferredActor->GetComponents())
    {
        UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
        if (!MeshComponent || MeshComponent->GetNumMaterials() <= 0)
        {
            continue;
        }

        SetPropertyView_MaterialOnly(MeshComponent, 0);
        RefreshActorPropertiesSection();

        TArray<UObject*> MaterialItems;
        GetPropertyItemsForSelectedEx(TEXT(""), false, MaterialItems);
        for (UObject* ItemObject : MaterialItems)
        {
            if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
            {
                if (MaterialItem->GetParamType() == EInspectorMatParamType::Scalar)
                {
                    TestMeshComponent = MeshComponent;
                    TestMaterialScalarItem = MaterialItem;
                    break;
                }
            }
        }

        if (TestMaterialScalarItem)
        {
            TestMaterialScalarItem->GetScalar(OriginalScalarValue, OriginalScalarError);
            break;
        }
    }

    if (!TestMaterialScalarItem)
    {
        PropertyViewMode = ERIPropertyViewMode::Full;
        RefreshActorPropertiesSection();
        OutReport = TEXT("StarLiveEditAndRunSelfTest=FAIL | No starred scalar material parameter available");
        return false;
    }

    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();

    UInspectorFunctionItem* TestFunctionItem = nullptr;
    {
        TArray<UInspectorFunctionItem*> FunctionItems;
        GetFunctionItemsForSelected(TEXT(""), FunctionItems);
        for (UInspectorFunctionItem* FunctionItem : FunctionItems)
        {
            if (FunctionItem && FunctionItem->IsValidItem() && FunctionItem->GetParameterCount() == 0)
            {
                TestFunctionItem = FunctionItem;
                break;
            }
        }

        if (!TestFunctionItem)
        {
            for (UInspectorFunctionItem* FunctionItem : FunctionItems)
            {
                if (FunctionItem && FunctionItem->IsValidItem())
                {
                    TestFunctionItem = FunctionItem;
                    break;
                }
            }
        }
    }

    if (!TestFunctionItem)
    {
        OutReport = TEXT("StarLiveEditAndRunSelfTest=FAIL | No callable function item available");
        return false;
    }

    const bool bPropertyWasFavorite = IsFavoriteForAnyItem(TestPropertyItem);
    const bool bMaterialWasFavorite = IsFavoriteForAnyItem(TestMaterialScalarItem);
    const bool bFunctionWasFavorite = IsFavoriteForAnyItem(TestFunctionItem);

    auto RestoreFavorites = [&]()
    {
        if (TestPropertyItem && IsFavoriteForAnyItem(TestPropertyItem) != bPropertyWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestPropertyItem);
        }
        if (TestMaterialScalarItem && IsFavoriteForAnyItem(TestMaterialScalarItem) != bMaterialWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestMaterialScalarItem);
        }
        if (TestFunctionItem && IsFavoriteForAnyItem(TestFunctionItem) != bFunctionWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestFunctionItem);
        }
    };

    if (!bPropertyWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestPropertyItem);
    }
    if (!bMaterialWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestMaterialScalarItem);
    }
    if (!bFunctionWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestFunctionItem);
    }

    RefreshActorGroupsSection();

    UInspectorPropertyRowWidget* PinnedPropertyRow = nullptr;
    UInspectorMaterialParamRowWidget* PinnedMaterialRow = nullptr;
    UInspectorFunctionRowWidget* PinnedFunctionRow = nullptr;
    if (ActorPinnedEntriesBoxStrong)
    {
        for (int32 ChildIndex = 0; ChildIndex < ActorPinnedEntriesBoxStrong->GetChildrenCount(); ++ChildIndex)
        {
            UWidget* Child = ActorPinnedEntriesBoxStrong->GetChildAt(ChildIndex);
            if (!PinnedPropertyRow)
            {
                if (UInspectorPropertyRowWidget* Row = Cast<UInspectorPropertyRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestPropertyItem))
                    {
                        PinnedPropertyRow = Row;
                        continue;
                    }
                }
            }

            if (!PinnedMaterialRow)
            {
                if (UInspectorMaterialParamRowWidget* Row = Cast<UInspectorMaterialParamRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestMaterialScalarItem))
                    {
                        PinnedMaterialRow = Row;
                        continue;
                    }
                }
            }

            if (!PinnedFunctionRow)
            {
                if (UInspectorFunctionRowWidget* Row = Cast<UInspectorFunctionRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestFunctionItem))
                    {
                        PinnedFunctionRow = Row;
                        continue;
                    }
                }
            }
        }
    }

    bool bPropertyEdited = false;
    FString PropertyAfterText;
    FString PropertyEditError;
    if (PinnedPropertyRow)
    {
        bPropertyEdited = PinnedPropertyRow->CommitTextValueForAutomation(PropertyPatchedText, PropertyEditError)
            && TestPropertyTargetObject
            && InspectorPropertyUtils::GetValueAsText(TestPropertyTargetObject, TestPropertyItem->GetPropertyFName(), PropertyAfterText)
            && RI_AreSelfTestPrimitiveValuesEquivalent(PropertyAfterText, PropertyPatchedText, PropertyKind);
    }

    const float TargetScalarValue = FMath::IsNearlyEqual(OriginalScalarValue, 0.35f, 0.001f) ? 0.65f : 0.35f;
    const FString TargetScalarText = FString::SanitizeFloat(TargetScalarValue);
    bool bMaterialEdited = false;
    FString MaterialEditError;
    float MaterialAfterValue = OriginalScalarValue;
    if (PinnedMaterialRow)
    {
        bMaterialEdited = PinnedMaterialRow->CommitScalarValueForAutomation(TargetScalarText, MaterialEditError)
            && TestMaterialScalarItem->GetScalar(MaterialAfterValue, MaterialEditError)
            && FMath::IsNearlyEqual(MaterialAfterValue, TargetScalarValue, 0.001f);
    }

    bool bFunctionRan = false;
    FString FunctionRunError;
    if (PinnedFunctionRow)
    {
        bFunctionRan = PinnedFunctionRow->InvokeForAutomation(FunctionRunError);
    }

    FString RestoreError;
    if (TestPropertyTargetObject)
    {
        ApplyPropertyTextNow(TestPropertyTargetObject, TestPropertyItem->GetPropertyFName(), PropertyOriginalText, RestoreError);
    }
    if (TestMaterialScalarItem)
    {
        TestMaterialScalarItem->SetScalar(OriginalScalarValue, RestoreError);
    }
    RestoreFavorites();
    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();
    RefreshActorGroupsSection();

    const bool bPassed = PinnedPropertyRow && PinnedMaterialRow && PinnedFunctionRow && bPropertyEdited && bMaterialEdited && bFunctionRan;
    OutReport = FString::Printf(
        TEXT("StarLiveEditAndRunSelfTest=%s | PropertyRow=%d PropertyEdit=%d MaterialRow=%d MaterialEdit=%d FunctionRow=%d FunctionRun=%d PropertyAfter=%s ScalarAfter=%.3f Function=%s Errors=%s|%s|%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        PinnedPropertyRow ? 1 : 0,
        bPropertyEdited ? 1 : 0,
        PinnedMaterialRow ? 1 : 0,
        bMaterialEdited ? 1 : 0,
        PinnedFunctionRow ? 1 : 0,
        bFunctionRan ? 1 : 0,
        *PropertyAfterText,
        MaterialAfterValue,
        TestFunctionItem ? *TestFunctionItem->GetFunctionName() : TEXT("None"),
        *PropertyEditError,
        *MaterialEditError,
        *FunctionRunError);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunStarLiveEditAndRunSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunStarLiveEditAndRunSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunStarPreciseNavigationSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    FString FoundationSummary;
    FString Error;
    if (!ApplyFabScreenshotFoundationState(FoundationSummary, Error))
    {
        OutReport = FString::Printf(TEXT("StarPreciseNavigationSelfTest=FAIL | FoundationError=%s"), *Error);
        return false;
    }

    if (!SetVisiblePageByName(TEXT("Actor"), Error))
    {
        OutReport = FString::Printf(TEXT("StarPreciseNavigationSelfTest=FAIL | ShowActorError=%s"), *Error);
        return false;
    }

    AActor* PreferredActor = ResolvePreferredFabScreenshotActor();
    if (!PreferredActor)
    {
        OutReport = TEXT("StarPreciseNavigationSelfTest=FAIL | Preferred actor unavailable");
        return false;
    }

    SetSelectedActor(PreferredActor);
    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();
    RefreshActorGroupsSection();

    UInspectorPropertyItem* TestPropertyItem = nullptr;
    {
        FName TestPropertyName = NAME_None;
        FString OriginalText;
        FString PatchedText;
        FString PropertyKind;
        double NumericOriginalValue = 0.0;
        double NumericTargetValue = 0.0;
        if (RI_SelectWritablePrimitivePropertyForSelfTest(
                PreferredActor,
                TestPropertyName,
                OriginalText,
                PatchedText,
                PropertyKind,
                NumericOriginalValue,
                NumericTargetValue))
        {
            TestPropertyItem = GetOrCreatePropertyItem(PreferredActor, TestPropertyName);
        }
    }

    UStaticMeshComponent* TestMeshComponent = nullptr;
    UInspectorMaterialParamItem* TestMaterialItem = nullptr;
    for (UActorComponent* Component : PreferredActor->GetComponents())
    {
        UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
        if (!MeshComponent || MeshComponent->GetNumMaterials() <= 0)
        {
            continue;
        }

        SetPropertyView_MaterialOnly(MeshComponent, 0);
        RefreshActorPropertiesSection();

        TArray<UObject*> MaterialItems;
        GetPropertyItemsForSelectedEx(TEXT(""), false, MaterialItems);
        for (UObject* ItemObject : MaterialItems)
        {
            if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
            {
                TestMeshComponent = MeshComponent;
                TestMaterialItem = MaterialItem;
                break;
            }
        }

        if (TestMaterialItem)
        {
            break;
        }
    }

    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();

    UInspectorFunctionItem* TestFunctionItem = nullptr;
    {
        TArray<UInspectorFunctionItem*> FunctionItems;
        GetFunctionItemsForSelected(TEXT(""), FunctionItems);
        for (UInspectorFunctionItem* FunctionItem : FunctionItems)
        {
            if (FunctionItem && FunctionItem->IsValidItem())
            {
                TestFunctionItem = FunctionItem;
                break;
            }
        }
    }

    if (!TestPropertyItem || !TestMaterialItem || !TestFunctionItem)
    {
        OutReport = TEXT("StarPreciseNavigationSelfTest=FAIL | Missing property/material/function candidate");
        return false;
    }

    const bool bPropertyWasFavorite = IsFavoriteForAnyItem(TestPropertyItem);
    const bool bMaterialWasFavorite = IsFavoriteForAnyItem(TestMaterialItem);
    const bool bFunctionWasFavorite = IsFavoriteForAnyItem(TestFunctionItem);

    auto RestoreFavorites = [&]()
    {
        if (IsFavoriteForAnyItem(TestPropertyItem) != bPropertyWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestPropertyItem);
        }
        if (IsFavoriteForAnyItem(TestMaterialItem) != bMaterialWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestMaterialItem);
        }
        if (IsFavoriteForAnyItem(TestFunctionItem) != bFunctionWasFavorite)
        {
            ToggleFavoriteForAnyItem(TestFunctionItem);
        }
    };

    if (!bPropertyWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestPropertyItem);
    }
    if (!bMaterialWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestMaterialItem);
    }
    if (!bFunctionWasFavorite)
    {
        ToggleFavoriteForAnyItem(TestFunctionItem);
    }

    RefreshActorGroupsSection();

    UInspectorPropertyRowWidget* PinnedPropertyRow = nullptr;
    UInspectorMaterialParamRowWidget* PinnedMaterialRow = nullptr;
    UInspectorFunctionRowWidget* PinnedFunctionRow = nullptr;
    if (ActorPinnedEntriesBoxStrong)
    {
        for (int32 ChildIndex = 0; ChildIndex < ActorPinnedEntriesBoxStrong->GetChildrenCount(); ++ChildIndex)
        {
            UWidget* Child = ActorPinnedEntriesBoxStrong->GetChildAt(ChildIndex);
            if (!PinnedPropertyRow)
            {
                if (UInspectorPropertyRowWidget* Row = Cast<UInspectorPropertyRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestPropertyItem))
                    {
                        PinnedPropertyRow = Row;
                    }
                }
            }
            if (!PinnedMaterialRow)
            {
                if (UInspectorMaterialParamRowWidget* Row = Cast<UInspectorMaterialParamRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestMaterialItem))
                    {
                        PinnedMaterialRow = Row;
                    }
                }
            }
            if (!PinnedFunctionRow)
            {
                if (UInspectorFunctionRowWidget* Row = Cast<UInspectorFunctionRowWidget>(Child))
                {
                    if (Row->IsDisplayingItem(TestFunctionItem))
                    {
                        PinnedFunctionRow = Row;
                    }
                }
            }
        }
    }

    bool bPropertyNavigateOk = false;
    bool bMaterialNavigateOk = false;
    bool bFunctionNavigateOk = false;
    FString PropertyNavigateError;
    FString MaterialNavigateError;
    FString FunctionNavigateError;

    if (PinnedPropertyRow)
    {
        bPropertyNavigateOk = PinnedPropertyRow->NavigateForAutomation(PropertyNavigateError)
            && ActorPropertiesSectionWidget.IsValid()
            && ActorPropertiesSectionWidget->FindPropertyRowForAutomation(TestPropertyItem) != nullptr;
    }

    if (PinnedMaterialRow)
    {
        bMaterialNavigateOk = PinnedMaterialRow->NavigateForAutomation(MaterialNavigateError)
            && TestMeshComponent
            && PropertyViewMode == ERIPropertyViewMode::MaterialOnly
            && GetFocusedInspectObject() == TestMeshComponent
            && ActorPropertiesSectionWidget.IsValid()
            && ActorPropertiesSectionWidget->FindMaterialRowForAutomation(TestMaterialItem) != nullptr;
    }

    if (PinnedFunctionRow)
    {
        bFunctionNavigateOk = PinnedFunctionRow->NavigateForAutomation(FunctionNavigateError)
            && ActorFunctionsSectionWidget.IsValid()
            && ActorFunctionsSectionWidget->FindFunctionRowForAutomation(TestFunctionItem) != nullptr;
    }

    RestoreFavorites();
    PropertyViewMode = ERIPropertyViewMode::Full;
    RefreshActorPropertiesSection();
    RefreshActorFunctionsSection();
    RefreshActorGroupsSection();

    const bool bPassed = bPropertyNavigateOk && bMaterialNavigateOk && bFunctionNavigateOk;
    OutReport = FString::Printf(
        TEXT("StarPreciseNavigationSelfTest=%s | Property=%d Material=%d Function=%d Errors=%s|%s|%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bPropertyNavigateOk ? 1 : 0,
        bMaterialNavigateOk ? 1 : 0,
        bFunctionNavigateOk ? 1 : 0,
        *PropertyNavigateError,
        *MaterialNavigateError,
        *FunctionNavigateError);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunStarPreciseNavigationSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunStarPreciseNavigationSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunSettingsHotkeyRebindSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    const FRIEditableSettings OriginalSettings = GetEditableSettings();

    const FKey OriginalToggle = OriginalSettings.ToggleKey;
    const FKey CandidateToggle = (OriginalToggle == EKeys::F9) ? EKeys::F10 : EKeys::F9;

    FRIEditableSettings PreviewSettings = OriginalSettings;
    PreviewSettings.ToggleKey = CandidateToggle;

    FString Error;
    if (!PreviewApplySettings(PreviewSettings, Error))
    {
        OutReport = FString::Printf(TEXT("SettingsHotkeySelfTest=FAIL | PreviewError=%s"), *Error);
        return false;
    }

    bool bHasNewBinding = false;
    bool bHasOldBinding = false;
    if (InspectorInputComponent)
    {
        for (const FInputKeyBinding& Binding : InspectorInputComponent->KeyBindings)
        {
            if (Binding.Chord.Key == CandidateToggle)
            {
                bHasNewBinding = true;
            }
            if (Binding.Chord.Key == OriginalToggle)
            {
                bHasOldBinding = true;
            }
        }
    }

    ReloadSettingsFromConfig();
    bool bRestoredOriginal = false;
    if (InspectorInputComponent)
    {
        for (const FInputKeyBinding& Binding : InspectorInputComponent->KeyBindings)
        {
            if (Binding.Chord.Key == LastSavedSettingsSnapshot.ToggleKey)
            {
                bRestoredOriginal = true;
                break;
            }
        }
    }

    const bool bPassed = bHasNewBinding && !bHasOldBinding && bRestoredOriginal;
    OutReport = FString::Printf(
        TEXT("SettingsHotkeySelfTest=%s | Old=%s New=%s NewBinding=%s OldBinding=%s Restored=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *OriginalToggle.GetDisplayName().ToString(),
        *CandidateToggle.GetDisplayName().ToString(),
        bHasNewBinding ? TEXT("yes") : TEXT("no"),
        bHasOldBinding ? TEXT("yes") : TEXT("no"),
        bRestoredOriginal ? TEXT("yes") : TEXT("no"));
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunSettingsHotkeyRebindSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunSettingsHotkeyRebindSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunSettingsPageLayoutSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("SettingsPageLayoutSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("SettingsPageLayoutSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    Close();
    Open();
    ShowSettingsPage();

    UPanelWidget* HostPanel = FindSettingsHostPanel();
    UInspectorSettingsPageWidget* Page = SettingsPageWidget.Get();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();

    bool bHostContainsPage = false;
    int32 VisibleLegacySiblingCount = 0;
    FString VisibleLegacySiblingNames;
    if (HostPanel && Page)
    {
        const int32 ChildCount = HostPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = HostPanel->GetChildAt(ChildIndex);
            if (Child == Page)
            {
                bHostContainsPage = true;
                continue;
            }

            if (Child && Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
                if (!VisibleLegacySiblingNames.IsEmpty())
                {
                    VisibleLegacySiblingNames += TEXT(",");
                }
                VisibleLegacySiblingNames += Child->GetName();
            }
        }
    }

    if (Page)
    {
        Page->TakeWidget();
        Page->RefreshFromSubsystem();
    }

    const bool bScrollOk = Page && Page->HasPageScrollRoot();
    const bool bFooterOk = Page && Page->HasFooterControls();
    const bool bStatusOk = Page && Page->HasStatusSection();
    const bool bInteractionOk = Page && Page->HasInteractionSection();
    const FString SessionText = Page ? Page->GetSessionValueText() : FString();
    const FString ActorText = Page ? Page->GetSelectedActorValueText() : FString();
    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    const bool bPassed = HostPanel && Page && bHostContainsPage && Switcher && ActiveIndex == SettingsPageIndex
        && VisibleLegacySiblingCount == 0
        && bScrollOk && bFooterOk && bStatusOk && bInteractionOk;

    OutReport = FString::Printf(
        TEXT("SettingsPageLayoutSelfTest=%s | ActiveIndex=%d Host=%s HostHasPage=%d VisibleLegacy=%d Scroll=%d Footer=%d Status=%d Interaction=%d Session=%s Actor=%s LegacyNames=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        ActiveIndex,
        HostPanel ? *HostPanel->GetName() : TEXT("None"),
        bHostContainsPage ? 1 : 0,
        VisibleLegacySiblingCount,
        bScrollOk ? 1 : 0,
        bFooterOk ? 1 : 0,
        bStatusOk ? 1 : 0,
        bInteractionOk ? 1 : 0,
        SessionText.IsEmpty() ? TEXT("None") : *SessionText,
        ActorText.IsEmpty() ? TEXT("None") : *ActorText,
        VisibleLegacySiblingNames.IsEmpty() ? TEXT("None") : *VisibleLegacySiblingNames);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunSettingsPageLayoutSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunSettingsPageLayoutSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunThemePresetPreviewSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("ThemePresetPreviewSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("ThemePresetPreviewSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    if (RI_GetThemePresetOverrideValue() != -1)
    {
        OutReport = TEXT("ThemePresetPreviewSelfTest=BLOCKED | ri.ThemePreset override active");
        return false;
    }

    const ERuntimeInspectorThemePreset OriginalPreset = GetThemePreset();
    const ERuntimeInspectorThemePreset AlternatePreset = OriginalPreset == ERuntimeInspectorThemePreset::StudioSlate
        ? ERuntimeInspectorThemePreset::SoftContrast
        : ERuntimeInspectorThemePreset::StudioSlate;

    auto PresetLabel = [](ERuntimeInspectorThemePreset InPreset) -> const TCHAR*
    {
        return InPreset == ERuntimeInspectorThemePreset::SoftContrast
            ? TEXT("SoftContrast")
            : TEXT("StudioSlate");
    };

    auto RestorePreset = [this, OriginalPreset]()
    {
        FString RestoreError;
        PreviewApplyThemePreset(OriginalPreset, RestoreError);
        if (bThemePreviewRefreshScheduled)
        {
            HandleThemePreviewRefreshTimerElapsed();
        }
    };

    Close();
    Open();
    ShowSettingsPage();

    FString PreviewError;
    if (!PreviewApplyThemePreset(AlternatePreset, PreviewError))
    {
        OutReport = FString::Printf(TEXT("ThemePresetPreviewSelfTest=FAIL | PreviewError=%s"), *PreviewError);
        return false;
    }

    const bool bRefreshScheduled = bThemePreviewRefreshScheduled;
    if (bThemePreviewRefreshScheduled)
    {
        HandleThemePreviewRefreshTimerElapsed();
    }

    UPanelWidget* HostPanel = FindSettingsHostPanel();
    UInspectorSettingsPageWidget* Page = SettingsPageWidget.Get();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();

    bool bHostContainsPage = false;
    int32 VisibleLegacySiblingCount = 0;
    if (HostPanel && Page)
    {
        const int32 ChildCount = HostPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = HostPanel->GetChildAt(ChildIndex);
            if (Child == Page)
            {
                bHostContainsPage = true;
                continue;
            }

            if (Child && Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
            }
        }
    }

    if (Page)
    {
        Page->RefreshFromSubsystem();
    }

    const bool bSettingsActive = Switcher && Switcher->GetActiveWidgetIndex() == SettingsPageIndex;
    const bool bFooterOk = Page && Page->HasFooterControls();
    const bool bInteractionOk = Page && Page->HasInteractionSection();
    const bool bThemeChanged = GetThemePreset() == AlternatePreset
        && static_cast<int32>(RICompactUI::GetActiveThemePreset()) == (AlternatePreset == ERuntimeInspectorThemePreset::SoftContrast ? 1 : 0);

    RestorePreset();

    const bool bThemeRestored = GetThemePreset() == OriginalPreset
        && static_cast<int32>(RICompactUI::GetActiveThemePreset()) == (OriginalPreset == ERuntimeInspectorThemePreset::SoftContrast ? 1 : 0);
    const bool bPassed = bRefreshScheduled
        && bThemeChanged
        && bThemeRestored
        && bSettingsActive
        && HostPanel
        && Page
        && bHostContainsPage
        && VisibleLegacySiblingCount == 0
        && bFooterOk
        && bInteractionOk;

    OutReport = FString::Printf(
        TEXT("ThemePresetPreviewSelfTest=%s | Original=%s Alternate=%s Scheduled=%d Changed=%d Restored=%d ActiveIndex=%d SettingsIndex=%d HostHasPage=%d VisibleLegacy=%d Footer=%d Interaction=%d"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        PresetLabel(OriginalPreset),
        PresetLabel(AlternatePreset),
        bRefreshScheduled ? 1 : 0,
        bThemeChanged ? 1 : 0,
        bThemeRestored ? 1 : 0,
        Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE,
        SettingsPageIndex,
        bHostContainsPage ? 1 : 0,
        VisibleLegacySiblingCount,
        bFooterOk ? 1 : 0,
        bInteractionOk ? 1 : 0);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunThemePresetPreviewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunThemePresetPreviewSelfTest(Report);
    return Report;
#endif
}

AActor* UInspectorWorldSubsystem::ResolvePreferredFabScreenshotActor() const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return nullptr;
#else
    UWorld* World = GetWorld();
    if (!World)
    {
        return SelectedActor.Get();
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!Candidate)
        {
            continue;
        }

        const FString BaseName = RI_ExtractActorBaseName(Candidate->GetName());
        const FString ClassName = Candidate->GetClass() ? Candidate->GetClass()->GetName() : FString();
        if (BaseName.Equals(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase))
        {
            return Candidate;
        }
    }

    return SelectedActor.Get();
#endif
}

bool UInspectorWorldSubsystem::ApplyFabScreenshotFoundationState(FString& OutSummary, FString& OutError)
{
    OutSummary.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutError = TEXT("PIE with local player required");
        return false;
    }

    FString ThemeError;
    if (!PreviewApplyThemePreset(ERuntimeInspectorThemePreset::SoftContrast, ThemeError))
    {
        OutError = ThemeError.IsEmpty() ? TEXT("Theme preset preview failed") : ThemeError;
        return false;
    }

    if (bThemePreviewRefreshScheduled)
    {
        HandleThemePreviewRefreshTimerElapsed();
    }

    FString ResolutionError;
    if (!ApplyFabScreenshotViewportResolution(ResolutionError))
    {
        OutError = ResolutionError.IsEmpty() ? TEXT("Failed to apply Fab screenshot viewport resolution") : ResolutionError;
        return false;
    }

    FString ScaleError;
    if (!ApplyFabScreenshotApplicationScale(0.65f, ScaleError))
    {
        OutError = ScaleError.IsEmpty() ? TEXT("Failed to apply Fab screenshot application scale") : ScaleError;
        return false;
    }

    if (HasStagedPatch())
    {
        FString ClearSummary;
        FString ClearDetails;
        if (!ExecuteFileClearStagedAction(ClearSummary, ClearDetails))
        {
            OutError = ClearSummary.IsEmpty() ? TEXT("Failed to clear staged patch") : ClearSummary;
            return false;
        }
    }

    AActor* PreferredActor = ResolvePreferredFabScreenshotActor();
    if (SelectedActor.Get() != PreferredActor)
    {
        SetSelectedActor(PreferredActor);
    }

    SetRemoteSessionUIContext(FString(), FString(), FString(), FString());
    Close();
    Open();

    FString PanelTransformError;
    if (!ApplyFabScreenshotPanelTransform(PanelTransformError))
    {
        OutError = PanelTransformError.IsEmpty() ? TEXT("Failed to apply Fab screenshot panel transform") : PanelTransformError;
        return false;
    }

    ShowFilePage();

    if (UInspectorFilePageWidget* FilePage = FilePageWidget.Get())
    {
        FilePage->TakeWidget();
        FilePage->ApplyPresentationCollapsedState(true, true, true);
        FilePage->RefreshFromSubsystem();
    }

    EnsureTestPageInjected();
    if (UInspectorTestPageWidget* TestPage = TestPageWidget.Get())
    {
        TestPage->TakeWidget();
        TestPage->ApplyPresentationCollapsedState(true, true, true, true);
        TestPage->RefreshFromSubsystem();
    }

    const FString ActorLabel = PreferredActor ? PreferredActor->GetName() : TEXT("No selected actor");
    const FString StagedState = HasStagedPatch() ? TEXT("Staged patch present") : TEXT("No staged patch");
    OutSummary = FString::Printf(TEXT("Fab screenshot state ready | Actor=%s | Theme=SoftContrast | Page=Changes | Staged=%s"), *ActorLabel, *StagedState);
    return true;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabScreenshotViewportResolution(FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    APlayerController* PC = GetLocalPC();
    if (!PC)
    {
        OutError = TEXT("Local player controller unavailable");
        return false;
    }

    if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        GameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
        GameUserSettings->SetScreenResolution(FIntPoint(1600, 900));
        GameUserSettings->ApplyResolutionSettings(false);
    }

    PC->ConsoleCommand(TEXT("r.SetRes 1600x900w"), true);
    return true;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabScreenshotApplicationScale(float InScale, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!FSlateApplication::IsInitialized())
    {
        OutError = TEXT("Slate application unavailable");
        return false;
    }

    if (!bFabScreenshotApplicationScaleCaptured)
    {
        SavedFabScreenshotApplicationScale = FSlateApplication::Get().GetApplicationScale();
        bFabScreenshotApplicationScaleCaptured = true;
    }

    FSlateApplication::Get().SetApplicationScale(InScale);
    return true;
#endif
}

void UInspectorWorldSubsystem::RestoreFabScreenshotApplicationScale()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bFabScreenshotApplicationScaleCaptured)
    {
        return;
    }

    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().SetApplicationScale(SavedFabScreenshotApplicationScale);
    }

    bFabScreenshotApplicationScaleCaptured = false;
    SavedFabScreenshotApplicationScale = 1.0f;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabScreenshotPanelTransform(FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel)
    {
        OutError = TEXT("Inspector panel widget unavailable");
        return false;
    }

    EnsurePanelInteractionInitialized();

    if (!bFabScreenshotPanelTransformCaptured)
    {
        SavedFabScreenshotPanelTranslation = Panel->GetRenderTransform().Translation;
        bFabScreenshotPanelTransformCaptured = true;
    }

    if (USizeBox* SizeBox = PanelSizeBox.Get(); SizeBox && !bFabScreenshotPanelHeightCaptured)
    {
        SavedFabScreenshotPanelHeight = SizeBox->GetHeightOverride();
        bFabScreenshotPanelHeightCaptured = true;
    }
    if (USizeBox* SizeBox = PanelSizeBox.Get(); SizeBox && !bFabScreenshotPanelWidthCaptured)
    {
        SavedFabScreenshotPanelWidth = SizeBox->GetWidthOverride();
        bFabScreenshotPanelWidthCaptured = true;
    }

    CacheInitialPanelWidth();
    CacheInitialPanelHeight();
    if (USizeBox* SizeBox = PanelSizeBox.Get())
    {
        SizeBox->SetWidthOverride(FMath::Max(PanelDefaultWidth, 1180.0f));
        SizeBox->SetHeightOverride(PanelDefaultHeight > 1.0f ? PanelDefaultHeight : 660.0f);
    }

    FWidgetTransform Transform = Panel->GetRenderTransform();
    Transform.Translation = FVector2D(-640.0f, 0.0f);
    Panel->SetRenderTransform(Transform);
    return true;
#endif
}

void UInspectorWorldSubsystem::RestoreFabScreenshotPanelTransform()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bFabScreenshotPanelTransformCaptured)
    {
        return;
    }

    if (UUserWidget* Panel = PanelWidget.Get())
    {
        FWidgetTransform Transform = Panel->GetRenderTransform();
        Transform.Translation = SavedFabScreenshotPanelTranslation;
        Panel->SetRenderTransform(Transform);
    }

    if (USizeBox* SizeBox = PanelSizeBox.Get(); SizeBox && bFabScreenshotPanelHeightCaptured)
    {
        if (SavedFabScreenshotPanelHeight > 1.0f)
        {
            SizeBox->SetHeightOverride(SavedFabScreenshotPanelHeight);
        }
        else
        {
            SizeBox->ClearHeightOverride();
        }
    }
    if (USizeBox* SizeBox = PanelSizeBox.Get(); SizeBox && bFabScreenshotPanelWidthCaptured)
    {
        if (SavedFabScreenshotPanelWidth > 1.0f)
        {
            SizeBox->SetWidthOverride(SavedFabScreenshotPanelWidth);
        }
        else
        {
            SizeBox->ClearWidthOverride();
        }
    }

    bFabScreenshotPanelTransformCaptured = false;
    SavedFabScreenshotPanelTranslation = FVector2D::ZeroVector;
    bFabScreenshotPanelHeightCaptured = false;
    SavedFabScreenshotPanelHeight = 0.0f;
    bFabScreenshotPanelWidthCaptured = false;
    SavedFabScreenshotPanelWidth = 0.0f;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabRemoteSessionScreenshotState(FString& OutSummary, FString& OutError)
{
    OutSummary.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    FString FoundationSummary;
    if (!ApplyFabScreenshotFoundationState(FoundationSummary, OutError))
    {
        return false;
    }

    const FString LeftSession = TEXT("local_editor_current");
    const FString RightSession = TEXT("local_pie_current");

    FRIRuntimeSessionTargetSetCompareRequest Request;
    Request.LeftSessionId = LeftSession;
    Request.RightSessionId = RightSession;
    Request.NameFilter = TEXT("BP_TestVarsActor");
    Request.ClassFilter = TEXT("BP_TestVarsActor");
    SetActiveRemoteSessionTargetSetCompareRequest(Request);

    FString CompareSummary;
    FString CompareDetails;
    bool bCompareOk = ExecuteFileBuildRemoteSessionTargetSetCompareAction(Request, CompareSummary, CompareDetails);
    if (bCompareOk)
    {
        const FRIRuntimeSessionTargetSetCompareReport CurrentReport = GetLastRuntimeSessionTargetSetCompareReport();
        const bool bEmptyResult = CurrentReport.LineCount <= 0;
        if (bEmptyResult)
        {
            Request.NameFilter = TEXT("TestVarsActor");
            Request.ClassFilter.Reset();
            bCompareOk = ExecuteFileBuildRemoteSessionTargetSetCompareAction(Request, CompareSummary, CompareDetails);
        }
    }
    if (!bCompareOk)
    {
        OutError = CompareSummary.IsEmpty() ? TEXT("Failed to build remote session compare state") : CompareSummary;
        return false;
    }

    SetActiveRemoteSessionTargetSetCompareRequest(Request);
    SetRemoteSessionUIContext(
        RightSession,
        FString::Printf(TEXT("%s -> %s"), *LeftSession, *RightSession),
        Request.NameFilter.IsEmpty() ? TEXT("All runtime targets") : Request.NameFilter,
        RI_WorkflowId_MainlineRemoteSessionCompareFoundation.ToString());

    ShowFilePage();
    if (UInspectorFilePageWidget* FilePage = FilePageWidget.Get())
    {
        FilePage->TakeWidget();
        FilePage->RefreshFromSubsystem();
        FilePage->ApplyPresentationCollapsedState(true, true, false);
    }

    OutSummary = FString::Printf(TEXT("Fab screenshot state ready | State=%s | Compare=%s"), *FoundationSummary, *CompareSummary);
    return true;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabPromoteOrAuditScreenshotState(FString& OutSummary, FString& OutError)
{
    OutSummary.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    FString FoundationSummary;
    if (!ApplyFabScreenshotFoundationState(FoundationSummary, OutError))
    {
        return false;
    }

    AActor* PreferredActor = ResolvePreferredFabScreenshotActor();
    if (!PreferredActor)
    {
        OutError = TEXT("Preferred Fab screenshot actor unavailable");
        return false;
    }

    if (SelectedActor.Get() != PreferredActor)
    {
        SetSelectedActor(PreferredActor);
    }

    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
            PreferredActor,
            TestProperty,
            OriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue))
    {
        OutError = TEXT("No writable primitive property available for Fab screenshot state");
        return false;
    }

    FString ApplyError;
    if (!ApplyPropertyTextNow(PreferredActor, TestProperty, PatchedText, ApplyError))
    {
        OutError = ApplyError.IsEmpty() ? TEXT("Failed to apply screenshot patch value") : ApplyError;
        return false;
    }

    FString StageError;
    if (!StageSelectionAsPatch(StageError))
    {
        FString RestoreError;
        ApplyPropertyTextNow(PreferredActor, TestProperty, OriginalText, RestoreError);
        OutError = StageError.IsEmpty() ? TEXT("Failed to stage screenshot patch") : StageError;
        return false;
    }

    FString AuditSummary;
    FString AuditDetails;
    if (!ExecuteFileBuildPatchVsSourceAuditAction(AuditSummary, AuditDetails))
    {
        FString RestoreError;
        ApplyPropertyTextNow(PreferredActor, TestProperty, OriginalText, RestoreError);
        OutError = AuditSummary.IsEmpty() ? TEXT("Failed to build patch-vs-source audit") : AuditSummary;
        return false;
    }

    FString RestoreError;
    const bool bRestored = ApplyPropertyTextNow(PreferredActor, TestProperty, OriginalText, RestoreError);
    if (!bRestored)
    {
        OutError = RestoreError.IsEmpty() ? TEXT("Failed to restore actor property after screenshot setup") : RestoreError;
        return false;
    }

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::PatchVsSource);
    ShowFilePage();
    if (UInspectorFilePageWidget* FilePage = FilePageWidget.Get())
    {
        FilePage->TakeWidget();
        FilePage->ApplyPresentationCollapsedState(false, true, true);
        FilePage->RefreshFromSubsystem();
    }

    OutSummary = FString::Printf(
        TEXT("Fab screenshot state ready | Mode=%s | Foundation=%s | Property=%s | Audit=%s"),
        TEXT("promote_or_audit"),
        *FoundationSummary,
        *TestProperty.ToString(),
        *AuditSummary);
    return true;
#endif
}

bool UInspectorWorldSubsystem::ApplyFabScreenshotStateByName(const FString& InShotName, FString& OutSummary, FString& OutError)
{
    const FString ShotName = InShotName.TrimStartAndEnd().ToLower();
    if (ShotName.IsEmpty() || ShotName == TEXT("foundation"))
    {
        return ApplyFabScreenshotFoundationState(OutSummary, OutError);
    }

    if (ShotName == TEXT("remote_session"))
    {
        return ApplyFabRemoteSessionScreenshotState(OutSummary, OutError);
    }

    if (ShotName == TEXT("promote_or_audit"))
    {
        return ApplyFabPromoteOrAuditScreenshotState(OutSummary, OutError);
    }

    OutSummary.Reset();
    OutError = FString::Printf(TEXT("Unknown Fab screenshot shot: %s"), *InShotName);
    return false;
}

bool UInspectorWorldSubsystem::SetVisiblePageByName(const FString& InPageName, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!bOpen)
    {
        Open();
        if (!bOpen)
        {
            OutError = TEXT("RuntimeInspector failed to open");
            return false;
        }
    }

    const FString PageName = InPageName.TrimStartAndEnd().ToLower();
    if (PageName.IsEmpty() || PageName == TEXT("changes") || PageName == TEXT("file") || PageName == TEXT("snapshot"))
    {
        ShowFilePage();
        return true;
    }

    if (PageName == TEXT("actor") || PageName == TEXT("inspect"))
    {
        HandleActorTabClicked();
        return true;
    }

    if (PageName == TEXT("settings"))
    {
        ShowSettingsPage();
        return true;
    }

    if (PageName == TEXT("tools") || PageName == TEXT("test") || PageName == TEXT("diagnostics"))
    {
        ShowTestPage();
        return true;
    }

    OutError = FString::Printf(TEXT("Unknown RuntimeInspector page: %s"), *InPageName);
    return false;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotFoundationSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FabScreenshotFoundationSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    FString Summary;
    FString Error;
    if (!ApplyFabScreenshotFoundationState(Summary, Error))
    {
        OutReport = FString::Printf(TEXT("FabScreenshotFoundationSelfTest=FAIL | Apply=%s"), *Error);
        return false;
    }

    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    UInspectorFilePageWidget* FilePage = FilePageWidget.Get();
    UInspectorTestPageWidget* TestPage = TestPageWidget.Get();
    const FRIRuntimeActorRoleSummary RoleSummary = GetSelectedActorRoleSummary();
    const bool bThemeOk = GetThemePreset() == ERuntimeInspectorThemePreset::SoftContrast
        && static_cast<int32>(RICompactUI::GetActiveThemePreset()) == 1;
    const bool bChangesActive = GetVisiblePage() == ERIVisiblePage::Changes;
    const bool bStagedClean = !HasStagedPatch();
    const bool bDiagnosticsCollapsed = FilePage && !FilePage->IsDiagnosticsSectionExpanded();
    const bool bAdvancedCollapsed = FilePage && !FilePage->IsAuditsSectionExpanded() && !FilePage->IsPresetsSectionExpanded();
    const bool bToolsTestsCollapsed = TestPage && !TestPage->IsTestsSectionExpanded();
    const bool bToolsRemoteCollapsed = TestPage && !TestPage->IsRemoteSessionSectionExpanded();
    const bool bToolsDiagnosticsCollapsed = TestPage && !TestPage->IsDiagnosticsSectionExpanded();
    const bool bToolsOverrideCollapsed = TestPage && !TestPage->IsRemoteOverrideSectionExpanded();
    const bool bToolsCollapsed = bToolsTestsCollapsed
        && bToolsRemoteCollapsed
        && bToolsDiagnosticsCollapsed
        && bToolsOverrideCollapsed;
    const bool bActorOk = !RoleSummary.ActorPath.IsEmpty()
        || (FilePage && FilePage->GetSelectedActorSummaryLabel().Equals(TEXT("No selected actor"), ESearchCase::CaseSensitive));
    const bool bPassed = bThemeOk && bChangesActive && bStagedClean && bDiagnosticsCollapsed && bAdvancedCollapsed && bToolsCollapsed && bActorOk;

    OutReport = FString::Printf(
        TEXT("FabScreenshotFoundationSelfTest=%s | Theme=%s ActiveIndex=%d SnapshotIndex=%d Actor=%s StagedClean=%d AdvancedCollapsed=%d DiagnosticsCollapsed=%d ToolsCollapsed=%d Sections=%d/%d/%d/%d Summary=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bThemeOk ? TEXT("SoftContrast") : TEXT("mismatch"),
        Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE,
        static_cast<int32>(ERIVisiblePage::Changes),
        RoleSummary.ActorPath.IsEmpty() ? TEXT("empty") : *RoleSummary.ActorPath,
        bStagedClean ? 1 : 0,
        bAdvancedCollapsed ? 1 : 0,
        bDiagnosticsCollapsed ? 1 : 0,
        bToolsCollapsed ? 1 : 0,
        bToolsTestsCollapsed ? 1 : 0,
        bToolsRemoteCollapsed ? 1 : 0,
        bToolsDiagnosticsCollapsed ? 1 : 0,
        bToolsOverrideCollapsed ? 1 : 0,
        *Summary);
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotPageSelfTest(const FString& InPageName, ERIVisiblePage ExpectedPage, const FString& InTestLabel, FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = FString::Printf(TEXT("%s=BLOCKED | RuntimeInspector disabled"), *InTestLabel);
    return false;
#else
    FString Summary;
    FString Error;
    if (!ApplyFabScreenshotFoundationState(Summary, Error))
    {
        OutReport = FString::Printf(TEXT("%s=FAIL | Apply=%s"), *InTestLabel, *Error);
        return false;
    }

    if (!SetVisiblePageByName(InPageName, Error))
    {
        OutReport = FString::Printf(TEXT("%s=FAIL | ShowPage=%s"), *InTestLabel, *Error);
        return false;
    }

    if (ExpectedPage == ERIVisiblePage::Actor)
    {
        if (bDeferredOpenActorRefreshScheduled)
        {
            HandleDeferredOpenActorRefreshTimerElapsed();
        }
        SetContentSwitcherIndex(0);
        RefreshPanel(EInspectorRefreshReason::StructureChanged);
    }

    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    if (!Switcher)
    {
        Switcher = FindContentSwitcher();
        ContentSwitcher = Switcher;
    }

    const int32 ExpectedIndex =
        ExpectedPage == ERIVisiblePage::Changes ? 1 :
        ExpectedPage == ERIVisiblePage::Settings ? SettingsPageIndex :
        ExpectedPage == ERIVisiblePage::Tools ? TestPageIndex :
        0;
    const bool bSwitcherOk = Switcher != nullptr;
    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;

    const bool bThemeOk = GetThemePreset() == ERuntimeInspectorThemePreset::SoftContrast
        && static_cast<int32>(RICompactUI::GetActiveThemePreset()) == 1;
    const bool bPageOk = bSwitcherOk && ActiveIndex == ExpectedIndex;
    const bool bVisiblePageOk = GetVisiblePage() == ExpectedPage;
    const bool bStagedClean = !HasStagedPatch();
    const bool bActorCellHiddenOk = ExpectedPage != ERIVisiblePage::Actor
        || !SharedContextActorCell.IsValid()
        || SharedContextActorCell->GetVisibility() != ESlateVisibility::Visible;
    const TCHAR* VisiblePageLabel = RI_GetVisiblePageDisplayLabel(bSwitcherOk ? GetVisiblePage() : ERIVisiblePage::Actor);
    int32 ActiveTabCount = 0;
    bool bTabWidthOk = true;
    FString TabWidthSummary;
    {
        const TArray<UButton*> Buttons = { ActorTabButton.Get(), FileTabButton.Get(), SettingsTabButton.Get(), TestTabButton.Get() };
        float BaselineWidth = -1.0f;
        bool bAllFillSlots = true;
        for (UButton* Button : Buttons)
        {
            if (!Button)
            {
                bTabWidthOk = false;
                continue;
            }

            if (FMath::IsNearlyEqual(Button->GetRenderOpacity(), 1.0f, KINDA_SMALL_NUMBER))
            {
                ++ActiveTabCount;
            }

            float Width = Button->GetCachedGeometry().GetLocalSize().X;
            if (Width <= KINDA_SMALL_NUMBER)
            {
                if (USizeBox* SizeBox = RI_FindFirstSizeBoxRecursive(Button))
                {
                    Width = SizeBox->GetMinDesiredWidth();
                }
            }

            bool bFillSlot = false;
            if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Button->Slot))
            {
                bFillSlot = Slot->GetSize().SizeRule == ESlateSizeRule::Fill;
            }
            bAllFillSlots = bAllFillSlots && bFillSlot;

            if (!TabWidthSummary.IsEmpty())
            {
                TabWidthSummary += TEXT(",");
            }
            TabWidthSummary += FString::Printf(TEXT("%.1f:%d"), Width, bFillSlot ? 1 : 0);

            if (Width <= KINDA_SMALL_NUMBER)
            {
                continue;
            }

            if (BaselineWidth < 0.0f)
            {
                BaselineWidth = Width;
            }
            else if (!FMath::IsNearlyEqual(BaselineWidth, Width, 0.1f))
            {
                bTabWidthOk = false;
            }
        }

        if (BaselineWidth < 0.0f)
        {
            bTabWidthOk = bAllFillSlots;
        }
        else
        {
            bTabWidthOk = bTabWidthOk && bAllFillSlots;
        }
    }

    FString GroupSwitcherActiveName = TEXT("None");
    FString CustomGroupsVisibility = TEXT("None");
    FString CustomGroupsSize = TEXT("None");
    FString GroupListVisibility = TEXT("None");
    FString TreeListVisibility = TEXT("None");
    FString PinListVisibility = TEXT("None");
    int32 CustomGroupsCount = INDEX_NONE;
    int32 GroupListCount = INDEX_NONE;
    int32 TreeListCount = INDEX_NONE;
    int32 PinListCount = INDEX_NONE;
    int32 GroupDisplayedCount = INDEX_NONE;
    int32 TreeDisplayedCount = INDEX_NONE;
    int32 PinDisplayedCount = INDEX_NONE;
    int32 SidebarGroupCount = INDEX_NONE;
    int32 SidebarPinnedCount = INDEX_NONE;
    FString GroupListSize = TEXT("None");
    FString TreeListSize = TEXT("None");
    FString PinListSize = TEXT("None");

    if (ExpectedPage == ERIVisiblePage::Actor)
    {
        if (UUserWidget* Panel = PanelWidget.Get())
        {
            if (Panel->WidgetTree)
            {
                auto DescribeVisibility = [](UWidget* Widget) -> FString
                {
                    if (!Widget)
                    {
                        return TEXT("Missing");
                    }

                    switch (Widget->GetVisibility())
                    {
                    case ESlateVisibility::Visible: return TEXT("Visible");
                    case ESlateVisibility::Collapsed: return TEXT("Collapsed");
                    case ESlateVisibility::Hidden: return TEXT("Hidden");
                    case ESlateVisibility::HitTestInvisible: return TEXT("HitTestInvisible");
                    case ESlateVisibility::SelfHitTestInvisible: return TEXT("SelfHitTestInvisible");
                    default: return TEXT("Other");
                    }
                };

                if (UWidgetSwitcher* GroupSwitcher = Cast<UWidgetSwitcher>(Panel->WidgetTree->FindWidget(TEXT("WS_Group"))))
                {
                    if (UWidget* ActiveGroupChild = GroupSwitcher->GetActiveWidget())
                    {
                        GroupSwitcherActiveName = ActiveGroupChild->GetName();
                    }
                }

                if (UInspectorGroupsSectionWidget* GroupsSection = ActorGroupsSectionWidget.Get())
                {
                    SidebarGroupCount = GroupsSection->GetEntryWidgetCountForDebug();
                    SidebarPinnedCount = GroupsSection->GetPinnedEntryWidgetCountForDebug();
                }

                if (USizeBox* GroupsHostBox = ActorGroupsSectionHostBox.Get())
                {
                    CustomGroupsVisibility = DescribeVisibility(GroupsHostBox);
                    if (UVerticalBox* EntriesBox = ActorGroupsEntriesBoxStrong)
                    {
                        CustomGroupsCount = EntriesBox->GetChildrenCount();
                    }
                    const FVector2D LocalSize = GroupsHostBox->GetCachedGeometry().GetLocalSize();
                    CustomGroupsSize = FString::Printf(TEXT("%.1fx%.1f"), LocalSize.X, LocalSize.Y);
                }

                if (UListView* GroupList = Cast<UListView>(Panel->WidgetTree->FindWidget(TEXT("LV_Group"))))
                {
                    GroupListVisibility = DescribeVisibility(GroupList);
                    GroupListCount = GroupList->GetNumItems();
                    GroupDisplayedCount = GroupList->GetDisplayedEntryWidgets().Num();
                    const FVector2D LocalSize = GroupList->GetCachedGeometry().GetLocalSize();
                    GroupListSize = FString::Printf(TEXT("%.1fx%.1f"), LocalSize.X, LocalSize.Y);
                }

                if (UTreeView* TreeList = Cast<UTreeView>(Panel->WidgetTree->FindWidget(TEXT("LV_TreeGroup"))))
                {
                    TreeListVisibility = DescribeVisibility(TreeList);
                    TreeListCount = TreeList->GetNumItems();
                    TreeDisplayedCount = TreeList->GetDisplayedEntryWidgets().Num();
                    const FVector2D LocalSize = TreeList->GetCachedGeometry().GetLocalSize();
                    TreeListSize = FString::Printf(TEXT("%.1fx%.1f"), LocalSize.X, LocalSize.Y);
                }

                if (UListView* PinList = Cast<UListView>(Panel->WidgetTree->FindWidget(TEXT("LV_Pin"))))
                {
                    PinListVisibility = DescribeVisibility(PinList);
                    PinListCount = PinList->GetNumItems();
                    PinDisplayedCount = PinList->GetDisplayedEntryWidgets().Num();
                    const FVector2D LocalSize = PinList->GetCachedGeometry().GetLocalSize();
                    PinListSize = FString::Printf(TEXT("%.1fx%.1f"), LocalSize.X, LocalSize.Y);
                }
            }
        }
    }

    const bool bPassed = bThemeOk && bSwitcherOk && bPageOk && bVisiblePageOk && bStagedClean && bActorCellHiddenOk
        && ActiveTabCount == 1 && bTabWidthOk;
    OutReport = FString::Printf(
        TEXT("%s=%s | Requested=%s | Visible=%s | Switcher=%d | ActiveIndex=%d | ExpectedIndex=%d | Theme=%s | StagedClean=%d | ActorCellHidden=%d | ActiveTabs=%d | TabWidths=%s WidthsOk=%d | GroupHost=%s | Sidebar=%d/%d | CustomGroups=%s/%d@%s | GroupList=%s/%d/%d@%s | TreeList=%s/%d/%d@%s | PinList=%s/%d/%d@%s | Summary=%s"),
        *InTestLabel,
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *InPageName,
        VisiblePageLabel,
        bSwitcherOk ? 1 : 0,
        ActiveIndex,
        ExpectedIndex,
        bThemeOk ? TEXT("SoftContrast") : TEXT("mismatch"),
        bStagedClean ? 1 : 0,
        bActorCellHiddenOk ? 1 : 0,
        ActiveTabCount,
        TabWidthSummary.IsEmpty() ? TEXT("None") : *TabWidthSummary,
        bTabWidthOk ? 1 : 0,
        *GroupSwitcherActiveName,
        SidebarGroupCount,
        SidebarPinnedCount,
        *CustomGroupsVisibility,
        CustomGroupsCount,
        *CustomGroupsSize,
        *GroupListVisibility,
        GroupListCount,
        GroupDisplayedCount,
        *GroupListSize,
        *TreeListVisibility,
        TreeListCount,
        TreeDisplayedCount,
        *TreeListSize,
        *PinListVisibility,
        PinListCount,
        PinDisplayedCount,
        *PinListSize,
        *Summary);
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotActorPageSelfTest(FString& OutReport)
{
    return RunFabScreenshotPageSelfTest(TEXT("Inspect"), ERIVisiblePage::Actor, TEXT("FabScreenshotActorPageSelfTest"), OutReport);
}

FString UInspectorWorldSubsystem::RunFabScreenshotActorPageSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotActorPageSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotSettingsPageSelfTest(FString& OutReport)
{
    return RunFabScreenshotPageSelfTest(TEXT("Settings"), ERIVisiblePage::Settings, TEXT("FabScreenshotSettingsPageSelfTest"), OutReport);
}

FString UInspectorWorldSubsystem::RunFabScreenshotSettingsPageSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotSettingsPageSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotToolsPageSelfTest(FString& OutReport)
{
    return RunFabScreenshotPageSelfTest(TEXT("Diagnostics"), ERIVisiblePage::Tools, TEXT("FabScreenshotToolsPageSelfTest"), OutReport);
}

FString UInspectorWorldSubsystem::RunFabScreenshotToolsPageSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotToolsPageSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotRemoteSessionSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FabScreenshotRemoteSessionSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    FString Summary;
    FString Error;
    if (!ApplyFabRemoteSessionScreenshotState(Summary, Error))
    {
        OutReport = FString::Printf(TEXT("FabScreenshotRemoteSessionSelfTest=FAIL | Apply=%s"), *Error);
        return false;
    }

    const FRIRuntimeSessionTargetSetCompareReport CompareReport = GetLastRuntimeSessionTargetSetCompareReport();
    UInspectorFilePageWidget* FilePage = FilePageWidget.Get();
    const bool bPageOk = GetVisiblePage() == ERIVisiblePage::Changes;
    const bool bCompareOk = CompareReport.LineCount > 0;
    const bool bDiagnosticsExpanded = FilePage && FilePage->IsDiagnosticsSectionExpanded();
    const bool bPassed = bPageOk && bCompareOk;
    OutReport = FString::Printf(
        TEXT("FabScreenshotRemoteSessionSelfTest=%s | Visible=%s | CompareLines=%d | DiagnosticsExpanded=%d | Summary=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        GetVisiblePage() == ERIVisiblePage::Changes ? TEXT("Changes") : TEXT("Other"),
        CompareReport.LineCount,
        bDiagnosticsExpanded ? 1 : 0,
        *Summary);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFabScreenshotRemoteSessionSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotRemoteSessionSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFabScreenshotPromoteOrAuditSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FabScreenshotPromoteOrAuditSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    FString Summary;
    FString Error;
    if (!ApplyFabPromoteOrAuditScreenshotState(Summary, Error))
    {
        OutReport = FString::Printf(TEXT("FabScreenshotPromoteOrAuditSelfTest=FAIL | Apply=%s"), *Error);
        return false;
    }

    const FRIAuditReport AuditReport = GetLastAuditReport();
    UInspectorFilePageWidget* FilePage = FilePageWidget.Get();
    const bool bPageOk = GetVisiblePage() == ERIVisiblePage::Changes;
    const bool bAuditOk = AuditReport.Lines.Num() > 0;
    const bool bModeOk = GetActiveFileAuditViewMode() == ERIAuditComparisonMode::PatchVsSource;
    const bool bStagedPatch = HasStagedPatch();
    const bool bAuditsExpanded = FilePage && FilePage->IsAuditsSectionExpanded();
    const bool bPassed = bPageOk && bAuditOk && bModeOk && bStagedPatch && bAuditsExpanded;
    OutReport = FString::Printf(
        TEXT("FabScreenshotPromoteOrAuditSelfTest=%s | Visible=%s | AuditLines=%d | Mode=%s | Staged=%d | AuditsExpanded=%d | Summary=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        GetVisiblePage() == ERIVisiblePage::Changes ? TEXT("Changes") : TEXT("Other"),
        AuditReport.Lines.Num(),
        bModeOk ? TEXT("PatchVsSource") : TEXT("Other"),
        bStagedPatch ? 1 : 0,
        bAuditsExpanded ? 1 : 0,
        *Summary);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFabScreenshotPromoteOrAuditSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotPromoteOrAuditSelfTest(Report);
    return Report;
#endif
}

FString UInspectorWorldSubsystem::RunFabScreenshotFoundationSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFabScreenshotFoundationSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunPatchPresetRoundtripSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    AActor* SelectedActorPtr = SelectedActor.Get();
    if (!SelectedActorPtr)
    {
        OutReport = TEXT("PatchPresetRoundtripSelfTest=FAIL | No selected actor");
        return false;
    }

    const FString SelectedActorPath = SelectedActorPtr->GetPathName();
    for (const TPair<FString, FString>& Pair : ModifiedValueByKey)
    {
        if (Pair.Key.StartsWith(FString::Printf(TEXT("P|%s|"), *SelectedActorPath))
            || Pair.Key.StartsWith(FString::Printf(TEXT("M|%s|"), *SelectedActorPath)))
        {
            OutReport = TEXT("PatchPresetRoundtripSelfTest=BLOCKED | Selected actor already has modified values");
            return false;
        }
    }

    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIApplyResult PreviousLastApplyResult = LastPatchApplyResult;

    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    FString PresetPath;
    FRIPatchPresetMetadata Metadata;

    auto RestoreState = [&]()
    {
        if (!PresetPath.IsEmpty())
        {
            FString DeleteError;
            DeletePatchPreset(PresetPath, DeleteError);
        }

        if (SelectedActorPtr && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(SelectedActorPtr, TestProperty, OriginalText, RestoreError);
            }
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }
        LastPatchApplyResult = PreviousLastApplyResult;
    };

    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
        SelectedActorPtr,
        TestProperty,
        OriginalText,
        PatchedText,
        PropertyKind,
        NumericOriginalValue,
        NumericTargetValue))
    {
        OutReport = TEXT("PatchPresetRoundtripSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    FString Error;
    if (!ApplyPropertyTextNow(SelectedActorPtr, TestProperty, PatchedText, Error))
    {
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    FString AfterDirectApply;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterDirectApply);
    const bool bDirectApplyOk = (PropertyKind == TEXT("bool"))
        ? !AfterDirectApply.Equals(OriginalText, ESearchCase::IgnoreCase)
        : FMath::IsNearlyEqual(FCString::Atod(*AfterDirectApply), NumericTargetValue, 0.001);
    if (!bDirectApplyOk)
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | DirectApplyMismatch=%s"), *AfterDirectApply);
        return false;
    }

    if (!StageSelectionAsPatch(Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | StageError=%s"), *Error);
        return false;
    }

    const FRIPatchBundle CapturedBundle = GetStagedPatch();
    if (CapturedBundle.Operations.Num() <= 0)
    {
        RestoreState();
        OutReport = TEXT("PatchPresetRoundtripSelfTest=FAIL | Captured bundle empty");
        return false;
    }

    Metadata.PresetId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    Metadata.DisplayName = TEXT("SelfTest_PatchPresetRoundtrip");
    Metadata.Category = TEXT("SelfTest");
    Metadata.Description = TEXT("Temporary preset used by RuntimeInspector self test");
    Metadata.ApplicabilityScope = ERIPatchPresetApplicabilityScope::CurrentSelection;

    if (!SavePatchPreset(Metadata, PresetPath, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | SaveError=%s"), *Error);
        return false;
    }

    TArray<FRIPatchPresetMetadata> Presets;
    if (!ListPatchPresets(Presets, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | ListError=%s"), *Error);
        return false;
    }

    const bool bListed = Presets.ContainsByPredicate([&Metadata](const FRIPatchPresetMetadata& Candidate)
    {
        return Candidate.PresetId == Metadata.PresetId;
    });

    FRIPatchPresetMetadata LoadedMetadata;
    FRIPatchBundle LoadedBundle;
    if (!LoadPatchPreset(Metadata.PresetId, LoadedMetadata, LoadedBundle, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | LoadError=%s"), *Error);
        return false;
    }

    FRIApplyResult FirstRollbackResult;
    if (!RollbackStagedPatch(FirstRollbackResult))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | FirstRollback=%s"), *FirstRollbackResult.Summary);
        return false;
    }

    FString AfterFirstRollback;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterFirstRollback);
    const bool bFirstRollbackOk = (PropertyKind == TEXT("bool"))
        ? AfterFirstRollback.Equals(OriginalText, ESearchCase::IgnoreCase)
        : FMath::IsNearlyEqual(FCString::Atod(*AfterFirstRollback), NumericOriginalValue, 0.001);

    FRIApplyResult PresetApplyResult;
    if (!ApplyPatchPreset(Metadata.PresetId, PresetApplyResult, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | PresetApplyError=%s"), *Error);
        return false;
    }

    FString AfterPresetApply;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterPresetApply);
    const bool bPresetApplyOk = (PropertyKind == TEXT("bool"))
        ? !AfterPresetApply.Equals(OriginalText, ESearchCase::IgnoreCase)
        : FMath::IsNearlyEqual(FCString::Atod(*AfterPresetApply), NumericTargetValue, 0.001);

    FRIPatchBundle RetargetedBundle;
    if (!RetargetPatchBundleToSelection(LoadedMetadata, LoadedBundle, RetargetedBundle, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | RetargetError=%s"), *Error);
        return false;
    }

    FRIApplyResult FinalRollbackResult;
    if (!RollbackPatchBundle(RetargetedBundle, FinalRollbackResult))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PatchPresetRoundtripSelfTest=FAIL | FinalRollback=%s"), *FinalRollbackResult.Summary);
        return false;
    }

    FString AfterFinalRollback;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterFinalRollback);
    const bool bFinalRollbackOk = (PropertyKind == TEXT("bool"))
        ? AfterFinalRollback.Equals(OriginalText, ESearchCase::IgnoreCase)
        : FMath::IsNearlyEqual(FCString::Atod(*AfterFinalRollback), NumericOriginalValue, 0.001);

    const bool bPassed = bListed
        && LoadedBundle.Operations.Num() == CapturedBundle.Operations.Num()
        && bFirstRollbackOk
        && bPresetApplyOk
        && bFinalRollbackOk;

    OutReport = FString::Printf(
        TEXT("PatchPresetRoundtripSelfTest=%s | Property=%s Kind=%s Original=%s AfterDirect=%s Listed=%s LoadedOps=%d AfterRollback=%s AfterPreset=%s FinalRollback=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        *PropertyKind,
        *OriginalText,
        *AfterDirectApply,
        bListed ? TEXT("yes") : TEXT("no"),
        LoadedBundle.Operations.Num(),
        *AfterFirstRollback,
        *AfterPresetApply,
        *AfterFinalRollback);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPatchPresetRoundtripSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPatchPresetRoundtripSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunPromotePreviewSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    AActor* SelectedActorPtr = SelectedActor.Get();
    if (!SelectedActorPtr)
    {
        OutReport = TEXT("PromotePreviewSelfTest=FAIL | No selected actor");
        return false;
    }

    if (!Cast<UBlueprintGeneratedClass>(SelectedActorPtr->GetClass()))
    {
        OutReport = TEXT("PromotePreviewSelfTest=BLOCKED | Selected actor is not Blueprint-backed");
        return false;
    }

    const FString SelectedActorPath = SelectedActorPtr->GetPathName();
    for (const TPair<FString, FString>& Pair : ModifiedValueByKey)
    {
        if (Pair.Key.StartsWith(FString::Printf(TEXT("P|%s|"), *SelectedActorPath))
            || Pair.Key.StartsWith(FString::Printf(TEXT("M|%s|"), *SelectedActorPath)))
        {
            OutReport = TEXT("PromotePreviewSelfTest=BLOCKED | Selected actor already has modified values");
            return false;
        }
    }

    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIApplyResult PreviousLastApplyResult = LastPatchApplyResult;

    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;

    auto RestoreState = [&]()
    {
        if (SelectedActorPtr && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(SelectedActorPtr, TestProperty, OriginalText, RestoreError);
            }
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastPatchApplyResult = PreviousLastApplyResult;
    };

    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
        SelectedActorPtr,
        TestProperty,
        OriginalText,
        PatchedText,
        PropertyKind,
        NumericOriginalValue,
        NumericTargetValue))
    {
        OutReport = TEXT("PromotePreviewSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    FString Error;
    if (!ApplyPropertyTextNow(SelectedActorPtr, TestProperty, PatchedText, Error))
    {
        OutReport = FString::Printf(TEXT("PromotePreviewSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    if (!StageSelectionAsPatch(Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PromotePreviewSelfTest=FAIL | StageError=%s"), *Error);
        return false;
    }

    FRIPromotePreview Preview;
    if (!CreatePromotePreview(GetStagedPatch(), Preview, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PromotePreviewSelfTest=FAIL | PreviewError=%s"), *Error);
        return false;
    }

    FRIApplyResult RollbackResult;
    if (!RollbackStagedPatch(RollbackResult))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("PromotePreviewSelfTest=FAIL | Rollback=%s"), *RollbackResult.Summary);
        return false;
    }

    FString AfterRollback;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterRollback);
    bool bRollbackOk = false;
    if (PropertyKind == TEXT("bool"))
    {
        bRollbackOk = AfterRollback.Equals(OriginalText, ESearchCase::IgnoreCase);
    }
    else
    {
        bRollbackOk = FMath::IsNearlyEqual(FCString::Atod(*AfterRollback), NumericOriginalValue, 0.001);
    }

    const bool bPatchedValueMatched = PropertyKind == TEXT("bool")
        ? (PatchedText != OriginalText)
        : FMath::IsNearlyEqual(FCString::Atod(*PatchedText), NumericTargetValue, 0.001);

    const bool bPassed = bPatchedValueMatched
        && Preview.SupportedOperationCount > 0
        && Preview.TargetAssetPaths.Num() > 0
        && !Preview.DiffText.IsEmpty()
        && bRollbackOk;

    OutReport = FString::Printf(
        TEXT("PromotePreviewSelfTest=%s | Supported=%d Unsupported=%d Assets=%d Property=%s Kind=%s Original=%s Patched=%s AfterRollback=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Preview.SupportedOperationCount,
        Preview.UnsupportedOperationCount,
        Preview.TargetAssetPaths.Num(),
        *TestProperty.ToString(),
        *PropertyKind,
        *OriginalText,
        *PatchedText,
        *AfterRollback);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPromotePreviewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPromotePreviewSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunPromoteConfigSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutReport = TEXT("PromoteConfigSelfTest=FAIL | Settings object unavailable");
        return false;
    }

    FString OriginalText;
    if (!InspectorPropertyUtils::GetValueAsText(Settings, TEXT("OutlinePPWeight"), OriginalText))
    {
        OutReport = TEXT("PromoteConfigSelfTest=FAIL | Failed to read original OutlinePPWeight");
        return false;
    }

    const float OriginalValue = FCString::Atof(*OriginalText);
    const float TargetValue = FMath::IsNearlyEqual(OriginalValue, 0.65f, 0.001f) ? 0.35f : 0.65f;

    FRIPatchOperation Op;
    Op.Target.TargetKind = ERIPatchTargetKind::Actor;
    Op.Field.FieldKind = ERIPatchFieldKind::Property;
    Op.Field.FieldPath = TEXT("OutlinePPWeight");
    Op.BaselineValue = OriginalText;
    Op.PatchedValue = FString::SanitizeFloat(TargetValue);
    Op.SourceTag = TEXT("Config:/Script/RuntimeInspector.RuntimeInspectorSettings");

    FRIPatchBundle Bundle;
    Bundle.BundleId = TEXT("SelfTest_PromoteConfig");
    Bundle.DisplayName = TEXT("SelfTest Promote Config");
    Bundle.Operations.Add(Op);

    auto PromoteSingleValue = [this](const FString& PatchedText, FString& OutAppliedText, FString& OutError) -> bool
    {
        FRIPatchOperation PromoteOp;
        PromoteOp.Target.TargetKind = ERIPatchTargetKind::Actor;
        PromoteOp.Field.FieldKind = ERIPatchFieldKind::Property;
        PromoteOp.Field.FieldPath = TEXT("OutlinePPWeight");
        PromoteOp.PatchedValue = PatchedText;
        PromoteOp.SourceTag = TEXT("Config:/Script/RuntimeInspector.RuntimeInspectorSettings");

        FRIPatchBundle PromoteBundle;
        PromoteBundle.BundleId = TEXT("SelfTest_PromoteConfigApply");
        PromoteBundle.Operations.Add(PromoteOp);

        FRIPromoteResult PromoteResult;
        FString PromoteError;
        const bool bOk = PromotePatchToSource(PromoteBundle, PromoteResult, PromoteError);
        if (!PromoteError.IsEmpty())
        {
            OutError = PromoteError;
        }
        else if (!PromoteResult.ReportText.IsEmpty() && !bOk)
        {
            OutError = PromoteResult.ReportText;
        }

        InspectorPropertyUtils::GetValueAsText(GetMutableDefault<URuntimeInspectorSettings>(), TEXT("OutlinePPWeight"), OutAppliedText);
        return bOk;
    };

    FRIPromotePreview Preview;
    FString Error;
    if (!CreatePromotePreview(Bundle, Preview, Error))
    {
        OutReport = FString::Printf(TEXT("PromoteConfigSelfTest=FAIL | PreviewError=%s"), *Error);
        return false;
    }

    FString AfterPromote;
    FString PromoteError;
    const bool bPromoteOk = PromoteSingleValue(Op.PatchedValue, AfterPromote, PromoteError);

    FString AfterRestore;
    FString RestoreError;
    const bool bRestoreOk = PromoteSingleValue(OriginalText, AfterRestore, RestoreError);

    const bool bPassed = Preview.SupportedOperationCount == 1
        && bPromoteOk
        && bRestoreOk
        && FMath::IsNearlyEqual(FCString::Atof(*AfterPromote), TargetValue, 0.001f)
        && FMath::IsNearlyEqual(FCString::Atof(*AfterRestore), OriginalValue, 0.001f);

    OutReport = FString::Printf(
        TEXT("PromoteConfigSelfTest=%s | PreviewSupported=%d AfterPromote=%s AfterRestore=%s PromoteError=%s RestoreError=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Preview.SupportedOperationCount,
        *AfterPromote,
        *AfterRestore,
        PromoteError.IsEmpty() ? TEXT("-") : *PromoteError,
        RestoreError.IsEmpty() ? TEXT("-") : *RestoreError);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPromoteConfigSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPromoteConfigSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunPromoteBlueprintApplySelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        OutReport = TEXT("PromoteBlueprintApplySelfTest=BLOCKED | PIE game world is required");
        return false;
    }

    UBlueprint* Blueprint = RI_LoadPreferredSelfTestBlueprint();
    UClass* BlueprintClass = Blueprint ? Cast<UClass>(Blueprint->GeneratedClass) : nullptr;
    if (!Blueprint || !BlueprintClass)
    {
        OutReport = TEXT("PromoteBlueprintApplySelfTest=FAIL | Test Blueprint asset unavailable");
        return false;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = MakeUniqueObjectName(World, BlueprintClass, TEXT("RI_PromoteBlueprintSelfTest"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* SpawnedActor = World->SpawnActor<AActor>(BlueprintClass, FTransform::Identity, SpawnParams);
    if (!SpawnedActor)
    {
        OutReport = TEXT("PromoteBlueprintApplySelfTest=FAIL | Failed to spawn test Blueprint actor");
        return false;
    }
    TWeakObjectPtr<AActor> TestActor = SpawnedActor;

    const FRIPromoteResult PreviousPromoteResult = LastPromoteResult;
    auto Cleanup = [&]()
    {
        LastPromoteResult = PreviousPromoteResult;
        if (TestActor.IsValid())
        {
            TestActor->Destroy();
        }
    };

    FName TestProperty = NAME_None;
    FString RuntimeOriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
        TestActor.Get(),
        TestProperty,
        RuntimeOriginalText,
        PatchedText,
        PropertyKind,
        NumericOriginalValue,
        NumericTargetValue))
    {
        Cleanup();
        OutReport = TEXT("PromoteBlueprintApplySelfTest=FAIL | No writable primitive Blueprint-backed actor property found");
        return false;
    }

    UObject* SourceObject = Cast<UClass>(Blueprint->GeneratedClass) ? Cast<UClass>(Blueprint->GeneratedClass)->GetDefaultObject() : nullptr;
    FString SourceOriginalText;
    if (!SourceObject || !InspectorPropertyUtils::GetValueAsText(SourceObject, TestProperty, SourceOriginalText))
    {
        Cleanup();
        OutReport = TEXT("PromoteBlueprintApplySelfTest=FAIL | Failed to read Blueprint source default");
        return false;
    }

    FString RuntimeApplyError;
    if (!ApplyPropertyTextNow(TestActor.Get(), TestProperty, PatchedText, RuntimeApplyError))
    {
        Cleanup();
        OutReport = FString::Printf(TEXT("PromoteBlueprintApplySelfTest=FAIL | RuntimeApply=%s"), *RuntimeApplyError);
        return false;
    }

    auto BuildActorBundle = [&](const FString& DesiredValue) -> FRIPatchBundle
    {
        UClass* CurrentBlueprintClass = Blueprint ? Cast<UClass>(Blueprint->GeneratedClass) : BlueprintClass;

        FRIPatchOperation Operation;
        Operation.Target.TargetKind = ERIPatchTargetKind::Actor;
        Operation.Target.ActorPath = TestActor.IsValid() ? TestActor->GetPathName() : FString();
        Operation.Target.ActorClass = TestActor.IsValid()
            ? TestActor->GetClass()->GetPathName()
            : (CurrentBlueprintClass ? CurrentBlueprintClass->GetPathName() : FString());
        Operation.Target.ActorBaseName = TestActor.IsValid()
            ? RI_ExtractActorBaseName(TestActor->GetName())
            : (CurrentBlueprintClass ? RI_ExtractActorBaseName(CurrentBlueprintClass->GetName()) : FString());
        Operation.Field.FieldKind = ERIPatchFieldKind::Property;
        Operation.Field.FieldPath = TestProperty.ToString();
        Operation.Field.DisplayName = TestProperty.ToString();
        Operation.BaselineValue = RuntimeOriginalText;
        Operation.PatchedValue = DesiredValue;
        Operation.SourceTag = FString::Printf(TEXT("Blueprint:%s"), *Blueprint->GetPathName());

        FRIPatchBundle Bundle;
        Bundle.BundleId = TEXT("SelfTest_PromoteBlueprintApply");
        Bundle.DisplayName = TEXT("SelfTest Promote Blueprint Apply");
        Bundle.Operations.Add(Operation);
        return Bundle;
    };

    FRIPatchBundle PromoteBundle = BuildActorBundle(PatchedText);
    FRIPromotePreview Preview;
    FString PreviewError;
    if (!CreatePromotePreview(PromoteBundle, Preview, PreviewError))
    {
        Cleanup();
        OutReport = FString::Printf(TEXT("PromoteBlueprintApplySelfTest=FAIL | Preview=%s"), *PreviewError);
        return false;
    }

    FRIPromoteResult PromoteResult;
    FString PromoteError;
    const bool bPromoteOk = PromotePatchToSource(PromoteBundle, PromoteResult, PromoteError);

    SourceObject = Cast<UClass>(Blueprint->GeneratedClass) ? Cast<UClass>(Blueprint->GeneratedClass)->GetDefaultObject() : nullptr;
    FString AfterPromoteText;
    const bool bReadAfterPromote = SourceObject && InspectorPropertyUtils::GetValueAsText(SourceObject, TestProperty, AfterPromoteText);

    if (!TestActor.IsValid())
    {
        UClass* CurrentBlueprintClass = Blueprint ? Cast<UClass>(Blueprint->GeneratedClass) : BlueprintClass;
        if (CurrentBlueprintClass)
        {
            SpawnParams.Name = MakeUniqueObjectName(World, CurrentBlueprintClass, TEXT("RI_PromoteBlueprintSelfTestRestore"));
            TestActor = World->SpawnActor<AActor>(CurrentBlueprintClass, FTransform::Identity, SpawnParams);
        }
    }

    FRIPatchBundle RestoreBundle = BuildActorBundle(SourceOriginalText);
    FRIPromoteResult RestoreResult;
    FString RestoreError;
    const bool bRestoreOk = TestActor.IsValid() && PromotePatchToSource(RestoreBundle, RestoreResult, RestoreError);

    SourceObject = Cast<UClass>(Blueprint->GeneratedClass) ? Cast<UClass>(Blueprint->GeneratedClass)->GetDefaultObject() : nullptr;
    FString AfterRestoreText;
    const bool bReadAfterRestore = SourceObject && InspectorPropertyUtils::GetValueAsText(SourceObject, TestProperty, AfterRestoreText);

    const bool bPassed = Preview.SupportedOperationCount == 1
        && bPromoteOk
        && bReadAfterPromote
        && RI_AreSelfTestPrimitiveValuesEquivalent(AfterPromoteText, PatchedText, PropertyKind)
        && bRestoreOk
        && bReadAfterRestore
        && RI_AreSelfTestPrimitiveValuesEquivalent(AfterRestoreText, SourceOriginalText, PropertyKind);

    OutReport = FString::Printf(
        TEXT("PromoteBlueprintApplySelfTest=%s | Property=%s Preview=%s Apply=%s Restore=%s SourceOriginal=%s AfterPromote=%s AfterRestore=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        Preview.SupportedOperationCount == 1 ? TEXT("ok") : TEXT("bad"),
        bPromoteOk ? TEXT("ok") : (PromoteError.IsEmpty() ? TEXT("fail") : *PromoteError),
        bRestoreOk ? TEXT("ok") : (RestoreError.IsEmpty() ? TEXT("fail") : *RestoreError),
        *SourceOriginalText,
        bReadAfterPromote ? *AfterPromoteText : TEXT("<unreadable>"),
        bReadAfterRestore ? *AfterRestoreText : TEXT("<unreadable>"));

    Cleanup();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPromoteBlueprintApplySelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPromoteBlueprintApplySelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunPromoteMaterialApplySelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        OutReport = TEXT("PromoteMaterialApplySelfTest=BLOCKED | PIE game world is required");
        return false;
    }

    UMaterialInstanceConstant* MIC = RI_LoadPreferredSelfTestMIC();
    UStaticMesh* MeshAsset = RI_LoadPreferredSelfTestMesh();
    if (!MIC || !MeshAsset)
    {
        OutReport = TEXT("PromoteMaterialApplySelfTest=FAIL | Test MIC or mesh asset unavailable");
        return false;
    }

    TArray<FMaterialParameterInfo> VectorInfos;
    TArray<FGuid> VectorIds;
    MIC->GetAllVectorParameterInfo(VectorInfos, VectorIds);
    if (VectorInfos.Num() <= 0)
    {
        OutReport = TEXT("PromoteMaterialApplySelfTest=FAIL | No writable vector parameter found on test MIC");
        return false;
    }

    const FName PreferredNames[] = { TEXT("Color"), TEXT("EmissiveColor"), TEXT("BaseColor") };
    FName ParamName = VectorInfos[0].Name;
    for (const FName PreferredName : PreferredNames)
    {
        const bool bFound = VectorInfos.ContainsByPredicate([PreferredName](const FMaterialParameterInfo& Info)
        {
            return Info.Name == PreferredName;
        });
        if (bFound)
        {
            ParamName = PreferredName;
            break;
        }
    }

    const FMaterialParameterInfo ParamInfo(ParamName);
    FLinearColor OriginalSourceColor = FLinearColor::Black;
    if (!MIC->GetVectorParameterValue(ParamInfo, OriginalSourceColor))
    {
        OutReport = TEXT("PromoteMaterialApplySelfTest=FAIL | Failed to read source MIC vector parameter");
        return false;
    }

    const FLinearColor TargetColor = RI_MakeDistinctSelfTestColor(OriginalSourceColor);
    const FString OriginalSourceText = RI_FormatLinearColorText(OriginalSourceColor);
    const FString TargetText = RI_FormatLinearColorText(TargetColor);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = MakeUniqueObjectName(World, AStaticMeshActor::StaticClass(), TEXT("RI_PromoteMaterialSelfTest"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* TestActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform::Identity, SpawnParams);
    if (!TestActor || !TestActor->GetStaticMeshComponent())
    {
        if (TestActor)
        {
            TestActor->Destroy();
        }
        OutReport = TEXT("PromoteMaterialApplySelfTest=FAIL | Failed to spawn static mesh actor");
        return false;
    }

    UStaticMeshComponent* MeshComp = TestActor->GetStaticMeshComponent();
    MeshComp->SetStaticMesh(MeshAsset);
    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MIC, MeshComp);
    if (!MID)
    {
        TestActor->Destroy();
        OutReport = TEXT("PromoteMaterialApplySelfTest=FAIL | Failed to create runtime MID");
        return false;
    }

    const FRIPromoteResult PreviousPromoteResult = LastPromoteResult;
    auto Cleanup = [&]()
    {
        LastPromoteResult = PreviousPromoteResult;
        if (IsValid(TestActor))
        {
            TestActor->Destroy();
        }
    };

    MeshComp->SetMaterial(0, MID);
    MID->SetVectorParameterValue(ParamName, TargetColor);

    FRIPatchOperation Operation;
    Operation.Target.TargetKind = ERIPatchTargetKind::MaterialSlot;
    Operation.Target.ActorPath = TestActor->GetPathName();
    Operation.Target.ActorClass = TestActor->GetClass()->GetPathName();
    Operation.Target.ActorBaseName = RI_ExtractActorBaseName(TestActor->GetName());
    Operation.Target.ComponentPath = MeshComp->GetPathName();
    Operation.Target.ComponentName = MeshComp->GetName();
    Operation.Target.ComponentClass = MeshComp->GetClass()->GetPathName();
    Operation.Target.MaterialSlotIndex = 0;
    Operation.Target.MaterialSlotName = RI_GetMeshMaterialSlotName(MeshComp, 0);
    Operation.Field.FieldKind = ERIPatchFieldKind::MaterialVector;
    Operation.Field.FieldPath = ParamName.ToString();
    Operation.Field.DisplayName = ParamName.ToString();
    Operation.BaselineValue = OriginalSourceText;
    Operation.PatchedValue = RI_GetMaterialParamValueText(MeshComp, 0, EInspectorChangeType::MaterialVector, ParamName);
    Operation.SourceTag = FString::Printf(TEXT("Material:%s"), *MIC->GetPathName());

    FRIPatchBundle PromoteBundle;
    PromoteBundle.BundleId = TEXT("SelfTest_PromoteMaterialApply");
    PromoteBundle.DisplayName = TEXT("SelfTest Promote Material Apply");
    PromoteBundle.Operations.Add(Operation);

    FRIPromotePreview Preview;
    FString PreviewError;
    if (!CreatePromotePreview(PromoteBundle, Preview, PreviewError))
    {
        Cleanup();
        OutReport = FString::Printf(TEXT("PromoteMaterialApplySelfTest=FAIL | Preview=%s"), *PreviewError);
        return false;
    }

    FRIPromoteResult PromoteResult;
    FString PromoteError;
    const bool bPromoteOk = PromotePatchToSource(PromoteBundle, PromoteResult, PromoteError);

    FLinearColor AfterPromoteColor = FLinearColor::Black;
    const bool bReadAfterPromote = MIC->GetVectorParameterValue(ParamInfo, AfterPromoteColor);

    FRIPatchOperation RestoreOperation = Operation;
    RestoreOperation.PatchedValue = OriginalSourceText;
    FRIPatchBundle RestoreBundle;
    RestoreBundle.BundleId = TEXT("SelfTest_PromoteMaterialRestore");
    RestoreBundle.DisplayName = TEXT("SelfTest Promote Material Restore");
    RestoreBundle.Operations.Add(RestoreOperation);

    FRIPromoteResult RestoreResult;
    FString RestoreError;
    const bool bRestoreOk = PromotePatchToSource(RestoreBundle, RestoreResult, RestoreError);

    FLinearColor AfterRestoreColor = FLinearColor::Black;
    const bool bReadAfterRestore = MIC->GetVectorParameterValue(ParamInfo, AfterRestoreColor);

    const bool bPassed = Preview.SupportedOperationCount == 1
        && bPromoteOk
        && bReadAfterPromote
        && AfterPromoteColor.Equals(TargetColor, 0.0005f)
        && bRestoreOk
        && bReadAfterRestore
        && AfterRestoreColor.Equals(OriginalSourceColor, 0.0005f);

    OutReport = FString::Printf(
        TEXT("PromoteMaterialApplySelfTest=%s | Param=%s Preview=%s Apply=%s Restore=%s SourceOriginal=%s AfterPromote=%s AfterRestore=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *ParamName.ToString(),
        Preview.SupportedOperationCount == 1 ? TEXT("ok") : TEXT("bad"),
        bPromoteOk ? TEXT("ok") : (PromoteError.IsEmpty() ? TEXT("fail") : *PromoteError),
        bRestoreOk ? TEXT("ok") : (RestoreError.IsEmpty() ? TEXT("fail") : *RestoreError),
        *OriginalSourceText,
        bReadAfterPromote ? *RI_FormatLinearColorText(AfterPromoteColor) : TEXT("<unreadable>"),
        bReadAfterRestore ? *RI_FormatLinearColorText(AfterRestoreColor) : TEXT("<unreadable>"));

    Cleanup();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPromoteMaterialApplySelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPromoteMaterialApplySelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunAuditReportSelfTest(FString& OutReport)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeInspector disabled");
    return false;
#else
    OutReport.Reset();

    AActor* SelectedActorPtr = SelectedActor.Get();
    if (!SelectedActorPtr)
    {
        OutReport = TEXT("AuditReportSelfTest=FAIL | No selected actor");
        return false;
    }

    const FString SelectedActorPath = SelectedActorPtr->GetPathName();
    for (const TPair<FString, FString>& Pair : ModifiedValueByKey)
    {
        if (Pair.Key.StartsWith(FString::Printf(TEXT("P|%s|"), *SelectedActorPath))
            || Pair.Key.StartsWith(FString::Printf(TEXT("M|%s|"), *SelectedActorPath)))
        {
            OutReport = TEXT("AuditReportSelfTest=BLOCKED | Selected actor already has modified values");
            return false;
        }
    }

    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIApplyResult PreviousLastApplyResult = LastPatchApplyResult;
    const FRIAuditReport PreviousLastAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;

    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    FString TxtReportPath;
    FString JsonReportPath;

    auto RestoreState = [&]()
    {
        if (SelectedActorPtr && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(SelectedActorPtr, TestProperty, OriginalText, RestoreError);
            }
        }

        if (!TxtReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*TxtReportPath, false, true);
        }
        if (!JsonReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*JsonReportPath, false, true);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastPatchApplyResult = PreviousLastApplyResult;
        LastAuditReport = PreviousLastAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
    };

    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
        SelectedActorPtr,
        TestProperty,
        OriginalText,
        PatchedText,
        PropertyKind,
        NumericOriginalValue,
        NumericTargetValue))
    {
        OutReport = TEXT("AuditReportSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    FString Error;
    if (!ApplyPropertyTextNow(SelectedActorPtr, TestProperty, PatchedText, Error))
    {
        OutReport = FString::Printf(TEXT("AuditReportSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    if (!StageSelectionAsPatch(Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("AuditReportSelfTest=FAIL | StageError=%s"), *Error);
        return false;
    }

    const FRIPatchBundle CapturedBundle = GetStagedPatch();
    if (CapturedBundle.Operations.Num() <= 0)
    {
        RestoreState();
        OutReport = TEXT("AuditReportSelfTest=FAIL | Captured bundle empty");
        return false;
    }

    FRIAuditReport BaselineReport;
    if (!BuildAuditReport(ERIAuditComparisonMode::BaselineVsCurrent, CapturedBundle, BaselineReport, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("AuditReportSelfTest=FAIL | BaselineAuditError=%s"), *Error);
        return false;
    }

    FRIAuditReport CurrentVsPatchReport;
    if (!BuildAuditReport(ERIAuditComparisonMode::CurrentVsPatch, CapturedBundle, CurrentVsPatchReport, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("AuditReportSelfTest=FAIL | CurrentVsPatchError=%s"), *Error);
        return false;
    }

    auto CountForProperty = [&TestProperty](const FRIAuditReport& Report, bool bExpectedDifferent) -> int32
    {
        int32 Count = 0;
        for (const FRIAuditLine& Line : Report.Lines)
        {
            if (Line.Field.FieldPath == TestProperty.ToString() && Line.bDifferent == bExpectedDifferent)
            {
                ++Count;
            }
        }
        return Count;
    };

    const int32 BaselineDiffCount = CountForProperty(BaselineReport, true);
    const int32 CurrentVsPatchSameCount = CountForProperty(CurrentVsPatchReport, false);

    FString ExportError;
    const bool bTxtExportOk = ExportLastAuditReportToFile(false, TxtReportPath, ExportError);
    FString JsonExportError;
    const bool bJsonExportOk = ExportLastAuditReportToFile(true, JsonReportPath, JsonExportError);

    FRIApplyResult RollbackResult;
    if (!RollbackStagedPatch(RollbackResult))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("AuditReportSelfTest=FAIL | Rollback=%s"), *RollbackResult.Summary);
        return false;
    }

    FString AfterRollback;
    InspectorPropertyUtils::GetValueAsText(SelectedActorPtr, TestProperty, AfterRollback);
    const bool bRollbackOk = (PropertyKind == TEXT("bool"))
        ? AfterRollback.Equals(OriginalText, ESearchCase::IgnoreCase)
        : FMath::IsNearlyEqual(FCString::Atod(*AfterRollback), NumericOriginalValue, 0.001);

    const bool bPassed = BaselineReport.Lines.Num() > 0
        && CurrentVsPatchReport.Lines.Num() > 0
        && BaselineDiffCount > 0
        && CurrentVsPatchSameCount > 0
        && bTxtExportOk
        && bJsonExportOk
        && bRollbackOk;

    OutReport = FString::Printf(
        TEXT("AuditReportSelfTest=%s | Property=%s BaselineLines=%d BaselineDiff=%d CurrentVsPatchLines=%d CurrentVsPatchSame=%d TxtExport=%s JsonExport=%s AfterRollback=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        BaselineReport.Lines.Num(),
        BaselineDiffCount,
        CurrentVsPatchReport.Lines.Num(),
        CurrentVsPatchSameCount,
        bTxtExportOk ? TEXT("yes") : *ExportError,
        bJsonExportOk ? TEXT("yes") : *JsonExportError,
        *AfterRollback);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunAuditReportSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunAuditReportSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunContextStripSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("ContextStripSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("ContextStripSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;

    auto RestoreState = [&]()
    {
        StagedPatchBundle = PreviousStagedPatch;
        bHasStagedPatch = bHadStagedPatch;
        SetSelectedActor(PreviousSelectedActor);
        UpdateSharedContextStrip();
    };

    Close();
    Open();
    EnsureSharedContextStripInjected();

    UUserWidget* Widget = PanelWidget.Get();
    UBorder* StripBorder = Widget && Widget->WidgetTree
        ? Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStrip")))
        : nullptr;
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    UPanelWidget* StripParent = StripBorder ? Cast<UPanelWidget>(StripBorder->GetParent()) : nullptr;
    UVerticalBox* VerticalStripHost = Cast<UVerticalBox>(StripParent);
    const int32 DefaultActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;

    const bool bHostOk = SharedContextStripHostPanel.IsValid();
    const bool bWidgetsOk = SharedContextActorText && SharedContextClassText && SharedContextSourceText && SharedContextStagedText;
    const bool bHostNamedOk = VerticalStripHost && VerticalStripHost->GetName() == TEXT("RI_SharedContextStripHost");
    const bool bParentVerticalOk = VerticalStripHost != nullptr;
    const bool bParentNotOverlayOk = StripParent && !StripParent->IsA<UOverlay>();
    const bool bSharedParentOk = VerticalStripHost && Switcher && Switcher->GetParent() == VerticalStripHost;
    const int32 StripIndex = RI_GetPanelChildIndex(VerticalStripHost, StripBorder);
    const int32 SwitcherIndex = RI_GetPanelChildIndex(VerticalStripHost, Switcher);
    const bool bStripBeforeSwitcherOk = StripIndex != INDEX_NONE && SwitcherIndex != INDEX_NONE && StripIndex < SwitcherIndex;
    const bool bStripAutomaticOk = RI_IsVerticalSlotRule(StripBorder, ESlateSizeRule::Automatic);
    const bool bSwitcherFillOk = RI_IsVerticalSlotRule(Switcher, ESlateSizeRule::Fill);
    const bool bDefaultActorPageOk = DefaultActiveIndex == 0;

    SetSelectedActor(nullptr);
    StagedPatchBundle = FRIPatchBundle();
    bHasStagedPatch = false;
    UpdateSharedContextStrip();

    const FString EmptyActorText = SharedContextActorText ? SharedContextActorText->GetText().ToString() : FString();
    const FString EmptyClassText = SharedContextClassText ? SharedContextClassText->GetText().ToString() : FString();
    const FString EmptySourceText = SharedContextSourceText ? SharedContextSourceText->GetText().ToString() : FString();
    const FString EmptyStagedText = SharedContextStagedText ? SharedContextStagedText->GetText().ToString() : FString();

    const bool bEmptyActorOk = EmptyActorText.Equals(TEXT("No selected actor"), ESearchCase::CaseSensitive);
    const bool bEmptyClassOk = EmptyClassText.Equals(TEXT("No actor class"), ESearchCase::CaseSensitive);
    const bool bEmptySourceOk = EmptySourceText.Equals(TEXT("No source asset"), ESearchCase::CaseSensitive);
    const bool bEmptyStagedOk = EmptyStagedText.Equals(TEXT("No staged patch"), ESearchCase::CaseSensitive);

    AActor* TestActor = PreviousSelectedActor;
    if (!TestActor)
    {
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (*It)
                {
                    TestActor = *It;
                    break;
                }
            }
        }
    }

    if (!TestActor)
    {
        RestoreState();
        OutReport = TEXT("ContextStripSelfTest=BLOCKED | No actor available");
        return false;
    }

    SetSelectedActor(TestActor);
    FRIPatchBundle SyntheticBundle;
    SyntheticBundle.BundleId = TEXT("SelfTest_ContextStrip");
    SyntheticBundle.DisplayName = TEXT("SelfTest Context Strip");
    SyntheticBundle.Operations.AddDefaulted(1);
    StagedPatchBundle = SyntheticBundle;
    bHasStagedPatch = true;
    UpdateSharedContextStrip();

    const FString SelectedActorText = SharedContextActorText ? SharedContextActorText->GetText().ToString() : FString();
    const FString SelectedClassText = SharedContextClassText ? SharedContextClassText->GetText().ToString() : FString();
    const FString SelectedSourceText = SharedContextSourceText ? SharedContextSourceText->GetText().ToString() : FString();
    const FString SelectedStagedText = SharedContextStagedText ? SharedContextStagedText->GetText().ToString() : FString();

    const FString ExpectedActorText = RI_GetActorDisplayLabel(TestActor);
    const FString ExpectedClassText = TestActor->GetClass() ? TestActor->GetClass()->GetName() : FString();
    const FString ExpectedSourceText = TestActor->GetClass() ? TestActor->GetClass()->GetPathName() : FString();
    const bool bSelectedActorOk = SelectedActorText.Equals(ExpectedActorText, ESearchCase::CaseSensitive);
    const bool bSelectedClassOk = SelectedClassText.Equals(ExpectedClassText, ESearchCase::CaseSensitive);
    const bool bSelectedSourceOk = SelectedSourceText.Equals(ExpectedSourceText, ESearchCase::CaseSensitive);
    const bool bSelectedStagedOk = SelectedStagedText.Equals(TEXT("Staged (1 ops)"), ESearchCase::CaseSensitive);

    const bool bPassed = bHostOk
        && bWidgetsOk
        && bHostNamedOk
        && bParentVerticalOk
        && bParentNotOverlayOk
        && bSharedParentOk
        && bStripBeforeSwitcherOk
        && bStripAutomaticOk
        && bSwitcherFillOk
        && bDefaultActorPageOk
        && bEmptyActorOk
        && bEmptyClassOk
        && bEmptySourceOk
        && bEmptyStagedOk
        && bSelectedActorOk
        && bSelectedClassOk
        && bSelectedSourceOk
        && bSelectedStagedOk;

    OutReport = FString::Printf(
        TEXT("ContextStripSelfTest=%s | Host=%s Widgets=%d HostName=%d ParentVertical=%d NotOverlay=%d SharedParent=%d Order=%d StripAuto=%d SwitcherFill=%d DefaultActor=%d Empty=%d/%d/%d/%d Selected=%d/%d/%d/%d Actor=%s Class=%s Source=%s Staged=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        SharedContextStripHostPanel.IsValid() ? *SharedContextStripHostPanel->GetName() : TEXT("None"),
        bWidgetsOk ? 1 : 0,
        bHostNamedOk ? 1 : 0,
        bParentVerticalOk ? 1 : 0,
        bParentNotOverlayOk ? 1 : 0,
        bSharedParentOk ? 1 : 0,
        bStripBeforeSwitcherOk ? 1 : 0,
        bStripAutomaticOk ? 1 : 0,
        bSwitcherFillOk ? 1 : 0,
        bDefaultActorPageOk ? 1 : 0,
        bEmptyActorOk ? 1 : 0,
        bEmptyClassOk ? 1 : 0,
        bEmptySourceOk ? 1 : 0,
        bEmptyStagedOk ? 1 : 0,
        bSelectedActorOk ? 1 : 0,
        bSelectedClassOk ? 1 : 0,
        bSelectedSourceOk ? 1 : 0,
        bSelectedStagedOk ? 1 : 0,
        *SelectedActorText,
        *SelectedClassText,
        *SelectedSourceText,
        *SelectedStagedText);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunContextStripSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunContextStripSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFilePageInjectionSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FilePageInjectionSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("FilePageInjectionSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const ERIAuditComparisonMode PreviousActiveAuditView = ActiveFileAuditMode;

    auto RestoreState = [&]()
    {
        SetSelectedActor(PreviousSelectedActor);
        StagedPatchBundle = PreviousStagedPatch;
        bHasStagedPatch = bHadStagedPatch;
        ActiveFileAuditMode = PreviousActiveAuditView;
        InvalidateFileManagementSummaryCache();
        UpdateSharedContextStrip();
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    Close();
    Open();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();
    const int32 DefaultActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    ShowFilePage();

    AActor* TestActor = GetSelectedActor();
    if (!TestActor)
    {
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (*It && !RI_IsUndesirableDefaultSelectionActor(*It))
                {
                    TestActor = *It;
                    break;
                }
            }
        }
    }

    if (TestActor && TestActor != GetSelectedActor())
    {
        SetSelectedActor(TestActor);
        InvalidateFileManagementSummaryCache();
        UpdateSharedContextStrip();
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    }

    StagedPatchBundle = FRIPatchBundle();
    bHasStagedPatch = false;
    InvalidateFileManagementSummaryCache();
    UpdateSharedContextStrip();
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    if (UInspectorFilePageWidget* ExistingFilePage = FilePageWidget.Get())
    {
        ExistingFilePage->RefreshFromSubsystem();
    }

    UPanelWidget* HostPanel = FindFileHostPanel();
    UInspectorFilePageWidget* Page = FilePageWidget.Get();

    bool bHostContainsPage = false;
    int32 VisibleLegacySiblingCount = 0;
    FString VisibleLegacySiblingNames;
    if (HostPanel && Page)
    {
        const int32 ChildCount = HostPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = HostPanel->GetChildAt(ChildIndex);
            if (Child == Page)
            {
                bHostContainsPage = true;
                continue;
            }

            if (Child && Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
                if (!VisibleLegacySiblingNames.IsEmpty())
                {
                    VisibleLegacySiblingNames += TEXT(",");
                }
                VisibleLegacySiblingNames += Child->GetName();
            }
        }
    }

    FRIFileManagementSummary Summary;
    FString SummaryError;
    const bool bSummaryOk = GetFileManagementSummary(Summary, SummaryError);
    const bool bHasSelectedActor = GetSelectedActor() != nullptr;
    const bool bHasPromoteResult = Summary.bHasLastPromoteResult || !Summary.LastPromoteSummary.TrimStartAndEnd().IsEmpty();
    const bool bScrollRootOk = Page && Page->HasPageScrollRoot();
    const bool bSelectedActorOk = Page && !Page->GetSelectedActorSummaryLabel().IsEmpty() && !Page->GetSelectedActorSummaryLabel().Equals(TEXT("-"), ESearchCase::CaseSensitive);
    const bool bSelectedClassOk = Page && !Page->GetSelectedActorClassLabel().IsEmpty() && !Page->GetSelectedActorClassLabel().Equals(TEXT("-"), ESearchCase::CaseSensitive);
    const bool bSelectedSourceOk = Page && !Page->GetSelectedActorSourceLabel().IsEmpty() && !Page->GetSelectedActorSourceLabel().Equals(TEXT("-"), ESearchCase::CaseSensitive);
    const bool bNextStepOk = Page && !Page->GetNextStepLabel().IsEmpty() && !Page->GetNextStepLabel().Equals(TEXT("-"), ESearchCase::CaseSensitive);
    const bool bActionGuideOk = Page && !Page->GetActionGuideLabel().IsEmpty() && !Page->GetActionGuideLabel().Equals(TEXT("-"), ESearchCase::CaseSensitive);
    const bool bRemoteSectionHidden = Page && !Page->HasRemoteSessionSection();
    const bool bDiagnosticsSectionHidden = Page && !Page->HasDiagnosticsSection();
    const bool bEmbeddedSettingsHidden = Page && !Page->HasEmbeddedSettingsSection();
    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    const bool bFileSwitcherFillOk = RI_IsVerticalSlotRule(Switcher, ESlateSizeRule::Fill);
    const bool bDefaultActorPageOk = DefaultActiveIndex == 0;

    const auto CheckButtonState = [](UButton* Button, bool bExpectedEnabled, const TCHAR* ExpectedEnabledTooltip, const TCHAR* ExpectedDisabledReason)
    {
        if (!Button)
        {
            return false;
        }

        const bool bEnabled = Button->GetIsEnabled();
        const FString Tooltip = Button->GetToolTipText().ToString();
        if (bEnabled != bExpectedEnabled)
        {
            return false;
        }

        if (!bExpectedEnabled)
        {
            return Tooltip.Contains(ExpectedDisabledReason);
        }

        if (Tooltip.IsEmpty())
        {
            return true;
        }

        return true;
    };

    const auto DescribeButtonState = [](UButton* Button)
    {
        if (!Button)
        {
            return FString(TEXT("null"));
        }

        const FString Tooltip = Button->GetToolTipText().ToString().ReplaceCharWithEscapedChar();
        return FString::Printf(TEXT("%d|%s"), Button->GetIsEnabled() ? 1 : 0, *Tooltip);
    };

    const auto CheckOptionalButtonState = [&CheckButtonState](UButton* Button, bool bExpectedEnabled, const TCHAR* ExpectedEnabledTooltip, const TCHAR* ExpectedDisabledReason)
    {
        return Button == nullptr || CheckButtonState(Button, bExpectedEnabled, ExpectedEnabledTooltip, ExpectedDisabledReason);
    };

    HandleActorTabClicked();
    const int32 ActorActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    UUserWidget* Widget = PanelWidget.Get();
    UBorder* StripBorder = Widget && Widget->WidgetTree
        ? Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStrip")))
        : nullptr;
    UVerticalBox* StripHost = Widget && Widget->WidgetTree
        ? Cast<UVerticalBox>(Widget->WidgetTree->FindWidget(TEXT("RI_SharedContextStripHost")))
        : nullptr;
    const bool bActorHostOk = StripHost && StripHost->GetName() == TEXT("RI_SharedContextStripHost");
    const bool bActorStripNotOverlay = StripBorder && StripBorder->GetParent() == StripHost && !StripHost->IsA<UOverlay>();
    const int32 StripIndex = RI_GetPanelChildIndex(StripHost, StripBorder);
    const int32 SwitcherIndex = RI_GetPanelChildIndex(StripHost, Switcher);
    const bool bActorOrderOk = StripIndex != INDEX_NONE && SwitcherIndex != INDEX_NONE && StripIndex < SwitcherIndex;
    const bool bActorStripAutomaticOk = RI_IsVerticalSlotRule(StripBorder, ESlateSizeRule::Automatic);
    const bool bActorSwitcherFillOk = RI_IsVerticalSlotRule(Switcher, ESlateSizeRule::Fill);
    const bool bActorLayoutOk = ActorActiveIndex == 0 && bActorHostOk && bActorStripNotOverlay && bActorOrderOk && bActorStripAutomaticOk && bActorSwitcherFillOk;

    const bool bInitialStageButtonsOk = CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileStagePatch")) : nullptr, bHasSelectedActor, TEXT("Stage the current runtime edits as a patch."), TEXT("Select an actor first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FilePreviewPromote")) : nullptr, Summary.bHasStagedPatch, TEXT("Preview the staged patch on the source side."), TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FilePromoteApply")) : nullptr, Summary.bHasStagedPatch && Summary.bHasPromotePreview, TEXT("Write the staged patch back to source."), Summary.bHasStagedPatch ? TEXT("Preview source changes first.") : TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileClearStaged")) : nullptr, Summary.bHasStagedPatch, TEXT("Discard the staged patch."), TEXT("Nothing is staged yet."))
        && CheckOptionalButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildRoleCompare")) : nullptr, Summary.bHasStagedPatch, TEXT("Build a runtime role compare report."), TEXT("Stage runtime changes first."))
        && CheckOptionalButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildRemoteSessionCompare")) : nullptr, true, TEXT("Build a remote session compare report."), TEXT(""));

    FRIPatchBundle SyntheticBundle;
    SyntheticBundle.BundleId = TEXT("SelfTest_FileButtons");
    SyntheticBundle.DisplayName = TEXT("SelfTest File Buttons");
    SyntheticBundle.Operations.AddDefaulted(1);
    StagedPatchBundle = SyntheticBundle;
    bHasStagedPatch = true;
    InvalidateFileManagementSummaryCache();
    if (Page)
    {
        Page->RefreshFromSubsystem();
    }

    FRIFileManagementSummary FinalSummary;
    FString FinalSummaryError;
    const bool bFinalSummaryOk = GetFileManagementSummary(FinalSummary, FinalSummaryError);

    const bool bStagedButtonsOk = CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FilePreviewPromote")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Preview the staged patch on the source side."), TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FilePromoteApply")) : nullptr, FinalSummary.bHasStagedPatch && FinalSummary.bHasPromotePreview, TEXT("Write the staged patch back to source."), FinalSummary.bHasStagedPatch ? TEXT("Preview source changes first.") : TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileClearStaged")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Discard the staged patch."), TEXT("Nothing is staged yet."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileExportPatch")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Export the staged patch bundle."), TEXT("Stage a runtime patch first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileSavePreset")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Save the staged patch as a preset."), TEXT("Stage a runtime patch first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileApplyLatestPreset")) : nullptr, FinalSummary.PresetCount > 0, TEXT("Apply the newest available preset."), TEXT("No presets are available yet."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildBaselineAudit")) : nullptr, bHasSelectedActor, TEXT("Compare the baseline actor state."), TEXT("Select an actor first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildAudit")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Compare the current runtime state against the staged patch."), TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildPatchVsSource")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Compare the staged patch against source."), TEXT("Stage runtime changes first."))
        && CheckButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildAppliedAudit")) : nullptr, FinalSummary.bHasLastPromoteResult || !FinalSummary.LastPromoteSummary.TrimStartAndEnd().IsEmpty(), TEXT("Verify the applied source after promote."), TEXT("Run Apply To Source first."))
        && CheckOptionalButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildRoleCompare")) : nullptr, FinalSummary.bHasStagedPatch, TEXT("Build a runtime role compare report."), TEXT("Stage runtime changes first."))
        && CheckOptionalButtonState(Page ? Page->GetNamedButton(TEXT("BTN_FileBuildRemoteSessionCompare")) : nullptr, true, TEXT("Build a remote session compare report."), TEXT(""));

    const bool bPassed = HostPanel && Page && bHostContainsPage && VisibleLegacySiblingCount == 0
        && bScrollRootOk && Switcher && ActiveIndex == 1 && bSummaryOk
        && bSelectedActorOk && bSelectedClassOk && bSelectedSourceOk
        && bNextStepOk && bActionGuideOk
        && bRemoteSectionHidden
        && bDiagnosticsSectionHidden
        && bEmbeddedSettingsHidden
        && bFileSwitcherFillOk
        && bDefaultActorPageOk
        && bActorLayoutOk;
    const bool bFinalPassed = bPassed && bInitialStageButtonsOk && bStagedButtonsOk && bFinalSummaryOk;

    OutReport = FString::Printf(
        TEXT("FilePageInjectionSelfTest=%s | ActiveIndex=%d DefaultActor=%d Host=%s HostHasPage=%d VisibleLegacy=%d ScrollRoot=%d SwitcherFill=%d Summary=%s Actor=%s Class=%s Source=%s Next=%s Guide=%s Remote=%s Diagnostics=%s EmbeddedSettings=%s ActorLayout=%d ActorHost=%d ActorOrder=%d ActorStripAuto=%d ActorSwitcherFill=%d Buttons=%d/%d StageBtn=%s PreviewBtn=%s ApplyBtn=%s ClearBtn=%s ExportBtn=%s SaveBtn=%s LatestPresetBtn=%s BaselineAuditBtn=%s AuditBtn=%s PatchVsSourceBtn=%s AppliedAuditBtn=%s RoleCompareBtn=%s RemoteSessionCompareBtn=%s Snapshots=%d Patches=%d Presets=%d Audits=%d Staged=%s Promote=%s LegacyNames=%s"),
        bFinalPassed ? TEXT("PASS") : TEXT("FAIL"),
        ActiveIndex,
        bDefaultActorPageOk ? 1 : 0,
        HostPanel ? *HostPanel->GetName() : TEXT("None"),
        bHostContainsPage ? 1 : 0,
        VisibleLegacySiblingCount,
        bScrollRootOk ? 1 : 0,
        bFileSwitcherFillOk ? 1 : 0,
        bSummaryOk ? TEXT("ok") : *SummaryError,
        bSelectedActorOk ? TEXT("ok") : *(Page ? Page->GetSelectedActorSummaryLabel() : FString(TEXT("None"))),
        bSelectedClassOk ? TEXT("ok") : *(Page ? Page->GetSelectedActorClassLabel() : FString(TEXT("None"))),
        bSelectedSourceOk ? TEXT("ok") : *(Page ? Page->GetSelectedActorSourceLabel() : FString(TEXT("None"))),
        bNextStepOk ? TEXT("ok") : *(Page ? Page->GetNextStepLabel() : FString(TEXT("None"))),
        bActionGuideOk ? TEXT("ok") : *(Page ? Page->GetActionGuideLabel() : FString(TEXT("None"))),
        bRemoteSectionHidden ? TEXT("hidden") : TEXT("visible"),
        bDiagnosticsSectionHidden ? TEXT("hidden") : TEXT("visible"),
        bEmbeddedSettingsHidden ? TEXT("hidden") : TEXT("visible"),
        bActorLayoutOk ? 1 : 0,
        bActorHostOk ? 1 : 0,
        bActorOrderOk ? 1 : 0,
        bActorStripAutomaticOk ? 1 : 0,
        bActorSwitcherFillOk ? 1 : 0,
        bInitialStageButtonsOk ? 1 : 0,
        bStagedButtonsOk ? 1 : 0,
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileStagePatch"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FilePreviewPromote"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FilePromoteApply"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileClearStaged"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileExportPatch"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileSavePreset"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileApplyLatestPreset"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildBaselineAudit"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildAudit"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildPatchVsSource"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildAppliedAudit"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildRoleCompare"))) : TEXT("null"),
        Page ? *DescribeButtonState(Page->GetNamedButton(TEXT("BTN_FileBuildRemoteSessionCompare"))) : TEXT("null"),
        bFinalSummaryOk ? FinalSummary.SnapshotCount : 0,
        bFinalSummaryOk ? FinalSummary.PatchBundleCount : 0,
        bFinalSummaryOk ? FinalSummary.PresetCount : 0,
        bFinalSummaryOk ? FinalSummary.AuditReportCount : 0,
        bFinalSummaryOk && FinalSummary.bHasStagedPatch ? TEXT("yes") : TEXT("no"),
        bFinalSummaryOk && !FinalSummary.PromotePreviewSummary.IsEmpty() ? *FinalSummary.PromotePreviewSummary : TEXT("n/a"),
        VisibleLegacySiblingNames.IsEmpty() ? TEXT("None") : *VisibleLegacySiblingNames);
    RestoreState();
    return bFinalPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFilePageInjectionSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFilePageInjectionSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunWorkflowPageViewSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("WorkflowPageViewSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("WorkflowPageViewSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    Close();
    Open();
    ShowTestPage();

    UPanelWidget* HostPanel = FindTestHostPanel();
    UInspectorTestPageWidget* Page = TestPageWidget.Get();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();

    bool bHostContainsPage = false;
    int32 VisibleLegacySiblingCount = 0;
    FString VisibleLegacySiblingNames;
    if (HostPanel && Page)
    {
        const int32 ChildCount = HostPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = HostPanel->GetChildAt(ChildIndex);
            if (Child == Page)
            {
                bHostContainsPage = true;
                continue;
            }

            if (Child && Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
                if (!VisibleLegacySiblingNames.IsEmpty())
                {
                    VisibleLegacySiblingNames += TEXT(",");
                }
                VisibleLegacySiblingNames += Child->GetName();
            }
        }
    }

    const TArray<FRIWorkflowDefinition> Workflows = GetAvailableWorkflows();
    const int32 ExpectedCount = Workflows.Num();
    const int32 RenderedCount = Page ? Page->GetRenderedWorkflowRowCount() : 0;
    const FString SelectedWorkflowLabel = Page ? Page->GetSelectedWorkflowLabel() : FString();
    const bool bPageScrollOk = Page && Page->HasPageScrollRoot();
    const bool bSelectionRowOk = Page && Page->HasWorkflowSelectionRow();
    const bool bRemoteSectionOk = Page && Page->HasRemoteSessionSection();
    const bool bWorkflowSectionOk = Page && Page->HasAvailableWorkflowSection();
    const bool bTestsSectionOk = Page && Page->HasAvailableTestsSection();
    const bool bReportSectionOk = Page && Page->HasReportSection();
    const bool bDiagnosticsSectionOk = Page && Page->HasDiagnosticsSection();
    const bool bActivityLogSectionOk = Page && Page->HasActivityLogSection();
    const bool bTestsCollapsedOk = Page && !Page->IsTestsSectionExpanded();
    const bool bRemoteCollapsedOk = Page && !Page->IsRemoteSessionSectionExpanded();
    const bool bDiagnosticsCollapsedOk = Page && !Page->IsDiagnosticsSectionExpanded();
    const bool bActivityLogCollapsedOk = Page && !Page->IsActivityLogSectionExpanded();
    const bool bNestedWorkflowRowOk = Page && Page->HasRenderedWorkflowRow(RI_WorkflowId_MainlineActorEndToEndClosure);
    const bool bRenderedCountOk = ExpectedCount > 0 && RenderedCount == ExpectedCount;
    const bool bSelectedWorkflowOk = !SelectedWorkflowLabel.IsEmpty() && !SelectedWorkflowLabel.Equals(TEXT("None"), ESearchCase::CaseSensitive);
    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    const bool bPassed = HostPanel && Page && bHostContainsPage && Switcher && ActiveIndex == TestPageIndex
        && VisibleLegacySiblingCount == 0
        && bPageScrollOk && bSelectionRowOk && bRemoteSectionOk && bWorkflowSectionOk
        && bTestsSectionOk && bReportSectionOk && bDiagnosticsSectionOk && bActivityLogSectionOk
        && bTestsCollapsedOk && bRemoteCollapsedOk && bDiagnosticsCollapsedOk && bActivityLogCollapsedOk
        && bNestedWorkflowRowOk && bRenderedCountOk && bSelectedWorkflowOk;

    OutReport = FString::Printf(
        TEXT("WorkflowPageViewSelfTest=%s | ActiveIndex=%d Host=%s HostHasPage=%d VisibleLegacy=%d Scroll=%d SelectionRow=%d Remote=%d Diagnostics=%d Activity=%d Workflows=%d Tests=%d Report=%d Collapsed=%d/%d/%d/%d Rendered=%d Expected=%d EndToEndRow=%d Selected=%s LegacyNames=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        ActiveIndex,
        HostPanel ? *HostPanel->GetName() : TEXT("None"),
        bHostContainsPage ? 1 : 0,
        VisibleLegacySiblingCount,
        bPageScrollOk ? 1 : 0,
        bSelectionRowOk ? 1 : 0,
        bRemoteSectionOk ? 1 : 0,
        bDiagnosticsSectionOk ? 1 : 0,
        bActivityLogSectionOk ? 1 : 0,
        bWorkflowSectionOk ? 1 : 0,
        bTestsSectionOk ? 1 : 0,
        bReportSectionOk ? 1 : 0,
        bTestsCollapsedOk ? 1 : 0,
        bRemoteCollapsedOk ? 1 : 0,
        bDiagnosticsCollapsedOk ? 1 : 0,
        bActivityLogCollapsedOk ? 1 : 0,
        RenderedCount,
        ExpectedCount,
        bNestedWorkflowRowOk ? 1 : 0,
        bSelectedWorkflowOk ? *SelectedWorkflowLabel : TEXT("None"),
        VisibleLegacySiblingNames.IsEmpty() ? TEXT("None") : *VisibleLegacySiblingNames);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunWorkflowPageViewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunWorkflowPageViewSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunTestPageLayoutSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("TestPageLayoutSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("TestPageLayoutSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    Close();
    Open();
    ShowTestPage();

    UPanelWidget* HostPanel = FindTestHostPanel();
    UInspectorTestPageWidget* Page = TestPageWidget.Get();
    UWidgetSwitcher* Switcher = ContentSwitcher.Get();

    bool bHostContainsPage = false;
    int32 VisibleLegacySiblingCount = 0;
    FString VisibleLegacySiblingNames;
    if (HostPanel && Page)
    {
        const int32 ChildCount = HostPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = HostPanel->GetChildAt(ChildIndex);
            if (Child == Page)
            {
                bHostContainsPage = true;
                continue;
            }

            if (Child && Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
                if (!VisibleLegacySiblingNames.IsEmpty())
                {
                    VisibleLegacySiblingNames += TEXT(",");
                }
                VisibleLegacySiblingNames += Child->GetName();
            }
        }
    }

    const bool bPageScrollOk = Page && Page->HasPageScrollRoot();
    const bool bRemoteSectionOk = Page && Page->HasRemoteSessionSection();
    const bool bWorkflowSectionOk = Page && Page->HasAvailableWorkflowSection();
    const bool bTestsSectionOk = Page && Page->HasAvailableTestsSection();
    const bool bReportSectionOk = Page && Page->HasReportSection();
    const bool bDiagnosticsSectionOk = Page && Page->HasDiagnosticsSection();
    const bool bActivityLogSectionOk = Page && Page->HasActivityLogSection();
    const bool bRemoteOverrideOk = Page && Page->HasRemoteOverrideSection();
    const bool bTestsCollapsedOk = Page && !Page->IsTestsSectionExpanded();
    const bool bRemoteCollapsedOk = Page && !Page->IsRemoteSessionSectionExpanded();
    const bool bDiagnosticsCollapsedOk = Page && !Page->IsDiagnosticsSectionExpanded();
    const bool bActivityLogCollapsedOk = Page && !Page->IsActivityLogSectionExpanded();
    const bool bOverrideCollapsedOk = Page && !Page->IsRemoteOverrideSectionExpanded();
    const int32 ActiveIndex = Switcher ? Switcher->GetActiveWidgetIndex() : INDEX_NONE;
    const bool bPassed = HostPanel && Page && bHostContainsPage && Switcher && ActiveIndex == TestPageIndex
        && VisibleLegacySiblingCount == 0
        && bPageScrollOk && bRemoteSectionOk && bWorkflowSectionOk && bTestsSectionOk && bReportSectionOk
        && bDiagnosticsSectionOk && bActivityLogSectionOk && bRemoteOverrideOk
        && bTestsCollapsedOk && bRemoteCollapsedOk && bDiagnosticsCollapsedOk && bActivityLogCollapsedOk && bOverrideCollapsedOk;

    OutReport = FString::Printf(
        TEXT("TestPageLayoutSelfTest=%s | ActiveIndex=%d Host=%s HostHasPage=%d VisibleLegacy=%d Scroll=%d Remote=%d Diagnostics=%d Activity=%d Override=%d Workflows=%d Tests=%d Report=%d Collapsed=%d/%d/%d/%d/%d LegacyNames=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        ActiveIndex,
        HostPanel ? *HostPanel->GetName() : TEXT("None"),
        bHostContainsPage ? 1 : 0,
        VisibleLegacySiblingCount,
        bPageScrollOk ? 1 : 0,
        bRemoteSectionOk ? 1 : 0,
        bDiagnosticsSectionOk ? 1 : 0,
        bActivityLogSectionOk ? 1 : 0,
        bRemoteOverrideOk ? 1 : 0,
        bWorkflowSectionOk ? 1 : 0,
        bTestsSectionOk ? 1 : 0,
        bReportSectionOk ? 1 : 0,
        bTestsCollapsedOk ? 1 : 0,
        bRemoteCollapsedOk ? 1 : 0,
        bDiagnosticsCollapsedOk ? 1 : 0,
        bActivityLogCollapsedOk ? 1 : 0,
        bOverrideCollapsedOk ? 1 : 0,
        VisibleLegacySiblingNames.IsEmpty() ? TEXT("None") : *VisibleLegacySiblingNames);
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunPanelInteractionSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("PanelInteractionSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("PanelInteractionSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    Close();
    Open();
    EnsurePanelInteractionInitialized();

    UUserWidget* Panel = PanelWidget.Get();
    if (!Panel)
    {
        OutReport = TEXT("PanelInteractionSelfTest=FAIL | PanelMissing");
        return false;
    }

    Panel->TakeWidget();
    Panel->ForceLayoutPrepass();
    CacheInitialPanelHeight();

    auto RefreshPanelGeometry = [&Panel]() -> FGeometry
    {
        for (int32 Attempt = 0; Attempt < 3; ++Attempt)
        {
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().Tick(ESlateTickType::All);
            }

            Panel->TakeWidget();
            Panel->ForceLayoutPrepass();
            if (TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget())
            {
                CachedWidget->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
            }
            Panel->ForceLayoutPrepass();

            const FGeometry Geometry = Panel->GetCachedGeometry();
            const FVector2D LocalSize = Geometry.GetLocalSize();
            if (LocalSize.X > 1.0f && LocalSize.Y > 1.0f)
            {
                return Geometry;
            }
        }

        return Panel->GetCachedGeometry();
    };

    const FVector2D OriginalTranslation = PanelTranslation;
    const float OriginalHeight = PanelHeight > 1.0f ? PanelHeight : PanelDefaultHeight;

    auto RestoreState = [&]()
    {
        HandlePanelPointerUp();
        PanelTranslation = OriginalTranslation;
        PanelHeight = OriginalHeight > 1.0f ? OriginalHeight : PanelDefaultHeight;
        ApplyPanelInteractionPresentation();
    };

    const FGeometry PanelGeometry = RefreshPanelGeometry();
    const FVector2D PanelSize = PanelGeometry.GetLocalSize();
    const bool bHasGeometry = PanelSize.X > 1.0f && PanelSize.Y > 1.0f;
    const TCHAR* InteractionMode = bHasGeometry ? TEXT("Geometry") : TEXT("Fallback");
    const FVector2D LogicalViewportSize = RI_GetLogicalViewportSize(GetWorld());
    const float ExpectedDefaultWidth = RI_ResolvePanelDefaultDimension(
        LogicalViewportSize.X,
        RI_DefaultPanelViewportWidthFraction,
        RI_MinUsablePanelWidth,
        RI_DefaultPanelWidthMax);
    const float ExpectedDefaultHeight = RI_ResolvePanelDefaultDimension(
        LogicalViewportSize.Y,
        RI_DefaultPanelViewportHeightFraction,
        RI_DefaultPanelHeightMin,
        0.0f);
    const FVector2D ExpectedDefaultPosition = RI_GetDefaultPanelCanvasPosition(GetWorld(), FVector2D(ExpectedDefaultWidth, ExpectedDefaultHeight));
    const FVector2D RootCanvasPosition = PanelRootCanvasSlot.IsValid() ? PanelRootCanvasSlot->GetPosition() : FVector2D::ZeroVector;
    const FVector2D RootCanvasSize = PanelRootCanvasSlot.IsValid() ? PanelRootCanvasSlot->GetSize() : FVector2D::ZeroVector;
    const bool bDefaultWidthOk = ExpectedDefaultWidth <= 1.0f || FMath::IsNearlyEqual(RootCanvasSize.X, ExpectedDefaultWidth, 6.0f);
    const bool bDefaultHeightOk = ExpectedDefaultHeight <= 1.0f || FMath::IsNearlyEqual(RootCanvasSize.Y, ExpectedDefaultHeight, 6.0f);
    const bool bDefaultXOk = ExpectedDefaultWidth <= 1.0f || FMath::IsNearlyEqual(RootCanvasPosition.X, ExpectedDefaultPosition.X, 10.0f);
    const bool bDefaultYOk = ExpectedDefaultHeight <= 1.0f || FMath::IsNearlyEqual(RootCanvasPosition.Y, ExpectedDefaultPosition.Y, 10.0f);
    const bool bDefaultNotTopLeft = RootCanvasPosition.X > 180.0f && RootCanvasPosition.Y <= 20.0f;

    bool bDragDown = false;
    bool bDragMove = false;
    bool bDragUp = false;
    if (bHasGeometry)
    {
        const FVector2D PanelAbsolutePos = PanelGeometry.GetAbsolutePosition();
        const float TitleStripHeight = FMath::Clamp(PanelSize.Y * 0.085f, 36.0f, 64.0f);
        FVector2D DragStart = PanelAbsolutePos + FVector2D(PanelSize.X * 0.5f, TitleStripHeight * 0.5f);
        if (UWidget* TitleBar = PanelTitleBarWidget.Get())
        {
            TitleBar->ForceLayoutPrepass();
            const FGeometry TitleGeometry = TitleBar->GetCachedGeometry();
            const FVector2D TitleSize = TitleGeometry.GetLocalSize();
            if (TitleSize.X > 1.0f && TitleSize.Y > 1.0f)
            {
                DragStart = TitleGeometry.GetAbsolutePosition() + FVector2D(TitleSize.X * 0.5f, FMath::Clamp(TitleSize.Y * 0.5f, 6.0f, TitleSize.Y - 4.0f));
            }
        }
        const FVector2D DragEnd = DragStart + FVector2D(-120.0f, 48.0f);
        bDragDown = HandlePanelPointerDownAt(DragStart);
        bDragMove = HandlePanelPointerMoveTo(DragEnd);
        bDragUp = HandlePanelPointerUp();
    }
    else
    {
        bDraggingPanel = true;
        bResizingPanelVertically = false;
        PanelInteractionStartCursor = FVector2D::ZeroVector;
        PanelInteractionStartTranslation = OriginalTranslation;
        bDragDown = true;
        bDragMove = HandlePanelPointerMoveTo(FVector2D(-120.0f, 48.0f));
        bDragUp = HandlePanelPointerUp();
    }

    const FVector2D DraggedTranslation = PanelTranslation;
    const bool bDragMoved = FVector2D::Distance(DraggedTranslation, OriginalTranslation) > 20.0f;

    const FGeometry ResizedPanelGeometry = RefreshPanelGeometry();
    const FVector2D ResizedPanelSize = ResizedPanelGeometry.GetLocalSize();
    const float HeightBeforeResize = PanelHeight > 1.0f ? PanelHeight : PanelDefaultHeight;
    bool bResizeDown = false;
    bool bResizeMove = false;
    bool bResizeUp = false;
    if (ResizedPanelSize.X > 1.0f && ResizedPanelSize.Y > 1.0f)
    {
        const FVector2D ResizeStart = ResizedPanelGeometry.GetAbsolutePosition() + FVector2D(ResizedPanelSize.X * 0.5f, ResizedPanelSize.Y - 4.0f);
        const FVector2D ResizeEnd = ResizeStart + FVector2D(0.0f, 96.0f);
        bResizeDown = HandlePanelPointerDownAt(ResizeStart);
        bResizeMove = HandlePanelPointerMoveTo(ResizeEnd);
        bResizeUp = HandlePanelPointerUp();
    }
    else
    {
        bDraggingPanel = false;
        bResizingPanelVertically = true;
        PanelInteractionStartCursor = FVector2D::ZeroVector;
        PanelInteractionStartHeight = HeightBeforeResize;
        bResizeDown = true;
        bResizeMove = HandlePanelPointerMoveTo(FVector2D(0.0f, 96.0f));
        bResizeUp = HandlePanelPointerUp();
    }
    const float HeightAfterResize = PanelHeight;
    const bool bResizeChanged = HeightAfterResize > HeightBeforeResize + 30.0f;

    const bool bPassed = bDefaultWidthOk && bDefaultHeightOk && bDefaultXOk && bDefaultYOk && bDefaultNotTopLeft
        && bDragDown && bDragMove && bDragUp && bDragMoved
        && bResizeDown && bResizeMove && bResizeUp && bResizeChanged;

    OutReport = FString::Printf(
        TEXT("PanelInteractionSelfTest=%s | Mode=%s DefaultSize=%d/%d Current=%.1fx%.1f Expected=%.1fx%.1f DefaultPos=%d/%d NotTopLeft=%d Pos=(%.1f,%.1f) ExpectedPos=(%.1f,%.1f) Drag=%d/%d/%d Delta=(%.1f,%.1f) Resize=%d/%d/%d Height=%.1f->%.1f"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        InteractionMode,
        bDefaultWidthOk ? 1 : 0,
        bDefaultHeightOk ? 1 : 0,
        RootCanvasSize.X,
        RootCanvasSize.Y,
        ExpectedDefaultWidth,
        ExpectedDefaultHeight,
        bDefaultXOk ? 1 : 0,
        bDefaultYOk ? 1 : 0,
        bDefaultNotTopLeft ? 1 : 0,
        RootCanvasPosition.X,
        RootCanvasPosition.Y,
        ExpectedDefaultPosition.X,
        ExpectedDefaultPosition.Y,
        bDragDown ? 1 : 0,
        bDragMove ? 1 : 0,
        bDragUp ? 1 : 0,
        DraggedTranslation.X - OriginalTranslation.X,
        DraggedTranslation.Y - OriginalTranslation.Y,
        bResizeDown ? 1 : 0,
        bResizeMove ? 1 : 0,
        bResizeUp ? 1 : 0,
        HeightBeforeResize,
        HeightAfterResize);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunPanelInteractionSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunPanelInteractionSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunActorPageStructureSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("ActorPageStructureSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("ActorPageStructureSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    FString Summary;
    FString Error;
    if (!ApplyFabScreenshotFoundationState(Summary, Error))
    {
        OutReport = FString::Printf(TEXT("ActorPageStructureSelfTest=FAIL | Apply=%s"), *Error);
        return false;
    }

    if (!SetVisiblePageByName(TEXT("Actor"), Error))
    {
        OutReport = FString::Printf(TEXT("ActorPageStructureSelfTest=FAIL | ShowPage=%s"), *Error);
        return false;
    }

    if (bDeferredOpenActorRefreshScheduled)
    {
        HandleDeferredOpenActorRefreshTimerElapsed();
    }

    SetContentSwitcherIndex(0);
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
    if (UUserWidget* Panel = PanelWidget.Get())
    {
        for (int32 Attempt = 0; Attempt < 3; ++Attempt)
        {
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().Tick(ESlateTickType::All);
            }

            Panel->TakeWidget();
            Panel->ForceLayoutPrepass();
            if (TSharedPtr<SWidget> CachedWidget = Panel->GetCachedWidget())
            {
                CachedWidget->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
            }
            Panel->ForceLayoutPrepass();
        }
    }

    const int32 GroupCount = ActorGroupsEntriesBoxStrong ? ActorGroupsEntriesBoxStrong->GetChildrenCount() : 0;
    UVerticalBox* PageStackHost = ActorWorkbenchPageStackHost.Get();
    UVerticalBox* ContentHost = ActorWorkbenchContentHost.Get();
    const bool bSidebarHostOk = ActorWorkbenchSidebarHost.IsValid() && ActorGroupsSectionHostBox.IsValid();
    const bool bWorkspaceHostOk = ContentHost && ActorPropertyFunctionHostBox.IsValid() && ActorWorkspaceSelectionBand.IsValid() && PageStackHost;
    const bool bSidebarFillOk = ActorGroupsSectionHostBox.IsValid() && RI_IsVerticalSlotRule(ActorGroupsSectionHostBox.Get(), ESlateSizeRule::Fill);
    const bool bWorkspaceFillOk = ActorPropertyFunctionHostBox.IsValid() && RI_IsVerticalSlotRule(ActorPropertyFunctionHostBox.Get(), ESlateSizeRule::Fill);
    const bool bSelectionBandOk = ActorWorkspaceSelectionBand.IsValid() && RI_IsVerticalSlotRule(ActorWorkspaceSelectionBand.Get(), ESlateSizeRule::Automatic);
    const int32 SelectionBandIndex = PageStackHost ? RI_GetPanelChildIndex(PageStackHost, ActorWorkspaceSelectionBand.Get()) : INDEX_NONE;
    const int32 WorkspaceHostIndex = PageStackHost ? RI_GetPanelChildIndex(PageStackHost, ActorWorkbenchBodyHost.Get()) : INDEX_NONE;
    const bool bSelectionBandOrderOk = SelectionBandIndex != INDEX_NONE && WorkspaceHostIndex != INDEX_NONE && SelectionBandIndex < WorkspaceHostIndex;
    const bool bPropertyBoxVisible = ActorPropertiesSectionWidget.IsValid()
        && ActorPropertiesSectionWidget->GetVisibility() == ESlateVisibility::Visible;
    const bool bFunctionBoxVisible = ActorFunctionsSectionWidget.IsValid()
        && ActorFunctionsSectionWidget->GetVisibility() == ESlateVisibility::Visible;
    const bool bPropertyScrollOk = ActorPropertiesSectionWidget.IsValid() && ActorPropertiesSectionWidget->HasPropertyScrollRoot();
    const bool bFunctionScrollOk = ActorFunctionsSectionWidget.IsValid() && ActorFunctionsSectionWidget->HasFunctionScrollRoot();
    const bool bFunctionSummaryOk = ActorFunctionsSectionWidget.IsValid() && !ActorFunctionsSectionWidget->HasFocusSummary();
    UWidget* FooterWidget = PanelWidget.IsValid() && PanelWidget->WidgetTree
        ? PanelWidget->WidgetTree->FindWidget(TEXT("Modified"))
        : nullptr;
    const bool bFooterOk = FooterWidget
        && FooterWidget->GetVisibility() != ESlateVisibility::Collapsed
        && FooterWidget->GetVisibility() != ESlateVisibility::Hidden
        && RI_IsVerticalSlotRule(FooterWidget, ESlateSizeRule::Automatic);
    const bool bLegacySelectionHeaderHidden = PanelWidget.IsValid() && PanelWidget->WidgetTree
        ? ([](UWidget* Widget)
        {
            return !Widget || Widget->GetVisibility() == ESlateVisibility::Collapsed || Widget->GetVisibility() == ESlateVisibility::Hidden;
        }(PanelWidget->WidgetTree->FindWidget(TEXT("HorizontalBox_379"))))
        : true;
    int32 VisibleLegacySiblingCount = 0;
    if (PageStackHost)
    {
        for (int32 ChildIndex = 0; ChildIndex < PageStackHost->GetChildrenCount(); ++ChildIndex)
        {
            UWidget* Child = PageStackHost->GetChildAt(ChildIndex);
            if (!Child || Child == ActorWorkspaceSelectionBand.Get() || Child == ActorWorkbenchBodyHost.Get() || Child == FooterWidget)
            {
                continue;
            }

            if (Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
            }
        }
    }
    if (ContentHost)
    {
        for (int32 ChildIndex = 0; ChildIndex < ContentHost->GetChildrenCount(); ++ChildIndex)
        {
            UWidget* Child = ContentHost->GetChildAt(ChildIndex);
            if (!Child || Child == ActorPropertyFunctionHostBox.Get())
            {
                continue;
            }

            if (Child->GetVisibility() != ESlateVisibility::Collapsed && Child->GetVisibility() != ESlateVisibility::Hidden)
            {
                ++VisibleLegacySiblingCount;
            }
        }
    }

    const float LeftColumnWidth = ActorGroupsSectionHostBox.IsValid() ? ActorGroupsSectionHostBox->GetCachedGeometry().GetLocalSize().X : 0.0f;
    const float RightColumnWidth = ActorPropertyFunctionHostBox.IsValid() ? ActorPropertyFunctionHostBox->GetCachedGeometry().GetLocalSize().X : 0.0f;
    const float TotalWidth = LeftColumnWidth + RightColumnWidth;
    const float LeftRatio = TotalWidth > 1.0f ? LeftColumnWidth / TotalWidth : 0.0f;
    const float RightRatio = TotalWidth > 1.0f ? RightColumnWidth / TotalWidth : 0.0f;
    const bool bColumnRatioOk = TotalWidth > 1.0f
        && RI_IsRatioNear(LeftRatio, 0.34f, 0.10f)
        && RI_IsRatioNear(RightRatio, 0.66f, 0.10f);
    const float PropertyHeight = ActorPropertiesSectionWidget.IsValid() ? ActorPropertiesSectionWidget->GetCachedGeometry().GetLocalSize().Y : 0.0f;
    const float FunctionHeight = ActorFunctionsSectionWidget.IsValid() ? ActorFunctionsSectionWidget->GetCachedGeometry().GetLocalSize().Y : 0.0f;
    const float TotalRightHeight = PropertyHeight + FunctionHeight;
    const float PropertyRatio = TotalRightHeight > 1.0f ? PropertyHeight / TotalRightHeight : 0.0f;
    const float FunctionRatio = TotalRightHeight > 1.0f ? FunctionHeight / TotalRightHeight : 0.0f;
    const bool bVerticalRatioOk = TotalRightHeight > 1.0f
        && RI_IsRatioNear(PropertyRatio, 0.44f, 0.12f)
        && RI_IsRatioNear(FunctionRatio, 0.56f, 0.12f);
    const bool bFunctionDominantOk = FunctionHeight > PropertyHeight;

    TArray<UObject*> ActorItems;
    GetPropertyItemsForSelectedEx(TEXT(""), false, ActorItems);

    UInspectorPropertyItem* FavoriteCandidate = nullptr;
    for (UObject* ItemObject : ActorItems)
    {
        if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
        {
            FavoriteCandidate = PropertyItem;
            break;
        }
    }

    int32 PinnedCount = 0;
    bool bStarReady = false;
    if (FavoriteCandidate)
    {
        if (!IsFavoriteForItem(FavoriteCandidate))
        {
            ToggleFavoriteForItem(FavoriteCandidate);
        }

        TArray<UObject*> PinnedItems;
        GetPinnedItemsForSelected(TEXT(""), PinnedItems);
        PinnedCount = PinnedItems.Num();
        bStarReady = PinnedCount > 0;
    }

    AActor* ActorPtr = SelectedActor.Get();
    bool bFocusedComponentOk = false;
    bool bColorItemFound = false;
    bool bTextInputItemFound = false;
    bool bBoolItemFound = false;
    bool bSwatchVisible = false;
    bool bMaterialScalarRowOk = false;
    bool bMaterialVectorRowOk = false;
    bool bMaterialFavoriteVisible = false;
    bool bMaterialTreeFound = false;
    bool bMaterialSingleClickExpandOk = false;
    bool bMaterialTreeExpandedOk = false;
    bool bMaterialSlotVisible = false;
    bool bMaterialSlotSelectionOk = false;
    FString MaterialTreeVisibleKeys = TEXT("None");
    FString FocusedComponentName = TEXT("None");
    FString ColorPropertyName = TEXT("None");
    FString TextInputPropertyName = TEXT("None");
    FString BoolPropertyName = TEXT("None");
    FString MaterialScalarName = TEXT("None");
    FString MaterialVectorName = TEXT("None");
    FString MaterialTreeComponentName = TEXT("None");
    FString MaterialSlotLabel = TEXT("None");
    float PropertyTextInputHeight = 0.f;
    float PropertyBoolHeight = 0.f;
    float PropertyColorHeight = 0.f;
    float PropertyFavoriteHeight = 0.f;
    float MaterialScalarHeight = 0.f;
    float MaterialVectorHeight = 0.f;
    float MaterialFavoriteHeight = 0.f;
    const float ExpectedValueControlHeight = RICompactUI::GetInputHeight();
    const float ExpectedFavoriteControlHeight = FMath::Max(18.f, ExpectedValueControlHeight - 4.f);
    const float HeightTolerance = 0.25f;
    bool bValueHeightContractOk = false;
    bool bTouchHeightContractOk = false;
    bool bSearchBindingOk = false;
    bool bSearchFilterOk = false;
    int32 UnfilteredSearchEntryCount = 0;
    int32 FilteredSearchEntryCount = 0;

    if (ActorPtr)
    {
        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                continue;
            }

            FString FocusError;
            if (!FocusSelectedActorComponentByName(Component->GetName(), FocusError))
            {
                continue;
            }

            bFocusedComponentOk = GetFocusedInspectObject() == Component;

            TArray<UObject*> ComponentItems;
            GetPropertyItemsForSelectedEx(TEXT(""), false, ComponentItems);
            for (UObject* ItemObject : ComponentItems)
            {
                UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject);
                if (!PropertyItem)
                {
                    continue;
                }

                const EInspectorValueType ValueType = PropertyItem->GetValueType();
                UInspectorPropertyRowWidget* Row = nullptr;
                if (APlayerController* PC = GetLocalPC())
                {
                    Row = CreateWidget<UInspectorPropertyRowWidget>(PC, UInspectorPropertyRowWidget::StaticClass());
                }
                else if (UWorld* World = GetWorld())
                {
                    Row = CreateWidget<UInspectorPropertyRowWidget>(World, UInspectorPropertyRowWidget::StaticClass());
                }

                if (!Row)
                {
                    continue;
                }

                Row->TakeWidget();
                Row->SetInspectorSubsystem(this);
                Row->SetPropertyItem(PropertyItem);

                if (!bColorItemFound && (ValueType == EInspectorValueType::LinearColor || ValueType == EInspectorValueType::Color))
                {
                    FocusedComponentName = Component->GetName();
                    ColorPropertyName = PropertyItem->GetPropertyName();
                    bColorItemFound = true;
                    bSwatchVisible = Row->IsColorSwatchVisibleForAutomation()
                        && !Row->IsReadOnlyValueVisibleForAutomation()
                        && !Row->IsValueTextBoxVisibleForAutomation();
                    PropertyColorHeight = Row->GetColorButtonHeightForAutomation();
                    PropertyFavoriteHeight = Row->GetFavoriteButtonHeightForAutomation();
                }
                else if (!bBoolItemFound && PropertyItem->IsEditable() && ValueType == EInspectorValueType::Bool)
                {
                    BoolPropertyName = PropertyItem->GetPropertyName();
                    bBoolItemFound = true;
                    PropertyBoolHeight = Row->GetValueControlHeightForAutomation();
                }
                else if (!bTextInputItemFound
                    && PropertyItem->IsEditable()
                    && ValueType != EInspectorValueType::Bool
                    && ValueType != EInspectorValueType::Enum
                    && ValueType != EInspectorValueType::LinearColor
                    && ValueType != EInspectorValueType::Color)
                {
                    TextInputPropertyName = PropertyItem->GetPropertyName();
                    bTextInputItemFound = Row->IsValueTextBoxVisibleForAutomation();
                    PropertyTextInputHeight = Row->GetValueControlHeightForAutomation();
                }
            }

            if (bColorItemFound && bTextInputItemFound && bBoolItemFound)
            {
                break;
            }
        }

        for (UActorComponent* Component : Components)
        {
            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
            if (!MeshComponent || MeshComponent->GetNumMaterials() <= 0)
            {
                continue;
            }

            UMaterialInterface* Material = MeshComponent->GetMaterial(0);
            if (!Material)
            {
                continue;
            }

            TArray<FMaterialParameterInfo> ScalarInfos;
            TArray<FMaterialParameterInfo> VectorInfos;
            TArray<FGuid> ParameterIds;

            Material->GetAllScalarParameterInfo(ScalarInfos, ParameterIds);
            ParameterIds.Reset();
            Material->GetAllVectorParameterInfo(VectorInfos, ParameterIds);

            if (ScalarInfos.Num() == 0 || VectorInfos.Num() == 0)
            {
                continue;
            }

            SetPropertyView_MaterialOnly(MeshComponent, 0);
            TArray<UObject*> MaterialItems;
            GetPropertyItemsForSelectedEx(TEXT(""), false, MaterialItems);

            UInspectorMaterialParamItem* ScalarItem = nullptr;
            UInspectorMaterialParamItem* VectorItem = nullptr;
            for (UObject* ItemObject : MaterialItems)
            {
                UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject);
                if (!MaterialItem)
                {
                    continue;
                }

                if (!ScalarItem && MaterialItem->GetParamType() == EInspectorMatParamType::Scalar)
                {
                    ScalarItem = MaterialItem;
                }
                else if (!VectorItem && MaterialItem->GetParamType() == EInspectorMatParamType::Vector)
                {
                    VectorItem = MaterialItem;
                }
            }

            UInspectorMaterialParamRowWidget* ScalarRow = nullptr;
            UInspectorMaterialParamRowWidget* VectorRow = nullptr;
            if (APlayerController* PC = GetLocalPC())
            {
                if (ScalarItem)
                {
                    ScalarRow = CreateWidget<UInspectorMaterialParamRowWidget>(PC, UInspectorMaterialParamRowWidget::StaticClass());
                }
                if (VectorItem)
                {
                    VectorRow = CreateWidget<UInspectorMaterialParamRowWidget>(PC, UInspectorMaterialParamRowWidget::StaticClass());
                }
            }
            else if (UWorld* World = GetWorld())
            {
                if (ScalarItem)
                {
                    ScalarRow = CreateWidget<UInspectorMaterialParamRowWidget>(World, UInspectorMaterialParamRowWidget::StaticClass());
                }
                if (VectorItem)
                {
                    VectorRow = CreateWidget<UInspectorMaterialParamRowWidget>(World, UInspectorMaterialParamRowWidget::StaticClass());
                }
            }

            if (ScalarRow && ScalarItem)
            {
                ScalarRow->TakeWidget();
                ScalarRow->SetInspectorSubsystem(this);
                ScalarRow->SetMaterialItem(ScalarItem);
                MaterialScalarName = ScalarItem->GetPropertyName();
                bMaterialScalarRowOk = ScalarRow->IsScalarTextBoxVisibleForAutomation()
                    && !ScalarRow->IsColorSwatchVisibleForAutomation()
                    && ScalarRow->HasFavoriteButtonForAutomation();
                bMaterialFavoriteVisible |= ScalarRow->HasFavoriteButtonForAutomation();
                MaterialScalarHeight = ScalarRow->GetValueControlHeightForAutomation();
                MaterialFavoriteHeight = ScalarRow->GetFavoriteButtonHeightForAutomation();
            }

            if (VectorRow && VectorItem)
            {
                VectorRow->TakeWidget();
                VectorRow->SetInspectorSubsystem(this);
                VectorRow->SetMaterialItem(VectorItem);
                MaterialVectorName = VectorItem->GetPropertyName();
                bMaterialVectorRowOk = !VectorRow->IsScalarValueVisibleForAutomation()
                    && VectorRow->IsColorSwatchVisibleForAutomation()
                    && VectorRow->HasFavoriteButtonForAutomation();
                bMaterialFavoriteVisible |= VectorRow->HasFavoriteButtonForAutomation();
                MaterialVectorHeight = VectorRow->GetColorButtonHeightForAutomation();
                MaterialFavoriteHeight = FMath::Max(MaterialFavoriteHeight, VectorRow->GetFavoriteButtonHeightForAutomation());
            }

            if (bMaterialScalarRowOk || bMaterialVectorRowOk)
            {
                break;
            }
        }

        for (UActorComponent* Component : Components)
        {
            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
            if (!MeshComponent || MeshComponent->GetNumMaterials() <= 0)
            {
                continue;
            }

            MaterialTreeComponentName = MeshComponent->GetName();
            SetGroupExpanded(TEXT("ROOT_COMPONENTS"), true);

            const FString ComponentKey = MakeComponentKey(ActorPtr, MeshComponent);
            SetGroupExpanded(ComponentKey, false);
            RefreshPanel(EInspectorRefreshReason::StructureChanged);

            TArray<FString> VisibleKeys;
            for (UInspectorGroupButtonProxy* Proxy : ActorGroupsClickProxies)
            {
                const FString StableKey = Proxy ? Proxy->GetStableKey() : FString();
                if (!StableKey.IsEmpty())
                {
                    VisibleKeys.Add(StableKey);
                }

                if (Proxy && StableKey == ComponentKey)
                {
                    Proxy->InvokeForAutomation();
                    bMaterialSingleClickExpandOk = true;
                }
            }

            if (VisibleKeys.Num() > 0)
            {
                MaterialTreeVisibleKeys = FString::Join(VisibleKeys, TEXT("|"));
            }

            UInspectorGroupItem* ComponentGroup = GetOrCreateGroupItem(ComponentKey);
            ComponentGroup->Kind = EInspectorGroupKind::Component;
            ComponentGroup->TargetObject = MeshComponent;
            ComponentGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *MeshComponent->GetName(), *MeshComponent->GetClass()->GetName());
            ComponentGroup->StableKey = ComponentKey;
            ComponentGroup->Depth = 1;
            ComponentGroup->bExpanded = GetGroupExpanded(ComponentKey, false);

            bMaterialSingleClickExpandOk = bMaterialSingleClickExpandOk && ComponentGroup->bExpanded;

            SetGroupExpanded(ComponentKey, true);
            ComponentGroup->bExpanded = true;

            TArray<UObject*> ComponentChildren;
            GetGroupTreeChildrenForItem(ComponentGroup, TEXT(""), ComponentChildren);

            UInspectorGroupItem* MaterialsRootItem = nullptr;
            for (UObject* ChildObject : ComponentChildren)
            {
                UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ChildObject);
                if (GroupItem && GroupItem->IsMaterialsRoot())
                {
                    MaterialsRootItem = GroupItem;
                    break;
                }
            }

            bMaterialTreeFound = MaterialsRootItem != nullptr;
            if (!MaterialsRootItem)
            {
                continue;
            }

            SetGroupExpanded(MaterialsRootItem->StableKey, true);
            RefreshPanel(EInspectorRefreshReason::StructureChanged);

            ComponentChildren.Reset();
            GetGroupTreeChildrenForItem(ComponentGroup, TEXT(""), ComponentChildren);
            for (UObject* ChildObject : ComponentChildren)
            {
                UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ChildObject);
                if (GroupItem && GroupItem->StableKey == MaterialsRootItem->StableKey)
                {
                    MaterialsRootItem = GroupItem;
                    break;
                }
            }

            bMaterialTreeExpandedOk = MaterialsRootItem && MaterialsRootItem->bExpanded;
            if (!MaterialsRootItem || !MaterialsRootItem->bExpanded)
            {
                continue;
            }

            TArray<UObject*> MaterialSlotChildren;
            GetGroupTreeChildrenForItem(MaterialsRootItem, TEXT(""), MaterialSlotChildren);
            UInspectorGroupItem* MaterialSlotItem = nullptr;
            for (UObject* ChildObject : MaterialSlotChildren)
            {
                UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ChildObject);
                if (GroupItem && GroupItem->MaterialSlotIndex != INDEX_NONE)
                {
                    MaterialSlotItem = GroupItem;
                    break;
                }
            }

            bMaterialSlotVisible = MaterialSlotItem != nullptr;
            if (!MaterialSlotItem)
            {
                continue;
            }

            MaterialSlotLabel = MaterialSlotItem->DisplayName;
            SetSelectedGroupItem(MaterialSlotItem);
            RequestActorPageRefresh();

            TArray<UObject*> MaterialOnlyItems;
            GetPropertyItemsForSelectedEx(TEXT(""), false, MaterialOnlyItems);
            const bool bHasMaterialParams = MaterialOnlyItems.ContainsByPredicate([](UObject* ItemObject)
            {
                return Cast<UInspectorMaterialParamItem>(ItemObject) != nullptr;
            });

            bMaterialSlotSelectionOk =
                PropertyViewMode == ERIPropertyViewMode::MaterialOnly &&
                ViewMeshComp.Get() == MeshComponent &&
                ViewMaterialSlot == MaterialSlotItem->MaterialSlotIndex &&
                bHasMaterialParams;
            break;
        }
    }

    SelectedInspectObject = SelectedActor.Get();
    SelectedGroupKey = TEXT("ROOT_ACTOR");
    PropertyViewMode = ERIPropertyViewMode::Full;
    ViewMeshComp = nullptr;
    ViewMaterialSlot = INDEX_NONE;
    RefreshPanel(EInspectorRefreshReason::StructureChanged);

    UnfilteredSearchEntryCount = ActorGroupsEntriesBoxStrong
        ? ActorGroupsEntriesBoxStrong->GetChildrenCount()
        : 0;

    FString SearchPageError;
    const bool bSearchPageReady = SetVisiblePageByName(TEXT("Actor"), SearchPageError);
    if (bSearchPageReady && PanelWidget.IsValid() && PanelWidget->WidgetTree)
    {
        if (UWidget* SearchWidget = PanelWidget->WidgetTree->FindWidget(TEXT("ETB_Search")))
        {
            RI_TrySetEditableSearchText(SearchWidget, FText::FromString(TEXT("Arrow")));
            CacheActorPageSearchTextFromPanel();
            RefreshActorGroupsSection();
            RefreshActorPropertiesSection();
            RefreshActorFunctionsSection();
            bSearchBindingOk = CurrentActorSearchText.Equals(TEXT("Arrow"), ESearchCase::IgnoreCase);
            FilteredSearchEntryCount = ActorGroupsEntriesBoxStrong
                ? ActorGroupsEntriesBoxStrong->GetChildrenCount()
                : 0;
            bSearchFilterOk = bSearchBindingOk
                && FilteredSearchEntryCount > 0
                && FilteredSearchEntryCount < UnfilteredSearchEntryCount;

            RI_TrySetEditableSearchText(SearchWidget, FText::GetEmpty());
            CacheActorPageSearchTextFromPanel();
            RefreshActorGroupsSection();
            RefreshActorPropertiesSection();
            RefreshActorFunctionsSection();
        }
    }

    bValueHeightContractOk =
        bTextInputItemFound
        && bBoolItemFound
        && bColorItemFound
        && bMaterialScalarRowOk
        && bMaterialVectorRowOk
        && FMath::Abs(PropertyTextInputHeight - ExpectedValueControlHeight) <= HeightTolerance
        && FMath::Abs(PropertyBoolHeight - ExpectedValueControlHeight) <= HeightTolerance
        && FMath::Abs(PropertyColorHeight - ExpectedValueControlHeight) <= HeightTolerance
        && FMath::Abs(MaterialScalarHeight - ExpectedValueControlHeight) <= HeightTolerance
        && FMath::Abs(MaterialVectorHeight - ExpectedValueControlHeight) <= HeightTolerance;

    bTouchHeightContractOk =
        PropertyFavoriteHeight > 0.f
        && MaterialFavoriteHeight > 0.f
        && PropertyColorHeight > 0.f
        && MaterialVectorHeight > 0.f
        && FMath::Abs(PropertyFavoriteHeight - ExpectedFavoriteControlHeight) <= HeightTolerance
        && FMath::Abs(MaterialFavoriteHeight - ExpectedFavoriteControlHeight) <= HeightTolerance
        && FMath::Abs(PropertyColorHeight - ExpectedValueControlHeight) <= HeightTolerance
        && FMath::Abs(MaterialVectorHeight - ExpectedValueControlHeight) <= HeightTolerance;

    const bool bPassed = GroupCount > 0
        && bSidebarHostOk
        && bWorkspaceHostOk
        && bSidebarFillOk
        && bWorkspaceFillOk
        && bSelectionBandOk
        && bSelectionBandOrderOk
        && bPropertyBoxVisible
        && bFunctionBoxVisible
        && bPropertyScrollOk
        && bFunctionScrollOk
        && bFunctionSummaryOk
        && bFooterOk
        && bLegacySelectionHeaderHidden
        && VisibleLegacySiblingCount == 0
        && bColumnRatioOk
        && bVerticalRatioOk
        && bFunctionDominantOk
        && bStarReady
        && bFocusedComponentOk
        && bColorItemFound
        && bMaterialScalarRowOk
        && bMaterialVectorRowOk
        && bMaterialFavoriteVisible
        && bSwatchVisible
        && bValueHeightContractOk
        && bTouchHeightContractOk
        && bSearchBindingOk
        && bSearchFilterOk
        && bMaterialSingleClickExpandOk
        && bMaterialTreeFound
        && bMaterialTreeExpandedOk
        && bMaterialSlotVisible
        && bMaterialSlotSelectionOk;

    OutReport = FString::Printf(
        TEXT("ActorPageStructureSelfTest=%s | Groups=%d | Sidebar=%d/%d Workspace=%d/%d Selection=%d/%d Footer=%d LegacyHeader=%d VisibleLegacy=%d | PropertyBox=%d Scroll=%d | FunctionBox=%d Scroll=%d SummaryHidden=%d | Columns=%d Left=%.2f Right=%.2f | Vertical=%d Property=%.2f Function=%.2f Dominant=%d | Starred=%d | FocusedComponent=%s | FocusOk=%d | ColorProperty=%s | ColorItem=%d | Swatch=%d | ValueHeights=%d Text=%s:%.1f Bool=%s:%.1f Color=%.1f MaterialScalar=%d(%s:%.1f) MaterialVector=%d(%s:%.1f) Touch=%d Favorite=%.1f/%.1f | Search=%d/%d Entries=%d->%d | MaterialStar=%d | MaterialTree=%d/%d/%d/%d/%d Component=%s Slot=%s Keys=%s | Summary=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        GroupCount,
        bSidebarHostOk ? 1 : 0,
        bSidebarFillOk ? 1 : 0,
        bWorkspaceHostOk ? 1 : 0,
        bWorkspaceFillOk ? 1 : 0,
        bSelectionBandOk ? 1 : 0,
        bSelectionBandOrderOk ? 1 : 0,
        bFooterOk ? 1 : 0,
        bLegacySelectionHeaderHidden ? 1 : 0,
        VisibleLegacySiblingCount,
        bPropertyBoxVisible ? 1 : 0,
        bPropertyScrollOk ? 1 : 0,
        bFunctionBoxVisible ? 1 : 0,
        bFunctionScrollOk ? 1 : 0,
        bFunctionSummaryOk ? 1 : 0,
        bColumnRatioOk ? 1 : 0,
        LeftRatio,
        RightRatio,
        bVerticalRatioOk ? 1 : 0,
        PropertyRatio,
        FunctionRatio,
        bFunctionDominantOk ? 1 : 0,
        PinnedCount,
        *FocusedComponentName,
        bFocusedComponentOk ? 1 : 0,
        *ColorPropertyName,
        bColorItemFound ? 1 : 0,
        bSwatchVisible ? 1 : 0,
        bValueHeightContractOk ? 1 : 0,
        *TextInputPropertyName,
        PropertyTextInputHeight,
        *BoolPropertyName,
        PropertyBoolHeight,
        PropertyColorHeight,
        bMaterialScalarRowOk ? 1 : 0,
        *MaterialScalarName,
        MaterialScalarHeight,
        bMaterialVectorRowOk ? 1 : 0,
        *MaterialVectorName,
        MaterialVectorHeight,
        bTouchHeightContractOk ? 1 : 0,
        PropertyFavoriteHeight,
        MaterialFavoriteHeight,
        bSearchBindingOk ? 1 : 0,
        bSearchFilterOk ? 1 : 0,
        UnfilteredSearchEntryCount,
        FilteredSearchEntryCount,
        bMaterialFavoriteVisible ? 1 : 0,
        bMaterialSingleClickExpandOk ? 1 : 0,
        bMaterialTreeFound ? 1 : 0,
        bMaterialTreeExpandedOk ? 1 : 0,
        bMaterialSlotVisible ? 1 : 0,
        bMaterialSlotSelectionOk ? 1 : 0,
        *MaterialTreeComponentName,
        *MaterialSlotLabel,
        *MaterialTreeVisibleKeys,
        *Summary);
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunRemoteSessionContextUISelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemoteSessionContextUISelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RemoteSessionContextUISelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    const FString PreviousPreferredSessionId = PreferredRemoteSessionId;
    const FString PreviousSelectionSummary = LastRemoteSessionSelectionSummary;
    const FString PreviousTargetQuery = LastRemoteSessionTargetQuery;
    const FString PreviousWorkflowId = LastRemoteSessionWorkflowId;

    auto RestoreState = [&]()
    {
        SetRemoteSessionUIContext(PreviousPreferredSessionId, PreviousSelectionSummary, PreviousTargetQuery, PreviousWorkflowId);
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    FRIRuntimeSessionInfo EnsuredSession;
    FString EnsureError;
    if (!EnsurePackagedRuntimeValidationSession(EnsuredSession, EnsureError))
    {
        OutReport = FString::Printf(TEXT("RemoteSessionContextUISelfTest=FAIL | PackagedLaunch=%s"), *EnsureError);
        return false;
    }

    FRIRuntimeSessionInfo ConnectedSession;
    FString ConnectError;
    const bool bConnectOk = ConnectRemoteRuntimeSession(EnsuredSession.SessionId, ConnectedSession, ConnectError);
    if (!bConnectOk)
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("RemoteSessionContextUISelfTest=FAIL | Connect=%s"), *ConnectError);
        return false;
    }

    const FString ExpectedSessionId = ConnectedSession.SessionId.IsEmpty() ? EnsuredSession.SessionId : ConnectedSession.SessionId;
    const FString ExpectedSelectionSummary = ConnectedSession.DisplayName.IsEmpty() ? ExpectedSessionId : ConnectedSession.DisplayName;
    const FString ExpectedTargetQuery = TEXT("BP_TestVarsActor");
    const FString ExpectedWorkflowId = RI_WorkflowId_MainlineRemotePackagedToSourceClosure.ToString();
    SetRemoteSessionUIContext(ExpectedSessionId, ExpectedSelectionSummary, ExpectedTargetQuery, ExpectedWorkflowId);

    Close();
    Open();

    EnsureFilePageInjected();
    ShowFilePage();
    UInspectorFilePageWidget* FilePage = FilePageWidget.Get();
    if (FilePage)
    {
        FilePage->TakeWidget();
        FilePage->RefreshFromSubsystem();
    }

    EnsureTestPageInjected();
    ShowTestPage();
    UInspectorTestPageWidget* TestPage = TestPageWidget.Get();
    if (TestPage)
    {
        TestPage->TakeWidget();
        TestPage->RefreshFromSubsystem();
    }

    FRIFileManagementSummary Summary;
    FString SummaryError;
    const bool bSummaryOk = GetFileManagementSummary(Summary, SummaryError);

    const FString TestSessionLabel = TestPage ? TestPage->GetSelectedRemoteSessionLabel() : FString();
    const FString TestTargetQuery = TestPage ? TestPage->GetRemoteSessionTargetQueryValue().TrimStartAndEnd() : FString();
    const FString TestWorkflowId = TestPage ? TestPage->GetRemoteSessionWorkflowValue().TrimStartAndEnd() : FString();

    const bool bFileRemoteHidden = FilePage && !FilePage->HasRemoteSessionSection();

    const bool bTestSessionOk = TestPage
        && (TestSessionLabel.Contains(ExpectedSessionId) || TestSessionLabel.Contains(ExpectedSelectionSummary));
    const bool bTestQueryOk = TestPage && TestTargetQuery.Equals(ExpectedTargetQuery, ESearchCase::CaseSensitive);
    const bool bTestWorkflowOk = TestPage && TestWorkflowId.Equals(ExpectedWorkflowId, ESearchCase::CaseSensitive);

    const bool bSummarySelectionOk = bSummaryOk
        && (Summary.LastRemoteSessionSelectionSummary.Contains(ExpectedSessionId) || Summary.LastRemoteSessionSelectionSummary.Contains(ExpectedSelectionSummary));
    const bool bSummaryQueryOk = bSummaryOk && Summary.LastRemoteSessionTargetQuery.Equals(ExpectedTargetQuery, ESearchCase::CaseSensitive);
    const bool bSummaryWorkflowOk = bSummaryOk && Summary.LastRemoteSessionWorkflowId.Equals(ExpectedWorkflowId, ESearchCase::CaseSensitive);

    const bool bPassed = bFileRemoteHidden
        && bTestSessionOk
        && bTestQueryOk
        && bTestWorkflowOk
        && bSummarySelectionOk
        && bSummaryQueryOk
        && bSummaryWorkflowOk;

    OutReport = FString::Printf(
        TEXT("RemoteSessionContextUISelfTest=%s | Session=%s Changes=%s Test=%s/%s/%s Summary=%s/%s/%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *ExpectedSessionId,
        bFileRemoteHidden ? TEXT("hidden") : TEXT("visible"),
        bTestSessionOk ? TEXT("session") : *TestSessionLabel,
        bTestQueryOk ? TEXT("query") : *TestTargetQuery,
        bTestWorkflowOk ? TEXT("workflow") : *TestWorkflowId,
        bSummarySelectionOk ? TEXT("session") : *(bSummaryOk ? Summary.LastRemoteSessionSelectionSummary : SummaryError),
        bSummaryQueryOk ? TEXT("query") : *(bSummaryOk ? Summary.LastRemoteSessionTargetQuery : SummaryError),
        bSummaryWorkflowOk ? TEXT("workflow") : *(bSummaryOk ? Summary.LastRemoteSessionWorkflowId : SummaryError));

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemoteSessionContextUISelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemoteSessionContextUISelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunWorkflowMatrixSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("WorkflowMatrixSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("WorkflowMatrixSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    FRIWorkflowMatrixRunResult MatrixResult;
    const bool bMatrixOk = RunWorkflowMatrixById(RI_WorkflowMatrixId_Default, MatrixResult);
    const bool bEntryCountOk = MatrixResult.EntryResults.Num() >= 3;
    const bool bHasActorApplyEntry = MatrixResult.EntryResults.ContainsByPredicate([](const FRIWorkflowMatrixEntryRunResult& Entry)
    {
        return Entry.WorkflowId == RI_WorkflowId_MainlineActorApplyFileClosure;
    });
    const bool bHasRemoteRuntimeEntry = MatrixResult.EntryResults.ContainsByPredicate([](const FRIWorkflowMatrixEntryRunResult& Entry)
    {
        return Entry.WorkflowId == RI_WorkflowId_MainlineRemoteRuntimeFoundation;
    });
    const bool bHasRemoteCompareEntry = MatrixResult.EntryResults.ContainsByPredicate([](const FRIWorkflowMatrixEntryRunResult& Entry)
    {
        return Entry.WorkflowId == RI_WorkflowId_MainlineRemoteSessionCompareMatrixFoundation;
    });
    const bool bPassed = bMatrixOk
        && bEntryCountOk
        && bHasActorApplyEntry
        && bHasRemoteRuntimeEntry
        && bHasRemoteCompareEntry
        && MatrixResult.FailedEntryCount == 0;

    OutReport = FString::Printf(
        TEXT("WorkflowMatrixSelfTest=%s | Matrix=%s Entries=%d Passed=%d Failed=%d RemoteRuntime=%s RemoteCompare=%s ActorApply=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *MatrixResult.MatrixId.ToString(),
        MatrixResult.EntryResults.Num(),
        MatrixResult.PassedEntryCount,
        MatrixResult.FailedEntryCount,
        bHasRemoteRuntimeEntry ? TEXT("yes") : TEXT("no"),
        bHasRemoteCompareEntry ? TEXT("yes") : TEXT("no"),
        bHasActorApplyEntry ? TEXT("yes") : TEXT("no"));
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunWorkflowMatrixSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunWorkflowMatrixSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFileWorkflowSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FileWorkflowSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("FileWorkflowSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIApplyResult PreviousLastApplyResult = LastPatchApplyResult;
    const FRIAuditReport PreviousLastAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;
    const FRIImportReport PreviousReport = LastImportReport;
    const FString PreviousImportedSnapshotPath = LastImportedSnapshotPath;

    TSet<FString> ExistingPatchFiles;
    {
        TArray<FString> Files;
        ListPatchBundleFiles(Files);
        ExistingPatchFiles.Append(Files);
    }

    TSet<FString> ExistingAuditFiles;
    {
        TArray<FString> Files;
        ListAuditReportFiles(Files);
        ExistingAuditFiles.Append(Files);
    }

    TSet<FString> ExistingPresetIds;
    {
        TArray<FRIPatchPresetMetadata> Presets;
        FString PresetListError;
        if (ListPatchPresets(Presets, PresetListError))
        {
            for (const FRIPatchPresetMetadata& Preset : Presets)
            {
                ExistingPresetIds.Add(Preset.PresetId);
            }
        }
    }

    AActor* TestActor = PreviousSelectedActor;
    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    FString ExportedSharedReportPath;

    auto TryPrepareActor = [&](AActor* CandidateActor) -> bool
    {
        return RI_SelectWritablePrimitivePropertyForSelfTest(
            CandidateActor,
            TestProperty,
            OriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue);
    };

    if (!TryPrepareActor(TestActor))
    {
        TestActor = nullptr;
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (TryPrepareActor(*It))
                {
                    TestActor = *It;
                    break;
                }
            }
        }
    }

    if (!TestActor)
    {
        OutReport = TEXT("FileWorkflowSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    auto RestoreState = [&]()
    {
        if (TestActor && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);
            }
        }

        TArray<FString> CurrentPatchFiles;
        ListPatchBundleFiles(CurrentPatchFiles);
        for (const FString& File : CurrentPatchFiles)
        {
            if (!ExistingPatchFiles.Contains(File))
            {
                IFileManager::Get().Delete(*File, false, true);
            }
        }

        TArray<FString> CurrentAuditFiles;
        ListAuditReportFiles(CurrentAuditFiles);
        for (const FString& File : CurrentAuditFiles)
        {
            if (!ExistingAuditFiles.Contains(File))
            {
                IFileManager::Get().Delete(*File, false, true);
            }
        }

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        TArray<FRIPatchPresetMetadata> CurrentPresets;
        FString PresetError;
        if (ListPatchPresets(CurrentPresets, PresetError))
        {
            for (const FRIPatchPresetMetadata& Preset : CurrentPresets)
            {
                if (!ExistingPresetIds.Contains(Preset.PresetId))
                {
                    FString DeleteError;
                    DeletePatchPreset(Preset.PresetId, DeleteError);
                }
            }
        }

        if (PreviousSelectedActor != TestActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastPatchApplyResult = PreviousLastApplyResult;
        LastAuditReport = PreviousLastAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
        LastImportReport = PreviousReport;
        LastImportedSnapshotPath = PreviousImportedSnapshotPath;
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    if (PreviousSelectedActor != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    FString Error;
    if (!ApplyPropertyTextNow(TestActor, TestProperty, PatchedText, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("FileWorkflowSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    FString StageSummary;
    FString StageDetails;
    const bool bStageOk = ExecuteFileStagePatchAction(StageSummary, StageDetails);
    const FString StageReportText = GetLastImportReportAsText(true, true);
    const bool bStageReportOk = bStageOk && StageReportText.Contains(StageSummary);

    FString ExportSummary;
    FString ExportDetails;
    const bool bExportOk = ExecuteFileExportPatchAction(ExportSummary, ExportDetails);
    const FString ExportReportText = GetLastImportReportAsText(true, true);
    const bool bExportReportOk = bExportOk && ExportReportText.Contains(ExportSummary);

    FString PresetSummary;
    FString PresetDetails;
    const bool bPresetOk = ExecuteFileSavePresetAction(PresetSummary, PresetDetails);
    const FString PresetReportText = GetLastImportReportAsText(true, true);
    const bool bPresetReportOk = bPresetOk && PresetReportText.Contains(PresetSummary);

    bool bApplyLatestPresetOk = false;
    bool bApplyLatestPresetReportOk = false;
    bool bApplyLatestPresetValueOk = false;
    FString ApplyLatestPresetSummary;
    FString ApplyLatestPresetDetails;
    if (bPresetOk)
    {
        FString RestoreError;
        ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);

        bApplyLatestPresetOk = ExecuteFileApplyLatestPresetAction(ApplyLatestPresetSummary, ApplyLatestPresetDetails);
        const FString ApplyLatestPresetReportText = GetLastImportReportAsText(true, true);
        bApplyLatestPresetReportOk = bApplyLatestPresetOk && ApplyLatestPresetReportText.Contains(ApplyLatestPresetSummary);

        FString AfterApplyLatestPreset;
        InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, AfterApplyLatestPreset);
        bApplyLatestPresetValueOk = (PropertyKind == TEXT("bool"))
            ? AfterApplyLatestPreset.Equals(PatchedText, ESearchCase::IgnoreCase)
            : FMath::IsNearlyEqual(FCString::Atod(*AfterApplyLatestPreset), NumericTargetValue, 0.001);
    }

    FString AuditSummary;
    FString AuditDetails;
    const bool bAuditOk = ExecuteFileBuildPatchVsSourceAuditAction(AuditSummary, AuditDetails);
    const FString AuditReportText = GetLastImportReportAsText(true, true);
    const bool bAuditReportOk = bAuditOk && AuditReportText.Contains(AuditSummary);

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    const bool bPassed = bStageReportOk
        && bExportReportOk
        && bPresetReportOk
        && bApplyLatestPresetReportOk
        && bApplyLatestPresetValueOk
        && bAuditReportOk
        && bClearReportOk
        && bSharedReportExportOk
        && !HasStagedPatch();

    OutReport = FString::Printf(
        TEXT("FileWorkflowSelfTest=%s | Property=%s Stage=%s Export=%s Preset=%s ApplyPreset=%s Audit=%s Clear=%s SharedExport=%s FinalStaged=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        bStageReportOk ? TEXT("ok") : *StageSummary,
        bExportReportOk ? TEXT("ok") : *ExportSummary,
        bPresetReportOk ? TEXT("ok") : *PresetSummary,
        (bApplyLatestPresetReportOk && bApplyLatestPresetValueOk) ? TEXT("ok") : *ApplyLatestPresetSummary,
        bAuditReportOk ? TEXT("ok") : *AuditSummary,
        bClearReportOk ? TEXT("ok") : *ClearSummary,
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError,
        HasStagedPatch() ? TEXT("yes") : TEXT("no"));

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFileWorkflowSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFileWorkflowSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFilePromoteWorkflowSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FilePromoteWorkflowSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutReport = TEXT("FilePromoteWorkflowSelfTest=FAIL | Settings object unavailable");
        return false;
    }

    const FRIEditableSettings PreviousEditableSettings = GetEditableSettings();
    const FRIEditableSettings PreviousSavedSnapshot = LastSavedSettingsSnapshot;
    const bool bPreviousSettingsDirty = bSettingsDirty;
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIPromoteResult PreviousPromoteResult = LastPromoteResult;
    const FRIAuditReport PreviousLastAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;

    FString OriginalText;
    if (!InspectorPropertyUtils::GetValueAsText(Settings, TEXT("OutlinePPWeight"), OriginalText))
    {
        OutReport = TEXT("FilePromoteWorkflowSelfTest=FAIL | Failed to read original OutlinePPWeight");
        return false;
    }

    const float OriginalValue = FCString::Atof(*OriginalText);
    const float TargetValue = FMath::IsNearlyEqual(OriginalValue, 0.65f, 0.001f) ? 0.35f : 0.65f;
    const FString TargetText = FString::SanitizeFloat(TargetValue);

    FRIPatchOperation Operation;
    Operation.Target.TargetKind = ERIPatchTargetKind::Actor;
    Operation.Field.FieldKind = ERIPatchFieldKind::Property;
    Operation.Field.FieldPath = TEXT("OutlinePPWeight");
    Operation.BaselineValue = OriginalText;
    Operation.PatchedValue = TargetText;
    Operation.SourceTag = TEXT("Config:/Script/RuntimeInspector.RuntimeInspectorSettings");

    FRIPatchBundle Bundle;
    Bundle.BundleId = TEXT("SelfTest_FilePromoteConfig");
    Bundle.DisplayName = TEXT("SelfTest File Promote Config");
    Bundle.Operations.Add(Operation);

    FString ExportedSharedReportPath;
    auto RestoreState = [&]()
    {
        FRIPatchBundle RestoreBundle;
        RestoreBundle.BundleId = TEXT("SelfTest_FilePromoteConfigRestore");
        RestoreBundle.DisplayName = TEXT("SelfTest File Promote Config Restore");

        FRIPatchOperation RestoreOp = Operation;
        RestoreOp.PatchedValue = OriginalText;
        RestoreOp.BaselineValue = TargetText;
        RestoreBundle.Operations.Add(RestoreOp);

        FRIPromoteResult RestoreResult;
        FString RestoreError;
        PromotePatchToSource(RestoreBundle, RestoreResult, RestoreError);

        FString PreviewRestoreError;
        PreviewApplySettings(PreviousEditableSettings, PreviewRestoreError);
        LastSavedSettingsSnapshot = PreviousSavedSnapshot;
        bSettingsDirty = bPreviousSettingsDirty;

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastPromoteResult = PreviousPromoteResult;
        LastAuditReport = PreviousLastAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
    };

    StagedPatchBundle = Bundle;
    bHasStagedPatch = true;

    FString PreviewSummary;
    FString PreviewDetails;
    const bool bPreviewOk = ExecuteFilePromotePreviewAction(PreviewSummary, PreviewDetails);
    const FString PreviewReportText = GetLastImportReportAsText(true, true);
    const bool bPreviewReportOk = bPreviewOk && PreviewReportText.Contains(PreviewSummary);

    auto EnsureComparePageReady = [&](FString& OutInjectionReport) -> UInspectorFilePageWidget*
    {
        Close();
        Open();
        ShowFilePage();
        UInspectorFilePageWidget* ComparePage = FilePageWidget.Get();
        const bool bInjectionOkLocal = ComparePage != nullptr || RunFilePageInjectionSelfTest(OutInjectionReport);
        if (!ComparePage)
        {
            ComparePage = FilePageWidget.Get();
        }
        if (ComparePage)
        {
            ComparePage->RefreshFromSubsystem();
        }
        if (bInjectionOkLocal && OutInjectionReport.IsEmpty())
        {
            OutInjectionReport = TEXT("ok");
        }
        return ComparePage;
    };

    FString PatchVsSourceSummary;
    FString PatchVsSourceDetails;
    const bool bPatchVsSourceAuditOk = ExecuteFileBuildPatchVsSourceAuditAction(PatchVsSourceSummary, PatchVsSourceDetails);
    const FString PatchVsSourceReportText = GetLastImportReportAsText(true, true);
    const bool bPatchVsSourceAuditReportOk = bPatchVsSourceAuditOk && PatchVsSourceReportText.Contains(PatchVsSourceSummary);
    const FRIAuditReport PatchVsSourceAudit = LastAuditReport;
    const bool bPatchVsSourceModeOk = PatchVsSourceAudit.Mode == ERIAuditComparisonMode::PatchVsSource;
    const bool bPatchVsSourceHasLines = PatchVsSourceAudit.Lines.Num() > 0;

    FString PatchSourceInjectionReport;
    UInspectorFilePageWidget* ComparePage = EnsureComparePageReady(PatchSourceInjectionReport);
    const bool bPatchSourcePageOk = ComparePage != nullptr;
    const FString PatchSourceModeLabel = ComparePage ? ComparePage->GetCompareModeLabel() : FString();
    const FString PatchSourcePairLabel = ComparePage ? ComparePage->GetComparePairLabel() : FString();
    const FString PatchSourceCacheLabel = ComparePage ? ComparePage->GetCompareCacheLabel() : FString();
    const int32 PatchSourceRenderedLineCount = ComparePage ? ComparePage->GetRenderedCompareLineCount() : 0;
    const int32 PatchSourceRenderedDifferentCount = ComparePage ? ComparePage->GetRenderedCompareDifferentCount() : 0;
    const bool bPatchSourceViewModeOk = PatchSourceModeLabel.Equals(TEXT("Patch vs Source"), ESearchCase::CaseSensitive);
    const bool bPatchSourceViewPairOk = PatchSourcePairLabel.Equals(TEXT("Patch -> SourcePreview"), ESearchCase::CaseSensitive);
    const bool bPatchSourceViewRenderedOk = PatchSourceRenderedLineCount > 0 && PatchSourceRenderedDifferentCount > 0;

    FString ApplySummary;
    FString ApplyDetails;
    const bool bApplyOk = ExecuteFilePromoteApplyAction(ApplySummary, ApplyDetails);
    const FString ApplyReportText = GetLastImportReportAsText(true, true);
    const bool bApplyReportOk = bApplyOk && ApplyReportText.Contains(ApplySummary);

    FString AfterPromoteText;
    InspectorPropertyUtils::GetValueAsText(GetMutableDefault<URuntimeInspectorSettings>(), TEXT("OutlinePPWeight"), AfterPromoteText);
    const bool bPromoteValueOk = FMath::IsNearlyEqual(FCString::Atof(*AfterPromoteText), TargetValue, 0.001f);

    FString AppliedAuditSummary;
    FString AppliedAuditDetails;
    const bool bAppliedAuditOk = ExecuteFileBuildAppliedPatchVsSourceAuditAction(AppliedAuditSummary, AppliedAuditDetails);
    const FString AppliedAuditReportText = GetLastImportReportAsText(true, true);
    const bool bAppliedAuditReportOk = bAppliedAuditOk && AppliedAuditReportText.Contains(AppliedAuditSummary);
    const FRIAuditReport AppliedAuditReport = LastAuditReport;
    int32 AppliedAuditDifferentCount = 0;
    for (const FRIAuditLine& Line : AppliedAuditReport.Lines)
    {
        if (Line.bDifferent)
        {
            ++AppliedAuditDifferentCount;
        }
    }
    const bool bAppliedAuditModeOk = AppliedAuditReport.Mode == ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote;
    const bool bAppliedAuditAligned = AppliedAuditReport.Lines.Num() > 0 && AppliedAuditDifferentCount == 0;

    FString AppliedInjectionReport;
    ComparePage = EnsureComparePageReady(AppliedInjectionReport);
    const bool bAppliedPageOk = ComparePage != nullptr;
    const FString AppliedModeLabel = ComparePage ? ComparePage->GetCompareModeLabel() : FString();
    const FString AppliedPairLabel = ComparePage ? ComparePage->GetComparePairLabel() : FString();
    const FString AppliedCacheLabel = ComparePage ? ComparePage->GetCompareCacheLabel() : FString();
    const int32 AppliedRenderedLineCount = ComparePage ? ComparePage->GetRenderedCompareLineCount() : 0;
    const int32 AppliedRenderedDifferentCount = ComparePage ? ComparePage->GetRenderedCompareDifferentCount() : 0;
    const bool bAppliedViewModeOk = AppliedModeLabel.Equals(TEXT("Applied Patch vs Source"), ESearchCase::CaseSensitive);
    const bool bAppliedViewPairOk = AppliedPairLabel.Equals(TEXT("Patch -> SourceApplied"), ESearchCase::CaseSensitive);
    const bool bAppliedViewRenderedOk = AppliedRenderedLineCount > 0 && AppliedRenderedDifferentCount == 0;
    const bool bCacheSummaryOk = PatchSourceCacheLabel.Contains(TEXT("Patch vs Source"))
        && AppliedCacheLabel.Contains(TEXT("Patch vs Source"))
        && AppliedCacheLabel.Contains(TEXT("Applied Patch vs Source"));

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::PatchVsSource);
    FString PatchSourceRevisitInjectionReport;
    ComparePage = EnsureComparePageReady(PatchSourceRevisitInjectionReport);
    const FString PatchSourceRevisitModeLabel = ComparePage ? ComparePage->GetCompareModeLabel() : FString();
    const FString PatchSourceRevisitPairLabel = ComparePage ? ComparePage->GetComparePairLabel() : FString();
    const bool bPatchSourceRevisitOk = PatchSourceRevisitModeLabel.Equals(TEXT("Patch vs Source"), ESearchCase::CaseSensitive)
        && PatchSourceRevisitPairLabel.Equals(TEXT("Patch -> SourcePreview"), ESearchCase::CaseSensitive);

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote);
    FString AppliedRevisitInjectionReport;
    ComparePage = EnsureComparePageReady(AppliedRevisitInjectionReport);
    const FString AppliedRevisitModeLabel = ComparePage ? ComparePage->GetCompareModeLabel() : FString();
    const FString AppliedRevisitPairLabel = ComparePage ? ComparePage->GetComparePairLabel() : FString();
    const bool bAppliedRevisitOk = AppliedRevisitModeLabel.Equals(TEXT("Applied Patch vs Source"), ESearchCase::CaseSensitive)
        && AppliedRevisitPairLabel.Equals(TEXT("Patch -> SourceApplied"), ESearchCase::CaseSensitive);

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    RestoreState();

    FString AfterRestoreText;
    InspectorPropertyUtils::GetValueAsText(GetMutableDefault<URuntimeInspectorSettings>(), TEXT("OutlinePPWeight"), AfterRestoreText);
    const bool bRestoreOk = FMath::IsNearlyEqual(FCString::Atof(*AfterRestoreText), OriginalValue, 0.001f);

    const bool bPassed = bPreviewReportOk
        && bPatchVsSourceAuditReportOk
        && bPatchVsSourceModeOk
        && bPatchVsSourceHasLines
        && bPatchSourcePageOk
        && bPatchSourceViewModeOk
        && bPatchSourceViewPairOk
        && bPatchSourceViewRenderedOk
        && bApplyReportOk
        && bPromoteValueOk
        && bAppliedAuditReportOk
        && bAppliedAuditModeOk
        && bAppliedAuditAligned
        && bAppliedPageOk
        && bAppliedViewModeOk
        && bAppliedViewPairOk
        && bAppliedViewRenderedOk
        && bCacheSummaryOk
        && bPatchSourceRevisitOk
        && bAppliedRevisitOk
        && bSharedReportExportOk
        && bClearReportOk
        && bRestoreOk;

    OutReport = FString::Printf(
        TEXT("FilePromoteWorkflowSelfTest=%s | Preview=%s PatchVsSource=%s PatchSourceInject=%s PatchSourceMode=%s PatchSourcePair=%s PatchSourceRendered=%d PatchSourceDiff=%d Apply=%s AppliedAudit=%s AppliedInject=%s AppliedMode=%s AppliedPair=%s AppliedRendered=%d AppliedDiff=%d Cache=%s Revisit=%s/%s ValueAfter=%s SharedExport=%s Clear=%s Restore=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bPreviewReportOk ? TEXT("ok") : *PreviewSummary,
        (bPatchVsSourceAuditReportOk && bPatchVsSourceModeOk && bPatchVsSourceHasLines) ? TEXT("ok") : *PatchVsSourceSummary,
        PatchSourceInjectionReport.IsEmpty() ? TEXT("-") : *PatchSourceInjectionReport,
        PatchSourceModeLabel.IsEmpty() ? TEXT("None") : *PatchSourceModeLabel,
        PatchSourcePairLabel.IsEmpty() ? TEXT("None") : *PatchSourcePairLabel,
        PatchSourceRenderedLineCount,
        PatchSourceRenderedDifferentCount,
        (bApplyReportOk && bPromoteValueOk) ? TEXT("ok") : *ApplySummary,
        (bAppliedAuditReportOk && bAppliedAuditModeOk && bAppliedAuditAligned) ? TEXT("ok") : *AppliedAuditSummary,
        AppliedInjectionReport.IsEmpty() ? TEXT("-") : *AppliedInjectionReport,
        AppliedModeLabel.IsEmpty() ? TEXT("None") : *AppliedModeLabel,
        AppliedPairLabel.IsEmpty() ? TEXT("None") : *AppliedPairLabel,
        AppliedRenderedLineCount,
        AppliedRenderedDifferentCount,
        bCacheSummaryOk ? TEXT("ok") : *(AppliedCacheLabel.IsEmpty() ? PatchSourceCacheLabel : AppliedCacheLabel),
        bPatchSourceRevisitOk ? TEXT("P/S") : *(PatchSourceRevisitModeLabel.IsEmpty() ? PatchSourceRevisitInjectionReport : PatchSourceRevisitModeLabel),
        bAppliedRevisitOk ? TEXT("Applied") : *(AppliedRevisitModeLabel.IsEmpty() ? AppliedRevisitInjectionReport : AppliedRevisitModeLabel),
        *AfterPromoteText,
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError,
        bClearReportOk ? TEXT("ok") : *ClearSummary,
        bRestoreOk ? TEXT("ok") : *AfterRestoreText);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFilePromoteWorkflowSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFilePromoteWorkflowSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFileCompareViewSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FileCompareViewSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("FileCompareViewSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIAuditReport PreviousAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;

    AActor* TestActor = PreviousSelectedActor;
    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    FString ExportedSharedReportPath;

    auto TryPrepareActor = [&](AActor* CandidateActor) -> bool
    {
        return RI_SelectWritablePrimitivePropertyForSelfTest(
            CandidateActor,
            TestProperty,
            OriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue);
    };

    if (!TryPrepareActor(TestActor))
    {
        TestActor = nullptr;
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (TryPrepareActor(*It))
                {
                    TestActor = *It;
                    break;
                }
            }
        }
    }

    if (!TestActor)
    {
        OutReport = TEXT("FileCompareViewSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    auto RestoreState = [&]()
    {
        if (TestActor && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);
            }
        }

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (PreviousSelectedActor != TestActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastAuditReport = PreviousAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    if (PreviousSelectedActor != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    FString Error;
    if (!ApplyPropertyTextNow(TestActor, TestProperty, PatchedText, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("FileCompareViewSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    FString StageSummary;
    FString StageDetails;
    const bool bStageOk = ExecuteFileStagePatchAction(StageSummary, StageDetails);
    const FString StageReportText = GetLastImportReportAsText(true, true);
    const bool bStageReportOk = bStageOk && StageReportText.Contains(StageSummary);

    auto EnsureComparePageReady = [&](FString& OutInjectionReport) -> UInspectorFilePageWidget*
    {
        Close();
        Open();
        ShowFilePage();
        UInspectorFilePageWidget* ComparePage = FilePageWidget.Get();
        const bool bInjectionOkLocal = ComparePage != nullptr || RunFilePageInjectionSelfTest(OutInjectionReport);
        if (!ComparePage)
        {
            ComparePage = FilePageWidget.Get();
        }
        if (ComparePage)
        {
            ComparePage->RefreshFromSubsystem();
        }
        if (bInjectionOkLocal && OutInjectionReport.IsEmpty())
        {
            OutInjectionReport = TEXT("ok");
        }
        return ComparePage;
    };

    FString BaselineSummary;
    FString BaselineDetails;
    const bool bBaselineAuditOk = ExecuteFileBuildBaselineAuditAction(BaselineSummary, BaselineDetails);
    const FString BaselineReportText = GetLastImportReportAsText(true, true);
    const bool bBaselineAuditReportOk = bBaselineAuditOk && BaselineReportText.Contains(BaselineSummary);

    FString BaselineInjectionReport;
    UInspectorFilePageWidget* Page = EnsureComparePageReady(BaselineInjectionReport);
    const bool bBaselinePageOk = Page != nullptr;
    const FString BaselineCompareSummary = Page ? Page->GetCompareDebugSummary() : TEXT("NoFilePage");
    const int32 BaselineRenderedLineCount = Page ? Page->GetRenderedCompareLineCount() : 0;
    const int32 BaselineRenderedDifferentCount = Page ? Page->GetRenderedCompareDifferentCount() : 0;
    const FString BaselineFirstField = Page ? Page->GetRenderedFirstAuditField() : FString();
    const FString BaselineModeLabel = Page ? Page->GetCompareModeLabel() : FString();
    const FString BaselinePairLabel = Page ? Page->GetComparePairLabel() : FString();
    const FString BaselineCacheLabel = Page ? Page->GetCompareCacheLabel() : FString();
    const FString BaselinePreviewText = Page ? Page->GetComparePreviewText() : FString();
    const bool bBaselineHasLines = LastAuditReport.Lines.Num() > 0;
    const bool bBaselineModeOk = BaselineModeLabel.Equals(TEXT("Baseline vs Current"), ESearchCase::CaseSensitive);
    const bool bBaselinePairOk = BaselinePairLabel.Equals(TEXT("Baseline -> Current"), ESearchCase::CaseSensitive);
    const bool bBaselineRenderedOk = BaselineRenderedLineCount > 0 && BaselineRenderedDifferentCount > 0;
    const bool bBaselineFieldOk = BaselineFirstField == TestProperty.ToString();
    const bool bBaselinePreviewOk = BaselinePreviewText.Contains(TestProperty.ToString());

    FString PatchSourceSummary;
    FString PatchSourceDetails;
    const bool bPatchSourceAuditOk = ExecuteFileBuildPatchVsSourceAuditAction(PatchSourceSummary, PatchSourceDetails);
    const FString PatchSourceReportText = GetLastImportReportAsText(true, true);
    const bool bPatchSourceAuditReportOk = bPatchSourceAuditOk && PatchSourceReportText.Contains(PatchSourceSummary);

    FString PatchSourceInjectionReport;
    Page = EnsureComparePageReady(PatchSourceInjectionReport);
    const bool bPatchSourcePageOk = Page != nullptr;
    const FString PatchSourceCompareSummary = Page ? Page->GetCompareDebugSummary() : TEXT("NoFilePage");
    const int32 PatchSourceRenderedLineCount = Page ? Page->GetRenderedCompareLineCount() : 0;
    const int32 PatchSourceRenderedDifferentCount = Page ? Page->GetRenderedCompareDifferentCount() : 0;
    const FString PatchSourceFirstField = Page ? Page->GetRenderedFirstAuditField() : FString();
    const FString PatchSourceModeLabel = Page ? Page->GetCompareModeLabel() : FString();
    const FString PatchSourcePairLabel = Page ? Page->GetComparePairLabel() : FString();
    const FString PatchSourceCacheLabel = Page ? Page->GetCompareCacheLabel() : FString();
    const FString PatchSourcePreviewText = Page ? Page->GetComparePreviewText() : FString();
    const bool bPatchSourceHasLines = LastAuditReport.Lines.Num() > 0;
    const bool bPatchSourceModeOk = PatchSourceModeLabel.Equals(TEXT("Patch vs Source"), ESearchCase::CaseSensitive);
    const bool bPatchSourcePairOk = PatchSourcePairLabel.Equals(TEXT("Patch -> SourcePreview"), ESearchCase::CaseSensitive);
    const bool bPatchSourceRenderedOk = PatchSourceRenderedLineCount > 0 && PatchSourceRenderedDifferentCount > 0;
    const bool bPatchSourceFieldOk = PatchSourceFirstField == TestProperty.ToString();
    const bool bPatchSourcePreviewOk = PatchSourcePreviewText.Contains(TestProperty.ToString());
    const bool bCacheSummaryOk = BaselineCacheLabel.Contains(TEXT("Baseline vs Current"))
        && PatchSourceCacheLabel.Contains(TEXT("Baseline vs Current"))
        && PatchSourceCacheLabel.Contains(TEXT("Patch vs Source"));

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::BaselineVsCurrent);
    FString BaselineRevisitInjectionReport;
    Page = EnsureComparePageReady(BaselineRevisitInjectionReport);
    const FString BaselineRevisitModeLabel = Page ? Page->GetCompareModeLabel() : FString();
    const FString BaselineRevisitPairLabel = Page ? Page->GetComparePairLabel() : FString();
    const bool bBaselineRevisitOk = BaselineRevisitModeLabel.Equals(TEXT("Baseline vs Current"), ESearchCase::CaseSensitive)
        && BaselineRevisitPairLabel.Equals(TEXT("Baseline -> Current"), ESearchCase::CaseSensitive);

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::PatchVsSource);
    FString PatchSourceRevisitInjectionReport;
    Page = EnsureComparePageReady(PatchSourceRevisitInjectionReport);
    const FString PatchSourceRevisitModeLabel = Page ? Page->GetCompareModeLabel() : FString();
    const FString PatchSourceRevisitPairLabel = Page ? Page->GetComparePairLabel() : FString();
    const bool bPatchSourceRevisitOk = PatchSourceRevisitModeLabel.Equals(TEXT("Patch vs Source"), ESearchCase::CaseSensitive)
        && PatchSourceRevisitPairLabel.Equals(TEXT("Patch -> SourcePreview"), ESearchCase::CaseSensitive);

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    const bool bPassed = bStageReportOk
        && bBaselineAuditReportOk
        && bBaselinePageOk
        && bBaselineHasLines
        && bBaselineModeOk
        && bBaselinePairOk
        && bBaselineRenderedOk
        && bBaselineFieldOk
        && bBaselinePreviewOk
        && bPatchSourceAuditReportOk
        && bPatchSourcePageOk
        && bPatchSourceHasLines
        && bPatchSourceModeOk
        && bPatchSourcePairOk
        && bPatchSourceRenderedOk
        && bPatchSourceFieldOk
        && bPatchSourcePreviewOk
        && bCacheSummaryOk
        && bBaselineRevisitOk
        && bPatchSourceRevisitOk
        && bClearReportOk
        && bSharedReportExportOk;

    OutReport = FString::Printf(
        TEXT("FileCompareViewSelfTest=%s | Property=%s Stage=%s Baseline=%s BaselineInject=%s BaselineMode=%s BaselinePair=%s BaselineRendered=%d BaselineDiff=%d PatchSource=%s PatchSourceInject=%s PatchSourceMode=%s PatchSourcePair=%s PatchSourceRendered=%d PatchSourceDiff=%d Cache=%s Revisit=%s/%s Clear=%s SharedExport=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        bStageReportOk ? TEXT("ok") : *StageSummary,
        (bBaselineAuditReportOk && bBaselineHasLines && bBaselineFieldOk && bBaselinePreviewOk) ? TEXT("ok") : *BaselineSummary,
        BaselineInjectionReport.IsEmpty() ? TEXT("-") : *BaselineInjectionReport,
        BaselineModeLabel.IsEmpty() ? TEXT("None") : *BaselineModeLabel,
        BaselinePairLabel.IsEmpty() ? TEXT("None") : *BaselinePairLabel,
        BaselineRenderedLineCount,
        BaselineRenderedDifferentCount,
        (bPatchSourceAuditReportOk && bPatchSourceHasLines && bPatchSourceFieldOk && bPatchSourcePreviewOk) ? TEXT("ok") : *PatchSourceSummary,
        PatchSourceInjectionReport.IsEmpty() ? TEXT("-") : *PatchSourceInjectionReport,
        PatchSourceModeLabel.IsEmpty() ? TEXT("None") : *PatchSourceModeLabel,
        PatchSourcePairLabel.IsEmpty() ? TEXT("None") : *PatchSourcePairLabel,
        PatchSourceRenderedLineCount,
        PatchSourceRenderedDifferentCount,
        bCacheSummaryOk ? TEXT("ok") : *(PatchSourceCacheLabel.IsEmpty() ? BaselineCacheLabel : PatchSourceCacheLabel),
        bBaselineRevisitOk ? TEXT("B/C") : *(BaselineRevisitModeLabel.IsEmpty() ? BaselineRevisitInjectionReport : BaselineRevisitModeLabel),
        bPatchSourceRevisitOk ? TEXT("P/S") : *(PatchSourceRevisitModeLabel.IsEmpty() ? PatchSourceRevisitInjectionReport : PatchSourceRevisitModeLabel),
        bClearReportOk ? TEXT("ok") : *ClearSummary,
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFileRemoteSessionCompareViewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFileRemoteSessionCompareViewSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFileRemoteSessionCompareViewSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FileRemoteSessionCompareViewSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("FileRemoteSessionCompareViewSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIRuntimeSessionTargetSetCompareReport PreviousRemoteCompareReport = LastRuntimeSessionTargetSetCompareReport;
    const FRIRuntimeSessionTargetSetCompareRequest PreviousRequest = ActiveRemoteSessionTargetSetCompareRequest;
    const FRIVerificationRunResult PreviousVerificationResult = LastVerificationRunResult;
    FString ExportedSharedReportPath;

    auto RestoreState = [&]()
    {
        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (SelectedActor.Get() != PreviousSelectedActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }

        LastImportReport = PreviousImportReport;
        LastRuntimeSessionTargetSetCompareReport = PreviousRemoteCompareReport;
        ActiveRemoteSessionTargetSetCompareRequest = PreviousRequest;
        LastVerificationRunResult = PreviousVerificationResult;
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    auto EnsureToolsPageReady = [&](FString& OutInjectionReport) -> UInspectorTestPageWidget*
    {
        Close();
        Open();
        ShowTestPage();
        UInspectorTestPageWidget* ComparePage = TestPageWidget.Get();
        const bool bInjectionOkLocal = ComparePage != nullptr || RunWorkflowPageViewSelfTest(OutInjectionReport);
        if (!ComparePage)
        {
            ComparePage = TestPageWidget.Get();
        }
        if (ComparePage)
        {
            ComparePage->RefreshFromSubsystem();
        }
        if (bInjectionOkLocal && OutInjectionReport.IsEmpty())
        {
            OutInjectionReport = TEXT("ok");
        }
        return ComparePage;
    };

    FString CompareSummary;
    FString CompareDetails;
    const FRIRuntimeSessionTargetSetCompareRequest Request = GetActiveRemoteSessionTargetSetCompareRequest();
    const FString ExpectedLeftSessionId = Request.LeftSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_editor_current") : Request.LeftSessionId.TrimStartAndEnd();
    const FString ExpectedRightSessionId = Request.RightSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_pie_current") : Request.RightSessionId.TrimStartAndEnd();
    const FString ExpectedNameFilter = Request.NameFilter.TrimStartAndEnd();
    const FString ExpectedClassFilter = Request.ClassFilter.TrimStartAndEnd();
    const bool bCompareOk = ExecuteFileBuildRemoteSessionTargetSetCompareAction(Request, CompareSummary, CompareDetails);
    const FString CompareReportText = GetLastImportReportAsText(true, true);
    const bool bCompareReportOk = bCompareOk && CompareReportText.Contains(CompareSummary);

    FString InjectionReport;
    UInspectorTestPageWidget* Page = EnsureToolsPageReady(InjectionReport);
    const bool bPageOk = Page != nullptr;
    const FRIRuntimeSessionTargetSetCompareReport CompareReport = GetLastRuntimeSessionTargetSetCompareReport();
    const FString SummaryText = Page ? Page->GetSessionCompareSummaryText() : FString();
    const FString SessionsText = Page ? Page->GetSessionComparePairText() : FString();
    const FString StatsText = Page ? Page->GetSessionCompareStatsText() : FString();
    const FString PreviewText = Page ? Page->GetSessionComparePreviewText() : FString();
    const bool bDiagnosticsSectionOk = Page && Page->HasDiagnosticsSection() && !Page->IsDiagnosticsSectionExpanded();

    const bool bSummaryOk = !CompareReport.Summary.IsEmpty()
        && (SummaryText.Contains(TEXT("SessionTargetSetCompare")) || SummaryText.Contains(TEXT("SessionTargetCompare")));
    const bool bSessionsOk = SessionsText.Contains(ExpectedLeftSessionId) && SessionsText.Contains(ExpectedRightSessionId);
    const bool bStatsOk = StatsText.Contains(TEXT("Shared=")) && StatsText.Contains(TEXT("Mismatch="));
    const bool bFilterStatsOk = (ExpectedNameFilter.IsEmpty() || StatsText.Contains(ExpectedNameFilter) || SummaryText.Contains(ExpectedNameFilter))
        && (ExpectedClassFilter.IsEmpty() || StatsText.Contains(ExpectedClassFilter) || SummaryText.Contains(ExpectedClassFilter));
    const bool bRenderedOk = CompareReport.Lines.Num() > 0;
    const bool bMismatchOk = StatsText.Contains(FString::Printf(TEXT("Mismatch=%d"), CompareReport.MismatchCount));
    const bool bPreviewOk = PreviewText.Contains(TEXT("L=")) && PreviewText.Contains(TEXT("R="));
    const bool bSharedOk = CompareReport.SharedTargetCount > 0;
    const bool bRequestEchoOk = CompareReport.LeftSessionId == ExpectedLeftSessionId
        && CompareReport.RightSessionId == ExpectedRightSessionId
        && CompareReport.NameFilter == ExpectedNameFilter
        && CompareReport.ClassFilter == ExpectedClassFilter;

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    const bool bPassed = bCompareReportOk
        && bPageOk
        && bSummaryOk
        && bSessionsOk
        && bStatsOk
        && bFilterStatsOk
        && bRenderedOk
        && bMismatchOk
        && bPreviewOk
        && bSharedOk
        && bDiagnosticsSectionOk
        && bRequestEchoOk
        && bSharedReportExportOk;

    OutReport = FString::Printf(
        TEXT("FileRemoteSessionCompareViewSelfTest=%s | Compare=%s Inject=%s Sessions=%s Rendered=%d Mismatch=%d Diagnostics=%s Filters=%s/%s Preview=%s SharedExport=%s Checks=%d/%d/%d/%d/%d/%d/%d/%d/%d/%d"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        (bCompareReportOk && bSummaryOk && bSessionsOk) ? TEXT("ok") : *CompareSummary,
        InjectionReport.IsEmpty() ? TEXT("-") : *InjectionReport,
        SessionsText.IsEmpty() ? TEXT("None") : *SessionsText,
        CompareReport.Lines.Num(),
        CompareReport.MismatchCount,
        bDiagnosticsSectionOk ? TEXT("collapsed") : TEXT("bad"),
        ExpectedNameFilter.IsEmpty() ? TEXT("-") : *ExpectedNameFilter,
        ExpectedClassFilter.IsEmpty() ? TEXT("-") : *ExpectedClassFilter,
        bPreviewOk ? TEXT("ok") : *PreviewText,
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError,
        bSummaryOk ? 1 : 0,
        bSessionsOk ? 1 : 0,
        bStatsOk ? 1 : 0,
        bFilterStatsOk ? 1 : 0,
        bRenderedOk ? 1 : 0,
        bMismatchOk ? 1 : 0,
        bPreviewOk ? 1 : 0,
        bSharedOk ? 1 : 0,
        bDiagnosticsSectionOk ? 1 : 0,
        bRequestEchoOk ? 1 : 0);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunFileRoleCompareViewSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunFileRoleCompareViewSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunFileRoleCompareViewSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("FileRoleCompareViewSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("FileRoleCompareViewSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIRuntimeRoleCompareReport PreviousRoleCompareReport = LastRuntimeRoleCompareReport;
    const FRIVerificationRunResult PreviousVerificationResult = LastVerificationRunResult;
    const TMap<FString, FString> PreviousBaselineValueByKey = BaselineValueByKey;
    const TMap<FString, FString> PreviousModifiedValueByKey = ModifiedValueByKey;

    AActor* TestActor = PreviousSelectedActor;
    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    FString ExportedSharedReportPath;

    auto TryPrepareActor = [&](AActor* CandidateActor) -> bool
    {
        return RI_SelectWritablePrimitivePropertyForSelfTest(
            CandidateActor,
            TestProperty,
            OriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue);
    };

    if (!TryPrepareActor(TestActor))
    {
        TestActor = nullptr;
        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (TryPrepareActor(*It))
                {
                    TestActor = *It;
                    break;
                }
            }
        }
    }

    if (!TestActor)
    {
        OutReport = TEXT("FileRoleCompareViewSelfTest=FAIL | No writable primitive actor property found");
        return false;
    }

    auto RestoreState = [&]()
    {
        if (TestActor && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, CurrentText) && CurrentText != OriginalText)
            {
                FString RestoreError;
                ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);
            }
        }

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (PreviousSelectedActor != TestActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastRuntimeRoleCompareReport = PreviousRoleCompareReport;
        LastVerificationRunResult = PreviousVerificationResult;
        BaselineValueByKey = PreviousBaselineValueByKey;
        ModifiedValueByKey = PreviousModifiedValueByKey;
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    if (PreviousSelectedActor != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    ClearModified();
    CaptureBaselineForSelection(true);

    FString Error;
    if (!ApplyPropertyTextNow(TestActor, TestProperty, PatchedText, Error))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("FileRoleCompareViewSelfTest=FAIL | ApplyError=%s"), *Error);
        return false;
    }

    FString StageSummary;
    FString StageDetails;
    const bool bStageOk = ExecuteFileStagePatchAction(StageSummary, StageDetails);
    const FString StageReportText = GetLastImportReportAsText(true, true);
    const bool bStageReportOk = bStageOk && StageReportText.Contains(StageSummary);

    auto EnsureToolsPageReady = [&](FString& OutInjectionReport) -> UInspectorTestPageWidget*
    {
        Close();
        Open();
        ShowTestPage();
        UInspectorTestPageWidget* ComparePage = TestPageWidget.Get();
        const bool bInjectionOkLocal = ComparePage != nullptr || RunWorkflowPageViewSelfTest(OutInjectionReport);
        if (!ComparePage)
        {
            ComparePage = TestPageWidget.Get();
        }
        if (ComparePage)
        {
            ComparePage->RefreshFromSubsystem();
        }
        if (bInjectionOkLocal && OutInjectionReport.IsEmpty())
        {
            OutInjectionReport = TEXT("ok");
        }
        return ComparePage;
    };

    FString RoleSummary;
    FString RoleDetails;
    const bool bRoleCompareOk = ExecuteFileBuildRuntimeRoleCompareAction(RoleSummary, RoleDetails);
    const FString RoleReportText = GetLastImportReportAsText(true, true);
    const bool bRoleReportOk = bRoleCompareOk && RoleReportText.Contains(RoleSummary);

    FString InjectionReport;
    UInspectorTestPageWidget* Page = EnsureToolsPageReady(InjectionReport);
    const bool bPageOk = Page != nullptr;
    const FRIRuntimeRoleCompareReport RoleCompareReport = GetLastRuntimeRoleCompareReport();
    const FString SummaryText = Page ? Page->GetRoleCompareSummaryText() : FString();
    const FString AvailableRoleText = Page ? Page->GetRoleCompareAvailableRoleText() : FString();
    const FString StatsText = Page ? Page->GetRoleCompareStatsText() : FString();
    const FString PreviewText = Page ? Page->GetRoleComparePreviewText() : FString();
    const bool bDiagnosticsSectionOk = Page && Page->HasDiagnosticsSection() && !Page->IsDiagnosticsSectionExpanded();

    const bool bRoleSummaryOk = !RoleCompareReport.Summary.IsEmpty() && SummaryText.Contains(TEXT("RuntimeRoleCompare=ok"));
    const bool bAvailableRoleOk = AvailableRoleText.Equals(TEXT("Authority"), ESearchCase::CaseSensitive);
    const bool bRenderedOk = RoleCompareReport.Lines.Num() > 0;
    const bool bMissingOk = RoleCompareReport.MissingRoleCount >= 2 && StatsText.Contains(TEXT("MissingRoles="));
    const bool bStatsOk = StatsText.Contains(TEXT("MissingRoles"));
    const bool bPreviewOk = PreviewText.Contains(TestProperty.ToString());

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    const bool bPassed = bStageReportOk
        && bRoleReportOk
        && bPageOk
        && bRoleSummaryOk
        && bAvailableRoleOk
        && bRenderedOk
        && bMissingOk
        && bStatsOk
        && bPreviewOk
        && bDiagnosticsSectionOk
        && bClearReportOk
        && bSharedReportExportOk;

    OutReport = FString::Printf(
        TEXT("FileRoleCompareViewSelfTest=%s | Property=%s Stage=%s Compare=%s Inject=%s AvailableRole=%s Rendered=%d Missing=%d Diagnostics=%s Preview=%s Clear=%s SharedExport=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *TestProperty.ToString(),
        bStageReportOk ? TEXT("ok") : *StageSummary,
        (bRoleReportOk && bRoleSummaryOk && bAvailableRoleOk) ? TEXT("ok") : *RoleSummary,
        InjectionReport.IsEmpty() ? TEXT("-") : *InjectionReport,
        AvailableRoleText.IsEmpty() ? TEXT("None") : *AvailableRoleText,
        RoleCompareReport.Lines.Num(),
        RoleCompareReport.MissingRoleCount,
        bDiagnosticsSectionOk ? TEXT("collapsed") : TEXT("bad"),
        bPreviewOk ? TEXT("ok") : *PreviewText,
        bClearReportOk ? TEXT("ok") : *ClearSummary,
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError);

    RestoreState();
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunActorPromoteFileWorkflowSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("ActorPromoteFileWorkflowSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("ActorPromoteFileWorkflowSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* InitialActor = SelectedActor.Get();
    if (!InitialActor)
    {
        OutReport = TEXT("ActorPromoteFileWorkflowSelfTest=BLOCKED | Selected actor required");
        return false;
    }

    UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(InitialActor->GetClass());
#if WITH_EDITOR
    UBlueprint* SourceBlueprint = BlueprintClass ? Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy) : nullptr;
#else
    UBlueprint* SourceBlueprint = nullptr;
#endif
    if (!BlueprintClass || !SourceBlueprint)
    {
        OutReport = FString::Printf(TEXT("ActorPromoteFileWorkflowSelfTest=FAIL | Selected actor is not Blueprint-backed: %s"), *InitialActor->GetPathName());
        return false;
    }

    TWeakObjectPtr<AActor> TestActor = InitialActor;
    const FString ExpectedActorPath = InitialActor->GetPathName();
    const FString ExpectedActorClass = InitialActor->GetClass()->GetPathName();
    const FString ExpectedActorBaseName = RI_ExtractActorBaseName(InitialActor->GetName());
    const FTransform InitialActorTransform = InitialActor->GetActorTransform();
#if WITH_EDITOR
    const FString InitialActorLabel = RI_GetActorDisplayLabel(InitialActor);
#else
    const FString InitialActorLabel = InitialActor->GetName();
#endif

    auto ResolveCurrentActor = [&]() -> AActor*
    {
        if (TestActor.IsValid())
        {
            return TestActor.Get();
        }

        UWorld* World = GetWorld();
        if (!World)
        {
            return nullptr;
        }

        AActor* ReacquiredActor = ResolveRuntimeActorTarget(ExpectedActorPath, ExpectedActorClass, ExpectedActorBaseName);
        if (ReacquiredActor)
        {
            TestActor = ReacquiredActor;
        }
        return ReacquiredActor;
    };

    auto ResolveActorInWorld = [&]() -> AActor*
    {
        AActor* ReacquiredActor = ResolveRuntimeActorTarget(ExpectedActorPath, ExpectedActorClass, ExpectedActorBaseName);
        if (ReacquiredActor)
        {
            TestActor = ReacquiredActor;
        }
        return ReacquiredActor;
    };

    auto ResolveOrRespawnCurrentActor = [&]() -> AActor*
    {
        if (AActor* CurrentActor = ResolveActorInWorld())
        {
            return CurrentActor;
        }

        UWorld* World = GetWorld();
        UClass* CurrentGeneratedClass = SourceBlueprint ? Cast<UClass>(SourceBlueprint->GeneratedClass) : nullptr;
        if (!World || !CurrentGeneratedClass)
        {
            return nullptr;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = MakeUniqueObjectName(World, CurrentGeneratedClass, FName(*ExpectedActorBaseName));
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* RespawnedActor = World->SpawnActor<AActor>(CurrentGeneratedClass, InitialActorTransform, SpawnParams);
        if (!RespawnedActor)
        {
            return nullptr;
        }

#if WITH_EDITOR
        if (!InitialActorLabel.IsEmpty())
        {
            RespawnedActor->SetActorLabel(InitialActorLabel, true);
        }
#endif

        TestActor = RespawnedActor;
        return RespawnedActor;
    };

    auto GetCurrentSourceObject = [&]() -> UObject*
    {
        UClass* CurrentGeneratedClass = SourceBlueprint ? Cast<UClass>(SourceBlueprint->GeneratedClass) : nullptr;
        return CurrentGeneratedClass ? CurrentGeneratedClass->GetDefaultObject() : nullptr;
    };

    FName TestProperty = NAME_None;
    FString RuntimeOriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
            InitialActor,
            TestProperty,
            RuntimeOriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue))
    {
        OutReport = FString::Printf(TEXT("ActorPromoteFileWorkflowSelfTest=FAIL | No writable primitive property on %s"), *InitialActor->GetPathName());
        return false;
    }

    UObject* SourceObject = GetCurrentSourceObject();
    FString SourceOriginalText;
    if (!SourceObject || !InspectorPropertyUtils::GetValueAsText(SourceObject, TestProperty, SourceOriginalText))
    {
        OutReport = FString::Printf(TEXT("ActorPromoteFileWorkflowSelfTest=FAIL | Failed to read source default for %s"), *TestProperty.ToString());
        return false;
    }

    const TWeakObjectPtr<AActor> PreviousSelectedActor = SelectedActor.Get();
    const FString PreviousSelectedActorPath = PreviousSelectedActor.IsValid() ? PreviousSelectedActor->GetPathName() : FString();
    const FString PreviousSelectedActorClass = PreviousSelectedActor.IsValid() ? PreviousSelectedActor->GetClass()->GetPathName() : FString();
    const FString PreviousSelectedActorBaseName = PreviousSelectedActor.IsValid() ? RI_ExtractActorBaseName(PreviousSelectedActor->GetName()) : FString();
    const bool bPreviousSelectedWasTestActor = !PreviousSelectedActorBaseName.IsEmpty()
        && PreviousSelectedActorBaseName == ExpectedActorBaseName
        && (PreviousSelectedActorClass.IsEmpty() || PreviousSelectedActorClass == ExpectedActorClass);
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIPromoteResult PreviousPromoteResult = LastPromoteResult;
    const FRIAuditReport PreviousAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;
    FString ExportedSharedReportPath;

    auto BuildSourceRestoreBundle = [&]() -> FRIPatchBundle
    {
        FRIPatchOperation Operation;
        Operation.Target.TargetKind = ERIPatchTargetKind::Actor;
        Operation.Target.ActorPath = ExpectedActorPath;
        Operation.Target.ActorClass = ExpectedActorClass;
        Operation.Target.ActorBaseName = ExpectedActorBaseName;
        Operation.Field.FieldKind = ERIPatchFieldKind::Property;
        Operation.Field.FieldPath = TestProperty.ToString();
        Operation.Field.DisplayName = TestProperty.ToString();
        Operation.BaselineValue = PatchedText;
        Operation.PatchedValue = SourceOriginalText;
        Operation.SourceTag = FString::Printf(TEXT("Blueprint:%s"), *SourceBlueprint->GetPathName());

        FRIPatchBundle Bundle;
        Bundle.BundleId = TEXT("SelfTest_ActorPromoteFileRestore");
        Bundle.DisplayName = TEXT("SelfTest Actor Promote File Restore");
        Bundle.Operations.Add(Operation);
        return Bundle;
    };

    auto ResolvePreviousSelectedActor = [&]() -> AActor*
    {
        if (PreviousSelectedActor.IsValid())
        {
            return PreviousSelectedActor.Get();
        }

        if (PreviousSelectedActorPath.IsEmpty())
        {
            return nullptr;
        }

        return ResolveRuntimeActorTarget(PreviousSelectedActorPath, PreviousSelectedActorClass, PreviousSelectedActorBaseName);
    };

    auto RestoreState = [&]()
    {
        UObject* CurrentSourceObject = GetCurrentSourceObject();
        FString CurrentSourceText;
        if (CurrentSourceObject
            && InspectorPropertyUtils::GetValueAsText(CurrentSourceObject, TestProperty, CurrentSourceText)
            && !RI_AreSelfTestPrimitiveValuesEquivalent(CurrentSourceText, SourceOriginalText, PropertyKind))
        {
            FRIPromoteResult RestorePromoteResult;
            FString RestorePromoteError;
            PromotePatchToSource(BuildSourceRestoreBundle(), RestorePromoteResult, RestorePromoteError);
        }

        AActor* CurrentActor = ResolveOrRespawnCurrentActor();
        if (CurrentActor)
        {
            FString CurrentRuntimeText;
            if (InspectorPropertyUtils::GetValueAsText(CurrentActor, TestProperty, CurrentRuntimeText)
                && !RI_AreSelfTestPrimitiveValuesEquivalent(CurrentRuntimeText, RuntimeOriginalText, PropertyKind))
            {
                FString RestoreRuntimeError;
                ApplyPropertyTextNow(CurrentActor, TestProperty, RuntimeOriginalText, RestoreRuntimeError);
            }
        }

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastPromoteResult = PreviousPromoteResult;
        LastAuditReport = PreviousAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;

        if (AActor* DesiredSelectedActor = ResolvePreviousSelectedActor())
        {
            if (SelectedActor.Get() != DesiredSelectedActor)
            {
                SetSelectedActor(DesiredSelectedActor);
            }
        }
        else if (bPreviousSelectedWasTestActor && ResolveOrRespawnCurrentActor())
        {
            if (SelectedActor.Get() != TestActor.Get())
            {
                SetSelectedActor(TestActor.Get());
            }
        }
        else if (SelectedActor.Get())
        {
            SetSelectedActor(nullptr);
        }

        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    if (ResolvePreviousSelectedActor() != InitialActor)
    {
        SetSelectedActor(InitialActor);
    }

    auto ActorSummaryMatchesExpected = [&](const FRIRuntimeActorRoleSummary& Summary) -> bool
    {
        return Summary.bHasActor
            && RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(Summary.ActorPath)) == ExpectedActorBaseName
            && (Summary.ActorClass.IsEmpty() || Summary.ActorClass == ExpectedActorClass);
    };

    const FRIRuntimeActorRoleSummary RoleBefore = GetSelectedActorRoleSummary();
    const bool bSelectedBeforeOk = ActorSummaryMatchesExpected(RoleBefore);

    FString RuntimeApplyError;
    if (!ApplyPropertyTextNow(InitialActor, TestProperty, PatchedText, RuntimeApplyError))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("ActorPromoteFileWorkflowSelfTest=FAIL | Apply=%s"), *RuntimeApplyError);
        return false;
    }

    FString AfterRuntimeApplyText;
    const bool bRuntimeApplyReadOk = InspectorPropertyUtils::GetValueAsText(InitialActor, TestProperty, AfterRuntimeApplyText);
    const bool bRuntimeApplyOk = bRuntimeApplyReadOk && RI_AreSelfTestPrimitiveValuesEquivalent(AfterRuntimeApplyText, PatchedText, PropertyKind);

    FString StageSummary;
    FString StageDetails;
    const bool bStageOk = ExecuteFileStagePatchAction(StageSummary, StageDetails);
    const FString StageReportText = GetLastImportReportAsText(true, true);
    const bool bStageReportOk = bStageOk && StageReportText.Contains(StageSummary);

    FString PreviewSummary;
    FString PreviewDetails;
    const bool bPreviewOk = ExecuteFilePromotePreviewAction(PreviewSummary, PreviewDetails);
    const FString PreviewReportText = GetLastImportReportAsText(true, true);
    const bool bPreviewReportOk = bPreviewOk && PreviewReportText.Contains(PreviewSummary);

    FString PatchSourceSummary;
    FString PatchSourceDetails;
    const bool bPatchSourceAuditOk = ExecuteFileBuildPatchVsSourceAuditAction(PatchSourceSummary, PatchSourceDetails);
    const FString PatchSourceReportText = GetLastImportReportAsText(true, true);
    const bool bPatchSourceReportOk = bPatchSourceAuditOk && PatchSourceReportText.Contains(PatchSourceSummary);
    FRIAuditReport PatchSourceReport;
    const bool bPatchSourceCachedOk = GetCachedAuditReport(ERIAuditComparisonMode::PatchVsSource, PatchSourceReport);
    int32 PatchSourceDifferentCount = 0;
    bool bPatchSourceHasPropertyDiff = false;
    if (bPatchSourceCachedOk)
    {
        for (const FRIAuditLine& Line : PatchSourceReport.Lines)
        {
            PatchSourceDifferentCount += Line.bDifferent ? 1 : 0;
            bPatchSourceHasPropertyDiff = bPatchSourceHasPropertyDiff || (Line.Field.FieldPath == TestProperty.ToString() && Line.bDifferent);
        }
    }

    FString ApplySummary;
    FString ApplyDetails;
    const bool bApplyOk = ExecuteFilePromoteApplyAction(ApplySummary, ApplyDetails);
    const FString ApplyReportText = GetLastImportReportAsText(true, true);
    const bool bApplyReportOk = bApplyOk && ApplyReportText.Contains(ApplySummary);

    UObject* SourceAfterPromoteObject = GetCurrentSourceObject();
    FString SourceAfterPromoteText;
    const bool bSourceAfterPromoteReadOk = SourceAfterPromoteObject
        && InspectorPropertyUtils::GetValueAsText(SourceAfterPromoteObject, TestProperty, SourceAfterPromoteText);
    const bool bSourcePromoteOk = bSourceAfterPromoteReadOk
        && RI_AreSelfTestPrimitiveValuesEquivalent(SourceAfterPromoteText, PatchedText, PropertyKind);

    FString AppliedSummary;
    FString AppliedDetails;
    const bool bAppliedAuditOk = ExecuteFileBuildAppliedPatchVsSourceAuditAction(AppliedSummary, AppliedDetails);
    const FString AppliedReportText = GetLastImportReportAsText(true, true);
    const bool bAppliedReportOk = bAppliedAuditOk && AppliedReportText.Contains(AppliedSummary);
    FRIAuditReport AppliedAuditReport;
    const bool bAppliedCachedOk = GetCachedAuditReport(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote, AppliedAuditReport);
    int32 AppliedDifferentCount = 0;
    if (bAppliedCachedOk)
    {
        for (const FRIAuditLine& Line : AppliedAuditReport.Lines)
        {
            AppliedDifferentCount += Line.bDifferent ? 1 : 0;
        }
    }
    const bool bAppliedAligned = bAppliedCachedOk && AppliedAuditReport.Lines.Num() > 0 && AppliedDifferentCount == 0;

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    FRIPromoteResult RestorePromoteResult;
    FString RestorePromoteError;
    const bool bSourceRestoreApplyOk = PromotePatchToSource(BuildSourceRestoreBundle(), RestorePromoteResult, RestorePromoteError);
    UObject* SourceAfterRestoreObject = GetCurrentSourceObject();
    FString SourceAfterRestoreText;
    const bool bSourceAfterRestoreReadOk = SourceAfterRestoreObject
        && InspectorPropertyUtils::GetValueAsText(SourceAfterRestoreObject, TestProperty, SourceAfterRestoreText);
    const bool bSourceRestoreOk = bSourceRestoreApplyOk
        && bSourceAfterRestoreReadOk
        && RI_AreSelfTestPrimitiveValuesEquivalent(SourceAfterRestoreText, SourceOriginalText, PropertyKind);

    AActor* CurrentActor = ResolveOrRespawnCurrentActor();
    FString RuntimeRestoreError;
    const bool bRuntimeRestoreApplyOk = CurrentActor
        && ApplyPropertyTextNow(CurrentActor, TestProperty, RuntimeOriginalText, RuntimeRestoreError);
    FString AfterRuntimeRestoreText;
    const bool bRuntimeAfterRestoreReadOk = CurrentActor
        && InspectorPropertyUtils::GetValueAsText(CurrentActor, TestProperty, AfterRuntimeRestoreText);
    const bool bRuntimeRestoreOk = bRuntimeRestoreApplyOk
        && bRuntimeAfterRestoreReadOk
        && RI_AreSelfTestPrimitiveValuesEquivalent(AfterRuntimeRestoreText, RuntimeOriginalText, PropertyKind);

    const FRIRuntimeActorRoleSummary RoleAfter = GetSelectedActorRoleSummary();
    const bool bSelectedAfterOk = ActorSummaryMatchesExpected(RoleAfter);

    const bool bPassed = bSelectedBeforeOk
        && bRuntimeApplyOk
        && bStageReportOk
        && bPreviewReportOk
        && bPatchSourceReportOk
        && bPatchSourceCachedOk
        && bPatchSourceHasPropertyDiff
        && PatchSourceDifferentCount > 0
        && bApplyReportOk
        && bSourcePromoteOk
        && bAppliedReportOk
        && bAppliedAligned
        && bSharedReportExportOk
        && bClearReportOk
        && bSourceRestoreOk
        && bRuntimeRestoreOk;

    const FString SelectionAfterText = bSelectedAfterOk
        ? TEXT("ok")
        : (RoleAfter.Summary.IsEmpty() ? TEXT("none") : RoleAfter.Summary);

    OutReport = FString::Printf(
        TEXT("ActorPromoteFileWorkflowSelfTest=%s | Actor=%s Property=%s Apply=%s Stage=%s Preview=%s PatchVsSource=%s PatchDiff=%d ApplyPromote=%s AppliedAudit=%s AppliedDiff=%d Clear=%s SourceRestore=%s RuntimeRestore=%s SharedExport=%s SelectionAfter=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *ExpectedActorPath,
        *TestProperty.ToString(),
        (bSelectedBeforeOk && bRuntimeApplyOk) ? TEXT("ok") : *(RuntimeApplyError.IsEmpty() ? AfterRuntimeApplyText : RuntimeApplyError),
        bStageReportOk ? TEXT("ok") : *StageSummary,
        bPreviewReportOk ? TEXT("ok") : *PreviewSummary,
        (bPatchSourceReportOk && bPatchSourceHasPropertyDiff) ? TEXT("ok") : *PatchSourceSummary,
        PatchSourceDifferentCount,
        (bApplyReportOk && bSourcePromoteOk) ? TEXT("ok") : *ApplySummary,
        (bAppliedReportOk && bAppliedAligned) ? TEXT("ok") : *AppliedSummary,
        AppliedDifferentCount,
        bClearReportOk ? TEXT("ok") : *ClearSummary,
        bSourceRestoreOk ? TEXT("ok") : *(RestorePromoteError.IsEmpty() ? SourceAfterRestoreText : RestorePromoteError),
        bRuntimeRestoreOk ? TEXT("ok") : *(RuntimeRestoreError.IsEmpty() ? AfterRuntimeRestoreText : RuntimeRestoreError),
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError,
        *SelectionAfterText);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunActorPromoteFileWorkflowSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunActorPromoteFileWorkflowSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunActorApplyFileWorkflowSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("ActorApplyFileWorkflowSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("ActorApplyFileWorkflowSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    AActor* TestActor = SelectedActor.Get();
    if (!TestActor)
    {
        OutReport = TEXT("ActorApplyFileWorkflowSelfTest=BLOCKED | Selected actor required");
        return false;
    }

    FName TestProperty = NAME_None;
    FString OriginalText;
    FString PatchedText;
    FString PropertyKind;
    double NumericOriginalValue = 0.0;
    double NumericTargetValue = 0.0;
    if (!RI_SelectWritablePrimitivePropertyForSelfTest(
            TestActor,
            TestProperty,
            OriginalText,
            PatchedText,
            PropertyKind,
            NumericOriginalValue,
            NumericTargetValue))
    {
        OutReport = FString::Printf(TEXT("ActorApplyFileWorkflowSelfTest=FAIL | No writable primitive property on %s"), *TestActor->GetPathName());
        return false;
    }

    const FString ExpectedActorPath = TestActor->GetPathName();
    const AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIAuditReport PreviousAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;
    FString ExportedSharedReportPath;

    auto RestoreState = [&]()
    {
        if (TestActor && !TestProperty.IsNone() && !OriginalText.IsEmpty())
        {
            FString CurrentText;
            if (InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, CurrentText)
                && !RI_AreSelfTestPrimitiveValuesEquivalent(CurrentText, OriginalText, PropertyKind))
            {
                FString RestoreError;
                ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);
            }
        }

        if (!ExportedSharedReportPath.IsEmpty())
        {
            IFileManager::Get().Delete(*ExportedSharedReportPath, false, true);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastAuditReport = PreviousAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;

        if (PreviousSelectedActor != TestActor)
        {
            SetSelectedActor(const_cast<AActor*>(PreviousSelectedActor));
        }

        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    };

    if (PreviousSelectedActor != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    const FRIRuntimeActorRoleSummary RoleBefore = GetSelectedActorRoleSummary();
    const bool bSelectedBeforeOk = RoleBefore.bHasActor && RoleBefore.ActorPath == ExpectedActorPath;

    FString ApplyError;
    if (!ApplyPropertyTextNow(TestActor, TestProperty, PatchedText, ApplyError))
    {
        RestoreState();
        OutReport = FString::Printf(TEXT("ActorApplyFileWorkflowSelfTest=FAIL | ApplyError=%s"), *ApplyError);
        return false;
    }

    FString AfterApplyText;
    const bool bReadAfterApply = InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, AfterApplyText);
    const bool bApplyValueOk = bReadAfterApply && RI_AreSelfTestPrimitiveValuesEquivalent(AfterApplyText, PatchedText, PropertyKind);

    FString StageSummary;
    FString StageDetails;
    const bool bStageOk = ExecuteFileStagePatchAction(StageSummary, StageDetails);
    const FString StageReportText = GetLastImportReportAsText(true, true);
    const bool bStageReportOk = bStageOk && StageReportText.Contains(StageSummary);

    FRIFileManagementSummary StageFileSummary;
    FString StageFileSummaryError;
    const bool bStageFileSummaryOk = GetFileManagementSummary(StageFileSummary, StageFileSummaryError)
        && StageFileSummary.bHasStagedPatch
        && StageFileSummary.StagedPatchOperationCount > 0;

    FString AuditSummary;
    FString AuditDetails;
    const bool bAuditOk = ExecuteFileBuildPatchVsSourceAuditAction(AuditSummary, AuditDetails);
    const FString AuditReportText = GetLastImportReportAsText(true, true);
    const bool bAuditReportOk = bAuditOk && AuditReportText.Contains(AuditSummary);

    FRIAuditReport CachedAuditReport;
    const bool bHasCachedAudit = GetCachedAuditReport(ERIAuditComparisonMode::PatchVsSource, CachedAuditReport);
    int32 DifferentCount = 0;
    bool bHasPropertyDiff = false;
    if (bHasCachedAudit)
    {
        for (const FRIAuditLine& Line : CachedAuditReport.Lines)
        {
            DifferentCount += Line.bDifferent ? 1 : 0;
            bHasPropertyDiff = bHasPropertyDiff || (Line.Field.FieldPath == TestProperty.ToString() && Line.bDifferent);
        }
    }

    SetActiveFileAuditViewMode(ERIAuditComparisonMode::PatchVsSource);
    FRIFileManagementSummary AuditFileSummary;
    FString AuditFileSummaryError;
    const bool bAuditFileSummaryOk = GetFileManagementSummary(AuditFileSummary, AuditFileSummaryError)
        && AuditFileSummary.ActiveAuditViewLabel.Contains(TEXT("Patch vs Source"))
        && AuditFileSummary.CachedAuditViewsSummary.Contains(TEXT("Patch vs Source"));

    FString SharedReportExportError;
    const bool bSharedReportExportOk = ExportLastImportReportToFile(false, ExportedSharedReportPath, SharedReportExportError)
        && IFileManager::Get().FileExists(*ExportedSharedReportPath);

    FString ClearSummary;
    FString ClearDetails;
    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const FString ClearReportText = GetLastImportReportAsText(true, true);
    const bool bClearReportOk = bClearOk && ClearReportText.Contains(ClearSummary);

    FRIFileManagementSummary ClearedFileSummary;
    FString ClearedFileSummaryError;
    const bool bClearFileSummaryOk = GetFileManagementSummary(ClearedFileSummary, ClearedFileSummaryError)
        && !ClearedFileSummary.bHasStagedPatch;

    FString RestoreError;
    const bool bRestoreOk = ApplyPropertyTextNow(TestActor, TestProperty, OriginalText, RestoreError);
    FString AfterRestoreText;
    const bool bReadAfterRestore = InspectorPropertyUtils::GetValueAsText(TestActor, TestProperty, AfterRestoreText);
    const bool bRestoreValueOk = bRestoreOk
        && bReadAfterRestore
        && RI_AreSelfTestPrimitiveValuesEquivalent(AfterRestoreText, OriginalText, PropertyKind);

    const FRIRuntimeActorRoleSummary RoleAfter = GetSelectedActorRoleSummary();
    const bool bSelectedAfterOk = RoleAfter.bHasActor && RoleAfter.ActorPath == ExpectedActorPath;

    const bool bPassed = bSelectedBeforeOk
        && bApplyValueOk
        && bStageReportOk
        && bStageFileSummaryOk
        && bAuditReportOk
        && bHasCachedAudit
        && bHasPropertyDiff
        && DifferentCount > 0
        && bAuditFileSummaryOk
        && bSharedReportExportOk
        && bClearReportOk
        && bClearFileSummaryOk
        && bRestoreValueOk
        && bSelectedAfterOk;

    OutReport = FString::Printf(
        TEXT("ActorApplyFileWorkflowSelfTest=%s | Actor=%s Property=%s Apply=%s Stage=%s StageSummary=%s Audit=%s AuditDiff=%d FileSummary=%s Clear=%s Restore=%s SharedExport=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *ExpectedActorPath,
        *TestProperty.ToString(),
        (bSelectedBeforeOk && bApplyValueOk) ? TEXT("ok") : *(ApplyError.IsEmpty() ? AfterApplyText : ApplyError),
        (bStageReportOk && bStageFileSummaryOk) ? TEXT("ok") : *StageSummary,
        StageFileSummaryError.IsEmpty() ? *StageFileSummary.StagedPatchLabel : *StageFileSummaryError,
        (bAuditReportOk && bHasPropertyDiff && bAuditFileSummaryOk) ? TEXT("ok") : *AuditSummary,
        DifferentCount,
        AuditFileSummaryError.IsEmpty() ? *AuditFileSummary.ActiveAuditViewLabel : *AuditFileSummaryError,
        (bClearReportOk && bClearFileSummaryOk) ? TEXT("ok") : *ClearSummary,
        bRestoreValueOk ? TEXT("ok") : *(RestoreError.IsEmpty() ? AfterRestoreText : RestoreError),
        bSharedReportExportOk ? TEXT("ok") : *SharedReportExportError);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunActorApplyFileWorkflowSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunActorApplyFileWorkflowSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRuntimeSessionRoleSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeSessionRoleSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RuntimeSessionRoleSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutReport = TEXT("RuntimeSessionRoleSelfTest=FAIL | No world");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    AActor* TestActor = PreviousSelectedActor;
    AActor* SpawnedActor = nullptr;

    auto RestoreState = [&]()
    {
        if (SpawnedActor && IsValid(SpawnedActor))
        {
            World->DestroyActor(SpawnedActor);
        }

        if (PreviousSelectedActor != TestActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }
    };

    if (!TestActor)
    {
        AActor* WorldSettingsActor = World->GetWorldSettings();
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Candidate = *It;
            if (!Candidate || Candidate == WorldSettingsActor)
            {
                continue;
            }

            TestActor = Candidate;
            break;
        }
    }

    if (!TestActor)
    {
        SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 120.f)));
        TestActor = SpawnedActor;
    }

    if (!TestActor)
    {
        OutReport = TEXT("RuntimeSessionRoleSelfTest=FAIL | No runtime actor available");
        return false;
    }

    if (PreviousSelectedActor != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    const FRIRuntimeSessionSummary SessionSummary = GetRuntimeSessionSummary();
    const FRIRuntimeActorRoleSummary RoleSummary = GetSelectedActorRoleSummary();
    const FString ExpectedRoleText = FString::Printf(TEXT("%s / %s"), *RoleSummary.LocalRoleLabel, *RoleSummary.RemoteRoleLabel);

    auto EnsureSettingsPageReady = [&](FString& OutInjectionReport) -> UInspectorSettingsPageWidget*
    {
        Close();
        Open();
        ShowSettingsPage();
        UInspectorSettingsPageWidget* Page = SettingsPageWidget.Get();
        if (Page)
        {
            Page->RefreshFromSubsystem();
            OutInjectionReport = TEXT("ok");
            return Page;
        }

        OutInjectionReport = TEXT("missing");
        return nullptr;
    };

    FString SettingsInjectionReport;
    UInspectorSettingsPageWidget* SettingsPage = EnsureSettingsPageReady(SettingsInjectionReport);
    const bool bSettingsPageOk = SettingsPage != nullptr;
    const FString SettingsSessionText = SettingsPage ? SettingsPage->GetSessionValueText() : FString();
    const FString SettingsNetText = SettingsPage ? SettingsPage->GetNetModeValueText() : FString();
    const FString SettingsActorText = SettingsPage ? SettingsPage->GetSelectedActorValueText() : FString();
    const FString SettingsRoleText = SettingsPage ? SettingsPage->GetSelectedRoleValueText() : FString();

    const bool bSessionOk = SessionSummary.bSessionAvailable
        && SessionSummary.bIsGameWorld
        && SessionSummary.bHasLocalPlayerController
        && !SessionSummary.WorldTypeLabel.IsEmpty()
        && !SessionSummary.NetModeLabel.IsEmpty();

    const bool bRoleOk = RoleSummary.bHasActor
        && RoleSummary.ActorPath == TestActor->GetPathName()
        && !RoleSummary.LocalRoleLabel.IsEmpty()
        && !RoleSummary.RemoteRoleLabel.IsEmpty()
        && RoleSummary.bHasAuthority == TestActor->HasAuthority();

    const bool bSettingsSessionOk = SettingsSessionText == SessionSummary.WorldTypeLabel;
    const bool bSettingsNetOk = SettingsNetText == SessionSummary.NetModeLabel;
    const bool bSettingsActorOk = SettingsActorText == RoleSummary.ActorPath;
    const bool bSettingsRoleOk = SettingsRoleText == ExpectedRoleText;

    const bool bPassed = bSessionOk
        && bRoleOk
        && bSettingsPageOk
        && bSettingsSessionOk
        && bSettingsNetOk
        && bSettingsActorOk
        && bSettingsRoleOk;
    OutReport = FString::Printf(
        TEXT("RuntimeSessionRoleSelfTest=%s | Session=%s World=%s Net=%s PC=%s Actor=%s LocalRole=%s RemoteRole=%s Authority=%s Rep=%s Move=%s Inject=%s SessionText=%s NetText=%s RoleText=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bSessionOk ? TEXT("ok") : *SessionSummary.Summary,
        SessionSummary.WorldTypeLabel.IsEmpty() ? TEXT("None") : *SessionSummary.WorldTypeLabel,
        SessionSummary.NetModeLabel.IsEmpty() ? TEXT("None") : *SessionSummary.NetModeLabel,
        SessionSummary.bHasLocalPlayerController ? TEXT("yes") : TEXT("no"),
        RoleSummary.bHasActor ? TEXT("ok") : *RoleSummary.Summary,
        RoleSummary.LocalRoleLabel.IsEmpty() ? TEXT("None") : *RoleSummary.LocalRoleLabel,
        RoleSummary.RemoteRoleLabel.IsEmpty() ? TEXT("None") : *RoleSummary.RemoteRoleLabel,
        RoleSummary.bHasAuthority ? TEXT("yes") : TEXT("no"),
        RoleSummary.bReplicates ? TEXT("yes") : TEXT("no"),
        RoleSummary.bReplicateMovement ? TEXT("yes") : TEXT("no"),
        *SettingsInjectionReport,
        SettingsSessionText.IsEmpty() ? TEXT("None") : *SettingsSessionText,
        SettingsNetText.IsEmpty() ? TEXT("None") : *SettingsNetText,
        SettingsRoleText.IsEmpty() ? TEXT("None") : *SettingsRoleText);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRuntimeSessionRoleSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRuntimeSessionRoleSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRuntimeRoleCompareSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RuntimeRoleCompareSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RuntimeRoleCompareSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutReport = TEXT("RuntimeRoleCompareSelfTest=FAIL | No world");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIRuntimeRoleCompareReport PreviousRoleCompareReport = LastRuntimeRoleCompareReport;
    const FRIVerificationRunResult PreviousVerificationResult = LastVerificationRunResult;
    const TMap<FString, FString> PreviousBaselineValueByKey = BaselineValueByKey;
    const TMap<FString, FString> PreviousModifiedValueByKey = ModifiedValueByKey;

    AActor* TestActor = PreviousSelectedActor;
    AActor* SpawnedActor = nullptr;
    FString OriginalValueText;
    bool bOriginalValueCaptured = false;

    auto RestoreState = [&]()
    {
        if (TestActor && IsValid(TestActor) && bOriginalValueCaptured)
        {
            FString RestoreError;
            ApplyPropertyTextImmediate(TestActor, TEXT("InitialLifeSpan"), OriginalValueText, RestoreError);
        }

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            ClearStagedPatch();
        }

        LastRuntimeRoleCompareReport = PreviousRoleCompareReport;
        LastVerificationRunResult = PreviousVerificationResult;
        BaselineValueByKey = PreviousBaselineValueByKey;
        ModifiedValueByKey = PreviousModifiedValueByKey;

        if (SpawnedActor && IsValid(SpawnedActor))
        {
            World->DestroyActor(SpawnedActor);
        }

        if (SelectedActor.Get() != PreviousSelectedActor)
        {
            SetSelectedActor(PreviousSelectedActor);
        }
    };

    if (!TestActor)
    {
        AActor* WorldSettingsActor = World->GetWorldSettings();
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Candidate = *It;
            if (!Candidate || Candidate == WorldSettingsActor)
            {
                continue;
            }

            TestActor = Candidate;
            break;
        }
    }

    if (!TestActor)
    {
        SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 160.f)));
        TestActor = SpawnedActor;
    }

    if (!TestActor)
    {
        OutReport = TEXT("RuntimeRoleCompareSelfTest=FAIL | No runtime actor available");
        return false;
    }

    if (SelectedActor.Get() != TestActor)
    {
        SetSelectedActor(TestActor);
    }

    ClearModified();
    CaptureBaselineForSelection(true);

    FProperty* Property = TestActor->GetClass()->FindPropertyByName(TEXT("InitialLifeSpan"));
    if (!Property)
    {
        RestoreState();
        OutReport = TEXT("RuntimeRoleCompareSelfTest=FAIL | Missing InitialLifeSpan");
        return false;
    }

    {
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(TestActor);
        Property->ExportTextItem_Direct(OriginalValueText, ValuePtr, nullptr, nullptr, PPF_None);
        bOriginalValueCaptured = true;
    }

    const FString PatchedValueText = OriginalValueText == TEXT("0.150000") ? TEXT("0.250000") : TEXT("0.150000");
    FString ApplyError;
    const bool bApplyOk = ApplyPropertyTextImmediate(TestActor, TEXT("InitialLifeSpan"), PatchedValueText, ApplyError);

    FString StageError;
    const bool bStageOk = bApplyOk && StageSelectionAsPatch(StageError);

    FRIRuntimeRoleCompareReport CompareReport;
    FString CompareError;
    const bool bCompareOk = bStageOk && CompareRuntimeRoles(CompareReport, CompareError);

    auto FindRoleState = [&](const FRIRuntimeRoleCompareLine& Line, ERIRuntimeCompareRole Role) -> const FRIRuntimeRoleFieldState*
    {
        for (const FRIRuntimeRoleFieldState& State : Line.RoleStates)
        {
            if (State.Role == Role)
            {
                return &State;
            }
        }
        return nullptr;
    };

    auto FindVerificationState = [&](const FRIRuntimeRoleVerificationLine& Line, ERIRuntimeCompareRole Role) -> const FRIRuntimeRoleVerificationState*
    {
        for (const FRIRuntimeRoleVerificationState& State : Line.RoleStates)
        {
            if (State.Role == Role)
            {
                return &State;
            }
        }
        return nullptr;
    };

    const FRIRuntimeRoleCompareLine* FirstLine = CompareReport.Lines.Num() > 0 ? &CompareReport.Lines[0] : nullptr;
    const FRIRuntimeRoleVerificationLine* FirstVerificationLine = CompareReport.VerificationLines.Num() > 0 ? &CompareReport.VerificationLines[0] : nullptr;
    const FRIRuntimeRoleFieldState* AuthorityState = FirstLine ? FindRoleState(*FirstLine, ERIRuntimeCompareRole::Authority) : nullptr;
    const FRIRuntimeRoleFieldState* AutonomousState = FirstLine ? FindRoleState(*FirstLine, ERIRuntimeCompareRole::AutonomousProxy) : nullptr;
    const FRIRuntimeRoleFieldState* SimulatedState = FirstLine ? FindRoleState(*FirstLine, ERIRuntimeCompareRole::SimulatedProxy) : nullptr;
    const FRIRuntimeRoleVerificationState* AvailableVerificationState = nullptr;
    if (FirstVerificationLine)
    {
        for (const FRIRuntimeRoleVerificationState& State : FirstVerificationLine->RoleStates)
        {
            if (State.bRoleAvailable)
            {
                AvailableVerificationState = &State;
                break;
            }
        }
    }

    const int32 UnavailableStateCount = FirstLine
        ? static_cast<int32>(FirstLine->RoleStates.FilterByPredicate([](const FRIRuntimeRoleFieldState& State) { return !State.bRoleAvailable; }).Num())
        : 0;

    const bool bReportOk = bCompareOk
        && CompareReport.ActorPath == TestActor->GetPathName()
        && !CompareReport.AvailableRoleLabel.IsEmpty()
        && CompareReport.ComparedLineCount > 0
        && FirstLine != nullptr
        && FirstLine->RoleStates.Num() == RI_GetRuntimeCompareRoles().Num()
        && FirstVerificationLine != nullptr
        && AuthorityState != nullptr
        && AutonomousState != nullptr
        && SimulatedState != nullptr
        && UnavailableStateCount >= 2
        && AvailableVerificationState != nullptr
        && AvailableVerificationState->bExecuted
        && AvailableVerificationState->bPassed;

    OutReport = FString::Printf(
        TEXT("RuntimeRoleCompareSelfTest=%s | Apply=%s Stage=%s Compare=%s AvailableRole=%s Lines=%d Missing=%d Verify=%d Authority=%s Autonomous=%s Simulated=%s"),
        bReportOk ? TEXT("PASS") : TEXT("FAIL"),
        bApplyOk ? TEXT("ok") : (ApplyError.IsEmpty() ? TEXT("fail") : *ApplyError),
        bStageOk ? TEXT("ok") : (StageError.IsEmpty() ? TEXT("fail") : *StageError),
        bCompareOk ? TEXT("ok") : (CompareError.IsEmpty() ? TEXT("fail") : *CompareError),
        CompareReport.AvailableRoleLabel.IsEmpty() ? TEXT("None") : *CompareReport.AvailableRoleLabel,
        CompareReport.ComparedLineCount,
        CompareReport.MissingRoleCount,
        CompareReport.VerificationMismatchCount,
        AuthorityState ? (AuthorityState->bRoleAvailable ? *AuthorityState->ValueText : TEXT("missing")) : TEXT("missing"),
        AutonomousState ? (AutonomousState->bRoleAvailable ? *AutonomousState->ValueText : TEXT("missing")) : TEXT("missing"),
        SimulatedState ? (SimulatedState->bRoleAvailable ? *SimulatedState->ValueText : TEXT("missing")) : TEXT("missing"));

    RestoreState();
    return bReportOk;
#endif
}

FString UInspectorWorldSubsystem::RunRuntimeRoleCompareSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRuntimeRoleCompareSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::PickActorUnderCursor()
{
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    if (!PC) return false;

    // 你也可以改成 ECC_Camera / 自定义 TraceChannel
    FHitResult Hit;
    const bool bHit = PC->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        /*bTraceComplex=*/ true,
        Hit
    );

    AActor* HitActor = Hit.GetActor();
    if (!bHit || !HitActor) return false;

    // ✅ 这里要复用你现有“选中一个Actor后”的逻辑
    // 常见是：SelectedActor = HitActor; Refresh/RequestRebuild/OnInspectorRefreshEx 等
    // 我不写伪代码：你项目里一般是走 SetSelectedActor / SetSelectedObject 这类函数
    // 直接调用你已有的“设置选中”的函数即可，例如：
    SetSelectedActor(HitActor);   // <- 用你项目里真实存在的函数名替换/对应
    return true;
#else
    return false;
#endif
}


void UInspectorWorldSubsystem::PickActorInView()
{
    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("Test:::::::PickActorInView"));
#if RUNTIME_INSPECTOR_ENABLED
    if (!bOpen) return;

    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    // 1) 拿鼠标在 viewport 内的坐标（像素）
    float MouseX = 0.f;
    float MouseY = 0.f;
    if (!PC->GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    // 2) 反投影到世界射线
    FVector WorldOrigin;
    FVector WorldDir;
    if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDir))
    {
        return;
    }

    const FVector Start = WorldOrigin;
    const FVector End = Start + WorldDir * 100000.f;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RuntimeInspectorPick), true);
    Params.bReturnPhysicalMaterial = false;

    // 可选：避免点到自己控制的Pawn/Character（如果你想）
    // if (APawn* Pawn = PC->GetPawn()) { Params.AddIgnoredActor(Pawn); }

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            SetSelectedActor(HitActor);
        }
    }
#endif
}


//void UInspectorWorldSubsystem::PickActorInView()
//{
//    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("Test:::::::PickActorInView"));
//#if RUNTIME_INSPECTOR_ENABLED
//    if (!bOpen) return;
//
//    APlayerController* PC = GetLocalPC();
//    if (!PC) return;
//
//    FVector CamLoc;
//    FRotator CamRot;
//    PC->GetPlayerViewPoint(CamLoc, CamRot);
//
//    const FVector Start = CamLoc;
//    const FVector End = Start + CamRot.Vector() * 100000.f;
//
//    FHitResult Hit;
//    FCollisionQueryParams Params(SCENE_QUERY_STAT(RuntimeInspectorPick), true);
//    Params.bReturnPhysicalMaterial = false;
//
//    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
//    {
//        if (AActor* HitActor = Hit.GetActor())
//        {
//            SetSelectedActor(HitActor);
//        }
//    }
//#endif
//}

void UInspectorWorldSubsystem::SetSelectedActor(AActor* NewActor)
{
#if RUNTIME_INSPECTOR_ENABLED
    AActor* Prev = SelectedActor.Get();
    if (Prev == NewActor)
    {
        return;
    }

    // 1) Unbind + clear visuals for previous actor
    if (Prev)
    {
        SetActorOutline(Prev, false);
    }
    OutlinedActor.Reset();
    UnbindFromSelectedActor();

    // 2) Set new selection + bind
    SelectedActor = NewActor;
    SelectedInspectObject = NewActor;
    SelectedMaterialSlotIndex = INDEX_NONE;
    SelectedGroupKey.Reset();
    BindToSelectedActor(NewActor);

    if (NewActor)
    {
        OutlinedActor = NewActor;
        if (const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>())
        {
            if (Settings->bEnableOutlinePP)
            {
                SetActorOutline(NewActor, true, /*StencilValue=*/ 1);
            }
        }
    }

#if RUNTIME_INSPECTOR_ENABLED
    // Selection changed: exit MaterialOnly view to avoid stale component/slot pointers.
    PropertyViewMode = ERIPropertyViewMode::Full;
    ViewMeshComp = nullptr;
    ViewMaterialSlot = INDEX_NONE;
#endif

    // 切换选中对象：清理 ItemPool，避免旧 Items 残留
    ClearItemPool();
    // New session baseline for this selection
    CaptureBaselineForSelection(/*bIncludeMaterialParams=*/ true);
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
#endif
}


void UInspectorWorldSubsystem::RefreshPanel()
{
#if RUNTIME_INSPECTOR_ENABLED
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);
#endif
}

void UInspectorWorldSubsystem::RequestActorPageRefresh()
{
#if RUNTIME_INSPECTOR_ENABLED
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
#endif
}

void UInspectorWorldSubsystem::RefreshPanel(EInspectorRefreshReason Reason)
{
#if RUNTIME_INSPECTOR_ENABLED
    BindActorSearchBox();
    CacheActorPageSearchTextFromPanel();
    UpdateSharedContextStrip();
    if (UUserWidget* W = PanelWidget.Get())
    {
        RI_EnsureInspectBodyLayout(W);
    }
    EnsureActorGroupsSectionInjected();
    EnsureActorPropertiesSectionInjected();
    EnsureActorFunctionsSectionInjected();
    UpdateActorWorkspaceSelectionBand();
    if (UUserWidget* W = PanelWidget.Get())
    {
        // 推荐：WBP 实现 OnInspectorRefreshEx(Reason)
        static const FName RefreshExName(TEXT("OnInspectorRefreshEx"));
        if (UFunction* Fn = W->GetClass()->FindFunctionByName(RefreshExName))
        {
            struct FParams
            {
                EInspectorRefreshReason Reason;
            };
            FParams Params{ Reason };
            W->ProcessEvent(Fn, &Params);
            RI_EnsureInspectBodyLayout(W);
            RefreshActorGroupsSection();
            UpdateActorWorkspaceSelectionBand();
            EnsureActorPropertiesSectionInjected();
            RefreshActorPropertiesSection();
            RefreshActorFunctionsSection();
            RI_ApplyLegacyActorHeaderVisibilityFix(W);
            RI_UpdateActorPropertyHeader(W, GetFocusedInspectObject());
            return;
        }

        // 兼容旧：OnInspectorRefresh()
        static const FName RefreshName(TEXT("OnInspectorRefresh"));
        if (UFunction* FnOld = W->GetClass()->FindFunctionByName(RefreshName))
        {
            W->ProcessEvent(FnOld, nullptr);
        }

        RI_EnsureInspectBodyLayout(W);
        RefreshActorGroupsSection();
        UpdateActorWorkspaceSelectionBand();
        EnsureActorPropertiesSectionInjected();
        RefreshActorPropertiesSection();
        RefreshActorFunctionsSection();
        RI_ApplyLegacyActorHeaderVisibilityFix(W);
        RI_UpdateActorPropertyHeader(W, GetFocusedInspectObject());
    }
#endif
}
void UInspectorWorldSubsystem::SetGroupExpanded(const FString& GroupKey, bool bExpanded)
{
#if RUNTIME_INSPECTOR_ENABLED
    GroupExpandedMap.Add(GroupKey, bExpanded);
    if (TObjectPtr<UObject>* Found = ItemPool.Find(GroupKey))
    {
        if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(*Found))
        {
            GroupItem->bExpanded = bExpanded;
        }
    }
#endif
}
void UInspectorWorldSubsystem::GetGroupTreeRootsForSelected(const FString& SearchText, TArray<UObject*>& OutRoots)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutRoots.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    const bool bSearchMode = !SearchText.IsEmpty();

    UInspectorGroupItem* ActorGroup = GetOrCreateGroupItem(TEXT("ROOT_ACTOR"));
    ActorGroup->Kind = EInspectorGroupKind::RootActor;
    ActorGroup->DisplayName = TEXT("Actor");
    ActorGroup->StableKey = TEXT("ROOT_ACTOR");
    ActorGroup->Depth = 0;
    ActorGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(ActorGroup->StableKey, true);
    OutRoots.Add(ActorGroup);

    UInspectorGroupItem* CompRoot = GetOrCreateGroupItem(TEXT("ROOT_COMPONENTS"));
    CompRoot->Kind = EInspectorGroupKind::RootComponents;
    CompRoot->DisplayName = TEXT("Components");
    CompRoot->StableKey = TEXT("ROOT_COMPONENTS");
    CompRoot->Depth = 0;
    CompRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(CompRoot->StableKey, true);
    OutRoots.Add(CompRoot);

    UInspectorGroupItem* PinnedRoot = GetOrCreateGroupItem(TEXT("PINNED_ROOT"));
    PinnedRoot->Kind = EInspectorGroupKind::RootActor;
    PinnedRoot->DisplayName = TEXT("Star");
    PinnedRoot->StableKey = TEXT("PINNED_ROOT");
    PinnedRoot->Depth = 0;
    PinnedRoot->bExpanded = true;
    OutRoots.Add(PinnedRoot);
#endif
}



static bool RI_Match(const FString& Haystack, const FString& Needle)
{
    return Needle.IsEmpty() || Haystack.Contains(Needle, ESearchCase::IgnoreCase);
}

static bool RI_IsPinnedRootKey(const FString& StableKey)
{
    return StableKey == TEXT("PINNED_ROOT")
        || StableKey == TEXT("ROOT_PINNED")
        || StableKey == TEXT("ROOT_STAR");
}

static bool RI_IsInspectableVisibleProperty(const FProperty* Prop)
{
    if (!Prop || !InspectorPropertyUtils::IsDisplayableProperty(Prop))
    {
        return false;
    }

    return Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
}

static bool RI_ObjectMatchesAttributeSearch(UObject* TargetObject, const FString& SearchText)
{
    if (!TargetObject || SearchText.IsEmpty())
    {
        return !SearchText.IsEmpty() ? false : true;
    }

    if (RI_Match(TargetObject->GetName(), SearchText)
        || RI_Match(TargetObject->GetClass()->GetName(), SearchText))
    {
        return true;
    }

    UClass* Class = TargetObject->GetClass();
    if (!Class)
    {
        return false;
    }

    for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        const FProperty* Prop = *It;
        if (!RI_IsInspectableVisibleProperty(Prop))
        {
            continue;
        }

        const FString RawName = Prop->GetName();
        const FString DisplayName = RI_GetPropertyDisplayNameRuntimeSafe(Prop);
        if (RI_Match(RawName, SearchText) || RI_Match(DisplayName, SearchText))
        {
            return true;
        }
    }

    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        UFunction* Function = *It;
        if (!Function)
        {
            continue;
        }

        if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable)
            || Function->HasAnyFunctionFlags(FUNC_Static | FUNC_Delegate | FUNC_MulticastDelegate | FUNC_BlueprintPure))
        {
            continue;
        }

        const UClass* OwnerClass = Function->GetOwnerClass();
        if (!RI_IsUserAuthoredFunctionOwnerClass(Class, OwnerClass))
        {
            continue;
        }

        const FString DisplayName = RI_GetFunctionDisplayNameRuntimeSafe(Function);
        const FString RawName = Function->GetName();
        if (RI_Match(DisplayName, SearchText) || RI_Match(RawName, SearchText))
        {
            return true;
        }
    }

    return false;
}

static bool RI_MaterialSlotMatchesAttributeSearch(UStaticMeshComponent* MeshComponent, int32 SlotIndex, const FString& SearchText)
{
    if (!MeshComponent || SearchText.IsEmpty() || SlotIndex < 0)
    {
        return !SearchText.IsEmpty() ? false : true;
    }

    UMaterialInterface* Material = MeshComponent->GetMaterial(SlotIndex);
    const FString SlotLabel = FString::Printf(TEXT("Element %d: %s"), SlotIndex, *GetNameSafe(Material));
    if (RI_Match(SlotLabel, SearchText))
    {
        return true;
    }

    if (!Material)
    {
        return false;
    }

    TArray<FMaterialParameterInfo> ParameterInfos;
    TArray<FGuid> ParameterIds;
    Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
    for (const FMaterialParameterInfo& Info : ParameterInfos)
    {
        if (RI_Match(Info.Name.ToString(), SearchText))
        {
            return true;
        }
    }

    ParameterInfos.Reset();
    ParameterIds.Reset();
    Material->GetAllVectorParameterInfo(ParameterInfos, ParameterIds);
    for (const FMaterialParameterInfo& Info : ParameterInfos)
    {
        if (RI_Match(Info.Name.ToString(), SearchText))
        {
            return true;
        }
    }

    return false;
}

static bool RI_ComponentMatchesAttributeSearch(UActorComponent* Component, const FString& SearchText)
{
    if (!Component || SearchText.IsEmpty())
    {
        return !SearchText.IsEmpty() ? false : true;
    }

    if (RI_Match(Component->GetName(), SearchText)
        || RI_Match(Component->GetClass()->GetName(), SearchText)
        || RI_ObjectMatchesAttributeSearch(Component, SearchText))
    {
        return true;
    }

    if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component))
    {
        const int32 MaterialCount = MeshComponent->GetNumMaterials();
        for (int32 SlotIndex = 0; SlotIndex < MaterialCount; ++SlotIndex)
        {
            if (RI_MaterialSlotMatchesAttributeSearch(MeshComponent, SlotIndex, SearchText))
            {
                return true;
            }
        }
    }

    return false;
}


void UInspectorWorldSubsystem::GetGroupTreeChildrenForItem(
    UInspectorGroupItem* Parent,
    const FString& SearchText,
    TArray<UObject*>& OutChildren)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutChildren.Reset();
    if (!Parent) return;

    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] GetTreeChildren ParentKey=%s Target=%s"),
        *Parent->StableKey, *GetNameSafe(Parent->TargetObject));

    const bool bSearchMode = !SearchText.IsEmpty();
    AActor* ActorPtr = SelectedActor.Get();
    // Actor 根：不需要子节点（你现在就是点 Actor 显示右侧属性）
    if (Parent->StableKey == TEXT("ROOT_ACTOR"))
    {
        return;
    }
    // ✅ 叶子：Slot 没孩子
    if (Parent->IsMaterialSlot())
    {
        return;
    }
    // ✅ MaterialsRoot：返回 Slot 子节点
    if (Parent->IsMaterialsRoot())
    {
        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject);
        if (!SMC) return;

        const int32 NumMats = SMC->GetNumMaterials();
        for (int32 Slot = 0; Slot < NumMats; ++Slot)
        {
            UMaterialInterface* Mat = SMC->GetMaterial(Slot);
            if (!Mat) continue;

            if (bSearchMode && !RI_MaterialSlotMatchesAttributeSearch(SMC, Slot, SearchText))
            {
                continue;
            }

            const FString SlotKey = Parent->StableKey + FString::Printf(TEXT(":MAT:%d"), Slot);

            UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
            SlotGroup->Kind = EInspectorGroupKind::Component;  // ✅ 保持你旧结构不变
            SlotGroup->TargetObject = SMC;
            SlotGroup->Depth= Parent->Depth+1;
            SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
            SlotGroup->StableKey = SlotKey;
            SlotGroup->MaterialSlotIndex = Slot;
            SlotGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(SlotKey, false);

            OutChildren.Add(SlotGroup);

            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] -> children=%d"), OutChildren.Num());
        }
        return;
    }
    // ✅ 普通组件节点：如果 TargetObject 是 StaticMeshComponent，则挂一个 MaterialsRoot
    if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject))
    {
        bool bShowMaterials = !bSearchMode || RI_Match(TEXT("Materials"), SearchText);
        if (!bShowMaterials)
        {
            const int32 NumMats = SMC->GetNumMaterials();
            for (int32 Slot = 0; Slot < NumMats; ++Slot)
            {
                if (RI_MaterialSlotMatchesAttributeSearch(SMC, Slot, SearchText))
                {
                    bShowMaterials = true;
                    break;
                }
            }
        }

        if (!bShowMaterials)
        {
            return;
        }

        const FString MatRootKey = Parent->StableKey + TEXT(":MATERIALS");

        UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
        MatRoot->Kind = EInspectorGroupKind::Component;       // ✅ 保持你旧结构不变
        MatRoot->TargetObject = SMC;                          // ✅ 关键：点击能拿到 InComp
        MatRoot->DisplayName = TEXT("Materials");
        MatRoot->StableKey = MatRootKey;
        MatRoot->Depth = Parent->Depth + 1;
        MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRootKey, false);
        OutChildren.Add(MatRoot);

        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] -> children=%d"), OutChildren.Num());
        return;
    }

    // Components 根：返回组件
    if (Parent->StableKey == TEXT("ROOT_COMPONENTS"))
    {
        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        for (UActorComponent* Comp : Components)
        {
            if (!Comp || (bSearchMode && !RI_ComponentMatchesAttributeSearch(Comp, SearchText)))
            {
                continue;
            }

            const FString CompKey = MakeComponentKey(ActorPtr, Comp);

            UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
            CompGroup->Kind = EInspectorGroupKind::Component;
            CompGroup->TargetObject = Comp;
            CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
            CompGroup->StableKey = CompKey;
            CompGroup->Depth = Parent->Depth + 1;
            CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);

            OutChildren.Add(CompGroup);
        }
        return;
    }
       

    // 2) 如果 Parent 是一个“真实组件节点”，并且它是 StaticMeshComponent -> 返回一个 MaterialsRoot
    //    注意：这里要排除 Parent 自己就是 MaterialsRoot/Slot 的情况（靠 StableKey 判断）
    //const bool bIsMaterialsRoot = Parent->StableKey.EndsWith(TEXT(":MATERIALS"));
    //const bool bIsMatSlot = Parent->StableKey.Contains(TEXT(":MATERIALS:MAT:"));
    //if (!bIsMaterialsRoot && !bIsMatSlot)
    //{
    //    if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject))
    //    {
    //        const FString MatRootKey = Parent->StableKey + TEXT(":MATERIALS");

    //        UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
    //        MatRoot->Kind = EInspectorGroupKind::Component;      // ✅ 沿用你原逻辑
    //        MatRoot->TargetObject = SMC;                         // ✅ 关键：UI 点击仍能拿到 InComp
    //        MatRoot->DisplayName = TEXT("Materials");
    //        MatRoot->StableKey = MatRootKey;

    //        // TreeView 的展开由 UI 控制；这里仅用于“初始展开状态”
    //        MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRootKey, false);

    //        OutChildren.Add(MatRoot);
    //    }
    //    return;
    //}

    //// 3) Parent 是 MaterialsRoot：返回 slots
    //if (bIsMaterialsRoot)
    //{
    //    UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject);
    //    if (!SMC) return;

    //    const int32 NumMats = SMC->GetNumMaterials();
    //    for (int32 Slot = 0; Slot < NumMats; ++Slot)
    //    {
    //        UMaterialInterface* Mat = SMC->GetMaterial(Slot);
    //        if (!Mat) continue;

    //        const FString SlotKey = Parent->StableKey + FString::Printf(TEXT(":MAT:%d"), Slot);

    //        UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
    //        SlotGroup->Kind = EInspectorGroupKind::Component;    // ✅ 沿用你原逻辑
    //        SlotGroup->TargetObject = SMC;
    //        SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
    //        SlotGroup->StableKey = SlotKey;
    //        SlotGroup->MaterialSlotIndex = Slot;

    //        // slot 通常是叶子；bExpanded 你留着也不影响
    //        SlotGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(SlotKey, false);

    //        OutChildren.Add(SlotGroup);
    //    }
    //    return;
    //}

    // 4) Parent 是 Slot：叶子节点，无 children
#endif
}

//void UInspectorWorldSubsystem::GetGroupTreeChildrenForItem(
//    UInspectorGroupItem* Parent, const FString& SearchText, TArray<UObject*>& OutChildren)
//{
//#if RUNTIME_INSPECTOR_ENABLED
//    OutChildren.Reset();
//    AActor* ActorPtr = SelectedActor.Get();
//    if (!ActorPtr || !Parent) return;
//
//    const bool bSearchMode = !SearchText.IsEmpty();
//
//    // Actor 根：不需要子节点（你现在就是点 Actor 显示右侧属性）
//    if (Parent->StableKey == TEXT("ROOT_ACTOR"))
//    {
//        return;
//    }
//
//    // Components 根：返回组件
//    if (Parent->StableKey == TEXT("ROOT_COMPONENTS"))
//    {
//        TArray<UActorComponent*> Components;
//        ActorPtr->GetComponents(Components);
//
//        for (UActorComponent* Comp : Components)
//        {
//            if (!IsWhitelistedComponent(Comp)) continue;
//
//            const FString CompKey = MakeComponentKey(ActorPtr, Comp);
//
//            UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
//            CompGroup->Kind = EInspectorGroupKind::Component;
//            CompGroup->TargetObject = Comp;
//            CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
//            CompGroup->StableKey = CompKey;
//            CompGroup->Depth = Parent->Depth + 1;
//            CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);
//
//            OutChildren.Add(CompGroup);
//        }
//        return;
//    }
//
//    // 组件节点：如果是 StaticMeshComponent，挂一个 MaterialsRoot
//    if (Parent->Kind == EInspectorGroupKind::Component)
//    {
//        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject);
//        if (!SMC) return;
//
//        const FString MatRootKey = Parent->StableKey + TEXT(":MATERIALS");
//
//        UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
//        MatRoot->Kind = EInspectorGroupKind::MaterialsRoot;
//        MatRoot->TargetObject = SMC;
//        MatRoot->DisplayName = TEXT("Materials");
//        MatRoot->StableKey = MatRootKey;
//        MatRoot->Depth = Parent->Depth + 1;
//        MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRootKey, false);
//
//        OutChildren.Add(MatRoot);
//        return;
//    }
//
//    // MaterialsRoot：挂 slots
//    if (Parent->Kind == EInspectorGroupKind::MaterialsRoot || Parent->IsMaterialsRoot())
//    {
//        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Parent->TargetObject);
//        if (!SMC) return;
//
//        const int32 NumMats = SMC->GetNumMaterials();
//        for (int32 Slot = 0; Slot < NumMats; ++Slot)
//        {
//            UMaterialInterface* Mat = SMC->GetMaterial(Slot);
//            if (!Mat) continue;
//
//            const FString SlotKey = Parent->StableKey + FString::Printf(TEXT(":MAT:%d"), Slot);
//
//            UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
//            SlotGroup->Kind = EInspectorGroupKind::MaterialSlot;
//            SlotGroup->TargetObject = SMC;
//            SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
//            SlotGroup->StableKey = SlotKey;
//            SlotGroup->MaterialSlotIndex = Slot;
//            SlotGroup->Depth = Parent->Depth + 1;
//
//            OutChildren.Add(SlotGroup);
//        }
//        return;
//    }
//#endif
//}


void UInspectorWorldSubsystem::GetGroupItemsForSelected(const FString& SearchText, TArray<UObject*>& OutGroups)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutGroups.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    const bool bSearchMode = !SearchText.IsEmpty();

    // Actor 根组
    UInspectorGroupItem* ActorGroup = GetOrCreateGroupItem(TEXT("ROOT_ACTOR"));
    ActorGroup->Kind = EInspectorGroupKind::RootActor;
    ActorGroup->DisplayName = TEXT("Actor");
    ActorGroup->StableKey = TEXT("ROOT_ACTOR");
    ActorGroup->Depth = 0;
    ActorGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(ActorGroup->StableKey, true);
    OutGroups.Add(ActorGroup);

    // Components 根组
    UInspectorGroupItem* CompRoot = GetOrCreateGroupItem(TEXT("ROOT_COMPONENTS"));
    CompRoot->Kind = EInspectorGroupKind::RootComponents;
    CompRoot->DisplayName = TEXT("Components");
    CompRoot->StableKey = TEXT("ROOT_COMPONENTS");
    CompRoot->Depth = 0;
    CompRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(CompRoot->StableKey, true);
    OutGroups.Add(CompRoot);

    if (!CompRoot->bExpanded) return;

    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);

    for (UActorComponent* Comp : Components)
    {
        if (!Comp || (bSearchMode && !RI_ComponentMatchesAttributeSearch(Comp, SearchText)))
        {
            continue;
        }

        const FString CompKey = MakeComponentKey(ActorPtr, Comp);

        UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
        CompGroup->Kind = EInspectorGroupKind::Component;
        CompGroup->TargetObject = Comp;
        CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
        CompGroup->StableKey = CompKey;
        CompGroup->Depth = 1;
        CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);
        OutGroups.Add(CompGroup);

        // Materials 结构（只加 Group，不加参数）
        if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
        {
            const FString MatRootKey = CompKey + TEXT(":MATERIALS");

            UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
            MatRoot->Kind = EInspectorGroupKind::Component;
            MatRoot->TargetObject = SMC; // ✅ 关键：让 UI 点击能拿到 InComp
            MatRoot->DisplayName = TEXT("Materials");
            MatRoot->StableKey = MatRootKey;
            MatRoot->Depth = 2;
            MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRootKey, false);
            OutGroups.Add(MatRoot);

            if (MatRoot->bExpanded)
            {
                const int32 NumMats = SMC->GetNumMaterials();
                for (int32 Slot = 0; Slot < NumMats; ++Slot)
                {
                    UMaterialInterface* Mat = SMC->GetMaterial(Slot);
                    if (!Mat) continue;
                    if (bSearchMode && !RI_MaterialSlotMatchesAttributeSearch(SMC, Slot, SearchText))
                    {
                        continue;
                    }

                    const FString SlotKey = MatRootKey + FString::Printf(TEXT(":MAT:%d"), Slot);

                    
                    UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
                    SlotGroup->Kind = EInspectorGroupKind::Component;
                    SlotGroup->TargetObject = SMC;
                    SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
                    SlotGroup->StableKey = SlotKey;
                    SlotGroup->MaterialSlotIndex = Slot;
                    SlotGroup->Depth = 3;
                    // Slot 索引你如果还没加字段，就先靠 StableKey 解析（见下）
                    SlotGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(SlotKey, false);
                    OutGroups.Add(SlotGroup);
                }
            }
        }
    }

    UInspectorGroupItem* PinnedRoot = GetOrCreateGroupItem(TEXT("PINNED_ROOT"));
    PinnedRoot->Kind = EInspectorGroupKind::RootActor;
    PinnedRoot->DisplayName = TEXT("Star");
    PinnedRoot->StableKey = TEXT("PINNED_ROOT");
    PinnedRoot->Depth = 0;
    PinnedRoot->bExpanded = true;
    OutGroups.Add(PinnedRoot);
#endif
}


void UInspectorWorldSubsystem::GetPropertyItemsForSelected(const FString& SearchText, TArray<UObject*>& OutItems)
{

#if RUNTIME_INSPECTOR_ENABLED


   
    OutItems.Reset();

    AActor* TargetActor = SelectedActor.Get();
    UObject* FocusedObject = SelectedInspectObject ? SelectedInspectObject.Get() : nullptr;

    if (!TargetActor) return;

    const bool bSearchMode = !SearchText.IsEmpty();



    if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
    {
        UMeshComponent* MC = ViewMeshComp.Get();
        if (!MC || ViewMaterialSlot == INDEX_NONE)
        {
            return;
        }

        // 可选：加一个小标题组，让右侧有层级（也可以不加）
        UInspectorGroupItem* MatOnlyGroup = GetOrCreateGroupItem(TEXT("VIEW_MATERIAL_ONLY"));
        MatOnlyGroup->Kind = EInspectorGroupKind::Component;
        MatOnlyGroup->DisplayName = TEXT("Material Parameters");
        MatOnlyGroup->StableKey = TEXT("VIEW_MATERIAL_ONLY");
        MatOnlyGroup->bExpanded = true;
        OutItems.Add(MatOnlyGroup);

        // 只支持 StaticMeshComponent 先（你当前就是这样做的）
        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MC);
        if (!SMC) return;

        UMaterialInterface* Mat = SMC->GetMaterial(ViewMaterialSlot);
        if (!Mat) return;

        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;

        // Scalar
        Infos.Reset(); Ids.Reset();
        Mat->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Scalar);
            OutItems.Add(Item);
        }

        // Vector
        Infos.Reset(); Ids.Reset();
        Mat->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Vector);
            OutItems.Add(Item);
        }
        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("Mode=MaterialOnly, Items=X"));
        return; // ✅ 关键：直接 return，阻止后面 Append 父级组件属性
    }

    if (RI_IsPinnedRootKey(SelectedGroupKey))
    {
        GetPinnedItemsForSelected(SearchText, OutItems);
        return;
    }

    UObject* EffectiveObject = TargetActor;
    FString OwnerPrefix;

    if (FocusedObject && FocusedObject != TargetActor)
    {
        EffectiveObject = FocusedObject;
        OwnerPrefix = FocusedObject->GetName();
    }

    AppendPropertiesForObject(EffectiveObject, SearchText, OutItems, OwnerPrefix, bSearchMode);

#endif

}

void UInspectorWorldSubsystem::GetPinnedItemsForSelected(const FString& SearchText, TArray<UObject*>& OutPinnedItems)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutPinnedItems.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    // 预取组件
    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);

    // 稳定顺序（避免 UI 抖动）
    TArray<FString> Keys = FavoriteKeys.Array();
    Keys.Sort();

    const bool bSearchMode = !SearchText.IsEmpty();

    for (const FString& K : Keys)
    {
        if (K.IsEmpty()) continue;

        // 如果你不希望 pinned 被搜索框影响，可以直接注释掉这一段
        if (bSearchMode && !K.Contains(SearchText)) continue;

       
        // ---------- 新格式：M|ActorPath|CompPath|Slot|TypeInt|ParamName ----------
        if (K.Contains(TEXT("|")))
        {
            TArray<FString> Parts;
            K.ParseIntoArray(Parts, TEXT("|"), false);

            if (Parts.Num() > 0 && Parts[0] == TEXT("M"))
            {
                if (Parts.Num() < 6)
                {
                    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] M key malformed: %s"), *K);
                    continue;
                }

                const FString& ActorPath = Parts[1];
                const FString& CompPath = Parts[2];
                const int32 Slot = FCString::Atoi(*Parts[3]);
                const int32 TypeInt = FCString::Atoi(*Parts[4]);
                const FName  ParamName(*Parts[5]);

                if (ParamName.IsNone())
                {
                    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] M key ParamName none: %s"), *K);
                    continue;
                }

                // 只接受当前选中 Actor 的 pinned
                if (ActorPtr->GetPathName() != ActorPath)
                {
                    continue;
                }

                // 1) 先用完整 PathName 在当前 Actor 组件里匹配（最准确）
                UStaticMeshComponent* SMC = nullptr;
                for (UActorComponent* Comp : Components)
                {
                    if (!Comp) continue;

                    if (Comp->GetPathName() == CompPath)
                    {
                        SMC = Cast<UStaticMeshComponent>(Comp);
                        break;
                    }
                }

                // 2) PathName 匹配不到就退化：按组件对象名匹配（解决 PIE/Undo 导致 Path 变化）
                if (!SMC)
                {
                    FString CompName = CompPath;
                    int32 DotPos = INDEX_NONE;
                    if (CompName.FindLastChar(TEXT('.'), DotPos))
                    {
                        CompName = CompName.Mid(DotPos + 1);
                    }

                    for (UActorComponent* Comp : Components)
                    {
                        if (!Comp) continue;
                        if (Comp->GetName() == CompName)
                        {
                            SMC = Cast<UStaticMeshComponent>(Comp);
                            if (SMC) break;
                        }
                    }
                }

                if (!SMC)
                {
                    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] M resolve FAILED. CompPath=%s  Key=%s"), *CompPath, *K);
                    continue;
                }

                // ✅ 复用池对象 + 再 Init 一次，确保绑定正确
                UInspectorMaterialParamItem* MatItem =
                    GetOrCreateMaterialItem(SMC, Slot, ParamName, (EInspectorMatParamType)TypeInt);

                if (!MatItem)
                {
                    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] M GetOrCreateMaterialItem null. SMC=%s Slot=%d Param=%s Type=%d"),
                        *GetNameSafe(SMC), Slot, *ParamName.ToString(), TypeInt);
                    continue;
                }

                MatItem->Init(SMC, Slot, ParamName, (EInspectorMatParamType)TypeInt);
                OutPinnedItems.Add(MatItem);
                continue;
            }

            if (Parts.Num() > 0 && Parts[0] == TEXT("F"))
            {
                if (Parts.Num() < 4)
                {
                    continue;
                }

                const FString& ActorPath = Parts[1];
                const FString& TargetPath = Parts[2];
                const FName FunctionName(*Parts[3]);
                if (FunctionName.IsNone() || ActorPtr->GetPathName() != ActorPath)
                {
                    continue;
                }

                UObject* TargetObject = nullptr;
                if (ActorPtr->GetPathName() == TargetPath)
                {
                    TargetObject = ActorPtr;
                }
                else
                {
                    FString TargetObjectName = TargetPath;
                    int32 DotPos = INDEX_NONE;
                    if (TargetObjectName.FindLastChar(TEXT('.'), DotPos))
                    {
                        TargetObjectName = TargetObjectName.Mid(DotPos + 1);
                    }

                    for (UActorComponent* Component : Components)
                    {
                        if (!Component)
                        {
                            continue;
                        }

                        if (Component->GetPathName() == TargetPath || Component->GetName() == TargetObjectName)
                        {
                            TargetObject = Component;
                            break;
                        }
                    }
                }

                if (!TargetObject)
                {
                    continue;
                }

                if (UInspectorFunctionItem* FunctionItem = GetOrCreateFunctionItem(TargetObject, FunctionName))
                {
                    UFunction* Function = TargetObject->FindFunction(FunctionName);
                    if (!Function)
                    {
                        continue;
                    }

                    TArray<FRIInspectorFunctionParameterDefinition> ParameterDefinitions;
                    if (!RI_BuildFunctionParameterDefinitions(Function, ParameterDefinitions))
                    {
                        continue;
                    }

                    const FString OwnerLabel = TargetObject == ActorPtr
                        ? TEXT("Actor")
                        : TargetObject->GetName();
                    FunctionItem->SetDisplayMetadata(
                        RI_GetFunctionDisplayNameRuntimeSafe(Function),
                        OwnerLabel,
                        RI_BuildFunctionSignature(Function, ParameterDefinitions),
                        RI_GetFunctionTooltipRuntimeSafe(Function));
                    FunctionItem->SetParameterDefinitions(ParameterDefinitions);
                    OutPinnedItems.Add(FunctionItem);
                }
                continue;
            }

            // 其它带 '|' 的 key（未来扩展用），先跳过
            continue;
        }

        // ---------- 旧格式：C:/Script/Engine.StaticMeshComponent:BoundsScale ----------
        int32 FirstColon = INDEX_NONE;
        if (!K.FindChar(TEXT(':'), FirstColon)) continue;

        const FString Prefix = K.Left(FirstColon);    // "A" or "C"
        const FString Rest = K.Mid(FirstColon + 1); // "/Script/...:Prop"

        int32 SecondColon = INDEX_NONE;
        if (!Rest.FindChar(TEXT(':'), SecondColon)) continue;

        const FString ClassPath = Rest.Left(SecondColon);   // "/Script/Engine.StaticMeshComponent"
        const FString PropStr = Rest.Mid(SecondColon + 1);// "BoundsScale"

        const FName PropName(*PropStr);
        if (PropName.IsNone()) continue;

        if (Prefix == TEXT("A"))
        {
            // 旧格式 A:ClassPath:Prop （如果你有这种 key）
            if (ActorPtr->GetClass()->GetPathName() == ClassPath)
            {
                if (UInspectorPropertyItem* Item = GetOrCreatePropertyItem(ActorPtr, PropName))
                {
                    OutPinnedItems.Add(Item);
                }
            }
            continue;
        }

        if (Prefix == TEXT("C"))
        {
            // ⚠️ 旧 key 没有组件实例信息，只能按“类”找第一个匹配的组件
            UActorComponent* FoundComp = nullptr;

            for (UActorComponent* Comp : Components)
            {
                if (!Comp) continue;

                if (Comp->GetClass()->GetPathName() == ClassPath)
                {
                    FoundComp = Comp;
                    break;
                }
            }

            if (FoundComp)
            {
                if (UInspectorPropertyItem* Item = GetOrCreatePropertyItem(FoundComp, PropName))
                {
                    OutPinnedItems.Add(Item);
                }
            }
            continue;
        }
    }

#endif
}

bool UInspectorWorldSubsystem::CanUndo() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return UndoStack.Num() > 0;
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::CanRedo() const
{
#if RUNTIME_INSPECTOR_ENABLED
    return RedoStack.Num() > 0;
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::RecordChange(const FInspectorChange& Change)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bApplyingHistory || bApplyingColorDialogPreview) return;

    // Track modified state (Only Modified/Snapshot)
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
    {
        const FString Key = MakePropertySnapshotKey(Change.Target.Get(), Change.PropertyName);
        if (!Key.IsEmpty())
        {
            TrackModifiedForKey(Key, Change.OldValueText, Change.NewValueText);
        }
        break;
    }
    case EInspectorChangeType::MaterialScalar:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        const FString Key = MakeMaterialSnapshotKey(Comp, Change.MaterialIndex, EInspectorMatParamType::Scalar, Change.ParamName);
        if (!Key.IsEmpty())
        {
            TrackModifiedForKey(Key, FString::SanitizeFloat(Change.OldScalar), FString::SanitizeFloat(Change.NewScalar));
        }
        break;
    }
    case EInspectorChangeType::MaterialVector:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        const FString Key = MakeMaterialSnapshotKey(Comp, Change.MaterialIndex, EInspectorMatParamType::Vector, Change.ParamName);
        if (!Key.IsEmpty())
        {
            const FString OldText = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Change.OldVector.R, Change.OldVector.G, Change.OldVector.B, Change.OldVector.A);
            const FString NewText = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Change.NewVector.R, Change.NewVector.G, Change.NewVector.B, Change.NewVector.A);
            TrackModifiedForKey(Key, OldText, NewText);
        }
        break;
    }
    default:
        break;
    }

    UndoStack.Add(Change);
    RedoStack.Reset();

    // 可选：打印一条日志
    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("[RI] Record: %s.%s %s -> %s"),
        *Change.DebugObjectName,
        *Change.PropertyName.ToString(),
        *Change.OldValueText,
        *Change.NewValueText);
#endif
}

bool UInspectorWorldSubsystem::ApplyChangeValue(UObject* Target, FName PropName, const FString& TextValue)
{
#if RUNTIME_INSPECTOR_ENABLED
    FString Err;
    const bool bOK = InspectorPropertyUtils::SetValueFromText(Target, PropName, TextValue, &Err);
    if (!bOK)
    {
        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] ApplyChangeValue failed: %s"), *Err);
    }
    return bOK;
#else
    return false;
#endif
}
void UInspectorWorldSubsystem::OnPickKeyPressed()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bOpen) return;

    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    if (Settings)
    {
        if (Settings->bPickKeyRequiresCtrl)
        {
            const bool bCtrl =
                PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);
            if (!bCtrl) return;
        }

        if (Settings->bPickKeyRequiresShift)
        {
            const bool bShift =
                PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
            if (!bShift) return;
        }
    }

    PickActorInView();
#endif
}


bool UInspectorWorldSubsystem::Undo()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UndoStack.Num() == 0) return false;

    FInspectorChange Change = UndoStack.Pop();

    // 按类型校验目标，而不是统一校验 Change.Target
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid() || Change.PropertyName.IsNone())
        {
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Undo property target invalid: %s.%s"),
                *GetNameSafe(Change.Target.Get()), *Change.PropertyName.ToString());
            return false;
        }
        break;

    case EInspectorChangeType::MaterialScalar:
    case EInspectorChangeType::MaterialVector:
        if (!Change.TargetComponent.IsValid() || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone())
        {
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Undo material target invalid: %s slot=%d param=%s"),
                *GetNameSafe(Change.TargetComponent.Get()), Change.MaterialIndex, *Change.ParamName.ToString());
            return false;
        }
        break;

    default:
        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Undo unknown change type"));
        return false;
    }

    bApplyingHistory = true;
    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ false);
    bApplyingHistory = false;

    if (bOK)
    {
        RedoStack.Add(Change);

        // Keep "Only Modified" state in sync after Undo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }
        RefreshPanel(EInspectorRefreshReason::UndoRedo);
    }
    return bOK;
//    if (!CanUndo()) return false;
//
//    FInspectorChange Change = UndoStack.Pop();
//
//   /* if (!Change.Target.IsValid())
//    {
//        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Undo target invalid"));
//        return false;
//    }*/
//
//    bApplyingHistory = true;
// //   const bool bOK = ApplyChangeValue(Change.Target.Get(), Change.PropertyName, Change.OldValueText);
//    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ false);
//    bApplyingHistory = false;
//
//    if (bOK)
//    {
//        RedoStack.Add(Change);
//      //  RefreshPanel(); // 先简单粗暴刷新（后面可做更细的行刷新）
//    }
//    return bOK;
//#else
//    return false;
#endif

    return false;
}

bool UInspectorWorldSubsystem::Redo()
{
#if RUNTIME_INSPECTOR_ENABLED

    if (RedoStack.Num() == 0) return false;

    FInspectorChange Change = RedoStack.Pop();

    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid() || Change.PropertyName.IsNone())
        {
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Redo property target invalid: %s.%s"),
                *GetNameSafe(Change.Target.Get()), *Change.PropertyName.ToString());
            return false;
        }
        break;

    case EInspectorChangeType::MaterialScalar:
    case EInspectorChangeType::MaterialVector:
        if (!Change.TargetComponent.IsValid() || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone())
        {
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Redo material target invalid: %s slot=%d param=%s"),
                *GetNameSafe(Change.TargetComponent.Get()), Change.MaterialIndex, *Change.ParamName.ToString());
            return false;
        }
        break;

    default:
        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Redo unknown change type"));
        return false;
    }

    bApplyingHistory = true;
    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ true);
    bApplyingHistory = false;

    if (bOK)
    {
        UndoStack.Add(Change);

        // Keep "Only Modified" state in sync after Redo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }

        // Keep "Only Modified" state in sync after Redo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }
        RefreshPanel(EInspectorRefreshReason::UndoRedo);
    }
    return bOK;
//    if (!CanRedo()) return false;
//
//    FInspectorChange Change = RedoStack.Pop();
//
// /*   if (!Change.Target.IsValid())
//    {
//        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Redo target invalid"));
//        return false;
//    }*/
//
//    bApplyingHistory = true;
//    //const bool bOK = ApplyChangeValue(Change.Target.Get(), Change.PropertyName, Change.NewValueText);
//    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ true);
//    bApplyingHistory = false;
//
//    if (bOK)
//    {
//        UndoStack.Add(Change);
//     //   RefreshPanel();
//    }
//    return bOK;
//#else
//    return false;
#endif

    return false;
}

void UInspectorWorldSubsystem::OnUndoKeyPressed()
{
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    const bool bCtrl =
        PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);

    const bool bShift =
        PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);

    if (!bCtrl) return;

    // Ctrl+Z 或 Ctrl+Shift+Z 都做 Undo（你也可以把 Ctrl+Shift+Z 设为 Redo）
    Undo();
#endif
}

void UInspectorWorldSubsystem::OnRedoKeyPressed()
{
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    const bool bCtrl =
        PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);

    if (!bCtrl) return;

    // Ctrl+Y -> Redo
    Redo();
#endif
}

void UInspectorWorldSubsystem::RegisterInputProcessor()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (InputProcessor.IsValid()) return;
    if (!FSlateApplication::IsInitialized()) return;

    // 可选：只在 PIE/Game world 里启用，避免编辑器世界误注册
    if (UWorld* W = GetWorld())
    {
        if (!(W->WorldType == EWorldType::PIE || W->WorldType == EWorldType::Game))
        {
            return;
        }
    }

    InputProcessor = MakeShared<FRuntimeInspectorInputProcessor>();
    InputProcessor->Subsystem = this;

    FInputPreprocessorRegistrationKey Info;
    Info.Type = EInputPreProcessorType::PreGame;   // ✅ 比 Game 更高一档
    Info.Priority = INT32_MAX;                     // ✅ 同 bucket 内尽量靠前


    FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, EInputPreProcessorType::PreGame);
    //// 第二个参数是 priority，0 通常够用；数字越小越早拿到（视版本实现）
    //FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
    //FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor.ToSharedRef(), INT32_MAX);
#endif
}

void UInspectorWorldSubsystem::UnregisterInputProcessor()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!InputProcessor.IsValid()) return;
    if (!FSlateApplication::IsInitialized())
    {
        InputProcessor.Reset();
       
        return;
    }

    FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
    InputProcessor.Reset();
#endif
}

FString UInspectorWorldSubsystem::MakeComponentKey(const AActor* Actor, const UActorComponent* Comp)
{
    if (!Actor || !Comp)
    {
        return TEXT("InvalidComponentKey");
    }

    return FString::Printf(TEXT("%s:%s:%s"),
        *Actor->GetPathName(),
        *Comp->GetFName().ToString(),
        *Comp->GetClass()->GetName());
}

bool UInspectorWorldSubsystem::IsWhitelistedComponent(const UActorComponent* Comp)
{
    if (!Comp) return false;

    // 白名单：StaticMesh / Light(含子类) / CharacterMovement
    if (Comp->IsA<UStaticMeshComponent>()) return true;
    if (Comp->IsA<ULightComponent>()) return true;
    if (Comp->IsA<UCharacterMovementComponent>()) return true;

    return false;
}

bool UInspectorWorldSubsystem::GetGroupExpanded(const FString& GroupKey, bool bDefault) const
{
    if (const bool* Found = GroupExpandedMap.Find(GroupKey))
    {
        return *Found;
    }
    return bDefault;
}

void UInspectorWorldSubsystem::ToggleGroupExpanded(const FString& GroupKey, bool bDefault)
{
    const bool Current = GetGroupExpanded(GroupKey, bDefault);
    GroupExpandedMap.Add(GroupKey, !Current);

    // 可选：调试用
    // UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("ToggleGroupExpanded Key=%s Current=%d New=%d"),
    //     *GroupKey, Current, !Current);
}

//void UInspectorWorldSubsystem::ToggleGroupExpanded(const FString& GroupKey)
//{
//    bool& b = GroupExpandedMap.FindOrAdd(GroupKey);
//    b = !b;
//	
//    // 这里不主动“推 UI 刷新”，因为你的 UI 刷新逻辑在 BP 里：
//    // 组标题点击后：ToggleGroupExpanded -> 再调用 GetPropertyItemsForSelected -> SetListItems
//}

bool UInspectorWorldSubsystem::DoesPropertyNameMatchSearch(const FString& PropertyName, const FString& SearchText) const
{
    if (SearchText.IsEmpty())
    {
        return true;
    }

    return PropertyName.Contains(SearchText, ESearchCase::IgnoreCase, ESearchDir::FromStart)
        || PropertyName.Contains(SearchText, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
}

void UInspectorWorldSubsystem::AppendPropertiesForObject(
    UObject* TargetObject,
    const FString& SearchText,
    TArray<UObject*>& OutItems,
    const FString& OwnerPrefixForUI,
    bool bSearchMode
)
{
    if (!TargetObject) return;

    UClass* Cls = TargetObject->GetClass();
    if (!Cls) return;

    TArray<FProperty*> DisplayableProperties;

    for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FProperty* Prop = *It;
        if (!Prop) continue;

        if (!InspectorPropertyUtils::IsDisplayableProperty(Prop)) continue;

        const bool bVisible =
            Prop->HasAnyPropertyFlags(CPF_Edit) ||
            Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
        if (!bVisible) continue;

        const FString PropNameStr = Prop->GetName();
        const FString PropDisplayName = RI_GetPropertyDisplayNameRuntimeSafe(Prop);
        if (!SearchText.IsEmpty()
            && !NameMatchesSearch(PropNameStr, SearchText)
            && !NameMatchesSearch(PropDisplayName, SearchText))
        {
            continue;
        }

        UInspectorPropertyItem* Item = GetOrCreatePropertyItem(TargetObject, Prop->GetFName());
        if (!Item)
        {
            continue;
        }

        DisplayableProperties.Add(Prop);
    }

    DisplayableProperties.Sort([](const FProperty& A, const FProperty& B)
    {
        const FString NameA = RI_GetPropertyDisplayNameRuntimeSafe(&A);
        const FString NameB = RI_GetPropertyDisplayNameRuntimeSafe(&B);
        return NameA < NameB;
    });

    for (FProperty* Prop : DisplayableProperties)
    {
        if (!Prop)
        {
            continue;
        }

        UInspectorPropertyItem* Item = GetOrCreatePropertyItem(TargetObject, Prop->GetFName());
        if (!Item)
        {
            continue;
        }

        Item->OwnerPrefix = OwnerPrefixForUI;
        OutItems.Add(Item);
    }

	//// 旧版参考v0.2（注释掉以免误导）：
    //if (!TargetObject) return;

    //UClass* Cls = TargetObject->GetClass();
    //if (!Cls) return;

    //for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    //{
    //    FProperty* Prop = *It;
    //    if (!Prop) continue;

    //    if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

    //    // 只列“编辑/可见”的，避免噪音（与你现有规则一致）
    //    const bool bVisible =
    //        Prop->HasAnyPropertyFlags(CPF_Edit) ||
    //        Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);

    //    if (!bVisible) continue;

    //    if (!IsSupportedByInspector(Prop)) continue;

    //    const FString PropNameStr = Prop->GetName();
    //    if (!NameMatchesSearch(PropNameStr, SearchText)) continue;

    //    UInspectorPropertyItem* Item = NewObject<UInspectorPropertyItem>(this);
    //    Item->Init(TargetObject, Prop->GetFName());    
    //    OutItems.Add(Item);
    //}
	//// 旧版参考v0.1（注释掉以免误导）：
    //if (!TargetObject)
    //{
    //    return;
    //}

    //UClass* Cls = TargetObject->GetClass();
    //if (!Cls)
    //{
    //    return;
    //}

    //// 只列“常见可展示”的基础属性类型，避免复杂容器/对象指针导致 UI 不可控
    //auto IsSupportedPropertyType = [](const FProperty* P) -> bool
    //    {
    //        if (!P) return false;

    //        if (P->IsA<FBoolProperty>()) return true;
    //        if (P->IsA<FEnumProperty>()) return true;
    //        if (const FByteProperty* BP = CastField<FByteProperty>(P)) { return BP->Enum != nullptr; }
    //        if (P->IsA<FNumericProperty>()) return true;
    //        if (P->IsA<FStrProperty>()) return true;
    //        if (P->IsA<FNameProperty>()) return true;
    //        if (P->IsA<FTextProperty>()) return true;

    //        return false;
    //    };

    //for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    //{
    //    FProperty* Prop = *It;
    //    if (!Prop) continue;

    //    // 过滤掉明显不该显示的
    //    if (Prop->HasAnyPropertyFlags(CPF_Deprecated))
    //    {
    //        continue;
    //    }

    //    // 只展示“Edit 或 BlueprintVisible”的（更贴近你现有逻辑）
    //    const bool bVisible =
    //        Prop->HasAnyPropertyFlags(CPF_Edit) ||
    //        Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);

    //    if (!bVisible)
    //    {
    //        continue;
    //    }

    //    if (!IsSupportedPropertyType(Prop))
    //    {
    //        continue;
    //    }

    //    const FString PropName = Prop->GetName();
    //    if (!DoesPropertyNameMatchSearch(PropName, SearchText))
    //    {
    //        continue;
    //    }

    //    // 这里用你已有的 UInspectorPropertyItem（交接包里它提供 ApplyFromText/GetValueType/IsEditable）
    //    UInspectorPropertyItem* Item = NewObject<UInspectorPropertyItem>(this);
    //    Item->Init(TargetObject, Prop->GetFName());
    //    // 假设你现有 Item 有类似字段/初始化逻辑；
    //    // 交接包写的是“持有 Target(weak)、PropertyName”，所以我们按最通用的方式赋值：
    //    // 1) 如果你 Item 里是公开 UPROPERTY，直接赋值即可；
    //    // 2) 如果你 Item 里是私有 + Initialize(...)，用你工程现有 Initialize。
    //    //
    //    // 为了保证“直接能编译”，这里采用反射安全赋值：要求你 Item 里确实存在名为 Target / PropertyName 的 UPROPERTY。
    //    // 如果你命名不同（例如 TargetObject / Property），按下方“常见命名对照”改两行即可。

    //    // ---- 常见命名：Target (TWeakObjectPtr<UObject>) + PropertyName (FName) ----
    //    // 你如果就是这套，完全不用改：
    //    {
    //        FProperty* TargetProp = Item->GetClass()->FindPropertyByName(TEXT("Target"));
    //        FProperty* NameProp = Item->GetClass()->FindPropertyByName(TEXT("PropertyName"));
    //        if (TargetProp && NameProp)
    //        {
    //            void* TargetAddr = TargetProp->ContainerPtrToValuePtr<void>(Item);
    //            void* NameAddr = NameProp->ContainerPtrToValuePtr<void>(Item);

    //            // 写 Target
    //            if (FObjectPropertyBase* ObjP = CastField<FObjectPropertyBase>(TargetProp))
    //            {
    //                ObjP->SetObjectPropertyValue(TargetAddr, TargetObject);
    //            }
    //            else if (FWeakObjectProperty* WeakObjP = CastField<FWeakObjectProperty>(TargetProp))
    //            {
    //                WeakObjP->SetObjectPropertyValue(TargetAddr, TargetObject);
    //            }

    //            // 写 PropertyName
    //            if (FNameProperty* NP = CastField<FNameProperty>(NameProp))
    //            {
    //                NP->SetPropertyValue(NameAddr, FName(*PropName));
    //            }
    //        }
    //    }

    //    // 可选：如果你想在 UI 显示 “OwnerPrefixForUI”（比如组件名），你可以在 Item 里加一个 UPROPERTY FString OwnerPrefix;
    //    // 这里同样用反射写入，不会破坏编译：
    //    if (!OwnerPrefixForUI.IsEmpty())
    //    {
    //        if (FProperty* OwnerProp = Item->GetClass()->FindPropertyByName(TEXT("OwnerPrefix")))
    //        {
    //            if (FStrProperty* SP = CastField<FStrProperty>(OwnerProp))
    //            {
    //                void* OwnerAddr = OwnerProp->ContainerPtrToValuePtr<void>(Item);
    //                SP->SetPropertyValue(OwnerAddr, OwnerPrefixForUI);
    //            }
    //        }
    //    }

    //    OutItems.Add(Item);
    //}
}

bool UInspectorWorldSubsystem::IsSupportedByInspector(const FProperty* Prop)
{
    if (!Prop) return false;

    if (Prop->IsA<FBoolProperty>())   return true;
    if (Prop->IsA<FIntProperty>())    return true;
    if (Prop->IsA<FFloatProperty>())  return true;
    if (Prop->IsA<FDoubleProperty>()) return true;
    if (Prop->IsA<FStrProperty>())    return true;
    if (Prop->IsA<FNameProperty>())   return true;

    if (Prop->IsA<FEnumProperty>())   return true;
    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        return ByteProp->Enum != nullptr;
    }

    // Allow a small set of commonly tuned struct types.
    if (const FStructProperty* SP = CastField<FStructProperty>(Prop))
    {
        const UScriptStruct* S = SP->Struct;
        if (S == TBaseStructure<FVector2D>::Get()) return true;
        if (S == TBaseStructure<FVector>::Get())   return true;
        if (S == TBaseStructure<FVector4>::Get())  return true;
        if (S == TBaseStructure<FRotator>::Get())  return true;
        if (S == TBaseStructure<FTransform>::Get())return true;
        if (S == TBaseStructure<FLinearColor>::Get()) return true;
        if (S == TBaseStructure<FColor>::Get()) return true;
    }

    return false;
}

bool UInspectorWorldSubsystem::NameMatchesSearch(const FString& Name, const FString& SearchText)
{
    if (SearchText.IsEmpty()) return true;
    return Name.Contains(SearchText, ESearchCase::IgnoreCase);
}

UMaterialInstanceDynamic* UInspectorWorldSubsystem::GetOrCreateMID(UPrimitiveComponent* Comp, int32 MaterialIndex)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Comp || MaterialIndex < 0) return nullptr;

    UMaterialInterface* Current = Comp->GetMaterial(MaterialIndex);
    if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Current))
    {
        return Existing;
    }

    UMaterialInterface* Parent = Current;
    if (!Parent)
    {
        Parent = UMaterial::GetDefaultMaterial(MD_Surface);
    }

    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Comp);
    if (!MID) return nullptr;

    Comp->SetMaterial(MaterialIndex, MID);
    return MID;
#else
    return nullptr;
#endif
}

bool UInspectorWorldSubsystem::ApplyChange(const FInspectorChange& Change, bool bUseNewValue)
{
#if RUNTIME_INSPECTOR_ENABLED
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid()) return false;
        return ApplyChangeValue(Change.Target.Get(), Change.PropertyName, bUseNewValue ? Change.NewValueText : Change.OldValueText);

    case EInspectorChangeType::MaterialScalar:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        if (!Comp || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone()) return false;
        UMaterialInstanceDynamic* MID = GetOrCreateMID(Comp, Change.MaterialIndex);
        if (!MID) return false;

        MID->SetScalarParameterValue(Change.ParamName, bUseNewValue ? Change.NewScalar : Change.OldScalar);
        Comp->MarkRenderStateDirty();
        return true;
    }

    case EInspectorChangeType::MaterialVector:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        if (!Comp || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone()) return false;
        UMaterialInstanceDynamic* MID = GetOrCreateMID(Comp, Change.MaterialIndex);
        if (!MID) return false;

        MID->SetVectorParameterValue(Change.ParamName, bUseNewValue ? Change.NewVector : Change.OldVector);
        Comp->MarkRenderStateDirty();
        return true;
    }

    default:
        return false;
    }
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::BindToSelectedActor(AActor* Actor)
{
    if (!Actor) return;
    Actor->OnDestroyed.AddDynamic(this, &UInspectorWorldSubsystem::HandleSelectedActorDestroyed);
}

void UInspectorWorldSubsystem::UnbindFromSelectedActor()
{
    if (AActor* Actor = SelectedActor.Get())
    {
        Actor->OnDestroyed.RemoveDynamic(this, &UInspectorWorldSubsystem::HandleSelectedActorDestroyed);
    }
}

void UInspectorWorldSubsystem::HandleSelectedActorDestroyed(AActor* DestroyedActor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (DestroyedActor && DestroyedActor == SelectedActor.Get())
    {
        SelectedActor = nullptr;
        ClearItemPool();
        RefreshPanel(EInspectorRefreshReason::TargetInvalid);
    }
#endif
}

void UInspectorWorldSubsystem::ClearItemPool()
{
    ItemPool.Reset();
}

UInspectorGroupItem* UInspectorWorldSubsystem::GetOrCreateGroupItem(const FString& Key)
{
    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorGroupItem>(*Found);
    }

    UInspectorGroupItem* NewItem = NewObject<UInspectorGroupItem>(this);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

UInspectorPropertyItem* UInspectorWorldSubsystem::GetOrCreatePropertyItem(UObject* TargetObject, FName PropertyName)
{
    const FString Key = FString::Printf(TEXT("PROP:%s:%s"),
        *GetNameSafe(TargetObject),
        *PropertyName.ToString());

    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorPropertyItem>(*Found);
    }

    UInspectorPropertyItem* NewItem = NewObject<UInspectorPropertyItem>(this);
    NewItem->Init(TargetObject, PropertyName);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

UInspectorMaterialParamItem* UInspectorWorldSubsystem::GetOrCreateMaterialItem(
    UMeshComponent* Comp, int32 Slot, FName ParamName, EInspectorMatParamType Type)
{
    const FString Key = FString::Printf(TEXT("MAT:%s:%d:%d:%s"),
        *GetNameSafe(Comp),
        Slot,
        (int32)Type,
        *ParamName.ToString());

    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorMaterialParamItem>(*Found);
    }

    UInspectorMaterialParamItem* NewItem = NewObject<UInspectorMaterialParamItem>(this);
    NewItem->Init(Comp, Slot, ParamName, Type);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

UInspectorFunctionItem* UInspectorWorldSubsystem::GetOrCreateFunctionItem(UObject* TargetObject, FName FunctionName)
{
    const FString Key = FString::Printf(TEXT("FUNC:%s:%s"),
        *GetNameSafe(TargetObject),
        *FunctionName.ToString());

    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorFunctionItem>(*Found);
    }

    UInspectorFunctionItem* NewItem = NewObject<UInspectorFunctionItem>(this);
    NewItem->Init(TargetObject, FunctionName);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

FString UInspectorWorldSubsystem::GetFavoritesFilePath() const
{
    // Saved/RuntimeInspector/Favorites.json
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Favorites.json"));
}

void UInspectorWorldSubsystem::LoadFavorites()
{
    FavoriteKeys.Reset();

    const FString Path = GetFavoritesFilePath();
    FString Content;
    if (!FPaths::FileExists(Path) || !FFileHelper::LoadFileToString(Content, *Path))
    {
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
    if (Root->TryGetArrayField(TEXT("favorites"), Arr) && Arr)
    {
        for (const TSharedPtr<FJsonValue>& V : *Arr)
        {
            FString Key;
            if (V.IsValid() && V->TryGetString(Key))
            {
                FavoriteKeys.Add(Key);
            }
        }
    }
}

void UInspectorWorldSubsystem::SaveFavorites() const
{
    const FString Path = GetFavoritesFilePath();
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> Arr;
    Arr.Reserve(FavoriteKeys.Num());
    for (const FString& Key : FavoriteKeys)
    {
        Arr.Add(MakeShared<FJsonValueString>(Key));
    }
    Root->SetArrayField(TEXT("favorites"), Arr);

    FString Out;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(Out, *Path);
}

FString UInspectorWorldSubsystem::MakeFavoriteKeyForProperty(UObject* TargetObject, FName PropertyName) const
{
    if (!TargetObject) return FString();

    // Actor vs Component 分开，避免同名属性误撞
    if (TargetObject->IsA(AActor::StaticClass()))
    {
        return FString::Printf(TEXT("A:%s:%s"),
            *TargetObject->GetClass()->GetPathName(),
            *PropertyName.ToString());
    }

    // 其他情况按 Component / UObject 也一样处理：用类路径
    return FString::Printf(TEXT("C:%s:%s"),
        *TargetObject->GetClass()->GetPathName(),
        *PropertyName.ToString());
}

bool UInspectorWorldSubsystem::IsFavoriteForItem(UInspectorPropertyItem* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return false;
    UObject* TargetObj = Item->GetTargetObject();     // 如果你没有这个 getter，就用 Item->Target.Get() 自己取
    const FName PropName = Item->GetPropertyFName();  // 同理：没有就加一个 getter（推荐）
    const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);
    return !Key.IsEmpty() && FavoriteKeys.Contains(Key);
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::ToggleFavoriteForItem(UInspectorPropertyItem* Item)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return;


    

    UObject* TargetObj = Item->GetTargetObject();
    const FName PropName = Item->GetPropertyFName();
    const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);

    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] ToggleFavoriteForItem: Item=%s  Key=%s  Before=%d"),
        *GetNameSafe(Item),
        *Key,
        FavoriteKeys.Num());

    if (Key.IsEmpty()) return;

    if (FavoriteKeys.Contains(Key))
    {
        FavoriteKeys.Remove(Key);
    }
    else
    {
        FavoriteKeys.Add(Key);
    }

    SaveFavorites();



    

    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] After=%d  Contains=%d"),
        FavoriteKeys.Num(),
        FavoriteKeys.Contains(Key));

    // 这里用你现有刷新机制：建议当作 UIStateChanged
    RefreshPanel(/*EInspectorRefreshReason::UIStateChanged*/);
#endif
}

bool UInspectorWorldSubsystem::OpenColorEditorForAnyItem(UObject* ItemObject)
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    return OpenColorEditorForItemInternal(ItemObject);
#endif
}

void UInspectorWorldSubsystem::InsertPinnedGroupIfNeeded(TArray<UObject*>& OutItems)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (FavoriteKeys.Num() == 0) return;

    TArray<UObject*> PinnedProps;
    TSet<UObject*> PinnedSet;

    // 1) 找出当前列表里已收藏的 PropertyItem
    for (UObject* Obj : OutItems)
    {
        if (!Obj) continue;

        UInspectorPropertyItem* PropItem = Cast<UInspectorPropertyItem>(Obj);
        if (!PropItem) continue;

        UObject* TargetObj = PropItem->GetTargetObject();
        const FName PropName = PropItem->GetPropertyFName();
        const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);

        if (!Key.IsEmpty() && FavoriteKeys.Contains(Key))
        {
            PinnedProps.Add(PropItem);
            PinnedSet.Add(PropItem);
        }
    }

    if (PinnedProps.Num() == 0) return;

    // 2) Pinned 组（你也可以改成从 ItemPool 复用一个 group，避免频繁 new）
    UInspectorGroupItem* PinnedGroup = NewObject<UInspectorGroupItem>(this);
    PinnedGroup->DisplayName = TEXT("⭐ Pinned");
    PinnedGroup->StableKey = TEXT("PINNED_ROOT");
    PinnedGroup->bExpanded = true;

    // 3) 生成新列表：PinnedGroup + PinnedProps + 原列表(去掉重复的 pinned)
    TArray<UObject*> NewList;
    NewList.Reserve(OutItems.Num() + 1);

    NewList.Add(PinnedGroup);
    for (UObject* P : PinnedProps)
    {
        if (P) NewList.Add(P);
    }

    for (UObject* Obj : OutItems)
    {
        if (!Obj) continue;
        if (PinnedSet.Contains(Obj)) continue; // ✅ 关键：去重，避免 ListView 断言
        NewList.Add(Obj);
    }

    OutItems = MoveTemp(NewList);
#endif
}

static FString MakeMaterialFavoriteKey(UInspectorMaterialParamItem* M)
{
    if (!M) return FString();

    UMeshComponent* Comp = M->GetMeshComponent(); // 下面我会告诉你怎么加
    if (!Comp) return FString();

    AActor* Owner = Comp->GetOwner();
    const FString ActorPath = Owner ? Owner->GetPathName() : TEXT("None");
    const FString CompPath = Comp->GetPathName();

    const int32 Slot = M->GetSlotIndex();
    const int32 TypeInt = (int32)M->GetParamType();
    const FName ParamName = M->GetParamName();

    return FString::Printf(TEXT("M|%s|%s|%d|%d|%s"),
        *ActorPath,
        *CompPath,
        Slot,
        TypeInt,
        *ParamName.ToString());
}

static FString MakeFunctionFavoriteKey(UInspectorFunctionItem* FunctionItem)
{
    if (!FunctionItem)
    {
        return FString();
    }

    UObject* TargetObject = FunctionItem->GetTargetObject();
    if (!TargetObject)
    {
        return FString();
    }

    AActor* OwnerActor = Cast<AActor>(TargetObject);
    UActorComponent* Component = Cast<UActorComponent>(TargetObject);
    if (!OwnerActor && Component)
    {
        OwnerActor = Component->GetOwner();
    }

    const FString ActorPath = OwnerActor ? OwnerActor->GetPathName() : TEXT("None");
    const FString TargetPath = TargetObject->GetPathName();
    return FString::Printf(TEXT("F|%s|%s|%s"),
        *ActorPath,
        *TargetPath,
        *FunctionItem->GetFunctionFName().ToString());
}

bool UInspectorWorldSubsystem::IsFavoriteForAnyItem(UObject* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item)
    {
        return false;
    }

    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(Item))
    {
        return IsFavoriteForItem(PropertyItem);
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(Item))
    {
        const FString Key = MakeMaterialFavoriteKey(MaterialItem);
        return !Key.IsEmpty() && FavoriteKeys.Contains(Key);
    }

    if (UInspectorFunctionItem* FunctionItem = Cast<UInspectorFunctionItem>(Item))
    {
        const FString Key = MakeFunctionFavoriteKey(FunctionItem);
        return !Key.IsEmpty() && FavoriteKeys.Contains(Key);
    }
#else
    (void)Item;
#endif
    return false;
}

void UInspectorWorldSubsystem::ToggleFavoriteForAnyItem(UObject* Item)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return;

    // 1) 普通属性
    if (UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Item))
    {
        // ✅ 复用你现有的逻辑（你之前已经有 ToggleFavoriteForItem / MakeFavoriteKey）
        ToggleFavoriteForItem(P); // <- 如果你函数名不是这个，改成你自己的
        return;
    }

    // 2) 材质参数
    if (UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Item))
    {
        const FString Key = MakeMaterialFavoriteKey(M);

        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] MaterialKey=%s"), *Key);

        if (Key.IsEmpty())
        {
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] ToggleFavoriteForAnyItem(Material): key empty. Item=%s"), *GetNameSafe(Item));
            return;
        }

        const int32 Before = FavoriteKeys.Num();
        if (FavoriteKeys.Contains(Key))
        {
            FavoriteKeys.Remove(Key);
        }
        else
        {
            FavoriteKeys.Add(Key);
        }

        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] ToggleFavoriteForAnyItem(Material): %s  Before=%d After=%d"),
            *Key, Before, FavoriteKeys.Num());

        SaveFavorites();
        RefreshPanel(EInspectorRefreshReason::StructureChanged);
        return;
    }

    if (UInspectorFunctionItem* FunctionItem = Cast<UInspectorFunctionItem>(Item))
    {
        const FString Key = MakeFunctionFavoriteKey(FunctionItem);
        if (Key.IsEmpty())
        {
            return;
        }

        if (FavoriteKeys.Contains(Key))
        {
            FavoriteKeys.Remove(Key);
        }
        else
        {
            FavoriteKeys.Add(Key);
        }

        SaveFavorites();
        RefreshPanel(EInspectorRefreshReason::StructureChanged);
        return;
    }

    
#endif
}

// =======================
// Snapshot / Only-Modified (v0.2)
// =======================

void UInspectorWorldSubsystem::ClearModified()
{
#if RUNTIME_INSPECTOR_ENABLED
    BaselineValueByKey.Reset();
    ModifiedValueByKey.Reset();
#endif
}

void UInspectorWorldSubsystem::ClearLastImportReport()
{
    LastImportReport = FRIImportReport();
}

void UInspectorWorldSubsystem::CacheSharedFileReport(bool bSuccess, const FString& Summary, const FString& Details, int32 AppliedCount, int32 SkippedCount, int32 MissingCount, int32 HardFailCount)
{
    LastImportReport = FRIImportReport();
    LastImportReport.bSuccess = bSuccess;
    LastImportReport.AppliedCount = AppliedCount;
    LastImportReport.SkippedCount = SkippedCount;
    LastImportReport.MissingCount = MissingCount;
    LastImportReport.HardFailCount = HardFailCount;
    LastImportReport.Summary = Summary;
    LastImportReport.Details = Details;
    InvalidateFileManagementSummaryCache();
}

FString UInspectorWorldSubsystem::GetImportReportsDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Reports"));
}

FString UInspectorWorldSubsystem::GetImportReportsDirectory() const
{
    return GetImportReportsDir();
}

FString UInspectorWorldSubsystem::GetPatchesDirectory() const
{
    return GetPatchesDir();
}

FString UInspectorWorldSubsystem::GetPresetsDirectory() const
{
    return GetPresetsDir();
}

bool UInspectorWorldSubsystem::ExecuteFileStagePatchAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    FString Error;
    const bool bOk = StageSelectionAsPatch(Error);
    if (bOk)
    {
        const FRIPatchBundle Bundle = GetStagedPatch();
        OutSummary = FString::Printf(TEXT("Staged patch (%d ops)"), Bundle.Operations.Num());
        OutDetails = FString::Printf(
            TEXT("Selection: %s\nBundle: %s\nOperations: %d"),
            Bundle.CapturedFromSelection.IsEmpty() ? TEXT("None") : *Bundle.CapturedFromSelection,
            Bundle.DisplayName.IsEmpty() ? TEXT("StagedPatch") : *Bundle.DisplayName,
            Bundle.Operations.Num());
        CacheSharedFileReport(true, OutSummary, OutDetails, Bundle.Operations.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Failed to stage patch") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildBaselineAuditAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (ModifiedValueByKey.Num() <= 0 || BaselineValueByKey.Num() <= 0)
    {
        OutSummary = TEXT("No baseline/current differences");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIAuditReport Report;
    FString Error;
    bool bOk = BuildAuditReport(ERIAuditComparisonMode::BaselineVsCurrent, FRIPatchBundle(), Report, Error);
    FString ExportPath;
    if (bOk)
    {
        FString ExportError;
        bOk = ExportLastAuditReportToFile(false, ExportPath, ExportError);
        if (!bOk)
        {
            Error = ExportError;
        }
    }

    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Baseline-vs-current audit built") : Report.Summary;
        OutDetails = FString::Printf(TEXT("%s\n\nExported: %s"), *GetLastAuditReportAsText(), *ExportPath);
        CacheSharedFileReport(true, OutSummary, OutDetails, Report.Lines.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Baseline-vs-current audit build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileExportPatchAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FString OutFilePath;
    FString Error;
    const FRIPatchBundle Bundle = GetStagedPatch();
    const bool bOk = ExportPatchBundle(Bundle, OutFilePath, Error);
    if (bOk)
    {
        OutSummary = FString::Printf(TEXT("Patch exported: %s"), *FPaths::GetCleanFilename(OutFilePath));
        OutDetails = FString::Printf(TEXT("Path: %s\nOperations: %d"), *OutFilePath, Bundle.Operations.Num());
        CacheSharedFileReport(true, OutSummary, OutDetails, Bundle.Operations.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Patch export failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileSavePresetAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    const FRIPatchBundle Bundle = GetStagedPatch();
    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    FRIPatchPresetMetadata Metadata;
    Metadata.PresetId = FString::Printf(TEXT("quick_%s"), *Timestamp);
    Metadata.DisplayName = Bundle.DisplayName.IsEmpty()
        ? FString::Printf(TEXT("QuickPreset_%s"), *Timestamp)
        : FString::Printf(TEXT("%s_%s"), *Bundle.DisplayName, *Timestamp);
    Metadata.Category = TEXT("QuickSave");
    Metadata.CreatedAt = FDateTime::UtcNow().ToIso8601();
    Metadata.Description = Bundle.CapturedFromSelection.IsEmpty()
        ? TEXT("Quick preset saved from staged runtime patch.")
        : FString::Printf(TEXT("Quick preset from %s"), *Bundle.CapturedFromSelection);
    Metadata.ApplicabilityScope = ERIPatchPresetApplicabilityScope::CurrentSelection;

    FString OutFilePath;
    FString Error;
    const bool bOk = SavePatchPreset(Metadata, OutFilePath, Error);
    if (bOk)
    {
        OutSummary = FString::Printf(TEXT("Preset saved: %s"), *Metadata.DisplayName);
        OutDetails = FString::Printf(TEXT("Path: %s\nScope: CurrentSelection\nOperations: %d"), *OutFilePath, Bundle.Operations.Num());
        CacheSharedFileReport(true, OutSummary, OutDetails, Bundle.Operations.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Preset save failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileApplyLatestPresetAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    TArray<FRIPatchPresetMetadata> Presets;
    FString Error;
    if (!ListPatchPresets(Presets, Error))
    {
        OutSummary = Error.IsEmpty() ? TEXT("Preset listing failed") : Error;
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Error, OutSummary, 2.0f);
        return false;
    }

    if (Presets.Num() <= 0)
    {
        OutSummary = TEXT("No presets");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    auto ParsePresetCreatedAt = [](const FString& InCreatedAt, FDateTime& OutCreatedAt) -> bool
    {
        if (InCreatedAt.IsEmpty())
        {
            return false;
        }

        return FDateTime::ParseIso8601(*InCreatedAt, OutCreatedAt)
            || FDateTime::Parse(InCreatedAt, OutCreatedAt);
    };

    Presets.Sort([&ParsePresetCreatedAt](const FRIPatchPresetMetadata& A, const FRIPatchPresetMetadata& B)
    {
        FDateTime CreatedA;
        FDateTime CreatedB;
        const bool bHasCreatedA = ParsePresetCreatedAt(A.CreatedAt, CreatedA);
        const bool bHasCreatedB = ParsePresetCreatedAt(B.CreatedAt, CreatedB);

        if (bHasCreatedA != bHasCreatedB)
        {
            return bHasCreatedA;
        }

        if (bHasCreatedA && CreatedA != CreatedB)
        {
            return CreatedA > CreatedB;
        }

        const FString DisplayA = A.DisplayName.IsEmpty() ? A.PresetId : A.DisplayName;
        const FString DisplayB = B.DisplayName.IsEmpty() ? B.PresetId : B.DisplayName;
        if (DisplayA != DisplayB)
        {
            return DisplayA > DisplayB;
        }

        return A.PresetId > B.PresetId;
    });

    const FRIPatchPresetMetadata& LatestPreset = Presets[0];
    FRIApplyResult ApplyResult;
    const bool bOk = ApplyPatchPreset(LatestPreset.PresetId, ApplyResult, Error);
    if (bOk)
    {
        OutSummary = FString::Printf(TEXT("Applied preset: %s"), *LatestPreset.DisplayName);
        OutDetails = FString::Printf(
            TEXT("Preset: %s\nCategory: %s\nResult: %s"),
            *LatestPreset.DisplayName,
            LatestPreset.Category.IsEmpty() ? TEXT("-") : *LatestPreset.Category,
            ApplyResult.Summary.IsEmpty() ? TEXT("Applied") : *ApplyResult.Summary);
        CacheSharedFileReport(true, OutSummary, OutDetails, ApplyResult.AppliedCount, ApplyResult.SkippedCount, 0, ApplyResult.FailedCount);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Preset apply failed") : Error;
    OutDetails = ApplyResult.Summary.IsEmpty() ? OutSummary : ApplyResult.Summary;
    CacheSharedFileReport(false, OutSummary, OutDetails, ApplyResult.AppliedCount, ApplyResult.SkippedCount, 0, ApplyResult.FailedCount > 0 ? ApplyResult.FailedCount : 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildAuditAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIAuditReport Report;
    FString Error;
    bool bOk = BuildAuditReport(ERIAuditComparisonMode::CurrentVsPatch, GetStagedPatch(), Report, Error);
    FString ExportPath;
    if (bOk)
    {
        FString ExportError;
        bOk = ExportLastAuditReportToFile(false, ExportPath, ExportError);
        if (!bOk)
        {
            Error = ExportError;
        }
    }

    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Audit built") : Report.Summary;
        OutDetails = FString::Printf(TEXT("%s\n\nExported: %s"), *GetLastAuditReportAsText(), *ExportPath);
        CacheSharedFileReport(true, OutSummary, OutDetails, Report.Lines.Num(), 0, 0, 0);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Audit build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildPatchVsSourceAuditAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIAuditReport Report;
    FString Error;
    bool bOk = BuildAuditReport(ERIAuditComparisonMode::PatchVsSource, GetStagedPatch(), Report, Error);
    FString ExportPath;
    if (bOk)
    {
        FString ExportError;
        bOk = ExportLastAuditReportToFile(false, ExportPath, ExportError);
        if (!bOk)
        {
            Error = ExportError;
        }
    }

    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Patch-vs-source audit built") : Report.Summary;
        OutDetails = FString::Printf(TEXT("%s\n\nExported: %s"), *GetLastAuditReportAsText(), *ExportPath);
        CacheSharedFileReport(true, OutSummary, OutDetails, Report.Lines.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Patch-vs-source audit build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildAppliedPatchVsSourceAuditAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIAuditReport Report;
    FString Error;
    bool bOk = BuildAuditReport(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote, GetStagedPatch(), Report, Error);
    FString ExportPath;
    if (bOk)
    {
        FString ExportError;
        bOk = ExportLastAuditReportToFile(false, ExportPath, ExportError);
        if (!bOk)
        {
            Error = ExportError;
        }
    }

    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Applied-patch-vs-source audit built") : Report.Summary;
        OutDetails = FString::Printf(TEXT("%s\n\nExported: %s"), *GetLastAuditReportAsText(), *ExportPath);
        CacheSharedFileReport(true, OutSummary, OutDetails, Report.Lines.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Applied-patch-vs-source audit build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildRuntimeRoleCompareAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIRuntimeRoleCompareReport Report;
    FString Error;
    const bool bOk = CompareRuntimeRoles(Report, Error);
    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Runtime role compare built") : Report.Summary;
        OutDetails = Report.Details.IsEmpty()
            ? OutSummary
            : FString::Printf(TEXT("%s\n\n%s"), *OutSummary, *Report.Details);
        CacheSharedFileReport(
            true,
            OutSummary,
            OutDetails,
            Report.ComparedLineCount,
            0,
            Report.MissingRoleCount,
            Report.MismatchCount + Report.VerificationMismatchCount);
        PushToast(ERIToastType::Success, TEXT("Runtime role compare built"), 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Runtime role compare build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileBuildRemoteSessionTargetSetCompareAction(FString& OutSummary, FString& OutDetails)
{
    return ExecuteFileBuildRemoteSessionTargetSetCompareAction(GetActiveRemoteSessionTargetSetCompareRequest(), OutSummary, OutDetails);
}

bool UInspectorWorldSubsystem::ExecuteFileBuildRemoteSessionTargetSetCompareAction(const FRIRuntimeSessionTargetSetCompareRequest& InRequest, FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutSummary = TEXT("PIE with local player required");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    const FString LeftSessionId = InRequest.LeftSessionId.TrimStartAndEnd().IsEmpty()
        ? TEXT("local_editor_current")
        : InRequest.LeftSessionId.TrimStartAndEnd();
    const FString RightSessionId = InRequest.RightSessionId.TrimStartAndEnd().IsEmpty()
        ? TEXT("local_pie_current")
        : InRequest.RightSessionId.TrimStartAndEnd();
    const FString NameFilter = InRequest.NameFilter.TrimStartAndEnd();
    const FString ClassFilter = InRequest.ClassFilter.TrimStartAndEnd();

    FRIRuntimeSessionTargetSetCompareReport Report;
    FString Error;
    const bool bOk = CompareRuntimeTargetSetsAcrossSessions(
        LeftSessionId,
        RightSessionId,
        NameFilter,
        ClassFilter,
        Report,
        Error);
    if (bOk)
    {
        OutSummary = Report.Summary.IsEmpty() ? TEXT("Remote session compare built") : Report.Summary;
        OutDetails = Report.Details.IsEmpty()
            ? OutSummary
            : FString::Printf(TEXT("%s\n\n%s"), *OutSummary, *Report.Details);
        CacheSharedFileReport(
            true,
            OutSummary,
            OutDetails,
            Report.SharedTargetCount,
            Report.MismatchCount,
            Report.LeftOnlyCount + Report.RightOnlyCount,
            0);
        PushToast(ERIToastType::Success, TEXT("Remote session compare built"), 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Remote session compare build failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFilePullPatchFromRemoteSessionAction(const FString& SessionId, const FString& ActorQuery, FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    FRIPatchBundle Bundle;
    FString Error;
    const bool bOk = PullPatchBundleFromRuntimeSession(SessionId, ActorQuery, Bundle, Error);
    if (bOk && Bundle.Operations.Num() > 0)
    {
        StagedPatchBundle = Bundle;
        bHasStagedPatch = true;

        OutSummary = LastRemotePatchPullSummary.IsEmpty()
            ? FString::Printf(TEXT("Pulled patch bundle from %s"), *SessionId)
            : LastRemotePatchPullSummary;
        OutDetails = FString::Printf(
            TEXT("%s\nSession: %s\nActorQuery: %s\nOperations: %d"),
            *OutSummary,
            SessionId.IsEmpty() ? TEXT("-") : *SessionId,
            ActorQuery.IsEmpty() ? TEXT("-") : *ActorQuery,
            Bundle.Operations.Num());
        CacheSharedFileReport(true, OutSummary, OutDetails, Bundle.Operations.Num(), 0, 0, 0);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Remote patch pull failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileRunRemoteWorkflowAction(const FString& SessionId, FName WorkflowId, const FString& ActorQuery, FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    const FRIWorkflowDefinition* LocalWorkflow = GetAvailableWorkflows().FindByPredicate([WorkflowId](const FRIWorkflowDefinition& Candidate)
    {
        return Candidate.WorkflowId == WorkflowId;
    });

    FRIWorkflowRunResult WorkflowResult;
    FString Error;
    bool bOk = false;
    if (LocalWorkflow && WorkflowId.ToString().StartsWith(TEXT("mainline_remote_packaged_"), ESearchCase::CaseSensitive))
    {
        FRIRuntimeSessionInfo ResolvedSession;
        FRIRuntimeSessionInfo ConnectedSession;
        FString ResolvedSessionId = SessionId;
        if (ResolvedSessionId.IsEmpty())
        {
            if (EnsurePackagedRuntimeValidationSession(ResolvedSession, Error))
            {
                ResolvedSessionId = ResolvedSession.SessionId;
            }
        }

        if (!ResolvedSessionId.IsEmpty() && ConnectRemoteRuntimeSession(ResolvedSessionId, ConnectedSession, Error))
        {
            bOk = RunWorkflowById(WorkflowId, WorkflowResult);
        }
    }
    else
    {
        bOk = RunWorkflowOnRuntimeSession(SessionId, WorkflowId, ActorQuery, WorkflowResult, Error);
    }

    if (bOk)
    {
        OutSummary = WorkflowResult.Summary.IsEmpty()
            ? FString::Printf(TEXT("Remote workflow %s completed"), *WorkflowId.ToString())
            : WorkflowResult.Summary;
        OutDetails = WorkflowResult.FullReport.IsEmpty() ? OutSummary : WorkflowResult.FullReport;
        CacheSharedFileReport(true, OutSummary, OutDetails, WorkflowResult.PassedStepCount, 0, 0, WorkflowResult.FailedStepCount);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = !Error.IsEmpty()
        ? Error
        : (WorkflowResult.Summary.IsEmpty() ? FString::Printf(TEXT("Remote workflow %s failed"), *WorkflowId.ToString()) : WorkflowResult.Summary);
    OutDetails = !WorkflowResult.FullReport.IsEmpty() ? WorkflowResult.FullReport : OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, WorkflowResult.PassedStepCount, 0, 0, WorkflowResult.FailedStepCount > 0 ? WorkflowResult.FailedStepCount : 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFilePromotePreviewAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIPromotePreview Preview;
    FString Error;
    const bool bOk = CreatePromotePreview(GetStagedPatch(), Preview, Error);
    if (bOk)
    {
        OutSummary = Preview.Summary.IsEmpty() ? TEXT("Promote preview ready") : Preview.Summary;
        OutDetails = Preview.DiffText.IsEmpty() ? OutSummary : Preview.DiffText;
        CacheSharedFileReport(true, OutSummary, OutDetails, Preview.SupportedOperationCount, 0, Preview.UnsupportedOperationCount, 0);
        PushToast(ERIToastType::Info, OutSummary, 1.5f);
        return true;
    }

    OutSummary = Error.IsEmpty() ? TEXT("Promote preview failed") : Error;
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFilePromoteApplyAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    LastPromoteResult = FRIPromoteResult();
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    if (!HasStagedPatch())
    {
        OutSummary = TEXT("No staged patch");
        OutDetails = OutSummary;
        LastPromoteResult = FRIPromoteResult();
        CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
        PushToast(ERIToastType::Warning, OutSummary, 1.5f);
        return false;
    }

    FRIPromoteResult Result;
    FString Error;
    const bool bOk = PromotePatchToSource(GetStagedPatch(), Result, Error);
    LastPromoteResult = Result;

    if (bOk)
    {
        OutSummary = Result.Summary.IsEmpty() ? TEXT("Promote applied") : Result.Summary;
        OutDetails = Result.ReportText.IsEmpty() ? OutSummary : Result.ReportText;
        CacheSharedFileReport(true, OutSummary, OutDetails, Result.PromotedCount, Result.SkippedCount, 0, Result.FailedCount);
        PushToast(ERIToastType::Success, OutSummary, 1.5f);
        return true;
    }

    OutSummary = !Error.IsEmpty()
        ? Error
        : (Result.Summary.IsEmpty() ? TEXT("Promote apply failed") : Result.Summary);
    OutDetails = !Result.ReportText.IsEmpty()
        ? Result.ReportText
        : OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, Result.PromotedCount, Result.SkippedCount, 0, Result.FailedCount > 0 ? Result.FailedCount : 1);
    PushToast(ERIToastType::Error, OutSummary, 2.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::ExecuteFileClearStagedAction(FString& OutSummary, FString& OutDetails)
{
    OutSummary.Reset();
    OutDetails.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutSummary = TEXT("RuntimeInspector unavailable");
    OutDetails = OutSummary;
    CacheSharedFileReport(false, OutSummary, OutDetails, 0, 0, 0, 1);
    return false;
#else
    ClearStagedPatch();
    OutSummary = TEXT("Staged patch cleared");
    OutDetails = TEXT("The current staged patch bundle has been removed from the session.");
    CacheSharedFileReport(true, OutSummary, OutDetails, 0, 0, 0, 0);
    PushToast(ERIToastType::Info, OutSummary, 1.2f);
    return true;
#endif
}

FString UInspectorWorldSubsystem::GetLastImportReportAsText(bool bIncludeDetails, bool bIncludeLists) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
#else
    const bool bHasAny =
        !LastImportReport.Summary.IsEmpty() ||
        !LastImportReport.Details.IsEmpty() ||
        LastImportReport.AppliedCount != 0 ||
        LastImportReport.SkippedCount != 0 ||
        LastImportReport.MissingErrors.Num() != 0 ||
        LastImportReport.HardErrors.Num() != 0 ||
        LastImportReport.Warnings.Num() != 0;

    if (!bHasAny)
    {
        return FString();
    }

    FString Out;
    if (!LastImportReport.Summary.IsEmpty())
    {
        Out += LastImportReport.Summary;
        Out += TEXT("\n");
    }

    Out += FString::Printf(TEXT("Success: %s\nApplied: %d\nSkipped: %d\nMissing: %d\nHardFail: %d\nWarnings: %d\n"),
        LastImportReport.bSuccess ? TEXT("true") : TEXT("false"),
        LastImportReport.AppliedCount,
        LastImportReport.SkippedCount,
        LastImportReport.MissingErrors.Num(),
        LastImportReport.HardErrors.Num(),
        LastImportReport.Warnings.Num());

    if (!LastImportedSnapshotPath.IsEmpty())
    {
        Out += FString::Printf(TEXT("Source: %s\n"), *LastImportedSnapshotPath);
    }

    if (bIncludeDetails && !LastImportReport.Details.IsEmpty())
    {
        Out += TEXT("\nDetails:\n");
        Out += LastImportReport.Details;
        Out += TEXT("\n");
    }

    if (bIncludeLists)
    {
        auto AppendList = [&](const TCHAR* Title, const TArray<FString>& Items)
        {
            if (Items.Num() <= 0) return;
            Out += TEXT("\n");
            Out += Title;
            Out += TEXT(":\n");
            for (const FString& S : Items)
            {
                Out += TEXT("- ");
                Out += S;
                Out += TEXT("\n");
            }
        };

        AppendList(TEXT("Missing"), LastImportReport.MissingErrors);
        AppendList(TEXT("HardFail"), LastImportReport.HardErrors);
        AppendList(TEXT("Warnings"), LastImportReport.Warnings);
    }

    return Out.TrimStartAndEnd();
#endif
}

bool UInspectorWorldSubsystem::CopyLastImportReportToClipboard(FString& OutCopiedText)
{
    OutCopiedText = GetLastImportReportAsText(true, true);
    if (OutCopiedText.IsEmpty())
    {
        return false;
    }

    FPlatformApplicationMisc::ClipboardCopy(*OutCopiedText);
    PushToast(ERIToastType::Info, TEXT("Import report copied to clipboard"), 1.5f);
    return true;
}

bool UInspectorWorldSubsystem::ExportLastImportReportToFile(bool bAsJson, FString& OutFilePath, FString& OutError)
{
    OutFilePath.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    FString Payload;
    if (bAsJson)
    {
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetNumberField(TEXT("schemaVersion"), 1);
        Root->SetStringField(TEXT("createdAtUtc"), FDateTime::UtcNow().ToIso8601());
        Root->SetStringField(TEXT("sourceFile"), LastImportedSnapshotPath);
        Root->SetBoolField(TEXT("success"), LastImportReport.bSuccess);
        Root->SetStringField(TEXT("summary"), LastImportReport.Summary);
        Root->SetStringField(TEXT("details"), LastImportReport.Details);
        Root->SetNumberField(TEXT("appliedCount"), LastImportReport.AppliedCount);
        Root->SetNumberField(TEXT("skippedCount"), LastImportReport.SkippedCount);

        auto ToJsonArray = [](const TArray<FString>& In)
        {
            TArray<TSharedPtr<FJsonValue>> A;
            A.Reserve(In.Num());
            for (const FString& S : In)
            {
                A.Add(MakeShared<FJsonValueString>(S));
            }
            return A;
        };

        Root->SetArrayField(TEXT("missingErrors"), ToJsonArray(LastImportReport.MissingErrors));
        Root->SetArrayField(TEXT("hardErrors"), ToJsonArray(LastImportReport.HardErrors));
        Root->SetArrayField(TEXT("warnings"), ToJsonArray(LastImportReport.Warnings));

        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
        if (!FJsonSerializer::Serialize(Root, Writer))
        {
            OutError = TEXT("Failed to serialize report to JSON");
            return false;
        }
    }
    else
    {
        Payload = GetLastImportReportAsText(true, true);
    }

    if (Payload.IsEmpty())
    {
        OutError = TEXT("No import report");
        return false;
    }

    const FString Dir = GetImportReportsDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FString MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("World");
    MapName.RemoveFromStart(GetWorld() ? GetWorld()->StreamingLevelsPrefix : FString());

    const TCHAR* Ext = bAsJson ? TEXT("json") : TEXT("txt");
    const FString FileName = FString::Printf(TEXT("ImportReport_%s_%s.%s"), *MapName, *Timestamp, Ext);
    OutFilePath = FPaths::Combine(Dir, FileName);

    if (!FFileHelper::SaveStringToFile(Payload, *OutFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutError = TEXT("Failed to write file");
        OutFilePath.Reset();
        return false;
    }

    PushToast(ERIToastType::Success, FString::Printf(TEXT("Report saved: %s"), *FPaths::GetCleanFilename(OutFilePath)), 2.0f);
    return true;
#endif
}

void UInspectorWorldSubsystem::CaptureBaselineForSelection(bool bIncludeMaterialParams)
{
#if RUNTIME_INSPECTOR_ENABLED
    BaselineValueByKey.Reset();
    ModifiedValueByKey.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    auto CaptureObjectProps = [&](UObject* Obj, const TSet<FName>* /*Whitelist*/)
    {
        if (!Obj) return;
        UClass* Cls = Obj->GetClass();
        if (!Cls) return;

        for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
        {
            FProperty* Prop = *It;
            if (!Prop) continue;
            if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

            const bool bVisible = Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
            if (!bVisible) continue;

            if (!IsSupportedByInspector(Prop)) continue;
            if (!InspectorPropertyUtils::CanSetFromText(Obj, Prop)) continue;

            FString ValText;
            if (!InspectorPropertyUtils::GetValueAsText(Obj, Prop->GetFName(), ValText))
            {
                continue;
            }

            const FString Key = MakePropertySnapshotKey(Obj, Prop->GetFName());
            if (!Key.IsEmpty())
            {
                BaselineValueByKey.Add(Key, ValText);
            }
        }
    };

    // Actor
    CaptureObjectProps(ActorPtr, nullptr);

    // Whitelisted components
    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);
    for (UActorComponent* Comp : Components)
    {
        CaptureObjectProps(Comp, nullptr);
    }

    if (bIncludeMaterialParams)
    {
        for (UActorComponent* Comp : Components)
        {
            UMeshComponent* MC = Cast<UMeshComponent>(Comp);
            if (!MC) continue;
            UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(MC);
            if (!Prim) continue;

            const int32 SlotCount = MC->GetNumMaterials();
            for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
            {
                UMaterialInterface* Mat = MC->GetMaterial(SlotIndex);
                if (!Mat) continue;

                TArray<FMaterialParameterInfo> Infos;
                TArray<FGuid> Ids;

                Infos.Reset(); Ids.Reset();
                Mat->GetAllScalarParameterInfo(Infos, Ids);
                for (const FMaterialParameterInfo& Info : Infos)
                {
                    const FString V = RI_GetMaterialParamValueText(Prim, SlotIndex, EInspectorChangeType::MaterialScalar, Info.Name);
                    const FString Key = MakeMaterialSnapshotKey(Prim, SlotIndex, EInspectorMatParamType::Scalar, Info.Name);
                    if (!Key.IsEmpty())
                    {
                        BaselineValueByKey.Add(Key, V);
                    }
                }

                Infos.Reset(); Ids.Reset();
                Mat->GetAllVectorParameterInfo(Infos, Ids);
                for (const FMaterialParameterInfo& Info : Infos)
                {
                    const FString V = RI_GetMaterialParamValueText(Prim, SlotIndex, EInspectorChangeType::MaterialVector, Info.Name);
                    const FString Key = MakeMaterialSnapshotKey(Prim, SlotIndex, EInspectorMatParamType::Vector, Info.Name);
                    if (!Key.IsEmpty())
                    {
                        BaselineValueByKey.Add(Key, V);
                    }
                }
            }
        }
    }
#endif
}

bool UInspectorWorldSubsystem::RequestApplyPropertyText(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!IsRIEnabled())
    {
        OutError = GetRIDisabledReason();
        return false;
    }
    if (!TargetObject || PropertyName.IsNone())
    {
        OutError = TEXT("Invalid target/property");
        return false;
    }

    // IMPORTANT:
    // UI entry widgets often call Apply on every OnTextChanged. During row regeneration we also
    // programmatically SetText to reflect the current value. If we don't guard this, we can get a
    // feedback loop:
    //   Refresh -> SetText -> OnTextChanged -> RequestApply -> Flush -> Refresh ...
    // which makes OnListItemObjectSet fire constantly and prevents typing.
    //
    // Cheap guard: if the requested text matches the current value text, do nothing.
    // (Most of our widgets use the same text source, so this breaks the loop.)
    {
        FString CurrentText;
        InspectorPropertyUtils::GetValueAsText(TargetObject, PropertyName, CurrentText);
        if (CurrentText == NewText)
        {
            return true;
        }
    }

    const FString Key = MakePropertySnapshotKey(TargetObject, PropertyName);
    if (Key.IsEmpty())
    {
        OutError = TEXT("Failed to build snapshot key");
        return false;
    }

    const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
    const bool bDebounce = Settings ? Settings->bEnableApplyDebounce : false;
    const float DebounceSeconds = Settings ? Settings->ApplyDebounceSeconds : 0.f;

    if (bDebounce && DebounceSeconds > 0.0f)
    {
        FRIPendingPropertyApply& P = PendingPropertyApplyByKey.FindOrAdd(Key);
        P.Target = TargetObject;
        P.PropertyName = PropertyName;
        P.PendingText = NewText;
        P.ApplyAtSeconds = FPlatformTime::Seconds() + (double)DebounceSeconds;
        return true;
    }

    return ApplyPropertyTextNow(TargetObject, PropertyName, NewText, OutError);
#endif
}

bool UInspectorWorldSubsystem::ApplyPropertyTextImmediate(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!IsRIEnabled())
    {
        OutError = GetRIDisabledReason();
        return false;
    }

    return ApplyPropertyTextNow(TargetObject, PropertyName, NewText, OutError);
#endif
}

bool UInspectorWorldSubsystem::ApplyPropertyTextNow(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!TargetObject || PropertyName.IsNone())
    {
        OutError = TEXT("Invalid target/property");
        return false;
    }

    // Old value (for undo + modified tracking)
    FString OldText;
    InspectorPropertyUtils::GetValueAsText(TargetObject, PropertyName, OldText);

    // Write
    if (!InspectorPropertyUtils::SetValueFromText(TargetObject, PropertyName, NewText, &OutError))
    {
        return false;
    }

    // New value (actual)
    FString NewTextActual;
    InspectorPropertyUtils::GetValueAsText(TargetObject, PropertyName, NewTextActual);

    // Fixups for common UE properties that are stored as fields but require setters / re-register to refresh visuals.
    if (UActorComponent* AC = Cast<UActorComponent>(TargetObject))
    {
        AC->MarkRenderStateDirty();
        AC->MarkRenderTransformDirty();
    }

    if (AActor* Actor = Cast<AActor>(TargetObject))
    {
        if (PropertyName == TEXT("bHidden") || PropertyName == TEXT("bHiddenInGame"))
        {
            Actor->SetActorHiddenInGame(Actor->IsHidden());
        }
    }

    if (USceneComponent* SC = Cast<USceneComponent>(TargetObject))
    {
        // Visibility
        if (PropertyName == TEXT("bVisible"))
        {
            SC->SetVisibility(SC->IsVisible(), true);
        }
        // HiddenInGame
        else if (PropertyName == TEXT("bHiddenInGame"))
        {
            SC->SetHiddenInGame(SC->bHiddenInGame, true);
        }
        // Mobility: setters ensure proper re-register
        else if (PropertyName == TEXT("Mobility"))
        {
            SC->SetMobility(SC->Mobility);
        }
    }

    if (ULightComponent* LC = Cast<ULightComponent>(TargetObject))
    {
        if (PropertyName == TEXT("Intensity"))
        {
            LC->SetIntensity(LC->Intensity);
        }
    }

    // Record undo/redo + modified state
    if (!IsApplyingHistory() && OldText != NewTextActual)
    {
        FInspectorChange Change;
        Change.ChangeType = EInspectorChangeType::Property;
        Change.Target = TargetObject;
        Change.PropertyName = PropertyName;
        Change.OldValueText = OldText;
        Change.NewValueText = NewTextActual;
        Change.DebugObjectName = GetNameSafe(TargetObject);
        RecordChange(Change);
    }

    return true;
#endif
}

void UInspectorWorldSubsystem::FlushPendingPropertyApplies()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (PendingPropertyApplyByKey.Num() == 0) return;

    const double Now = FPlatformTime::Seconds();
    bool bAnyApplied = false;

    // Iterate by copy of keys to allow removal while iterating.
    TArray<FString> Keys;
    PendingPropertyApplyByKey.GetKeys(Keys);

    for (const FString& Key : Keys)
    {
        FRIPendingPropertyApply* P = PendingPropertyApplyByKey.Find(Key);
        if (!P) continue;

        if (Now < P->ApplyAtSeconds)
        {
            continue;
        }

        UObject* Target = P->Target.Get();
        if (!Target)
        {
            PendingPropertyApplyByKey.Remove(Key);
            continue;
        }

        // Break potential UI feedback loops: if the pending text already matches the current value,
        // skip applying and avoid triggering a refresh.
        FString BeforeText;
        InspectorPropertyUtils::GetValueAsText(Target, P->PropertyName, BeforeText);
        if (BeforeText == P->PendingText)
        {
            PendingPropertyApplyByKey.Remove(Key);
            continue;
        }

        FString Err;
        const bool bOk = ApplyPropertyTextNow(Target, P->PropertyName, P->PendingText, Err);
        if (!bOk)
        {
            if (RI_IsDebugLogEnabled())
            {
                UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Debounced apply failed: %s.%s err=%s"), *GetNameSafe(Target), *P->PropertyName.ToString(), *Err);
            }
        }
        else
        {
            // Only consider as "applied" if it actually changed the stored value text.
            FString AfterText;
            InspectorPropertyUtils::GetValueAsText(Target, P->PropertyName, AfterText);
            if (BeforeText != AfterText)
            {
                bAnyApplied = true;
            }
        }

        PendingPropertyApplyByKey.Remove(Key);
    }

    if (bAnyApplied)
    {
        // Keep this light: refresh values without rebuilding structure.
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    }
#endif
}

FString UInspectorWorldSubsystem::GetSnapshotsDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
}

FString UInspectorWorldSubsystem::GetPatchesDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Patches"));
}

FString UInspectorWorldSubsystem::GetPresetsDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Presets"));
}

FString UInspectorWorldSubsystem::MakePropertySnapshotKey(UObject* TargetObject, FName PropertyName) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!TargetObject || PropertyName.IsNone()) return FString();

    FString ActorPath;
    FString CompPath;

    if (AActor* A = Cast<AActor>(TargetObject))
    {
        ActorPath = A->GetPathName();
    }
    else if (UActorComponent* C = Cast<UActorComponent>(TargetObject))
    {
        CompPath = C->GetPathName();
        if (AActor* Owner = C->GetOwner())
        {
            ActorPath = Owner->GetPathName();
        }
    }
    else
    {
        // Fallback: treat it as "component"-like object
        CompPath = TargetObject->GetPathName();
    }

    const FString ClassPath = TargetObject->GetClass() ? TargetObject->GetClass()->GetPathName() : TEXT("");

    // P|ActorPath|CompPath|ClassPath|PropName
    return FString::Printf(TEXT("P|%s|%s|%s|%s"),
        *ActorPath,
        *CompPath,
        *ClassPath,
        *PropertyName.ToString());
#else
    return FString();
#endif
}

FString UInspectorWorldSubsystem::MakeMaterialSnapshotKey(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorMatParamType Type, FName ParamName) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return FString();

    AActor* Owner = Comp->GetOwner();
    const FString ActorPath = Owner ? Owner->GetPathName() : TEXT("");
    const FString CompPath = Comp->GetPathName();

    // M|ActorPath|CompPath|Slot|TypeInt|ParamName
    return FString::Printf(TEXT("M|%s|%s|%d|%d|%s"),
        *ActorPath,
        *CompPath,
        SlotIndex,
        (int32)Type,
        *ParamName.ToString());
#else
    return FString();
#endif
}

AActor* UInspectorWorldSubsystem::ResolveRuntimeActorTarget(const FString& ActorPath, const FString& ActorClass, const FString& ActorBaseName) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return nullptr;
#else
    UWorld* World = GetWorld();
    if (!World || (ActorPath.IsEmpty() && ActorBaseName.IsEmpty()))
    {
        return nullptr;
    }

    if (!ActorPath.IsEmpty())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetPathName() == ActorPath)
            {
                return *It;
            }
        }
    }

    if (!ActorBaseName.IsEmpty())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Candidate = *It;
            if (!Candidate) continue;

            if (RI_ExtractActorBaseName(Candidate->GetName()) != ActorBaseName)
            {
                continue;
            }

            if (!ActorClass.IsEmpty() && !RI_ClassMatches(Candidate, ActorClass))
            {
                continue;
            }

            return Candidate;
        }
    }

    if (!ActorPath.IsEmpty())
    {
        const FString FallbackName = RI_ExtractTailAfterLastDot(ActorPath);
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == FallbackName)
            {
                return *It;
            }
        }
    }

    return nullptr;
#endif
}

UActorComponent* UInspectorWorldSubsystem::ResolveRuntimeComponentTarget(AActor* Owner, const FString& ActorPathForRemap, const FString& ComponentPath, const FString& ComponentName, const FString& ComponentClass) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return nullptr;
#else
    if (!Owner || ComponentPath.IsEmpty())
    {
        return nullptr;
    }

    FString EffectiveCompPath = ComponentPath;
    if (!ActorPathForRemap.IsEmpty() && ComponentPath.StartsWith(ActorPathForRemap, ESearchCase::CaseSensitive))
    {
        const FString OwnerActorPath = Owner->GetPathName();
        if (OwnerActorPath != ActorPathForRemap)
        {
            EffectiveCompPath = OwnerActorPath + ComponentPath.Mid(ActorPathForRemap.Len());
        }
    }

    TArray<UActorComponent*> Components;
    Owner->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        if (Component && Component->GetPathName() == EffectiveCompPath)
        {
            return Component;
        }
    }

    FString FallbackName = ComponentName;
    if (FallbackName.IsEmpty())
    {
        FallbackName = RI_ExtractTailAfterLastDot(EffectiveCompPath);
    }

    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        if (!FallbackName.IsEmpty() && Component->GetName() != FallbackName)
        {
            continue;
        }

        if (!ComponentClass.IsEmpty() && !RI_UClassMatches(Component->GetClass(), ComponentClass))
        {
            continue;
        }

        return Component;
    }

    return nullptr;
#endif
}

bool UInspectorWorldSubsystem::TryBuildPatchOperationFromModifiedKey(const FString& Key, const FString& PatchedValue, const FString* BaselineValuePtr, FRIPatchOperation& OutOperation) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    return false;
#else
    OutOperation = FRIPatchOperation();
    OutOperation.PatchedValue = PatchedValue;
    OutOperation.BaselineValue = BaselineValuePtr ? *BaselineValuePtr : FString();
    OutOperation.SourceTag = Key;

    FString ActorPath;
    FString ComponentPath;

    FString PropertyClassPath;
    FString PropertyName;
    if (RI_ParsePropertySnapshotKey(Key, ActorPath, ComponentPath, PropertyClassPath, PropertyName))
    {
        OutOperation.Target.TargetKind = ComponentPath.IsEmpty() ? ERIPatchTargetKind::Actor : ERIPatchTargetKind::Component;
        OutOperation.Target.ActorPath = ActorPath;
        OutOperation.Target.ComponentPath = ComponentPath;
        OutOperation.Target.ComponentName = ComponentPath.IsEmpty() ? FString() : RI_ExtractTailAfterLastDot(ComponentPath);
        OutOperation.Target.ComponentClass = ComponentPath.IsEmpty() ? FString() : PropertyClassPath;

        if (AActor* Actor = ResolveRuntimeActorTarget(ActorPath, FString(), RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath))))
        {
            OutOperation.Target.ActorClass = Actor->GetClass()->GetPathName();
            OutOperation.Target.ActorBaseName = RI_ExtractActorBaseName(Actor->GetName());
        }
        else
        {
            OutOperation.Target.ActorClass = FString();
            OutOperation.Target.ActorBaseName = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));
        }

        OutOperation.Field.FieldKind = ERIPatchFieldKind::Property;
        OutOperation.Field.FieldPath = PropertyName;
        OutOperation.Field.DisplayName = PropertyName;
        return true;
    }

    int32 MaterialSlotIndex = INDEX_NONE;
    EInspectorMatParamType MaterialType = EInspectorMatParamType::Scalar;
    FString ParamName;
    if (RI_ParseMaterialSnapshotKey(Key, ActorPath, ComponentPath, MaterialSlotIndex, MaterialType, ParamName))
    {
        OutOperation.Target.TargetKind = ERIPatchTargetKind::MaterialSlot;
        OutOperation.Target.ActorPath = ActorPath;
        OutOperation.Target.ComponentPath = ComponentPath;
        OutOperation.Target.ComponentName = RI_ExtractTailAfterLastDot(ComponentPath);
        OutOperation.Target.MaterialSlotIndex = MaterialSlotIndex;

        AActor* Actor = ResolveRuntimeActorTarget(ActorPath, FString(), RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath)));
        if (Actor)
        {
            OutOperation.Target.ActorClass = Actor->GetClass()->GetPathName();
            OutOperation.Target.ActorBaseName = RI_ExtractActorBaseName(Actor->GetName());

            if (UActorComponent* Component = ResolveRuntimeComponentTarget(Actor, ActorPath, ComponentPath, OutOperation.Target.ComponentName, FString()))
            {
                OutOperation.Target.ComponentClass = Component->GetClass()->GetPathName();
                if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Component))
                {
                    OutOperation.Target.MaterialSlotName = RI_GetMeshMaterialSlotName(MeshComp, MaterialSlotIndex);
                }
            }
        }
        else
        {
            OutOperation.Target.ActorBaseName = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));
        }

        OutOperation.Field.FieldKind = (MaterialType == EInspectorMatParamType::Scalar)
            ? ERIPatchFieldKind::MaterialScalar
            : ERIPatchFieldKind::MaterialVector;
        OutOperation.Field.FieldPath = ParamName;
        OutOperation.Field.DisplayName = ParamName;
        return true;
    }

    return false;
#endif
}

void UInspectorWorldSubsystem::SortPatchOperationsForApply(TArray<FRIPatchOperation>& InOutOperations) const
{
    auto GetRank = [](ERIPatchFieldKind FieldKind) -> int32
    {
        switch (FieldKind)
        {
        case ERIPatchFieldKind::Property: return 0;
        case ERIPatchFieldKind::MaterialScalar: return 1;
        case ERIPatchFieldKind::MaterialVector: return 2;
        default: return 99;
        }
    };

    InOutOperations.Sort([&](const FRIPatchOperation& A, const FRIPatchOperation& B)
    {
        const int32 RankA = GetRank(A.Field.FieldKind);
        const int32 RankB = GetRank(B.Field.FieldKind);
        if (RankA != RankB) return RankA < RankB;

        if (A.Target.ActorPath != B.Target.ActorPath) return A.Target.ActorPath < B.Target.ActorPath;
        if (A.Target.ComponentPath != B.Target.ComponentPath) return A.Target.ComponentPath < B.Target.ComponentPath;
        if (A.Target.MaterialSlotIndex != B.Target.MaterialSlotIndex) return A.Target.MaterialSlotIndex < B.Target.MaterialSlotIndex;
        if (A.Field.FieldPath != B.Field.FieldPath) return A.Field.FieldPath < B.Field.FieldPath;
        return A.SourceTag < B.SourceTag;
    });
}

void UInspectorWorldSubsystem::FinalizePatchApplyResult(FRIApplyResult& OutResult, const TCHAR* SummaryPrefix) const
{
    OutResult.bSuccess = OutResult.FailedCount == 0 && OutResult.AppliedCount > 0;
    OutResult.Summary = FString::Printf(
        TEXT("%s=%d Failed=%d Skipped=%d"),
        SummaryPrefix,
        OutResult.AppliedCount,
        OutResult.FailedCount,
        OutResult.SkippedCount);
}

bool UInspectorWorldSubsystem::ApplyPatchOperationValue(const FRIPatchOperation& Operation, const FString& ValueText, FRIPatchOperationResult& OutResult)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIPatchOperationResult();
    OutResult.Status = ERIApplyOperationStatus::WriteFailed;
    OutResult.Message = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutResult = FRIPatchOperationResult();
    OutResult.Target = Operation.Target;
    OutResult.Field = Operation.Field;

    AActor* TargetActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (!TargetActor)
    {
        OutResult.Status = ERIApplyOperationStatus::NotFound;
        OutResult.Message = TEXT("Actor not found");
        return false;
    }

    UObject* TargetObject = TargetActor;
    if (Operation.Field.FieldKind == ERIPatchFieldKind::Property && Operation.Target.TargetKind == ERIPatchTargetKind::Component)
    {
        UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
            TargetActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (!TargetComponent)
        {
            OutResult.Status = ERIApplyOperationStatus::NotFound;
            OutResult.Message = TEXT("Component not found");
            return false;
        }
        TargetObject = TargetComponent;
    }

    FString Error;
    if (Operation.Field.FieldKind == ERIPatchFieldKind::Property)
    {
        if (!ApplyPropertyTextNow(TargetObject, FName(*Operation.Field.FieldPath), ValueText, Error))
        {
            OutResult.Status = ERIApplyOperationStatus::WriteFailed;
            OutResult.Message = Error;
            return false;
        }

        OutResult.Status = ERIApplyOperationStatus::Applied;
        if (!InspectorPropertyUtils::GetValueAsText(TargetObject, FName(*Operation.Field.FieldPath), OutResult.ValueApplied))
        {
            OutResult.ValueApplied = ValueText;
        }
        return true;
    }

    UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
        TargetActor,
        Operation.Target.ActorPath,
        Operation.Target.ComponentPath,
        Operation.Target.ComponentName,
        Operation.Target.ComponentClass);
    UMeshComponent* MeshComp = Cast<UMeshComponent>(TargetComponent);
    if (!MeshComp)
    {
        OutResult.Status = ERIApplyOperationStatus::TypeMismatch;
        OutResult.Message = TEXT("Target component is not a mesh component");
        return false;
    }

    const EInspectorMatParamType MaterialType = (Operation.Field.FieldKind == ERIPatchFieldKind::MaterialScalar)
        ? EInspectorMatParamType::Scalar
        : EInspectorMatParamType::Vector;

    UInspectorMaterialParamItem* Item = NewObject<UInspectorMaterialParamItem>(this);
    Item->Init(MeshComp, Operation.Target.MaterialSlotIndex, FName(*Operation.Field.FieldPath), MaterialType);
    if (!Item->ApplyFromText(ValueText, Error))
    {
        OutResult.Status = ERIApplyOperationStatus::WriteFailed;
        OutResult.Message = Error;
        return false;
    }

    OutResult.Status = ERIApplyOperationStatus::Applied;
    OutResult.ValueApplied = Item->GetValueText();
    return true;
#endif
}

bool UInspectorWorldSubsystem::ResolvePatchPresetFile(const FString& InPresetIdOrPath, FString& OutPresetFilePath) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutPresetFilePath.Reset();
    return false;
#else
    OutPresetFilePath.Reset();

    const FString Candidate = InPresetIdOrPath.TrimStartAndEnd();
    if (Candidate.IsEmpty())
    {
        return false;
    }

    if (FPaths::FileExists(Candidate))
    {
        OutPresetFilePath = Candidate;
        return true;
    }

    const FString PresetsDir = GetPresetsDir();
    const FString ById = FPaths::Combine(
        PresetsDir,
        Candidate.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase) ? Candidate : Candidate + TEXT(".json"));
    if (FPaths::FileExists(ById))
    {
        OutPresetFilePath = ById;
        return true;
    }

    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(FPaths::Combine(PresetsDir, TEXT("*.json"))), true, false);
    Files.Sort();
    for (const FString& FileName : Files)
    {
        if (FPaths::GetBaseFilename(FileName) == Candidate)
        {
            OutPresetFilePath = FPaths::Combine(PresetsDir, FileName);
            return true;
        }
    }

    return false;
#endif
}

bool UInspectorWorldSubsystem::SerializePatchPresetToJson(const FRIPatchPresetMetadata& Metadata, const FRIPatchBundle& Bundle, FString& OutJson, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutJson.Reset();
    OutError.Reset();

    FString BundleJson;
    if (!SerializePatchBundleToJson(Bundle, BundleJson, OutError))
    {
        return false;
    }

    TSharedPtr<FJsonObject> BundleObject;
    const TSharedRef<TJsonReader<>> BundleReader = TJsonReaderFactory<>::Create(BundleJson);
    if (!FJsonSerializer::Deserialize(BundleReader, BundleObject) || !BundleObject.IsValid())
    {
        OutError = TEXT("Failed to serialize nested patch bundle");
        return false;
    }

    TSharedRef<FJsonObject> MetadataObject = MakeShared<FJsonObject>();
    MetadataObject->SetStringField(TEXT("presetId"), Metadata.PresetId);
    MetadataObject->SetStringField(TEXT("displayName"), Metadata.DisplayName);
    MetadataObject->SetStringField(TEXT("category"), Metadata.Category);
    MetadataObject->SetStringField(TEXT("createdAt"), Metadata.CreatedAt);
    MetadataObject->SetStringField(TEXT("description"), Metadata.Description);
    MetadataObject->SetStringField(TEXT("applicabilityScope"), RI_PatchPresetScopeToString(Metadata.ApplicabilityScope));

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), 1);
    Root->SetStringField(TEXT("kind"), TEXT("runtimePatchPreset"));
    Root->SetObjectField(TEXT("metadata"), MetadataObject);
    Root->SetObjectField(TEXT("bundle"), BundleObject.ToSharedRef());

    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutError = TEXT("JSON serialize failed");
        return false;
    }

    return true;
#endif
}

bool UInspectorWorldSubsystem::DeserializePatchPresetFromJson(const FString& InJson, FRIPatchPresetMetadata& OutMetadata, FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutMetadata = FRIPatchPresetMetadata();
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJson);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid JSON");
        return false;
    }

    FString Kind;
    Root->TryGetStringField(TEXT("kind"), Kind);
    if (!Kind.IsEmpty() && Kind != TEXT("runtimePatchPreset"))
    {
        OutError = TEXT("Unsupported patch preset kind");
        return false;
    }

    const TSharedPtr<FJsonObject>* MetadataObject = nullptr;
    if (!Root->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject || !(*MetadataObject).IsValid())
    {
        OutError = TEXT("Missing metadata");
        return false;
    }

    (*MetadataObject)->TryGetStringField(TEXT("presetId"), OutMetadata.PresetId);
    (*MetadataObject)->TryGetStringField(TEXT("displayName"), OutMetadata.DisplayName);
    (*MetadataObject)->TryGetStringField(TEXT("category"), OutMetadata.Category);
    (*MetadataObject)->TryGetStringField(TEXT("createdAt"), OutMetadata.CreatedAt);
    (*MetadataObject)->TryGetStringField(TEXT("description"), OutMetadata.Description);

    FString Scope;
    (*MetadataObject)->TryGetStringField(TEXT("applicabilityScope"), Scope);
    if (!Scope.IsEmpty() && !RI_ParsePatchPresetScope(Scope, OutMetadata.ApplicabilityScope))
    {
        OutError = TEXT("Invalid preset applicability scope");
        return false;
    }

    const TSharedPtr<FJsonObject>* BundleObject = nullptr;
    if (!Root->TryGetObjectField(TEXT("bundle"), BundleObject) || !BundleObject || !(*BundleObject).IsValid())
    {
        OutError = TEXT("Missing bundle");
        return false;
    }

    FString BundleJson;
    const TSharedRef<TJsonWriter<>> BundleWriter = TJsonWriterFactory<>::Create(&BundleJson);
    if (!FJsonSerializer::Serialize((*BundleObject).ToSharedRef(), BundleWriter))
    {
        OutError = TEXT("Failed to re-serialize nested patch bundle");
        return false;
    }

    return DeserializePatchBundleFromJson(BundleJson, OutBundle, OutError);
#endif
}

bool UInspectorWorldSubsystem::RetargetPatchBundleToSelection(const FRIPatchPresetMetadata& Metadata, const FRIPatchBundle& InBundle, FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    AActor* SelectedActorPtr = SelectedActor.Get();
    if (!SelectedActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    if (InBundle.Operations.Num() <= 0)
    {
        OutError = TEXT("Preset bundle has no operations");
        return false;
    }

    FString SourceActorClass;
    for (const FRIPatchOperation& Operation : InBundle.Operations)
    {
        if (!Operation.Target.ActorClass.IsEmpty())
        {
            SourceActorClass = Operation.Target.ActorClass;
            break;
        }
    }

    if (Metadata.ApplicabilityScope == ERIPatchPresetApplicabilityScope::ActorClass
        && !SourceActorClass.IsEmpty()
        && !RI_UClassMatches(SelectedActorPtr->GetClass(), SourceActorClass))
    {
        OutError = FString::Printf(
            TEXT("Selected actor class mismatch: expected %s, got %s"),
            *SourceActorClass,
            *SelectedActorPtr->GetClass()->GetPathName());
        return false;
    }

    const FString DestActorPath = SelectedActorPtr->GetPathName();
    const FString DestActorClass = SelectedActorPtr->GetClass()->GetPathName();
    const FString DestActorBaseName = RI_ExtractActorBaseName(SelectedActorPtr->GetName());

    OutBundle = InBundle;
    OutBundle.BundleId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    OutBundle.CapturedAtUtc = FDateTime::UtcNow().ToIso8601();
    OutBundle.CapturedFromSelection = DestActorPath;

    for (FRIPatchOperation& Operation : OutBundle.Operations)
    {
        const FString SourceActorPath = Operation.Target.ActorPath;
        const FString SourceComponentPath = Operation.Target.ComponentPath;

        Operation.Target.ActorPath = DestActorPath;
        Operation.Target.ActorClass = DestActorClass;
        Operation.Target.ActorBaseName = DestActorBaseName;

        if (Operation.Target.TargetKind == ERIPatchTargetKind::Actor)
        {
            continue;
        }

        if (UActorComponent* RetargetedComponent = ResolveRuntimeComponentTarget(
            SelectedActorPtr,
            SourceActorPath,
            SourceComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass))
        {
            Operation.Target.ComponentPath = RetargetedComponent->GetPathName();
            Operation.Target.ComponentName = RetargetedComponent->GetName();
            Operation.Target.ComponentClass = RetargetedComponent->GetClass()->GetPathName();

            if (Operation.Target.TargetKind == ERIPatchTargetKind::MaterialSlot)
            {
                if (UMeshComponent* MeshComp = Cast<UMeshComponent>(RetargetedComponent))
                {
                    Operation.Target.MaterialSlotName = RI_GetMeshMaterialSlotName(MeshComp, Operation.Target.MaterialSlotIndex);
                }
            }
            continue;
        }

        if (!SourceActorPath.IsEmpty() && !SourceComponentPath.IsEmpty() && SourceComponentPath.StartsWith(SourceActorPath, ESearchCase::CaseSensitive))
        {
            Operation.Target.ComponentPath = DestActorPath + SourceComponentPath.Mid(SourceActorPath.Len());
        }
    }

    return true;
#endif
}

bool UInspectorWorldSubsystem::SerializePatchBundleToJson(const FRIPatchBundle& Bundle, FString& OutJson, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutJson.Reset();
    OutError.Reset();

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), Bundle.Version);
    Root->SetStringField(TEXT("kind"), TEXT("runtimePatchBundle"));
    Root->SetStringField(TEXT("bundleId"), Bundle.BundleId);
    Root->SetStringField(TEXT("displayName"), Bundle.DisplayName);
    Root->SetStringField(TEXT("capturedAtUtc"), Bundle.CapturedAtUtc);
    Root->SetStringField(TEXT("capturedFromSelection"), Bundle.CapturedFromSelection);

    TArray<TSharedPtr<FJsonValue>> JsonOperations;
    JsonOperations.Reserve(Bundle.Operations.Num());

    for (const FRIPatchOperation& Operation : Bundle.Operations)
    {
        TSharedRef<FJsonObject> Target = MakeShared<FJsonObject>();
        Target->SetStringField(TEXT("targetKind"), RI_PatchTargetKindToString(Operation.Target.TargetKind));
        Target->SetStringField(TEXT("actorPath"), Operation.Target.ActorPath);
        Target->SetStringField(TEXT("actorClass"), Operation.Target.ActorClass);
        Target->SetStringField(TEXT("actorBaseName"), Operation.Target.ActorBaseName);
        Target->SetStringField(TEXT("componentPath"), Operation.Target.ComponentPath);
        Target->SetStringField(TEXT("componentName"), Operation.Target.ComponentName);
        Target->SetStringField(TEXT("componentClass"), Operation.Target.ComponentClass);
        Target->SetNumberField(TEXT("materialSlotIndex"), Operation.Target.MaterialSlotIndex);
        Target->SetStringField(TEXT("materialSlotName"), Operation.Target.MaterialSlotName);

        TSharedRef<FJsonObject> Field = MakeShared<FJsonObject>();
        Field->SetStringField(TEXT("fieldKind"), RI_PatchFieldKindToString(Operation.Field.FieldKind));
        Field->SetStringField(TEXT("fieldPath"), Operation.Field.FieldPath);
        Field->SetStringField(TEXT("displayName"), Operation.Field.DisplayName);

        TSharedRef<FJsonObject> JsonOperation = MakeShared<FJsonObject>();
        JsonOperation->SetObjectField(TEXT("target"), Target);
        JsonOperation->SetObjectField(TEXT("field"), Field);
        JsonOperation->SetStringField(TEXT("valueKind"), RI_PatchValueKindToString(Operation.ValueKind));
        JsonOperation->SetStringField(TEXT("baselineValue"), Operation.BaselineValue);
        JsonOperation->SetStringField(TEXT("patchedValue"), Operation.PatchedValue);
        JsonOperation->SetStringField(TEXT("sourceTag"), Operation.SourceTag);

        JsonOperations.Add(MakeShared<FJsonValueObject>(JsonOperation));
    }

    Root->SetArrayField(TEXT("operations"), JsonOperations);

    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutError = TEXT("JSON serialize failed");
        return false;
    }

    return true;
#endif
}

bool UInspectorWorldSubsystem::DeserializePatchBundleFromJson(const FString& InJson, FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJson);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid JSON");
        return false;
    }

    FString Kind;
    Root->TryGetStringField(TEXT("kind"), Kind);
    if (!Kind.IsEmpty() && Kind != TEXT("runtimePatchBundle"))
    {
        OutError = TEXT("Unsupported patch bundle kind");
        return false;
    }

    OutBundle.Version = Root->HasField(TEXT("schemaVersion")) ? static_cast<int32>(Root->GetNumberField(TEXT("schemaVersion"))) : 1;
    Root->TryGetStringField(TEXT("bundleId"), OutBundle.BundleId);
    Root->TryGetStringField(TEXT("displayName"), OutBundle.DisplayName);
    Root->TryGetStringField(TEXT("capturedAtUtc"), OutBundle.CapturedAtUtc);
    Root->TryGetStringField(TEXT("capturedFromSelection"), OutBundle.CapturedFromSelection);

    const TArray<TSharedPtr<FJsonValue>>* JsonOperations = nullptr;
    if (!Root->TryGetArrayField(TEXT("operations"), JsonOperations) || !JsonOperations)
    {
        OutError = TEXT("Missing operations[]");
        return false;
    }

    OutBundle.Operations.Reserve(JsonOperations->Num());
    for (const TSharedPtr<FJsonValue>& JsonValue : *JsonOperations)
    {
        const TSharedPtr<FJsonObject> JsonOperation = JsonValue.IsValid() ? JsonValue->AsObject() : nullptr;
        if (!JsonOperation.IsValid())
        {
            OutError = TEXT("Invalid operation entry");
            return false;
        }

        const TSharedPtr<FJsonObject>* JsonTarget = nullptr;
        const TSharedPtr<FJsonObject>* JsonField = nullptr;
        if (!JsonOperation->TryGetObjectField(TEXT("target"), JsonTarget) || !JsonTarget || !(*JsonTarget).IsValid())
        {
            OutError = TEXT("Missing operation.target");
            return false;
        }
        if (!JsonOperation->TryGetObjectField(TEXT("field"), JsonField) || !JsonField || !(*JsonField).IsValid())
        {
            OutError = TEXT("Missing operation.field");
            return false;
        }

        FRIPatchOperation Operation;

        FString TargetKind;
        (*JsonTarget)->TryGetStringField(TEXT("targetKind"), TargetKind);
        if (!RI_ParsePatchTargetKind(TargetKind, Operation.Target.TargetKind))
        {
            OutError = TEXT("Invalid target kind");
            return false;
        }

        (*JsonTarget)->TryGetStringField(TEXT("actorPath"), Operation.Target.ActorPath);
        (*JsonTarget)->TryGetStringField(TEXT("actorClass"), Operation.Target.ActorClass);
        (*JsonTarget)->TryGetStringField(TEXT("actorBaseName"), Operation.Target.ActorBaseName);
        (*JsonTarget)->TryGetStringField(TEXT("componentPath"), Operation.Target.ComponentPath);
        (*JsonTarget)->TryGetStringField(TEXT("componentName"), Operation.Target.ComponentName);
        (*JsonTarget)->TryGetStringField(TEXT("componentClass"), Operation.Target.ComponentClass);
        (*JsonTarget)->TryGetStringField(TEXT("materialSlotName"), Operation.Target.MaterialSlotName);
        Operation.Target.MaterialSlotIndex = (*JsonTarget)->HasField(TEXT("materialSlotIndex"))
            ? static_cast<int32>((*JsonTarget)->GetNumberField(TEXT("materialSlotIndex")))
            : INDEX_NONE;

        FString FieldKind;
        (*JsonField)->TryGetStringField(TEXT("fieldKind"), FieldKind);
        if (!RI_ParsePatchFieldKind(FieldKind, Operation.Field.FieldKind))
        {
            OutError = TEXT("Invalid field kind");
            return false;
        }

        (*JsonField)->TryGetStringField(TEXT("fieldPath"), Operation.Field.FieldPath);
        (*JsonField)->TryGetStringField(TEXT("displayName"), Operation.Field.DisplayName);

        FString ValueKind;
        JsonOperation->TryGetStringField(TEXT("valueKind"), ValueKind);
        if (!RI_ParsePatchValueKind(ValueKind, Operation.ValueKind))
        {
            OutError = TEXT("Invalid value kind");
            return false;
        }

        JsonOperation->TryGetStringField(TEXT("baselineValue"), Operation.BaselineValue);
        JsonOperation->TryGetStringField(TEXT("patchedValue"), Operation.PatchedValue);
        JsonOperation->TryGetStringField(TEXT("sourceTag"), Operation.SourceTag);

        OutBundle.Operations.Add(MoveTemp(Operation));
    }

    return true;
#endif
}

void UInspectorWorldSubsystem::TrackModifiedForKey(const FString& Key, const FString& OldText, const FString& NewText)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (Key.IsEmpty()) return;

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, OldText);
    }

    const FString& Baseline = BaselineValueByKey[Key];

    if (NewText == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, NewText);
    }
#endif
}

void UInspectorWorldSubsystem::UpdateModifiedStateFromCurrentValue(UObject* TargetObject, FName PropertyName)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!TargetObject || PropertyName.IsNone()) return;

    const FString Key = MakePropertySnapshotKey(TargetObject, PropertyName);
    if (Key.IsEmpty()) return;

    FString Current;
    if (!InspectorPropertyUtils::GetValueAsText(TargetObject, PropertyName, Current))
    {
        return;
    }

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, Current);
        ModifiedValueByKey.Remove(Key);
        return;
    }

    const FString& Baseline = BaselineValueByKey[Key];
    if (Current == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, Current);
    }
#endif
}

static FString RI_GetMaterialParamValueText(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName)
{
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return TEXT("");

    UMaterialInterface* Mat = Comp->GetMaterial(SlotIndex);
    if (!Mat) return TEXT("");

    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat))
    {
        if (ChangeType == EInspectorChangeType::MaterialScalar)
        {
            const float V = MID->K2_GetScalarParameterValue(ParamName);
            return FString::SanitizeFloat(V);
        }
        else
        {
            const FLinearColor C = MID->K2_GetVectorParameterValue(ParamName);
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
    }

    const FMaterialParameterInfo Info(ParamName);
    if (ChangeType == EInspectorChangeType::MaterialScalar)
    {
        float V = 0.f;
        if (Mat->GetScalarParameterValue(Info, V))
        {
            return FString::SanitizeFloat(V);
        }
        return TEXT("");
    }
    else
    {
        FLinearColor C;
        if (Mat->GetVectorParameterValue(Info, C))
        {
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
        return TEXT("");
    }
}

void UInspectorWorldSubsystem::UpdateModifiedStateFromCurrentMaterial(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return;

    const EInspectorMatParamType Type = (ChangeType == EInspectorChangeType::MaterialScalar)
        ? EInspectorMatParamType::Scalar
        : EInspectorMatParamType::Vector;

    const FString Key = MakeMaterialSnapshotKey(Comp, SlotIndex, Type, ParamName);
    if (Key.IsEmpty()) return;

    const FString Current = RI_GetMaterialParamValueText(Comp, SlotIndex, ChangeType, ParamName);

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, Current);
        ModifiedValueByKey.Remove(Key);
        return;
    }

    const FString& Baseline = BaselineValueByKey[Key];
    if (Current == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, Current);
    }
#endif
}

bool UInspectorWorldSubsystem::ExportSnapshot(bool bOnlyModified, FString& OutFilePath, FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED

    if (!IsRIEnabled())
    {
        OutError = TEXT("RuntimeInspector disabled (ri.Enable=0)");
        return false;
    }

    OutError.Reset();
    OutFilePath.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    const FString SelectedActorPath = ActorPtr->GetPathName();
    const FString SelectedActorClass = ActorPtr->GetClass()->GetPathName();
    const FString SelectedActorBaseName = RI_ExtractActorBaseName(ActorPtr->GetName());

    Root->SetNumberField(TEXT("schemaVersion"), 2);
    Root->SetStringField(TEXT("createdAtUtc"), FDateTime::UtcNow().ToIso8601());

    if (UWorld* World = GetWorld())
    {
        Root->SetStringField(TEXT("map"), World->GetMapName());

        FString LevelShortName = FPackageName::GetShortName(World->GetMapName());
        if (LevelShortName.StartsWith(TEXT("UEDPIE_")))
        {
            const int32 Under = LevelShortName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 6);
            if (Under != INDEX_NONE && Under + 1 < LevelShortName.Len())
            {
                LevelShortName = LevelShortName.Mid(Under + 1);
            }
        }
        Root->SetStringField(TEXT("levelName"), LevelShortName);
    }

    Root->SetStringField(TEXT("selectedActorPath"), SelectedActorPath);

    // v2 关键字段（用于抗 UAID 导入）
    Root->SetStringField(TEXT("selectedActorClass"), SelectedActorClass);
    Root->SetStringField(TEXT("selectedActorBaseName"), SelectedActorBaseName);


    TArray<TSharedPtr<FJsonValue>> Entries;

    auto AddPropertyEntry = [&](const FString& ActorPath,
        const FString& CompPath,
        const FString& ObjClassPath,
        const FString& ActorClassPath,
        const FString& ActorBaseName,
        const FString& PropName,
        const FString& ValueText,
        bool bHasValueInt=false,
        int64 ValueInt=0)
        {
            TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
            E->SetStringField(TEXT("kind"), TEXT("property"));
            E->SetStringField(TEXT("actorPath"), ActorPath);
            E->SetStringField(TEXT("componentPath"), CompPath);

            // 旧字段：对象 class（Actor 或 Component）
            E->SetStringField(TEXT("class"), ObjClassPath);

            // v2：用于定位 actor
            E->SetStringField(TEXT("actorClass"), ActorClassPath);
            E->SetStringField(TEXT("actorBaseName"), ActorBaseName);

            E->SetStringField(TEXT("property"), PropName);
            E->SetStringField(TEXT("value"), ValueText);

            // ✅ 新增：Enum/ByteEnum 等，用整数更稳
            if (bHasValueInt)
            {
                E->SetNumberField(TEXT("valueInt"), (double)ValueInt);
            }

            Entries.Add(MakeShared<FJsonValueObject>(E));
        };
    auto AddMaterialEntry = [&](const FString& ActorPath,
        const FString& CompPath,
        const FString& ActorClassPath,
        const FString& ActorBaseName,
        int32 SlotIndex,
        EInspectorMatParamType Type,
        const FString& ParamName,
        const FString& ValueText)
        {
            TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
            E->SetStringField(TEXT("kind"), (Type == EInspectorMatParamType::Scalar) ? TEXT("materialScalar") : TEXT("materialVector"));
            E->SetStringField(TEXT("actorPath"), ActorPath);
            E->SetStringField(TEXT("componentPath"), CompPath);

            // v2 新字段：用于导入定位 Actor
            E->SetStringField(TEXT("actorClass"), ActorClassPath);
            E->SetStringField(TEXT("actorBaseName"), ActorBaseName);

            E->SetNumberField(TEXT("slot"), SlotIndex);
            E->SetNumberField(TEXT("paramType"), (int32)Type);
            E->SetStringField(TEXT("param"), ParamName);
            E->SetStringField(TEXT("value"), ValueText);
            Entries.Add(MakeShared<FJsonValueObject>(E));
        };


    if (bOnlyModified)
    {
        // Only export keys in ModifiedValueByKey
        for (const TPair<FString, FString>& KV : ModifiedValueByKey)
        {
            const FString& Key = KV.Key;
            const FString& Val = KV.Value;

            if (Key.StartsWith(TEXT("P|")))
            {
                TArray<FString> Parts;
                Key.ParseIntoArray(Parts, TEXT("|"), false);
                // P|ActorPath|CompPath|ClassPath|PropName
                if (Parts.Num() >= 5)
                {
                    const FString ActorPath = Parts[1];
                    const FString ActorBase = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));

                    AddPropertyEntry(Parts[1], Parts[2], Parts[3], SelectedActorClass, ActorBase, Parts[4], Val);

                }
            }
            else if (Key.StartsWith(TEXT("M|")))
            {
                TArray<FString> Parts;
                Key.ParseIntoArray(Parts, TEXT("|"), false);
                // M|ActorPath|CompPath|Slot|TypeInt|ParamName
                if (Parts.Num() >= 6)
                {
                 
                    const FString ActorPath = Parts[1];
                    const FString ActorBase = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));

                    const int32 SlotIndex = FCString::Atoi(*Parts[3]);
                    const int32 TypeInt = FCString::Atoi(*Parts[4]);
                    const EInspectorMatParamType Type = (EInspectorMatParamType)TypeInt;

                    AddMaterialEntry(Parts[1], Parts[2], SelectedActorClass, ActorBase, SlotIndex, Type, Parts[5], Val);

                }
            }
        }
    }
    else
    {
        // Export Actor + whitelisted components (matching what the inspector can display by default).
        auto ExportObjectProps = [&](UObject* Obj, const TSet<FName>* Whitelist)
        {
            if (!Obj) return;
            UClass* Cls = Obj->GetClass();
            if (!Cls) return;

            /*const FString ActorPath = ActorPtr->GetPathName();
            const FString CompPath = Obj->IsA(AActor::StaticClass()) ? TEXT("") : Obj->GetPathName();
            const FString ClassPath = Cls->GetPathName();*/

            const FString ActorPath = SelectedActorPath;
            const FString CompPath = Obj->IsA(AActor::StaticClass()) ? TEXT("") : Obj->GetPathName();
            const FString ObjClassPath = Cls->GetPathName();

            for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
            {
                FProperty* Prop = *It;
                if (!Prop) continue;
                if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

                const bool bVisible = Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
                if (!bVisible) continue;

                if (!IsSupportedByInspector(Prop)) continue;
                if (Whitelist && !Whitelist->Contains(Prop->GetFName())) continue;

               

                FString ValText;
                if (!InspectorPropertyUtils::GetValueAsText(Obj, Prop->GetFName(), ValText))
                {
                    continue;
                }

                bool bHasValueInt = false;
                int64 ValueInt = 0;

                // 1) FEnumProperty（强类型 enum class / enum）
                if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
                {
                    if (const FNumericProperty* Under = EnumProp->GetUnderlyingProperty())
                    {
                        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
                        ValueInt = Under->GetSignedIntPropertyValue(ValuePtr);
                        bHasValueInt = true;
                    }
                }
                // 2) Byte + Enum（UE 很多 enum 是这种）
                else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
                {
                    if (ByteProp->Enum)
                    {
                        ValueInt = (int64)ByteProp->GetPropertyValue_InContainer(Obj);
                        bHasValueInt = true;
                    }
                }

                if (!InspectorPropertyUtils::CanSetFromText(Obj, Prop)) 
                {
                    continue;
                }

                AddPropertyEntry(ActorPath, CompPath, ObjClassPath, SelectedActorClass, SelectedActorBaseName,
                    Prop->GetName(), ValText,
                    bHasValueInt, ValueInt);

                //AddPropertyEntry(ActorPath, CompPath, ObjClassPath, SelectedActorClass, SelectedActorBaseName, Prop->GetName(), ValText);
            }
        };

        ExportObjectProps(ActorPtr, nullptr);

        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        for (UActorComponent* Comp : Components)
        {
            if (!IsWhitelistedComponent(Comp)) continue;

            const TSet<FName>* WL = GetWhitelistForWhitelistedComponent(Comp);
            ExportObjectProps(Comp, WL);
        }

        // If user is in MaterialOnly view, also export that slot parameters.
        if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
        {
            UMeshComponent* MC = ViewMeshComp.Get();
            if (MC && ViewMaterialSlot != INDEX_NONE)
            {
                UMaterialInterface* Mat = MC->GetMaterial(ViewMaterialSlot);
                if (Mat)
                {
                    const FString APath = ActorPtr->GetPathName();
                    const FString CPath = MC->GetPathName();

                    TArray<FMaterialParameterInfo> Infos;
                    TArray<FGuid> Ids;

                    Infos.Reset(); Ids.Reset();
                    Mat->GetAllScalarParameterInfo(Infos, Ids);
                    for (const FMaterialParameterInfo& Info : Infos)
                    {
                        const FString V = RI_GetMaterialParamValueText(Cast<UPrimitiveComponent>(MC), ViewMaterialSlot, EInspectorChangeType::MaterialScalar, Info.Name);
                        AddMaterialEntry(APath, CPath, SelectedActorClass, SelectedActorBaseName, ViewMaterialSlot, EInspectorMatParamType::Scalar, Info.Name.ToString(), V);
                    }

                    Infos.Reset(); Ids.Reset();
                    Mat->GetAllVectorParameterInfo(Infos, Ids);
                    for (const FMaterialParameterInfo& Info : Infos)
                    {
                        const FString V = RI_GetMaterialParamValueText(Cast<UPrimitiveComponent>(MC), ViewMaterialSlot, EInspectorChangeType::MaterialVector, Info.Name);
                        AddMaterialEntry(APath, CPath, SelectedActorClass, SelectedActorBaseName, ViewMaterialSlot, EInspectorMatParamType::Vector, Info.Name.ToString(), V);
                    }
                }
            }
        }
    }

    Root->SetArrayField(TEXT("entries"), Entries);

    FString OutJson;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
    {
        OutError = TEXT("JSON serialize failed");
        return false;
    }

    const FString Dir = GetSnapshotsDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));

    FString SafeMapName = TEXT("Map");
    if (UWorld* World = GetWorld())
    {
        SafeMapName = FPackageName::GetShortName(World->GetMapName());
        if (SafeMapName.StartsWith(TEXT("UEDPIE_")))
        {
            // Strip PIE prefix: UEDPIE_0_MapName -> MapName
            const int32 Under = SafeMapName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 6);
            if (Under != INDEX_NONE && Under + 1 < SafeMapName.Len())
            {
                SafeMapName = SafeMapName.Mid(Under + 1);
            }
        }
    }

    const FString FileName = FString::Printf(TEXT("Snapshot_%s_%s.json"), *SafeMapName, *Timestamp);
    OutFilePath = FPaths::Combine(Dir, FileName);

    if (!FFileHelper::SaveStringToFile(OutJson, *OutFilePath))
    {
        OutError = TEXT("Save failed");
        OutFilePath.Reset();
        PushToast(ERIToastType::Error, OutError, 3.0f);
        return false;
    }
    PushToast(ERIToastType::Success, FString::Printf(TEXT("Exported (%d changes)"), Entries.Num()), 1.5f);
    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("[RI] Snapshot exported: %s (entries=%d, onlyModified=%d)"), *OutFilePath, Entries.Num(), bOnlyModified ? 1 : 0);
    return true;
#else
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    PushToast(ERIToastType::Error, OutError, 3.0f);
    return false;
#endif
}

bool UInspectorWorldSubsystem::CaptureSelectionAsPatch(FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    TArray<FString> Keys;
    ModifiedValueByKey.GetKeys(Keys);
    Keys.Sort();

    const FString SelectedActorPath = ActorPtr->GetPathName();
    for (const FString& Key : Keys)
    {
        const FString* PatchedValuePtr = ModifiedValueByKey.Find(Key);
        if (!PatchedValuePtr)
        {
            continue;
        }

        FRIPatchOperation Operation;
        if (!TryBuildPatchOperationFromModifiedKey(Key, *PatchedValuePtr, BaselineValueByKey.Find(Key), Operation))
        {
            continue;
        }

        if (Operation.Target.ActorPath != SelectedActorPath)
        {
            continue;
        }

        OutBundle.Operations.Add(MoveTemp(Operation));
    }

    if (OutBundle.Operations.Num() <= 0)
    {
        OutError = TEXT("No modified values to capture");
        return false;
    }

    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    OutBundle.Version = 1;
    OutBundle.BundleId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    OutBundle.CapturedAtUtc = FDateTime::UtcNow().ToIso8601();
    OutBundle.CapturedFromSelection = SelectedActorPath;
    OutBundle.DisplayName = FString::Printf(
        TEXT("Patch_%s_%s"),
        *RI_ExtractActorBaseName(ActorPtr->GetName()),
        *Timestamp);

    return true;
#endif
}

bool UInspectorWorldSubsystem::StageSelectionAsPatch(FString& OutError)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    FRIPatchBundle Captured;
    if (!CaptureSelectionAsPatch(Captured, OutError))
    {
        return false;
    }

    StagedPatchBundle = MoveTemp(Captured);
    bHasStagedPatch = StagedPatchBundle.Operations.Num() > 0;
    InvalidateFileManagementSummaryCache();
    return bHasStagedPatch;
#endif
}

void UInspectorWorldSubsystem::ClearStagedPatch()
{
#if RUNTIME_INSPECTOR_ENABLED
    StagedPatchBundle = FRIPatchBundle();
    bHasStagedPatch = false;
    LastPatchApplyResult = FRIApplyResult();
    InvalidateFileManagementSummaryCache();
#endif
}

bool UInspectorWorldSubsystem::ApplyStagedPatch(FRIApplyResult& OutResult)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!HasStagedPatch())
    {
        OutResult = FRIApplyResult();
        OutResult.Summary = TEXT("No staged patch");
        LastPatchApplyResult = OutResult;
        return false;
    }

    const bool bSuccess = ApplyPatchBundle(StagedPatchBundle, OutResult);
    LastPatchApplyResult = OutResult;
    InvalidateFileManagementSummaryCache();
    return bSuccess;
#endif
}

bool UInspectorWorldSubsystem::RollbackStagedPatch(FRIApplyResult& OutResult)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    if (!HasStagedPatch())
    {
        OutResult = FRIApplyResult();
        OutResult.Summary = TEXT("No staged patch");
        LastPatchApplyResult = OutResult;
        return false;
    }

    const bool bSuccess = RollbackPatchBundle(StagedPatchBundle, OutResult);
    LastPatchApplyResult = OutResult;
    InvalidateFileManagementSummaryCache();
    return bSuccess;
#endif
}

bool UInspectorWorldSubsystem::SavePatchPreset(const FRIPatchPresetMetadata& InMetadata, FString& OutFilePath, FString& OutError)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    OutFilePath.Reset();
    return false;
#else
    OutFilePath.Reset();
    OutError.Reset();

    if (!HasStagedPatch())
    {
        OutError = TEXT("No staged patch");
        return false;
    }

    FRIPatchPresetMetadata Metadata = InMetadata;
    Metadata.DisplayName = Metadata.DisplayName.TrimStartAndEnd();
    if (Metadata.DisplayName.IsEmpty())
    {
        OutError = TEXT("Preset display name is required");
        return false;
    }

    if (StagedPatchBundle.Operations.Num() <= 0)
    {
        OutError = TEXT("Staged patch has no operations");
        return false;
    }

    if (Metadata.PresetId.IsEmpty())
    {
        Metadata.PresetId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    }
    if (Metadata.CreatedAt.IsEmpty())
    {
        Metadata.CreatedAt = FDateTime::UtcNow().ToIso8601();
    }

    FString Json;
    if (!SerializePatchPresetToJson(Metadata, StagedPatchBundle, Json, OutError))
    {
        return false;
    }

    const FString Dir = GetPresetsDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    OutFilePath = FPaths::Combine(Dir, Metadata.PresetId + TEXT(".json"));
    if (!FFileHelper::SaveStringToFile(Json, *OutFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutError = TEXT("Save failed");
        OutFilePath.Reset();
        return false;
    }

    InvalidateFileManagementSummaryCache();
    return true;
#endif
}

bool UInspectorWorldSubsystem::ListPatchPresets(TArray<FRIPatchPresetMetadata>& OutPresets, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    OutPresets.Reset();
    return false;
#else
    OutPresets.Reset();
    OutError.Reset();

    const FString Dir = GetPresetsDir();
    if (!IFileManager::Get().DirectoryExists(*Dir))
    {
        return true;
    }

    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(FPaths::Combine(Dir, TEXT("*.json"))), true, false);
    Files.Sort();

    for (const FString& FileName : Files)
    {
        const FString FullPath = FPaths::Combine(Dir, FileName);
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *FullPath))
        {
            continue;
        }

        FRIPatchPresetMetadata Metadata;
        FRIPatchBundle Bundle;
        FString LoadError;
        if (!DeserializePatchPresetFromJson(Content, Metadata, Bundle, LoadError))
        {
            continue;
        }

        OutPresets.Add(MoveTemp(Metadata));
    }

    OutPresets.Sort([](const FRIPatchPresetMetadata& A, const FRIPatchPresetMetadata& B)
    {
        const FString DisplayA = A.DisplayName.IsEmpty() ? A.PresetId : A.DisplayName;
        const FString DisplayB = B.DisplayName.IsEmpty() ? B.PresetId : B.DisplayName;
        if (DisplayA != DisplayB)
        {
            return DisplayA < DisplayB;
        }
        return A.PresetId < B.PresetId;
    });

    return true;
#endif
}

bool UInspectorWorldSubsystem::LoadPatchPreset(const FString& InPresetIdOrPath, FRIPatchPresetMetadata& OutMetadata, FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    OutMetadata = FRIPatchPresetMetadata();
    OutBundle = FRIPatchBundle();
    return false;
#else
    OutMetadata = FRIPatchPresetMetadata();
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    FString PresetFilePath;
    if (!ResolvePatchPresetFile(InPresetIdOrPath, PresetFilePath))
    {
        OutError = TEXT("Preset file not found");
        return false;
    }

    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *PresetFilePath))
    {
        OutError = TEXT("Failed to read preset file");
        return false;
    }

    return DeserializePatchPresetFromJson(Content, OutMetadata, OutBundle, OutError);
#endif
}

bool UInspectorWorldSubsystem::DeletePatchPreset(const FString& InPresetIdOrPath, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutError.Reset();

    FString PresetFilePath;
    if (!ResolvePatchPresetFile(InPresetIdOrPath, PresetFilePath))
    {
        OutError = TEXT("Preset file not found");
        return false;
    }

    if (!IFileManager::Get().Delete(*PresetFilePath, false, true))
    {
        OutError = TEXT("Delete failed");
        return false;
    }

    return true;
#endif
}

bool UInspectorWorldSubsystem::ApplyPatchPreset(const FString& InPresetIdOrPath, FRIApplyResult& OutResult, FString& OutError)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    OutError = OutResult.Summary;
    return false;
#else
    OutResult = FRIApplyResult();
    OutError.Reset();

    FRIPatchPresetMetadata Metadata;
    FRIPatchBundle Bundle;
    if (!LoadPatchPreset(InPresetIdOrPath, Metadata, Bundle, OutError))
    {
        OutResult.Summary = OutError;
        return false;
    }

    FRIPatchBundle RetargetedBundle;
    if (!RetargetPatchBundleToSelection(Metadata, Bundle, RetargetedBundle, OutError))
    {
        OutResult.Summary = OutError;
        return false;
    }

    const bool bSuccess = ApplyPatchBundle(RetargetedBundle, OutResult);
    if (!bSuccess && OutError.IsEmpty())
    {
        OutError = OutResult.Summary;
    }
    return bSuccess;
#endif
}

bool UInspectorWorldSubsystem::ExportPatchBundle(const FRIPatchBundle& InBundle, FString& OutFilePath, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutFilePath.Reset();
    OutError.Reset();

    FString Json;
    if (!SerializePatchBundleToJson(InBundle, Json, OutError))
    {
        return false;
    }

    const FString Dir = GetPatchesDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    const FString SafeName = FPaths::MakeValidFileName(
        InBundle.DisplayName.IsEmpty() ? TEXT("Patch") : InBundle.DisplayName);
    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    OutFilePath = FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s.json"), *SafeName, *Timestamp));

    if (!FFileHelper::SaveStringToFile(Json, *OutFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutError = TEXT("Save failed");
        OutFilePath.Reset();
        return false;
    }

    return true;
#endif
}

bool UInspectorWorldSubsystem::ImportPatchBundle(const FString& InFilePath, FRIPatchBundle& OutBundle, FString& OutError) const
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutBundle = FRIPatchBundle();
    OutError.Reset();

    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *InFilePath))
    {
        OutError = TEXT("Failed to read file");
        return false;
    }

    return DeserializePatchBundleFromJson(Content, OutBundle, OutError);
#endif
}

bool UInspectorWorldSubsystem::ApplyPatchBundle(const FRIPatchBundle& InBundle, FRIApplyResult& OutResult)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutResult = FRIApplyResult();

    if (InBundle.Operations.Num() <= 0)
    {
        OutResult.Summary = TEXT("No patch operations");
        return false;
    }

    TArray<FRIPatchOperation> SortedOperations = InBundle.Operations;
    SortPatchOperationsForApply(SortedOperations);

    bool bAnyApplied = false;
    for (const FRIPatchOperation& Operation : SortedOperations)
    {
        FRIPatchOperationResult OperationResult;
        const bool bApplied = ApplyPatchOperationValue(Operation, Operation.PatchedValue, OperationResult);
        OutResult.OperationResults.Add(OperationResult);

        if (bApplied)
        {
            bAnyApplied = true;
            OutResult.AppliedCount++;
        }
        else if (OperationResult.Status == ERIApplyOperationStatus::Skipped)
        {
            OutResult.SkippedCount++;
        }
        else
        {
            OutResult.FailedCount++;
        }
    }

    if (bAnyApplied)
    {
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    }

    FinalizePatchApplyResult(OutResult, TEXT("Applied"));
    LastPatchApplyResult = OutResult;
    return OutResult.bSuccess;
#endif
}

bool UInspectorWorldSubsystem::RollbackPatchBundle(const FRIPatchBundle& InBundle, FRIApplyResult& OutResult)
{
#if !RUNTIME_INSPECTOR_ENABLED
    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#else
    OutResult = FRIApplyResult();

    if (InBundle.Operations.Num() <= 0)
    {
        OutResult.Summary = TEXT("No patch operations");
        return false;
    }

    TArray<FRIPatchOperation> SortedOperations = InBundle.Operations;
    SortPatchOperationsForApply(SortedOperations);

    bool bAnyApplied = false;
    for (const FRIPatchOperation& Operation : SortedOperations)
    {
        FRIPatchOperationResult OperationResult;
        if (Operation.BaselineValue.IsEmpty())
        {
            OperationResult.Target = Operation.Target;
            OperationResult.Field = Operation.Field;
            OperationResult.Status = ERIApplyOperationStatus::Skipped;
            OperationResult.Message = TEXT("Missing baseline value");
            OutResult.OperationResults.Add(OperationResult);
            OutResult.SkippedCount++;
            continue;
        }

        const bool bApplied = ApplyPatchOperationValue(Operation, Operation.BaselineValue, OperationResult);
        OutResult.OperationResults.Add(OperationResult);

        if (bApplied)
        {
            bAnyApplied = true;
            OutResult.AppliedCount++;
        }
        else if (OperationResult.Status == ERIApplyOperationStatus::Skipped)
        {
            OutResult.SkippedCount++;
        }
        else
        {
            OutResult.FailedCount++;
        }
    }

    if (bAnyApplied)
    {
        RefreshPanel(EInspectorRefreshReason::ValuesChanged);
    }

    FinalizePatchApplyResult(OutResult, TEXT("RolledBack"));
    LastPatchApplyResult = OutResult;
    return OutResult.bSuccess;
#endif
}
static bool RI_SetEnumPropertyFromInt(UObject* TargetObj, const FName PropName, int64 ValueInt, FString& OutError)
{
    OutError.Reset();
    if (!TargetObj)
    {
        OutError = TEXT("TargetObj null");
        return false;
    }

    FProperty* Prop = TargetObj->GetClass()->FindPropertyByName(PropName);
    if (!Prop)
    {
        OutError = TEXT("Property not found");
        return false;
    }

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(TargetObj);

    // 1) FEnumProperty
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        FNumericProperty* Under = EnumProp->GetUnderlyingProperty();
        if (!Under)
        {
            OutError = TEXT("Enum underlying property missing");
            return false;
        }
        Under->SetIntPropertyValue(ValuePtr, ValueInt);
        return true;
    }

    // 2) Byte + Enum
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (ByteProp->Enum)
        {
            ByteProp->SetPropertyValue(ValuePtr, (uint8)ValueInt);
            return true;
        }
    }

    OutError = TEXT("Not an enum property");
    return false;
}
bool UInspectorWorldSubsystem::ImportSnapshot(const FString& InFilePath, FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED

    // Reset cached report
    LastImportReport = FRIImportReport();
    LastImportedSnapshotPath = InFilePath;

    auto SetEarlyFail = [&](const FString& Msg) -> bool
    {
        OutError = Msg;
        LastImportReport.bSuccess = false;
        LastImportReport.Summary = Msg;
        LastImportReport.Details = Msg;
        return false;
    };

    if (!IsRIEnabled())
    {
        return SetEarlyFail(TEXT("RuntimeInspector disabled (ri.Enable=0)"));
    }

    OutError.Reset();

    int32 AppliedCount = 0;
    int32 SkippedCount = 0;
    int32 HardFailCount = 0;
    FString CombinedWarnings;
    FString CombinedHardErrors;

    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *InFilePath))
    {
        return SetEarlyFail(TEXT("Failed to read file"));
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return SetEarlyFail(TEXT("Invalid JSON"));
    }

    const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
    if (!Root->TryGetArrayField(TEXT("entries"), Entries) || !Entries)
    {
        return SetEarlyFail(TEXT("Missing entries[]"));
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return SetEarlyFail(TEXT("World invalid"));
    }

    const int32 SchemaVersion = Root->HasField(TEXT("schemaVersion")) ? (int32)Root->GetNumberField(TEXT("schemaVersion")) : 1;

    auto ResolveActor = [&](const FString& ActorPath, const FString& ActorClass, const FString& ActorBaseName) -> AActor*
    {
        if (ActorPath.IsEmpty() && ActorBaseName.IsEmpty())
        {
            return nullptr;
        }

        // 1) 精确 path
        if (!ActorPath.IsEmpty())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (It->GetPathName() == ActorPath)
                {
                    return *It;
                }
            }
        }

        // 2) v2：class + baseName（抗 UAID）
        if (!ActorBaseName.IsEmpty())
        {
            AActor* Best = nullptr;

            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* A = *It;
                if (!A) continue;

                const FString ThisBase = RI_ExtractActorBaseName(A->GetName());
                if (ThisBase != ActorBaseName)
                {
                    continue;
                }

                // 如果有 class，就要求 class match；没有 class 也允许
                if (!ActorClass.IsEmpty() && !RI_ClassMatches(A, ActorClass))
                {
                    continue;
                }

                Best = A;
                break;
            }

            if (Best)
            {
                return Best;
            }
        }

        // 3) 兜底：你原来的 FallbackName（path 最后段）
        if (!ActorPath.IsEmpty())
        {
            FString FallbackName = RI_ExtractTailAfterLastDot(ActorPath);

            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (It->GetName() == FallbackName)
                {
                    return *It;
                }
            }
        }

        return nullptr;
    };

    auto ResolveComponent = [&](AActor* Owner, const FString& ComponentPath) -> UActorComponent*
    {
        if (!Owner || ComponentPath.IsEmpty()) return nullptr;

        TArray<UActorComponent*> Comps;
        Owner->GetComponents(Comps);

        for (UActorComponent* C : Comps)
        {
            if (C && C->GetPathName() == ComponentPath)
            {
                return C;
            }
        }

        FString FallbackName = ComponentPath;
        int32 Dot = INDEX_NONE;
        if (ComponentPath.FindLastChar(TEXT('.'), Dot))
        {
            FallbackName = ComponentPath.Mid(Dot + 1);
        }

        for (UActorComponent* C : Comps)
        {
            if (C && C->GetName() == FallbackName)
            {
                return C;
            }
        }
        return nullptr;
    };

    // We treat the current state as baseline for keys we touch in this import.
    // (That makes "Only Modified" show exactly what this snapshot changed.)

    bool bAllOK = true;
    int32 MissingCount = 0;
    FString CombinedMissingErrors;

    const bool bPrevApplying = bApplyingHistory;
    bApplyingHistory = true;

    for (const TSharedPtr<FJsonValue>& V : *Entries)
    {
        const TSharedPtr<FJsonObject> E = V.IsValid() ? V->AsObject() : nullptr;
        if (!E.IsValid()) continue;

        /*const FString Kind = E->GetStringField(TEXT("kind"));
        const FString ActorPath = E->GetStringField(TEXT("actorPath"));
        const FString CompPath = E->GetStringField(TEXT("componentPath"));

        AActor* TargetActor = ResolveActor(ActorPath);*/

        const FString Kind = E->GetStringField(TEXT("kind"));
        const FString ActorPath = E->GetStringField(TEXT("actorPath"));
        const FString CompPath = E->GetStringField(TEXT("componentPath"));

        // v2 新字段（兼容 v1：没有就从旧字段推）
        FString ActorClass;
        FString ActorBaseName;

        if (SchemaVersion >= 2)
        {
            E->TryGetStringField(TEXT("actorClass"), ActorClass);
            E->TryGetStringField(TEXT("actorBaseName"), ActorBaseName);
        }

        // v1 兼容：用 entry 的 "class" + actorPath 尾巴推 baseName
        if (ActorClass.IsEmpty())
        {
            if (SchemaVersion >= 2)
            {
                E->TryGetStringField(TEXT("actorClass"), ActorClass);
            }
            //E->TryGetStringField(TEXT("class"), ActorClass);
        }
        if (ActorBaseName.IsEmpty())
        {
            const FString FallbackInstName = RI_ExtractTailAfterLastDot(ActorPath);
            ActorBaseName = RI_ExtractActorBaseName(FallbackInstName);
        }

        AActor* TargetActor = ResolveActor(ActorPath, ActorClass, ActorBaseName);
        if (!TargetActor)
        {
            bAllOK = false;
            MissingCount++;
            CombinedMissingErrors += FString::Printf(
                TEXT("\nActor not found: path=%s base=%s class=%s"),
                *ActorPath, *ActorBaseName, *ActorClass
            );
            continue;
        }
        // --- Remap componentPath prefix if actor was resolved by fallback (UAID changed) ---
        auto RemapComponentPath = [&](const FString& InCompPath, const FString& OldActorPath, AActor* NewActor) -> FString
            {
                if (InCompPath.IsEmpty() || OldActorPath.IsEmpty() || !NewActor) return InCompPath;

                // 只有在 "旧ActorPath..." 这种情况下才做替换
                if (InCompPath.StartsWith(OldActorPath, ESearchCase::CaseSensitive))
                {
                    const FString NewActorPath = NewActor->GetPathName();
                    return NewActorPath + InCompPath.Mid(OldActorPath.Len());
                }
                return InCompPath;
            };

        const FString EffectiveCompPath = RemapComponentPath(CompPath, ActorPath, TargetActor);


        if (Kind == TEXT("property"))
        {
            const FString PropNameStr = E->GetStringField(TEXT("property"));
            const FString ValueText = E->GetStringField(TEXT("value"));

            UObject* TargetObj = TargetActor;
            if (!EffectiveCompPath.IsEmpty())
            {
                if (UActorComponent* C = ResolveComponent(TargetActor, EffectiveCompPath))
                {
                    TargetObj = C;
                }
                else
                {
                    bAllOK = false;
                    MissingCount++;
                    CombinedMissingErrors += FString::Printf(TEXT("\nComponent not found: %s"), *EffectiveCompPath);
                    continue;
                }
            }

            UInspectorPropertyItem* Temp = NewObject<UInspectorPropertyItem>(this);
            Temp->Init(TargetObj, FName(*PropNameStr));

  
            const FString OldText = Temp->GetValueText();

            double ValueIntD = 0.0;
            const bool bHasValueInt = E->TryGetNumberField(TEXT("valueInt"), ValueIntD);
            const int64 ValueInt = (int64)ValueIntD;

            FString Err;

            // ✅ 优先：如果有 valueInt，就尝试写 enum
            bool bApplied = false;
            if (bHasValueInt)
            {
                if (RI_SetEnumPropertyFromInt(TargetObj, FName(*PropNameStr), ValueInt, Err))
                {
                    bApplied = true;
                }
                else
                {
                    // 如果不是 enum，或者写失败，就继续走文本（Err 会被覆盖）
                    Err.Reset();
                }
            }

            if (!bApplied)
            {
                if (!Temp->ApplyFromText(ValueText, Err))
                {
                    // ✅ 软错误：不可写/不支持 -> 跳过但不算失败
                    if (Err.Contains(TEXT("not editable"), ESearchCase::IgnoreCase) ||
                        Err.Contains(TEXT("not supported"), ESearchCase::IgnoreCase))
                    {
                        SkippedCount++;
                        CombinedWarnings += FString::Printf(
                            TEXT("\nSkipped: obj=%s prop=%s val=%s (%s)"),
                            *GetNameSafe(TargetObj), *PropNameStr, *ValueText, *Err);
                        continue;
                    }

                    // ✅ 硬错误：解析失败/其它真正异常 -> 记为失败
                    HardFailCount++;
                    CombinedHardErrors += FString::Printf(
                        TEXT("\nApply failed: obj=%s prop=%s val=%s (%s)"),
                        *GetNameSafe(TargetObj), *PropNameStr, *ValueText, *Err);
                    continue;
                }

                // 成功
                AppliedCount++;
            }

            const FString NewText = Temp->GetValueText();
            const FString Key = MakePropertySnapshotKey(TargetObj, FName(*PropNameStr));
            TrackModifiedForKey(Key, OldText, NewText);
        }
        else if (Kind == TEXT("materialScalar") || Kind == TEXT("materialVector"))
        {
            const int32 SlotIndex = (int32)E->GetNumberField(TEXT("slot"));
            const FString ParamNameStr = E->GetStringField(TEXT("param"));
            const FString ValueText = E->GetStringField(TEXT("value"));

            if (EffectiveCompPath.IsEmpty())
            {
                bAllOK = false;
                MissingCount++;
                CombinedMissingErrors += FString::Printf(TEXT("\nMaterial entry missing componentPath (actor=%s)"), *ActorPath);
                continue;
            }

            UActorComponent* C = ResolveComponent(TargetActor, EffectiveCompPath);
            UMeshComponent* MC = C ? Cast<UMeshComponent>(C) : nullptr;
            if (!MC)
            {
                bAllOK = false;
                MissingCount++;
                CombinedMissingErrors += FString::Printf(TEXT("\nMeshComponent not found: %s"), *EffectiveCompPath);
                continue;
            }

            const EInspectorMatParamType Type = (Kind == TEXT("materialScalar")) ? EInspectorMatParamType::Scalar : EInspectorMatParamType::Vector;

            UInspectorMaterialParamItem* Temp = NewObject<UInspectorMaterialParamItem>(this);
            Temp->Init(MC, SlotIndex, FName(*ParamNameStr), Type);

            const FString OldText = Temp->GetValueText();
            FString Err;
            //if (!Temp->ApplyFromText(ValueText, Err))
            //{
            //    bAllOK = false;
            //    CombinedMissingErrors += FString::Printf(
            //        TEXT("\nApply failed: kind=%s comp=%s slot=%d param=%s val=%s (%s)"),
            //        *Kind, *GetNameSafe(MC), SlotIndex, *ParamNameStr, *ValueText, *Err);
            //    //CombinedMissingErrors += FString::Printf(TEXT("\nApply failed: %s slot=%d %s (%s)"), *GetNameSafe(MC), SlotIndex, *ParamNameStr, *Err);
            //    continue;
            //}
            if (!Temp->ApplyFromText(ValueText, Err))
            {
                // ✅ 软错误：不可写/不支持 -> 跳过但不算失败
                if (Err.Contains(TEXT("not editable"), ESearchCase::IgnoreCase) ||
                    Err.Contains(TEXT("not supported"), ESearchCase::IgnoreCase))
                {
                    SkippedCount++;
                    CombinedWarnings += FString::Printf(
                        TEXT("\nSkipped: obj=%s prop=%s val=%s (%s)"),
                        *GetNameSafe(MC), *ParamNameStr, *ValueText, *Err);
                    continue;
                }

                // ✅ 硬错误：解析失败/其它真正异常 -> 记为失败
                HardFailCount++;
                CombinedHardErrors += FString::Printf(
                    TEXT("\nApply failed: obj=%s prop=%s val=%s (%s)"),
                    *GetNameSafe(MC), *ParamNameStr, *ValueText, *Err);
                continue;
            }

            // 成功
            AppliedCount++;
            const FString NewText = Temp->GetValueText();
            const FString Key = MakeMaterialSnapshotKey(Cast<UPrimitiveComponent>(MC), SlotIndex, Type, FName(*ParamNameStr));
            TrackModifiedForKey(Key, OldText, NewText);
        }
    }

    bApplyingHistory = bPrevApplying;

    RefreshPanel(EInspectorRefreshReason::ValuesChanged);

    const bool bHasMissing = (!bAllOK) || (MissingCount > 0);
    const bool bHasHardFail = (HardFailCount > 0);
    const bool bAllOKFinal = (!bHasMissing && !bHasHardFail);

    auto SplitLines = [](const FString& In, TArray<FString>& Out)
    {
        Out.Reset();
        FString Trimmed = In.TrimStartAndEnd();
        if (!Trimmed.IsEmpty())
        {
            Trimmed.ParseIntoArrayLines(Out, true);
            for (FString& S : Out)
            {
                S = S.TrimStartAndEnd();
            }
        }
    };

    // Cache report for UI (Blueprint)
    LastImportReport.bSuccess = bAllOKFinal;
    LastImportReport.AppliedCount = AppliedCount;
    LastImportReport.SkippedCount = SkippedCount;
    LastImportReport.MissingCount = MissingCount;
    LastImportReport.HardFailCount = HardFailCount;
    SplitLines(CombinedMissingErrors, LastImportReport.MissingErrors);
    SplitLines(CombinedHardErrors, LastImportReport.HardErrors);
    SplitLines(CombinedWarnings, LastImportReport.Warnings);

    if (bAllOKFinal)
    {
        if (SkippedCount > 0)
        {
            OutError = CombinedWarnings.TrimStartAndEnd();
            LastImportReport.Summary = FString::Printf(TEXT("Imported with warnings: applied=%d skipped=%d"), AppliedCount, SkippedCount);
            LastImportReport.Details = OutError;
            PushToast(ERIToastType::Warning,
                FString::Printf(TEXT("Imported (%d applied, %d skipped — see log)"), AppliedCount, SkippedCount),
                3.0f);
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Import warnings: applied=%d skipped=%d\n%s"), AppliedCount, SkippedCount, *OutError);
        }
        else
        {
            LastImportReport.Summary = FString::Printf(TEXT("Imported: applied=%d"), AppliedCount);
            LastImportReport.Details.Reset();
            PushToast(ERIToastType::Success,
                FString::Printf(TEXT("Imported (%d applied)"), AppliedCount),
                1.5f);
            UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("[RI] Import ok: applied=%d"), AppliedCount);
        }
    }
    else
    {
        OutError = (CombinedMissingErrors + TEXT("\n") + CombinedHardErrors + TEXT("\n") + CombinedWarnings).TrimStartAndEnd();
        LastImportReport.Summary = FString::Printf(TEXT("Import failed: missing=%d hardFail=%d skipped=%d applied=%d"), MissingCount, HardFailCount, SkippedCount, AppliedCount);
        LastImportReport.Details = OutError;
        PushToast(ERIToastType::Error, TEXT("Import failed (see log)"), 3.5f);
        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Import failed: missing=%d hardFail=%d skipped=%d applied=%d\n%s"), MissingCount, HardFailCount, SkippedCount, AppliedCount, *OutError);
    }

    return bAllOKFinal;
//if (!bAllOK)
    //{
    //    OutError = CombinedErrors.TrimStartAndEnd();
    //    PushToast(ERIToastType::Warning, TEXT("Imported with errors (see log)"), 3.0f);
    //    UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning, TEXT("[RI] Import errors:\n%s"), *OutError);   // ✅新增
    //}
    //else {
    //    PushToast(ERIToastType::Success, TEXT("Imported"), 1.5f);
    //}
    //
    //UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Log, TEXT("[RI] Snapshot imported: %s (ok=%d)"), *InFilePath, bAllOK ? 1 : 0);
    //return bAllOK;
#else
    LastImportReport = FRIImportReport();
    LastImportReport.bSuccess = false;
    LastImportReport.Summary = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    LastImportReport.Details = LastImportReport.Summary;
    OutError = LastImportReport.Summary;
    return false;
#endif
}


static FString RI_ExtractActorShortNameFromPath(const FString& ActorPath)
{
    // ActorPath 形如：
    // /Game/...:PersistentLevel.BP_TestVarsActor_C_UAID_...
    int32 DotIdx = INDEX_NONE;
    if (ActorPath.FindLastChar(TEXT('.'), DotIdx))
    {
        FString Tail = ActorPath.Mid(DotIdx + 1); // BP_TestVarsActor_C_UAID_...
        // 去掉 _C_... 之后的部分，尽量友好
        int32 CIdx = INDEX_NONE;
        if (Tail.FindChar(TEXT('_'), CIdx))
        {
            // 先尝试截到 _C
            int32 CPos = Tail.Find(TEXT("_C"));
            if (CPos != INDEX_NONE)
            {
                return Tail.Left(CPos); // BP_TestVarsActor
            }
        }
        return Tail;
    }
    return ActorPath;
}

static int64 RI_ParseIsoUtcToUnixSeconds(const FString& IsoUtc)
{
    // "2026-01-05T22:08:26.576Z"
    FDateTime DT;
    if (FDateTime::ParseIso8601(*IsoUtc, DT))
    {
        return DT.ToUnixTimestamp();
    }
    return 0;
}

bool UInspectorWorldSubsystem::ReadSnapshotHeader(
    const FString& FullPath,
    FString& OutCreatedAtUtc,
    FString& OutMap,
    FString& OutSelectedActorPath,
    int32& OutEntryCount) const
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FullPath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    OutCreatedAtUtc = Root->GetStringField(TEXT("createdAtUtc"));
    OutMap = Root->GetStringField(TEXT("map"));
    OutSelectedActorPath = Root->GetStringField(TEXT("selectedActorPath"));

    OutEntryCount = 0;
    const TArray<TSharedPtr<FJsonValue>>* EntriesPtr = nullptr;
    if (Root->TryGetArrayField(TEXT("entries"), EntriesPtr) && EntriesPtr)
    {
        OutEntryCount = EntriesPtr->Num();
    }

    return true;
}

void UInspectorWorldSubsystem::GetSnapshotList(TArray<UObject*>& OutItems) const
{
    OutItems.Reset();

    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*Dir))
    {
        return;
    }

    TArray<FString> Files;
    PF.FindFilesRecursively(Files, *Dir, TEXT(".json"));

    TArray<UInspectorSnapshotItem*> Temp;

    for (const FString& FullPath : Files)
    {
        FString CreatedAtUtc, MapName, SelectedActorPath;
        int32 EntryCount = 0;
        if (!ReadSnapshotHeader(FullPath, CreatedAtUtc, MapName, SelectedActorPath, EntryCount))
        {
            continue;
        }

        UInspectorSnapshotItem* Item = NewObject<UInspectorSnapshotItem>(const_cast<UInspectorWorldSubsystem*>(this));
        Item->FullPath = FullPath;
        Item->FileName = FPaths::GetCleanFilename(FullPath);
        Item->CreatedAtUtc = CreatedAtUtc;
        Item->MapName = MapName;
        Item->SelectedActorPath = SelectedActorPath;
        Item->EntryCount = EntryCount;
        Item->ActorShortName = RI_ExtractActorShortNameFromPath(SelectedActorPath);
        Item->CreatedAtUnixSeconds = RI_ParseIsoUtcToUnixSeconds(CreatedAtUtc);

        Temp.Add(Item);
    }

    // 最新的排前面
    Temp.Sort([](const UInspectorSnapshotItem& A, const UInspectorSnapshotItem& B)
        {
            return A.CreatedAtUnixSeconds > B.CreatedAtUnixSeconds;
        });

    for (UInspectorSnapshotItem* It : Temp)
    {
        OutItems.Add(It);
    }
}

FString UInspectorWorldSubsystem::GetSnapshotDirectory() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
}

static bool RI_IsUnderDir(const FString& FilePath, const FString& DirPath)
{
    FString NormFile = FPaths::ConvertRelativePathToFull(FilePath);
    FString NormDir = FPaths::ConvertRelativePathToFull(DirPath);

    FPaths::NormalizeFilename(NormFile);
    FPaths::NormalizeFilename(NormDir);

    // 保证目录末尾有 /
    if (!NormDir.EndsWith(TEXT("/")))
    {
        NormDir += TEXT("/");
    }

    return NormFile.StartsWith(NormDir, ESearchCase::IgnoreCase);
}

void UInspectorWorldSubsystem::CopySnapshotPathToClipboard(const FString& FullPath)
{
    

    if (FullPath.IsEmpty())
    {
        PushToast(ERIToastType::Warning, TEXT("Empty path"), 2.0f);
    }
    else
    {
        // 允许复制任何字符串（即使不是合法路径），但你的 UI 传的一般是完整路径
        FPlatformApplicationMisc::ClipboardCopy(*FullPath);
        PushToast(ERIToastType::Info, TEXT("Copied"), 1.0f);
    }
}

bool UInspectorWorldSubsystem::DeleteSnapshotFile(const FString& FullPath, FString& OutError) 
{
    OutError.Reset();

    if (!IsRIEnabled())
    {
        OutError = TEXT("RuntimeInspector disabled (ri.Enable=0)");
        return false;
    }

    if (FullPath.IsEmpty())
    {
        OutError = TEXT("Empty path.");
        return false;
    }

    const FString SnapDir = GetSnapshotDirectory();

    // 安全：只允许删除 Snapshots 目录下的文件，避免误删用户任意路径
    if (!RI_IsUnderDir(FullPath, SnapDir))
    {
        OutError = FString::Printf(TEXT("Refuse to delete file outside snapshot dir: %s"), *SnapDir);
        return false;
    }

    // 只允许删 .json（你当前就是 json）
    if (!FullPath.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("Only .json files can be deleted.");
        return false;
    }

    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

    if (!PF.FileExists(*FullPath))
    {
        OutError = TEXT("File not found.");
        return false;
    }

    if (!PF.DeleteFile(*FullPath))
    {
        OutError = TEXT("Delete failed (platform file returned false).");
        return false;
    }
    PushToast(ERIToastType::Success, FString::Printf(TEXT("Deleted")), 1.2f);
    return true;
}

bool UInspectorWorldSubsystem::IsPropertyItemModified(const UInspectorPropertyItem* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return false;
    UObject* Obj = Item->GetTargetObject();
    const FName PropName = Item->GetPropertyFName();
    const FString Key = MakePropertySnapshotKey(Obj, PropName);
    return !Key.IsEmpty() && ModifiedValueByKey.Contains(Key);
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::IsMaterialItemModified(const UInspectorMaterialParamItem* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return false;

    UMeshComponent* MeshComp = Item->GetMeshComponent(); // 你如果没暴露 Getter，就在 MaterialParamItem 里加 BlueprintPure/C++ Getter
    if (!MeshComp) return false;

    UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(MeshComp);
    if (!Prim) return false;

    const FString Key = MakeMaterialSnapshotKey(
        Prim,
        Item->GetSlotIndex(),
        Item->GetParamType(),
        Item->GetParamName()
    );

    return !Key.IsEmpty() && ModifiedValueByKey.Contains(Key);
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::GetPropertyItemsForSelectedEx(const FString& SearchText, bool bOnlyModified, TArray<UObject*>& OutItems)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bOnlyModified)
    {
        GetPropertyItemsForSelected(SearchText, OutItems);
        return;
    }

    OutItems.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    const bool bSearchMode = !SearchText.IsEmpty();

    auto FilterOnlyModified = [&](TArray<UObject*>& Items)
        {
            Items.RemoveAll([&](UObject* Obj)
                {
                    if (UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Obj))
                    {
                        return !IsPropertyItemModified(P);
                    }
                    if (UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Obj))
                    {
                        return !IsMaterialItemModified(M);
                    }
                    return false; // GroupItem 不在这里删（组的删留由外层决定）
                });
        };

    if (RI_IsPinnedRootKey(SelectedGroupKey))
    {
        GetPinnedItemsForSelected(SearchText, OutItems);
        FilterOnlyModified(OutItems);
        return;
    }

    // ----- Focused：如果左侧选中了某个节点（组件/MaterialsRoot），右侧只显示该对象 -----
    // Slot 的情况不走这里：Slot 应该已经切到 PropertyViewMode==MaterialOnly（你在 SetSelectedGroupItem 里做）
    UObject* InspectObj = SelectedInspectObject ? SelectedInspectObject.Get() : ActorPtr;

    const bool bHasOverride =
        (SelectedInspectObject != nullptr) &&
        (PropertyViewMode != ERIPropertyViewMode::MaterialOnly);

    if (bHasOverride && SelectedInspectObject.Get() != ActorPtr)
    {
        TArray<UObject*> Props;
        AppendPropertiesForObject(InspectObj, SearchText, Props, TEXT(""), bSearchMode);

        // OnlyModified 模式下过滤
        FilterOnlyModified(Props);

        // 没有任何 modified（或搜索后为空）就直接空列表返回
        if (Props.Num() == 0)
        {
            return;
        }

        OutItems.Append(Props);
        return; // ✅ 关键：别再走下面 Actor/Components 全量逻辑
    }



    // ----- MaterialOnly：右侧显示当前选中的 Material Slot 参数 -----
    if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
    {
        UMeshComponent* MC = ViewMeshComp.Get();
        if (!MC || ViewMaterialSlot == INDEX_NONE)
        {
            return;
        }

        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MC);
        if (!SMC) return;

        UMaterialInterface* Mat = SMC->GetMaterial(ViewMaterialSlot);
        if (!Mat) return;

        TArray<UObject*> TempItems;

        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;

        // Scalar
        Infos.Reset(); Ids.Reset();
        Mat->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Scalar);

            if (!bOnlyModified || IsMaterialItemModified(Item))
            {
                TempItems.Add(Item);
            }
        }

        // Vector
        Infos.Reset(); Ids.Reset();
        Mat->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Vector);

            if (!bOnlyModified || IsMaterialItemModified(Item))
            {
                TempItems.Add(Item);
            }
        }

        // OnlyModify 且没有任何 modified 参数 → 列表保持空（这是正确的）
        if (bOnlyModified && TempItems.Num() == 0)
        {
            return;
        }

        // 标题组（可选，但体验更好）
        UInspectorGroupItem* MatOnlyGroup = GetOrCreateGroupItem(TEXT("VIEW_MATERIAL_ONLY"));
        MatOnlyGroup->Kind = EInspectorGroupKind::Component;
        MatOnlyGroup->DisplayName = TEXT("Material Parameters");
        MatOnlyGroup->StableKey = TEXT("VIEW_MATERIAL_ONLY");
        MatOnlyGroup->bExpanded = true;

        OutItems.Add(MatOnlyGroup);
        OutItems.Append(TempItems);

        return; // ✅ 必须 return，阻止走 Actor/Components 分支
    }
    AppendPropertiesForObject(ActorPtr, SearchText, OutItems, TEXT(""), bSearchMode);
    FilterOnlyModified(OutItems);
#endif
}

bool UInspectorWorldSubsystem::RevertModifiedForSelection(int32& OutRevertedCount, int32& OutFailedCount, FString& OutError)
{
#if RUNTIME_INSPECTOR_ENABLED
    OutRevertedCount = 0;
    OutFailedCount = 0;
    OutError.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    const FString SelectedActorPath = ActorPtr->GetPathName();

    // 先拷贝 keys（遍历过程中 ModifiedValueByKey 会被移除）
    TArray<FString> Keys;
    ModifiedValueByKey.GetKeys(Keys);

    // 用于 ResolveComponent（复用你 import 那套也行，这里给一个轻量版）
    auto ResolveComponent = [&](AActor* Owner, const FString& ComponentPath) -> UActorComponent*
        {
            if (!Owner || ComponentPath.IsEmpty()) return nullptr;

            TArray<UActorComponent*> Comps;
            Owner->GetComponents(Comps);

            for (UActorComponent* C : Comps)
            {
                if (C && C->GetPathName() == ComponentPath)
                {
                    return C;
                }
            }

            // fallback by name
            FString Tail = ComponentPath;
            int32 Dot = INDEX_NONE;
            if (ComponentPath.FindLastChar(TEXT('.'), Dot))
            {
                Tail = ComponentPath.Mid(Dot + 1);
            }

            for (UActorComponent* C : Comps)
            {
                if (C && C->GetName() == Tail)
                {
                    return C;
                }
            }
            return nullptr;
        };

    FString CombinedErr;

    for (const FString& Key : Keys)
    {
        // 只处理当前选中的 actor
        if (Key.StartsWith(TEXT("P|")))
        {
            TArray<FString> Parts;
            Key.ParseIntoArray(Parts, TEXT("|"), false);
            // P|ActorPath|CompPath|ClassPath|PropName
            if (Parts.Num() < 5) continue;

            const FString& ActorPath = Parts[1];
            const FString& CompPath = Parts[2];
            const FString& PropName = Parts[4];

            if (ActorPath != SelectedActorPath) continue;

            const FString* BaselinePtr = BaselineValueByKey.Find(Key);
            if (!BaselinePtr) continue;

            UObject* TargetObj = ActorPtr;
            if (!CompPath.IsEmpty())
            {
                if (UActorComponent* C = ResolveComponent(ActorPtr, CompPath))
                {
                    TargetObj = C;
                }
                else
                {
                    OutFailedCount++;
                    CombinedErr += FString::Printf(TEXT("\nComponent not found: %s"), *CompPath);
                    continue;
                }
            }

            UInspectorPropertyItem* Temp = NewObject<UInspectorPropertyItem>(this);
            Temp->Init(TargetObj, FName(*PropName));

            FString Err;
            if (Temp->ApplyFromText(*BaselinePtr, Err))
            {
                OutRevertedCount++;
            }
            else
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nRevert failed: %s.%s (%s)"),
                    *GetNameSafe(TargetObj), *PropName, *Err);
            }
        }
        else if (Key.StartsWith(TEXT("M|")))
        {
            TArray<FString> Parts;
            Key.ParseIntoArray(Parts, TEXT("|"), false);
            // M|ActorPath|CompPath|Slot|TypeInt|ParamName
            if (Parts.Num() < 6) continue;

            const FString& ActorPath = Parts[1];
            const FString& CompPath = Parts[2];
            const int32 SlotIndex = FCString::Atoi(*Parts[3]);
            const EInspectorMatParamType Type = (EInspectorMatParamType)FCString::Atoi(*Parts[4]);
            const FString& ParamName = Parts[5];

            if (ActorPath != SelectedActorPath) continue;

            const FString* BaselinePtr = BaselineValueByKey.Find(Key);
            if (!BaselinePtr) continue;

            UActorComponent* C = ResolveComponent(ActorPtr, CompPath);
            UMeshComponent* MC = C ? Cast<UMeshComponent>(C) : nullptr;
            if (!MC)
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nMeshComponent not found: %s"), *CompPath);
                continue;
            }

            UInspectorMaterialParamItem* Temp = NewObject<UInspectorMaterialParamItem>(this);
            Temp->Init(MC, SlotIndex, FName(*ParamName), Type);

            FString Err;
            if (Temp->ApplyFromText(*BaselinePtr, Err))
            {
                OutRevertedCount++;
            }
            else
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nRevert failed: %s slot=%d %s (%s)"),
                    *GetNameSafe(MC), SlotIndex, *ParamName, *Err);
            }
        }
    }

    // 统一刷新一次（避免每条都刷）
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);

    if (!CombinedErr.IsEmpty())
    {
        OutError = CombinedErr.TrimStartAndEnd();
    }

    // Toast（如果你有 PushToast）
    if (OutFailedCount == 0)
    {
        PushToast(ERIToastType::Success, FString::Printf(TEXT("Reset %d changes"), OutRevertedCount), 1.5f);
    }
    else
    {
        PushToast(ERIToastType::Warning, FString::Printf(TEXT("Reset %d, failed %d (see log)"), OutRevertedCount, OutFailedCount), 3.0f);
    }

    return OutRevertedCount > 0 && OutFailedCount == 0;

#else
    OutError = TEXT("Not available (RUNTIME_INSPECTOR_ENABLED=0)");
    return false;
#endif
}
bool UInspectorWorldSubsystem::IsItemModified(UObject* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return false;

    if (const UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Item))
    {
        return IsPropertyItemModified(P);
    }
    if (const UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Item))
    {
        return IsMaterialItemModified(M);
    }

    // GroupItem / 其它：不算“modified item”
    return false;
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::ClearSelectedGroupItem()
{
#if RUNTIME_INSPECTOR_ENABLED
    SelectedInspectObject = nullptr;
    SelectedMaterialSlotIndex = INDEX_NONE;
    SelectedGroupKey.Reset();
#endif
}

void UInspectorWorldSubsystem::SetSelectedGroupItem(UInspectorGroupItem* Item)
{
#if RUNTIME_INSPECTOR_ENABLED
    // 1) 每次选择都先清理“材质槽”状态，避免从上一次 slot 串到别的节点
    SelectedMaterialSlotIndex = INDEX_NONE;
    SelectedGroupKey = Item ? Item->StableKey : TEXT("");

    // 2) 默认回退：Actor
    AActor* ActorPtr = SelectedActor.Get();
    SelectedInspectObject = ActorPtr;

    if (!Item) return;

    // 3) ROOT_ACTOR：强制显示 Actor
    if (Item->StableKey == TEXT("ROOT_ACTOR"))
    {
        SelectedInspectObject = ActorPtr;
        return;
    }

    // 4) ROOT_COMPONENTS：一般不显示任何组件属性（你想显示 Actor 也行）
    if (Item->StableKey == TEXT("ROOT_COMPONENTS"))
    {
        SelectedInspectObject = ActorPtr;
        return;
    }

    if (RI_IsPinnedRootKey(Item->StableKey))
    {
        SelectedInspectObject = ActorPtr;
        PropertyViewMode = ERIPropertyViewMode::Full;
        return;
    }

    // 5) 其它：优先用 GroupItem.TargetObject（你的树里组件/SMC 都是这个）
    if (Item->TargetObject)
    {
        SelectedInspectObject = Item->TargetObject;
    }

    // 6) Slot：右侧显示材质参数（slot index 必须有效）
    //    你现在 Kind 可能都等于 Component，所以只用 StableKey/MaterialSlotIndex 判断
    const bool bIsSlot = (Item->MaterialSlotIndex != INDEX_NONE) ||
        Item->StableKey.Contains(TEXT(":MATERIALS:MAT:"));

    if (bIsSlot)
    {
        int32 SlotIndex = Item->MaterialSlotIndex;

        // 兜底：如果字段没填，就从 StableKey 解析 ":MAT:%d"
        if (SlotIndex == INDEX_NONE)
        {
            // 解析最后一个 ":MAT:" 后面的数字
            int32 MatPos = Item->StableKey.Find(TEXT(":MAT:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            if (MatPos != INDEX_NONE)
            {
                const FString Tail = Item->StableKey.Mid(MatPos + 5);
                int32 Parsed = INDEX_NONE;
                if (LexTryParseString(Parsed, *Tail))
                {
                    SlotIndex = Parsed;
                }
            }
        }

        if (SlotIndex != INDEX_NONE)
        {
            SelectedMaterialSlotIndex = SlotIndex;
        }
    }

    // 选中 slot -> 右侧进入材质参数模式
    if (SelectedMaterialSlotIndex != INDEX_NONE)
    {
        ViewMeshComp = Cast<UMeshComponent>(SelectedInspectObject.Get()); // 你 TargetObject 放的是 SMC/MC
        ViewMaterialSlot = SelectedMaterialSlotIndex;
        PropertyViewMode = ERIPropertyViewMode::MaterialOnly;
    }
    else
    {
        // 选中 Actor/组件/MaterialsRoot -> 回到普通属性模式
        // 这里用你项目里“默认模式”的枚举值替换（比如 All / Default / ActorAndComponents 等）
        PropertyViewMode = ERIPropertyViewMode::Full;
    }
#endif
}

bool UInspectorWorldSubsystem::FocusSelectedActorComponentByName(const FString& ComponentName, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    const FString Wanted = ComponentName.TrimStartAndEnd();
    if (Wanted.IsEmpty())
    {
        OutError = TEXT("Component name required");
        return false;
    }

    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);

    UActorComponent* Match = nullptr;
    for (UActorComponent* Comp : Components)
    {
        if (!Comp)
        {
            continue;
        }

        if (Comp->GetName().Equals(Wanted, ESearchCase::IgnoreCase))
        {
            Match = Comp;
            break;
        }
    }

    if (!Match)
    {
        for (UActorComponent* Comp : Components)
        {
            if (!Comp)
            {
                continue;
            }

            if (Comp->GetName().Contains(Wanted, ESearchCase::IgnoreCase))
            {
                Match = Comp;
                break;
            }
        }
    }

    if (!Match)
    {
        OutError = FString::Printf(TEXT("Component not found: %s"), *Wanted);
        return false;
    }

    SelectedGroupKey = MakeComponentKey(ActorPtr, Match);
    SelectedInspectObject = Match;
    SelectedMaterialSlotIndex = INDEX_NONE;
    PropertyViewMode = ERIPropertyViewMode::Full;
    ViewMeshComp = nullptr;
    ViewMaterialSlot = INDEX_NONE;
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
    return true;
#endif
}

bool UInspectorWorldSubsystem::NavigateToPinnedItem(UObject* ItemObject, FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!ItemObject)
    {
        OutError = TEXT("Pinned target no longer exists");
        return false;
    }

    FString ShowPageError;
    if (!SetVisiblePageByName(TEXT("Actor"), ShowPageError))
    {
        OutError = ShowPageError.IsEmpty() ? TEXT("Failed to show Actor page") : ShowPageError;
        return false;
    }

    if (PanelWidget.IsValid() && PanelWidget->WidgetTree)
    {
        RI_TrySetEditableSearchText(PanelWidget->WidgetTree->FindWidget(TEXT("ETB_Search")), FText::GetEmpty());
        if (UCheckBox* OnlyModifyToggle = Cast<UCheckBox>(PanelWidget->WidgetTree->FindWidget(TEXT("Toggle_OnModify"))))
        {
            OnlyModifyToggle->SetIsChecked(false);
        }
    }
    CurrentActorSearchText.Reset();

    auto ResolveActorFromTarget = [](UObject* TargetObject) -> AActor*
    {
        if (AActor* Actor = Cast<AActor>(TargetObject))
        {
            return Actor;
        }
        if (UActorComponent* Component = Cast<UActorComponent>(TargetObject))
        {
            return Component->GetOwner();
        }
        return nullptr;
    };

    auto ScrollToResolvedItem = [this](UObject* ResolvedItem) -> bool
    {
        if (!ResolvedItem)
        {
            return false;
        }

        if (UInspectorPropertiesSectionWidget* SectionWidget = ActorPropertiesSectionWidget.Get())
        {
            SectionWidget->RefreshFromSubsystem();
            return SectionWidget->ScrollToItemForAutomation(ResolvedItem);
        }

        return false;
    };

    auto FindResolvedPropertyItem = [this](UObject* TargetObject, FName PropertyName) -> UInspectorPropertyItem*
    {
        TArray<UObject*> Items;
        GetPropertyItemsForSelectedEx(TEXT(""), false, Items);
        for (UObject* Candidate : Items)
        {
            if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(Candidate))
            {
                if (PropertyItem->GetTargetObject() == TargetObject && PropertyItem->GetPropertyFName() == PropertyName)
                {
                    return PropertyItem;
                }
            }
        }
        return nullptr;
    };

    auto FindResolvedMaterialItem = [this](UMeshComponent* MeshComponent, int32 SlotIndex, FName ParamName, EInspectorMatParamType ParamType) -> UInspectorMaterialParamItem*
    {
        TArray<UObject*> Items;
        GetPropertyItemsForSelectedEx(TEXT(""), false, Items);
        for (UObject* Candidate : Items)
        {
            if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(Candidate))
            {
                if (MaterialItem->GetMeshComponent() == MeshComponent
                    && MaterialItem->GetSlotIndex() == SlotIndex
                    && MaterialItem->GetParamName() == ParamName
                    && MaterialItem->GetParamType() == ParamType)
                {
                    return MaterialItem;
                }
            }
        }
        return nullptr;
    };

    auto FindResolvedFunctionItem = [this](UObject* TargetObject, FName FunctionName) -> UInspectorFunctionItem*
    {
        TArray<UInspectorFunctionItem*> Items;
        GetFunctionItemsForSelected(TEXT(""), Items);
        for (UInspectorFunctionItem* Candidate : Items)
        {
            if (Candidate
                && Candidate->GetTargetObject() == TargetObject
                && Candidate->GetFunctionFName() == FunctionName)
            {
                return Candidate;
            }
        }
        return nullptr;
    };

    if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(ItemObject))
    {
        UObject* TargetObject = PropertyItem->GetTargetObject();
        AActor* TargetActor = ResolveActorFromTarget(TargetObject);
        if (!TargetActor)
        {
            OutError = TEXT("Pinned property target actor is unavailable");
            return false;
        }

        if (SelectedActor.Get() != TargetActor)
        {
            SetSelectedActor(TargetActor);
        }

        if (UActorComponent* Component = Cast<UActorComponent>(TargetObject))
        {
            if (!FocusSelectedActorComponentByName(Component->GetName(), OutError))
            {
                return false;
            }
        }
        else
        {
            SelectedInspectObject = TargetActor;
            SelectedGroupKey = TEXT("ROOT_ACTOR");
            SelectedMaterialSlotIndex = INDEX_NONE;
            PropertyViewMode = ERIPropertyViewMode::Full;
            ViewMeshComp = nullptr;
            ViewMaterialSlot = INDEX_NONE;
            RefreshPanel(EInspectorRefreshReason::StructureChanged);
        }

        if (UInspectorPropertyItem* ResolvedPropertyItem = FindResolvedPropertyItem(TargetObject, PropertyItem->GetPropertyFName()))
        {
            if (ScrollToResolvedItem(ResolvedPropertyItem))
            {
                return true;
            }
        }

        OutError = FString::Printf(TEXT("Property row not found: %s"), *PropertyItem->GetPropertyName());
        return false;
    }

    if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(ItemObject))
    {
        UMeshComponent* MeshComponent = MaterialItem->GetMeshComponent();
        AActor* TargetActor = MeshComponent ? MeshComponent->GetOwner() : nullptr;
        if (!MeshComponent || !TargetActor)
        {
            OutError = TEXT("Pinned material target is unavailable");
            return false;
        }

        if (SelectedActor.Get() != TargetActor)
        {
            SetSelectedActor(TargetActor);
        }

        if (!FocusSelectedActorComponentByName(MeshComponent->GetName(), OutError))
        {
            return false;
        }

        SetGroupExpanded(TEXT("ROOT_COMPONENTS"), true);
        const FString ComponentKey = MakeComponentKey(TargetActor, MeshComponent);
        SetGroupExpanded(ComponentKey, true);
        RefreshPanel(EInspectorRefreshReason::StructureChanged);

        UInspectorGroupItem* ComponentGroup = GetOrCreateGroupItem(ComponentKey);
        if (!ComponentGroup)
        {
            OutError = TEXT("Failed to resolve component group");
            return false;
        }

        ComponentGroup->Kind = EInspectorGroupKind::Component;
        ComponentGroup->TargetObject = MeshComponent;
        ComponentGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *MeshComponent->GetName(), *MeshComponent->GetClass()->GetName());
        ComponentGroup->StableKey = ComponentKey;
        ComponentGroup->Depth = 1;
        ComponentGroup->bExpanded = true;

        TArray<UObject*> ComponentChildren;
        GetGroupTreeChildrenForItem(ComponentGroup, TEXT(""), ComponentChildren);

        UInspectorGroupItem* MaterialsRootItem = nullptr;
        for (UObject* ChildObject : ComponentChildren)
        {
            if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ChildObject))
            {
                if (GroupItem->IsMaterialsRoot())
                {
                    MaterialsRootItem = GroupItem;
                    break;
                }
            }
        }

        if (!MaterialsRootItem)
        {
            OutError = TEXT("Materials node not found");
            return false;
        }

        SetGroupExpanded(MaterialsRootItem->StableKey, true);
        RefreshPanel(EInspectorRefreshReason::StructureChanged);

        TArray<UObject*> MaterialSlotChildren;
        GetGroupTreeChildrenForItem(MaterialsRootItem, TEXT(""), MaterialSlotChildren);

        UInspectorGroupItem* MaterialSlotItem = nullptr;
        for (UObject* ChildObject : MaterialSlotChildren)
        {
            if (UInspectorGroupItem* GroupItem = Cast<UInspectorGroupItem>(ChildObject))
            {
                if (GroupItem->MaterialSlotIndex == MaterialItem->GetSlotIndex())
                {
                    MaterialSlotItem = GroupItem;
                    break;
                }
            }
        }

        if (!MaterialSlotItem)
        {
            OutError = TEXT("Material slot node not found");
            return false;
        }

        SetSelectedGroupItem(MaterialSlotItem);
        RequestActorPageRefresh();

        if (UInspectorMaterialParamItem* ResolvedMaterialItem = FindResolvedMaterialItem(
            MeshComponent,
            MaterialItem->GetSlotIndex(),
            MaterialItem->GetParamName(),
            MaterialItem->GetParamType()))
        {
            if (ScrollToResolvedItem(ResolvedMaterialItem))
            {
                return true;
            }
        }

        OutError = FString::Printf(TEXT("Material parameter row not found: %s"), *MaterialItem->GetPropertyName());
        return false;
    }

    if (UInspectorFunctionItem* FunctionItem = Cast<UInspectorFunctionItem>(ItemObject))
    {
        UObject* TargetObject = FunctionItem->GetTargetObject();
        AActor* TargetActor = ResolveActorFromTarget(TargetObject);
        if (!TargetObject || !TargetActor)
        {
            OutError = TEXT("Pinned function target is unavailable");
            return false;
        }

        if (SelectedActor.Get() != TargetActor)
        {
            SetSelectedActor(TargetActor);
        }

        if (UActorComponent* Component = Cast<UActorComponent>(TargetObject))
        {
            if (!FocusSelectedActorComponentByName(Component->GetName(), OutError))
            {
                return false;
            }
        }
        else
        {
            SelectedInspectObject = TargetActor;
            SelectedGroupKey = TEXT("ROOT_ACTOR");
            SelectedMaterialSlotIndex = INDEX_NONE;
            PropertyViewMode = ERIPropertyViewMode::Full;
            ViewMeshComp = nullptr;
            ViewMaterialSlot = INDEX_NONE;
            RefreshPanel(EInspectorRefreshReason::StructureChanged);
        }

        if (UInspectorFunctionsSectionWidget* SectionWidget = ActorFunctionsSectionWidget.Get())
        {
            SectionWidget->RefreshFromSubsystem();
            if (UInspectorFunctionItem* ResolvedFunctionItem = FindResolvedFunctionItem(TargetObject, FunctionItem->GetFunctionFName()))
            {
                if (SectionWidget->ScrollToItemForAutomation(ResolvedFunctionItem))
                {
                    return true;
                }
            }
        }

        OutError = FString::Printf(TEXT("Function row not found: %s"), *FunctionItem->GetDisplayName());
        return false;
    }

    OutError = TEXT("Pinned item type is not navigable");
    return false;
#endif
}

void UInspectorWorldSubsystem::SetActorOutline(AActor* Actor, bool bEnable, int32 StencilValue)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Actor) return;

    if (bEnable)
    {
        if (!EnsureCustomDepthStencilEnabled())
        {
            return; // 不满足条件就不做，避免“开了也没效果”
        }
    }


    TInlineComponentArray<UPrimitiveComponent*> Comps;
    Actor->GetComponents<UPrimitiveComponent>(Comps);

    for (UPrimitiveComponent* Prim : Comps)
    {
        if (!Prim) continue;
        if (!Prim->IsRegistered()) continue;

        Prim->SetRenderCustomDepth(bEnable);

        if (bEnable)
        {
            Prim->SetCustomDepthStencilValue(StencilValue);
        }

        Prim->MarkRenderStateDirty();
    }

#endif
}

bool UInspectorWorldSubsystem::EnsureCustomDepthStencilEnabled()
{
#if RUNTIME_INSPECTOR_ENABLED
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
    const int32 Val = CVar ? CVar->GetInt() : 0;

    // 3 = enabled + stencil writes enabled
    if (Val >= 3) return true;

    if (!bWarnedCustomDepthStencil)
    {
        bWarnedCustomDepthStencil = true;

        UE_CLOG(RI_IsDebugLogEnabled(), LogRuntimeInspector, Warning,
            TEXT("[RI] Outline requires Custom Depth-Stencil Pass = 'Enabled with Stencil'. ")
            TEXT("Please enable it in Project Settings -> Rendering -> Postprocessing -> Custom Depth-Stencil Pass, then restart editor. ")
            TEXT("(r.CustomDepth current=%d; need >=3)"), Val);

        // 如果你有 RI Toast，就在这里弹一条
        // RI_Toast(TEXT("Outline needs Custom Depth-Stencil Pass = Enabled with Stencil (restart editor)."));
    }

    return false;
#else
    return false;
#endif
}

static void SetBlendableWeight(FPostProcessSettings& PPS, UObject* Blendable, float Weight)
{
    if (!Blendable) return;

    TArray<FWeightedBlendable>& Arr = PPS.WeightedBlendables.Array;
    for (FWeightedBlendable& B : Arr)
    {
        if (B.Object == Blendable)
        {
            B.Weight = Weight;
            return;
        }
    }

    Arr.Add(FWeightedBlendable(Weight, Blendable));
}

UCameraComponent* UInspectorWorldSubsystem::FindOutlineCamera(APlayerController* PC) const
{
    if (!PC) return nullptr;

    // 优先 ViewTarget（更通用：相机可能不在 Pawn 上）
    if (AActor* VT = PC->GetViewTarget())
    {
        if (UCameraComponent* Cam = VT->FindComponentByClass<UCameraComponent>())
        {
            return Cam;
        }
    }

    // 再退化到 Pawn
    if (APawn* P = PC->GetPawn())
    {
        return P->FindComponentByClass<UCameraComponent>();
    }

    return nullptr;
}


void UInspectorWorldSubsystem::EnableOutlinePP(bool bEnable)
{
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    UCameraComponent* Cam = FindOutlineCamera(PC);
    if (!Cam) return;

    const URuntimeInspectorSettings* S = GetDefault<URuntimeInspectorSettings>();
    if (!S) return;

    if (bEnable)
    {
        if (!S->bEnableOutlinePP) return;

        UMaterialInterface* Mat = S->OutlinePostProcessMaterial.LoadSynchronous();
        if (!Mat)
        {
            PushToast(ERIToastType::Warning, TEXT("Outline PP material not set. (Project Settings -> Plugins -> Runtime Inspector -> Outline)"));
            return;
        }

        if (!OutlineMID || OutlineMID->Parent != Mat)
        {
            OutlineMID = UMaterialInstanceDynamic::Create(Mat, Cam);
        }

        // 确保相机 PP 权重 > 0，否则相机 PP 不生效
        if (!bSavedCamPPBlendWeightValid)
        {
            SavedCamPPBlendWeight = Cam->PostProcessBlendWeight;
            bSavedCamPPBlendWeightValid = true;
        }
        if (Cam->PostProcessBlendWeight <= 0.0f)
        {
            Cam->PostProcessBlendWeight = 1.0f;
        }

        SetBlendableWeight(Cam->PostProcessSettings, OutlineMID, S->OutlinePPWeight);
    }
    else
    {
        if (OutlineMID)
        {
            SetBlendableWeight(Cam->PostProcessSettings, OutlineMID, 0.0f);
        }

        if (bSavedCamPPBlendWeightValid)
        {
            Cam->PostProcessBlendWeight = SavedCamPPBlendWeight;
            bSavedCamPPBlendWeightValid = false;
        }
    }
#endif
}
static FString RI_GetActorDisplayLabel(const AActor* Actor)
{
    if (!Actor)
    {
        return FString();
    }

#if WITH_EDITOR
    return Actor->GetActorLabel();
#else
    return Actor->GetName();
#endif
}
