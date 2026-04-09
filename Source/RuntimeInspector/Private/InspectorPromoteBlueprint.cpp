#include "InspectorWorldSubsystem.h"
#include "InspectorPropertyUtils.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

namespace
{
    static bool RI_AddUniqueText(TArray<FString>& InOutValues, const FString& Value)
    {
        if (Value.IsEmpty() || InOutValues.Contains(Value))
        {
            return false;
        }

        InOutValues.Add(Value);
        return true;
    }

    static FString RI_NormalizeComponentKey(FString Value)
    {
        Value.TrimStartAndEndInline();
        Value.ReplaceInline(TEXT("_GEN_VARIABLE"), TEXT(""), ESearchCase::IgnoreCase);
        Value.ReplaceInline(TEXT("GEN_VARIABLE"), TEXT(""), ESearchCase::IgnoreCase);
        Value.ReplaceInline(TEXT("Default__"), TEXT(""), ESearchCase::IgnoreCase);
        Value.ReplaceInline(TEXT(":"), TEXT(""));
        return Value.ToLower();
    }

    static UBlueprint* RI_GetSourceBlueprintFromGeneratedClass(UBlueprintGeneratedClass* BlueprintClass)
    {
#if WITH_EDITOR
        return BlueprintClass ? Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy) : nullptr;
#else
        return nullptr;
#endif
    }

#if WITH_EDITOR
    static bool RI_SaveBlueprintAsset(UBlueprint* Blueprint, FString& OutError)
    {
        OutError.Reset();

        if (!Blueprint)
        {
            OutError = TEXT("Blueprint is invalid");
            return false;
        }

        UPackage* Package = Blueprint->GetOutermost();
        if (!Package)
        {
            OutError = TEXT("Blueprint package is invalid");
            return false;
        }

        const FString PackageName = Package->GetName();
        const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_None;

        Package->MarkPackageDirty();
        if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
        {
            OutError = FString::Printf(TEXT("Failed to save package: %s"), *Filename);
            return false;
        }

        return true;
    }
#endif
}

bool UInspectorWorldSubsystem::CreatePromotePreview(const FRIPatchBundle& InBundle, FRIPromotePreview& OutPreview, FString& OutError) const
{
    OutError.Reset();
    OutPreview = FRIPromotePreview();

    if (InBundle.Operations.Num() <= 0)
    {
        OutError = TEXT("Patch bundle has no operations");
        return false;
    }

    for (const FRIPatchOperation& Operation : InBundle.Operations)
    {
        FRIPromoteOperationPreview Preview;
        bool bHandled = false;
        if (Operation.SourceTag.StartsWith(TEXT("Config:"), ESearchCase::IgnoreCase)
            || Operation.SourceTag.StartsWith(TEXT("DataAsset:"), ESearchCase::IgnoreCase)
            || Operation.SourceTag.StartsWith(TEXT("DataTable:"), ESearchCase::IgnoreCase))
        {
            bHandled = BuildDataPromotePreview(Operation, Preview);
        }
        else if (Operation.Field.FieldKind == ERIPatchFieldKind::Property)
        {
            bHandled = BuildBlueprintPromotePreview(Operation, Preview);
        }
        else
        {
            bHandled = BuildMaterialPromotePreview(Operation, Preview);
        }

        if (!bHandled)
        {
            Preview = FRIPromoteOperationPreview();
            Preview.Operation = Operation;
            Preview.Message = TEXT("Failed to build promote preview");
        }

        OutPreview.Operations.Add(Preview);
    }

    FinalizePromotePreview(OutPreview);
    return true;
}

bool UInspectorWorldSubsystem::PromotePatchToSource(const FRIPatchBundle& InBundle, FRIPromoteResult& OutResult, FString& OutError)
{
    OutError.Reset();
    OutResult = FRIPromoteResult();

    FRIPromotePreview Preview;
    if (!CreatePromotePreview(InBundle, Preview, OutError))
    {
        return false;
    }

    for (const FRIPromoteOperationPreview& OperationPreview : Preview.Operations)
    {
        FRIPromoteOperationResult Result;
        Result.Operation = OperationPreview.Operation;
        Result.AssetPath = OperationPreview.AssetPath;

        if (!OperationPreview.bSupported)
        {
            Result.Status = OperationPreview.Message.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
                ? ERIPromoteOperationStatus::NotFound
                : ERIPromoteOperationStatus::Skipped;
            Result.Message = OperationPreview.Message;
            OutResult.OperationResults.Add(Result);
            continue;
        }

        bool bPromoted = false;
        if (OperationPreview.Operation.SourceTag.StartsWith(TEXT("Config:"), ESearchCase::IgnoreCase)
            || OperationPreview.Operation.SourceTag.StartsWith(TEXT("DataAsset:"), ESearchCase::IgnoreCase)
            || OperationPreview.Operation.SourceTag.StartsWith(TEXT("DataTable:"), ESearchCase::IgnoreCase))
        {
            bPromoted = PromoteDataOperationToSource(OperationPreview.Operation, Result);
        }
        else if (OperationPreview.Operation.Field.FieldKind == ERIPatchFieldKind::Property)
        {
            bPromoted = PromoteBlueprintOperationToSource(OperationPreview.Operation, Result);
        }
        else
        {
            bPromoted = PromoteMaterialOperationToSource(OperationPreview.Operation, Result);
        }

        if (!bPromoted && OutError.IsEmpty() && Result.Message.IsEmpty())
        {
            Result.Message = TEXT("Promote-to-source failed");
        }

        OutResult.OperationResults.Add(Result);
    }

    FinalizePromoteResult(OutResult);
    InvalidateFileManagementSummaryCache();
    return OutResult.bSuccess;
}

