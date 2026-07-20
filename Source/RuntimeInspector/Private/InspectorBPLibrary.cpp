#include "InspectorBPLibrary.h"
#include "UObject/Package.h"
#include "InspectorSettingsPageWidget.h"
#include "InspectorWorldSubsystem.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

UInspectorWorldSubsystem* UInspectorBPLibrary::GetInspectorSubsystem(UObject* WorldContextObject)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UInspectorWorldSubsystem>();
#else
    return nullptr;
#endif
}

void UInspectorBPLibrary::ToggleInspector(UObject* WorldContextObject)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!WorldContextObject) return;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    if (UInspectorWorldSubsystem* Sub = World->GetSubsystem<UInspectorWorldSubsystem>())
    {
        Sub->Toggle();
    }
#endif
}

bool UInspectorBPLibrary::GenerateSettingsPageBlueprintLayout()
{
#if WITH_EDITOR
    static const TCHAR* SettingsPageAssetPath = TEXT("/RuntimeInspector/UI/WBP_SettingsPage.WBP_SettingsPage");

    UWidgetBlueprint* SettingsPageBlueprint = LoadObject<UWidgetBlueprint>(nullptr, SettingsPageAssetPath);
    if (!SettingsPageBlueprint)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI][UMG] Failed to load settings page blueprint asset: %s"), SettingsPageAssetPath);
        return false;
    }

    UInspectorSettingsPageWidget* TemplateWidget = NewObject<UInspectorSettingsPageWidget>(GetTransientPackage(), NAME_None, RF_Transient);
    UWidgetTree* ExportTree = NewObject<UWidgetTree>(TemplateWidget, TEXT("RI_SettingsExportTree"), RF_Transient);
    TemplateWidget->BuildFallbackWidgetTreeForExport(ExportTree);
    if (!ExportTree->RootWidget)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI][UMG] Failed to build fallback settings tree for export."));
        return false;
    }

    SettingsPageBlueprint->Modify();
    if (SettingsPageBlueprint->WidgetTree)
    {
        SettingsPageBlueprint->WidgetTree->Rename(
            nullptr,
            GetTransientPackage(),
            REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty | REN_ForceNoResetLoaders);
    }

    SettingsPageBlueprint->WidgetTree = DuplicateObject<UWidgetTree>(ExportTree, SettingsPageBlueprint, TEXT("WidgetTree"));
    if (!SettingsPageBlueprint->WidgetTree || !SettingsPageBlueprint->WidgetTree->RootWidget)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI][UMG] Failed to duplicate fallback settings tree into blueprint."));
        return false;
    }

    SettingsPageBlueprint->WidgetTree->Modify();
    SettingsPageBlueprint->WidgetTree->SetFlags(RF_Transactional);
    SettingsPageBlueprint->MarkPackageDirty();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(SettingsPageBlueprint);
    FKismetEditorUtilities::CompileBlueprint(SettingsPageBlueprint);

    UPackage* Package = SettingsPageBlueprint->GetOutermost();
    if (!Package)
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI][UMG] Settings page blueprint has no package."));
        return false;
    }

    const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    const bool bSaved = UPackage::SavePackage(Package, SettingsPageBlueprint, *PackageFilename, SaveArgs);
    if (bSaved)
    {
        UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][UMG] GenerateSettingsPageBlueprintLayout OK Asset=%s"), *PackageFilename);
    }
    else
    {
        UE_LOG(LogRuntimeInspector, Error, TEXT("[RI][UMG] GenerateSettingsPageBlueprintLayout FAILED Asset=%s"), *PackageFilename);
    }
    return bSaved;
#else
    return false;
#endif
}
