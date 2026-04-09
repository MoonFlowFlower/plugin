#include "InspectorWorldSubsystem.h"
#include "InspectorPropertyUtils.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
    static bool RI_ParseLinearColorText(const FString& InText, FLinearColor& OutColor)
    {
        TArray<FString> Parts;
        InText.Replace(TEXT(" "), TEXT(",")).ParseIntoArray(Parts, TEXT(","), true);
        if (Parts.Num() < 3)
        {
            return false;
        }

        float R = 0.0f;
        float G = 0.0f;
        float B = 0.0f;
        float A = 1.0f;
        if (!LexTryParseString(R, *Parts[0]) || !LexTryParseString(G, *Parts[1]) || !LexTryParseString(B, *Parts[2]))
        {
            return false;
        }
        if (Parts.Num() >= 4 && !LexTryParseString(A, *Parts[3]))
        {
            return false;
        }

        OutColor = FLinearColor(R, G, B, A);
        return true;
    }

    static UMaterialInstanceConstant* RI_ResolveWritableMIC(UMaterialInterface* Material)
    {
        if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Material))
        {
            return MIC;
        }

        if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material))
        {
            return Cast<UMaterialInstanceConstant>(MID->Parent);
        }

        return nullptr;
    }

#if WITH_EDITOR
    static bool RI_SaveAssetPackage(UObject* Asset, FString& OutError)
    {
        OutError.Reset();

        if (!Asset)
        {
            OutError = TEXT("Asset is invalid");
            return false;
        }

        UPackage* Package = Asset->GetOutermost();
        if (!Package)
        {
            OutError = TEXT("Asset package is invalid");
            return false;
        }

        const FString PackageName = Package->GetName();
        const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_None;

        Package->MarkPackageDirty();
        if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
        {
            OutError = FString::Printf(TEXT("Failed to save package: %s"), *Filename);
            return false;
        }

        return true;
    }
#endif
}

bool UInspectorWorldSubsystem::BuildMaterialPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const
{
    OutPreview = FRIPromoteOperationPreview();
    OutPreview.Operation = Operation;
    OutPreview.DesiredSourceValue = Operation.PatchedValue;

    if (Operation.Target.TargetKind != ERIPatchTargetKind::MaterialSlot
        || (Operation.Field.FieldKind != ERIPatchFieldKind::MaterialScalar && Operation.Field.FieldKind != ERIPatchFieldKind::MaterialVector))
    {
        OutPreview.Message = TEXT("Not a material-slot patch operation");
        return true;
    }

    AActor* RuntimeActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (!RuntimeActor)
    {
        OutPreview.Message = TEXT("Runtime actor not found");
        return true;
    }

    UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
        RuntimeActor,
        Operation.Target.ActorPath,
        Operation.Target.ComponentPath,
        Operation.Target.ComponentName,
        Operation.Target.ComponentClass);
    UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent);
    if (!MeshComponent)
    {
        OutPreview.Message = TEXT("Target component is not a mesh component");
        return true;
    }

    UMaterialInterface* Material = MeshComponent->GetMaterial(Operation.Target.MaterialSlotIndex);
    UMaterialInstanceConstant* MIC = RI_ResolveWritableMIC(Material);
    if (!MIC)
    {
        OutPreview.Message = TEXT("Material slot does not resolve to a writable Material Instance Constant");
        return true;
    }

    const FName ParamName(*Operation.Field.FieldPath);
    const FMaterialParameterInfo Info(ParamName);
    OutPreview.AssetPath = MIC->GetPathName();

    if (Operation.Field.FieldKind == ERIPatchFieldKind::MaterialScalar)
    {
        float CurrentValue = 0.0f;
        if (!MIC->GetScalarParameterValue(Info, CurrentValue))
        {
            OutPreview.Message = TEXT("Scalar parameter not found on source MIC");
            return true;
        }

        OutPreview.CurrentSourceValue = FString::SanitizeFloat(CurrentValue);
    }
    else
    {
        FLinearColor CurrentColor = FLinearColor::Black;
        if (!MIC->GetVectorParameterValue(Info, CurrentColor))
        {
            OutPreview.Message = TEXT("Vector parameter not found on source MIC");
            return true;
        }

        OutPreview.CurrentSourceValue = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), CurrentColor.R, CurrentColor.G, CurrentColor.B, CurrentColor.A);
    }

    OutPreview.bSupported = true;
    OutPreview.Message = TEXT("Ready to promote to MIC source");
    OutPreview.DiffText = FString::Printf(
        TEXT("%s :: %s -> %s"),
        *OutPreview.AssetPath,
        *OutPreview.CurrentSourceValue,
        *OutPreview.DesiredSourceValue);
    return true;
}

bool UInspectorWorldSubsystem::PromoteMaterialOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult)
{
    OutResult = FRIPromoteOperationResult();
    OutResult.Operation = Operation;

    FRIPromoteOperationPreview Preview;
    BuildMaterialPromotePreview(Operation, Preview);
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
    if (!RuntimeActor)
    {
        OutResult.Status = ERIPromoteOperationStatus::NotFound;
        OutResult.Message = TEXT("Runtime actor not found");
        return false;
    }

    UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
        RuntimeActor,
        Operation.Target.ActorPath,
        Operation.Target.ComponentPath,
        Operation.Target.ComponentName,
        Operation.Target.ComponentClass);
    UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent);
    if (!MeshComponent)
    {
        OutResult.Status = ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = TEXT("Target component is not a mesh component");
        return false;
    }

    UMaterialInterface* Material = MeshComponent->GetMaterial(Operation.Target.MaterialSlotIndex);
    UMaterialInstanceConstant* MIC = RI_ResolveWritableMIC(Material);
    if (!MIC)
    {
        OutResult.Status = ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = TEXT("Material slot does not resolve to a writable Material Instance Constant");
        return false;
    }

    const FName ParamName(*Operation.Field.FieldPath);
    const FMaterialParameterInfo Info(ParamName);

    if (Operation.Field.FieldKind == ERIPatchFieldKind::MaterialScalar)
    {
        float NewValue = 0.0f;
        if (!LexTryParseString(NewValue, *Operation.PatchedValue))
        {
            OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
            OutResult.Message = TEXT("Invalid scalar text");
            return false;
        }

        MIC->Modify();
        MIC->SetScalarParameterValueEditorOnly(Info, NewValue);
        MIC->PostEditChange();
        OutResult.ValueWritten = FString::SanitizeFloat(NewValue);
    }
    else
    {
        FLinearColor NewColor = FLinearColor::Black;
        if (!RI_ParseLinearColorText(Operation.PatchedValue, NewColor))
        {
            OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
            OutResult.Message = TEXT("Invalid vector text");
            return false;
        }

        MIC->Modify();
        MIC->SetVectorParameterValueEditorOnly(Info, NewColor);
        MIC->PostEditChange();
        OutResult.ValueWritten = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), NewColor.R, NewColor.G, NewColor.B, NewColor.A);
    }

    FString SaveError;
    if (!RI_SaveAssetPackage(MIC, SaveError))
    {
        OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
        OutResult.Message = SaveError;
        return false;
    }

    OutResult.Status = ERIPromoteOperationStatus::Promoted;
    OutResult.Message = TEXT("Promoted to MIC source");
    return true;
#endif
}