void UInspectorWorldSubsystem::FinalizePromotePreview(FRIPromotePreview& OutPreview) const
{
    OutPreview.TargetAssetPaths.Reset();
    OutPreview.SupportedOperationCount = 0;
    OutPreview.UnsupportedOperationCount = 0;

    TArray<FString> DiffLines;
    for (const FRIPromoteOperationPreview& Operation : OutPreview.Operations)
    {
        if (Operation.bSupported)
        {
            ++OutPreview.SupportedOperationCount;
        }
        else
        {
            ++OutPreview.UnsupportedOperationCount;
        }

        RI_AddUniqueText(OutPreview.TargetAssetPaths, Operation.AssetPath);

        if (!Operation.DiffText.IsEmpty())
        {
            DiffLines.Add(Operation.DiffText);
        }
        else
        {
            DiffLines.Add(FString::Printf(
                TEXT("%s :: %s"),
                Operation.AssetPath.IsEmpty() ? TEXT("<unresolved>") : *Operation.AssetPath,
                *Operation.Message));
        }
    }

    OutPreview.bCanPromote = OutPreview.SupportedOperationCount > 0;
    OutPreview.Summary = FString::Printf(
        TEXT("PromotePreview Supported=%d Unsupported=%d Assets=%d"),
        OutPreview.SupportedOperationCount,
        OutPreview.UnsupportedOperationCount,
        OutPreview.TargetAssetPaths.Num());
    OutPreview.DiffText = FString::Join(DiffLines, TEXT("\n"));
}

void UInspectorWorldSubsystem::FinalizePromoteResult(FRIPromoteResult& OutResult) const
{
    OutResult.TargetAssetPaths.Reset();
    OutResult.PromotedCount = 0;
    OutResult.FailedCount = 0;
    OutResult.SkippedCount = 0;

    TArray<FString> ReportLines;
    for (const FRIPromoteOperationResult& Operation : OutResult.OperationResults)
    {
        RI_AddUniqueText(OutResult.TargetAssetPaths, Operation.AssetPath);

        switch (Operation.Status)
        {
        case ERIPromoteOperationStatus::Promoted:
            ++OutResult.PromotedCount;
            break;
        case ERIPromoteOperationStatus::Skipped:
        case ERIPromoteOperationStatus::Unsupported:
            ++OutResult.SkippedCount;
            break;
        default:
            ++OutResult.FailedCount;
            break;
        }

        FString StatusText;
        switch (Operation.Status)
        {
        case ERIPromoteOperationStatus::Promoted: StatusText = TEXT("Promoted"); break;
        case ERIPromoteOperationStatus::PreviewSupported: StatusText = TEXT("PreviewSupported"); break;
        case ERIPromoteOperationStatus::Unsupported: StatusText = TEXT("Unsupported"); break;
        case ERIPromoteOperationStatus::NotFound: StatusText = TEXT("NotFound"); break;
        case ERIPromoteOperationStatus::WriteFailed: StatusText = TEXT("WriteFailed"); break;
        default: StatusText = TEXT("Skipped"); break;
        }

        ReportLines.Add(FString::Printf(
            TEXT("[%s] %s :: %s"),
            *StatusText,
            Operation.AssetPath.IsEmpty() ? TEXT("<unresolved>") : *Operation.AssetPath,
            Operation.Message.IsEmpty() ? TEXT("(no message)") : *Operation.Message));
    }

    OutResult.bSuccess = OutResult.PromotedCount > 0 && OutResult.FailedCount == 0;
    OutResult.Summary = FString::Printf(
        TEXT("PromoteResult Promoted=%d Failed=%d Skipped=%d Assets=%d"),
        OutResult.PromotedCount,
        OutResult.FailedCount,
        OutResult.SkippedCount,
        OutResult.TargetAssetPaths.Num());
    OutResult.ReportText = FString::Join(ReportLines, TEXT("\n"));
}

bool UInspectorWorldSubsystem::BuildBlueprintPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const
{
    OutPreview = FRIPromoteOperationPreview();
    OutPreview.Operation = Operation;
    OutPreview.DesiredSourceValue = Operation.PatchedValue;

    if (Operation.Field.FieldKind != ERIPatchFieldKind::Property
        || (Operation.Target.TargetKind != ERIPatchTargetKind::Actor && Operation.Target.TargetKind != ERIPatchTargetKind::Component))
    {
        OutPreview.Message = TEXT("Not a Blueprint property patch operation");
        return true;
    }

    AActor* RuntimeActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (!RuntimeActor)
    {
        OutPreview.Message = TEXT("Runtime actor not found");
        return true;
    }

    UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(RuntimeActor->GetClass());
    UBlueprint* Blueprint = RI_GetSourceBlueprintFromGeneratedClass(BlueprintClass);
    if (!BlueprintClass || !Blueprint)
    {
        OutPreview.Message = TEXT("Runtime target is not backed by a Blueprint-generated class");
        return true;
    }

    UObject* SourceObject = BlueprintClass->GetDefaultObject();
    if (!SourceObject)
    {
        OutPreview.Message = TEXT("Blueprint CDO is invalid");
        return true;
    }

    if (Operation.Target.TargetKind == ERIPatchTargetKind::Component)
    {
        UActorComponent* RuntimeComponent = ResolveRuntimeComponentTarget(
            RuntimeActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (!RuntimeComponent)
        {
            OutPreview.Message = TEXT("Runtime component not found");
            return true;
        }

        USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
        if (!SCS)
        {
            OutPreview.Message = TEXT("Blueprint has no SimpleConstructionScript");
            return true;
        }

        const FString DesiredComponentName = RI_NormalizeComponentKey(Operation.Target.ComponentName);
        const FString RuntimeComponentName = RI_NormalizeComponentKey(RuntimeComponent->GetName());
        const FString RuntimeClassPath = RuntimeComponent->GetClass()->GetPathName();

        UActorComponent* MatchedTemplate = nullptr;
        for (USCS_Node* Node : SCS->GetAllNodes())
        {
            if (!Node || !Node->ComponentTemplate)
            {
                continue;
            }

            if (!Operation.Target.ComponentClass.IsEmpty()
                && Node->ComponentTemplate->GetClass()->GetPathName() != Operation.Target.ComponentClass
                && RuntimeClassPath != Node->ComponentTemplate->GetClass()->GetPathName())
            {
                continue;
            }

            const FString NodeVariableName = RI_NormalizeComponentKey(Node->GetVariableName().ToString());
            const FString TemplateName = RI_NormalizeComponentKey(Node->ComponentTemplate->GetName());
            const bool bNameMatch = (!DesiredComponentName.IsEmpty() && (NodeVariableName == DesiredComponentName || TemplateName == DesiredComponentName))
                || (!RuntimeComponentName.IsEmpty() && (NodeVariableName == RuntimeComponentName || TemplateName == RuntimeComponentName));
            if (!bNameMatch)
            {
                continue;
            }

            if (MatchedTemplate && MatchedTemplate != Node->ComponentTemplate)
            {
                OutPreview.Message = TEXT("Blueprint component template mapping is ambiguous");
                return true;
            }

            MatchedTemplate = Node->ComponentTemplate;
        }

        if (!MatchedTemplate)
        {
            OutPreview.Message = TEXT("No matching Blueprint-owned component template found");
            return true;
        }

        SourceObject = MatchedTemplate;
    }

    const FName PropertyName(*Operation.Field.FieldPath);
    FProperty* Property = SourceObject->GetClass()->FindPropertyByName(PropertyName);
    if (!Property || !InspectorPropertyUtils::CanSetFromText(SourceObject, Property))
    {
        OutPreview.Message = TEXT("Property is not writable on the source object");
        return true;
    }

    if (!InspectorPropertyUtils::GetValueAsText(SourceObject, PropertyName, OutPreview.CurrentSourceValue))
    {
        OutPreview.Message = TEXT("Failed to read source property value");
        return true;
    }

    OutPreview.AssetPath = Blueprint->GetPathName();
    OutPreview.bSupported = true;
    OutPreview.Message = TEXT("Ready to promote to Blueprint source");
    OutPreview.DiffText = FString::Printf(
        TEXT("%s :: %s.%s %s -> %s"),
        *OutPreview.AssetPath,
        *SourceObject->GetClass()->GetName(),
        *Operation.Field.FieldPath,
        *OutPreview.CurrentSourceValue,
        *OutPreview.DesiredSourceValue);
    return true;
}

bool UInspectorWorldSubsystem::PromoteBlueprintOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult)
{
    OutResult = FRIPromoteOperationResult();
    OutResult.Operation = Operation;

    FRIPromoteOperationPreview Preview;
    BuildBlueprintPromotePreview(Operation, Preview);
    OutResult.AssetPath = Preview.AssetPath;

    if (!Preview.bSupported)
    {
        OutResult.Status = Preview.Message.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
            ? ERIPromoteOperationStatus::NotFound
            : ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = Preview.Message;
        return false;
    }

#if !WITH_EDITOR
    OutResult.Status = ERIPromoteOperationStatus::Unsupported;
    OutResult.Message = TEXT("Promote-to-source requires WITH_EDITOR");
    return false;
#else
    AActor* RuntimeActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    UBlueprintGeneratedClass* BlueprintClass = RuntimeActor ? Cast<UBlueprintGeneratedClass>(RuntimeActor->GetClass()) : nullptr;
    UBlueprint* Blueprint = RI_GetSourceBlueprintFromGeneratedClass(BlueprintClass);
    if (!RuntimeActor || !BlueprintClass || !Blueprint)
    {
        OutResult.Status = ERIPromoteOperationStatus::NotFound;
        OutResult.Message = TEXT("Blueprint source target no longer resolves");
        return false;
    }

    UObject* SourceObject = BlueprintClass->GetDefaultObject();
    if (Operation.Target.TargetKind == ERIPatchTargetKind::Component)
    {
        UActorComponent* RuntimeComponent = ResolveRuntimeComponentTarget(
            RuntimeActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (!RuntimeComponent || !Blueprint->SimpleConstructionScript)
        {
            OutResult.Status = ERIPromoteOperationStatus::NotFound;
            OutResult.Message = TEXT("Blueprint component source no longer resolves");
            return false;
        }

        const FString DesiredComponentName = RI_NormalizeComponentKey(Operation.Target.ComponentName);
        const FString RuntimeComponentName = RI_NormalizeComponentKey(RuntimeComponent->GetName());
        const FString RuntimeClassPath = RuntimeComponent->GetClass()->GetPathName();

        UActorComponent* MatchedTemplate = nullptr;
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (!Node || !Node->ComponentTemplate)
            {
                continue;
            }

            if (!Operation.Target.ComponentClass.IsEmpty()
                && Node->ComponentTemplate->GetClass()->GetPathName() != Operation.Target.ComponentClass
                && RuntimeClassPath != Node->ComponentTemplate->GetClass()->GetPathName())
            {
                continue;
            }

            const FString NodeVariableName = RI_NormalizeComponentKey(Node->GetVariableName().ToString());
            const FString TemplateName = RI_NormalizeComponentKey(Node->ComponentTemplate->GetName());
            const bool bNameMatch = (!DesiredComponentName.IsEmpty() && (NodeVariableName == DesiredComponentName || TemplateName == DesiredComponentName))
                || (!RuntimeComponentName.IsEmpty() && (NodeVariableName == RuntimeComponentName || TemplateName == RuntimeComponentName));
            if (!bNameMatch)
            {
                continue;
            }

            if (MatchedTemplate && MatchedTemplate != Node->ComponentTemplate)
            {
                OutResult.Status = ERIPromoteOperationStatus::Unsupported;
                OutResult.Message = TEXT("Blueprint component template mapping is ambiguous");
                return false;
            }

            MatchedTemplate = Node->ComponentTemplate;
        }

        if (!MatchedTemplate)
        {
            OutResult.Status = ERIPromoteOperationStatus::NotFound;
            OutResult.Message = TEXT("No matching Blueprint-owned component template found");
            return false;
        }

        SourceObject = MatchedTemplate;
    }

    const FName PropertyName(*Operation.Field.FieldPath);
    FString SetError;
    SourceObject->Modify();
    if (!InspectorPropertyUtils::SetValueFromText(SourceObject, PropertyName, Operation.PatchedValue, &SetError))
    {
        OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
        OutResult.Message = SetError;
        return false;
    }

    FString FinalValue;
    InspectorPropertyUtils::GetValueAsText(SourceObject, PropertyName, FinalValue);
    OutResult.ValueWritten = FinalValue;

    Blueprint->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    FString SaveError;
    if (!RI_SaveBlueprintAsset(Blueprint, SaveError))
    {
        OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
        OutResult.Message = SaveError;
        return false;
    }

    OutResult.Status = ERIPromoteOperationStatus::Promoted;
    OutResult.Message = TEXT("Promoted to Blueprint source");
    return true;
#endif
}
