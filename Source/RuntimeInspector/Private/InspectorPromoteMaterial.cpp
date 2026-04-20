#include "InspectorWorldSubsystem.h"
#include "InspectorPropertyUtils.h"

#include "Components/MeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#endif
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
    enum class ERIMaterialPromoteSourceKind : uint8
    {
        Unsupported,
        MIC,
        Material
    };

    struct FRIMaterialPromoteSource
    {
        ERIMaterialPromoteSourceKind Kind = ERIMaterialPromoteSourceKind::Unsupported;
        UMaterialInstanceConstant* MIC = nullptr;
        UMaterial* Material = nullptr;
        FString AssetPath;
        FString Message;
    };

    static const TCHAR* RI_MaterialPromoteSourceKindToString(ERIMaterialPromoteSourceKind Kind)
    {
        switch (Kind)
        {
        case ERIMaterialPromoteSourceKind::MIC:
            return TEXT("MIC");
        case ERIMaterialPromoteSourceKind::Material:
            return TEXT("Material");
        default:
            return TEXT("Unsupported");
        }
    }

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

    static FString RI_FormatMaterialPromoteColor(const FLinearColor& Color)
    {
        return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Color.R, Color.G, Color.B, Color.A);
    }

    static FString RI_ExtractMaterialSourceAssetPath(const FString& SourceTag)
    {
        static const FString Prefix = TEXT("Material:");
        if (!SourceTag.StartsWith(Prefix, ESearchCase::IgnoreCase))
        {
            return FString();
        }

        return SourceTag.RightChop(Prefix.Len()).TrimStartAndEnd();
    }

    static UMaterialInterface* RI_LoadMaterialSourceAsset(const FString& AssetPath)
    {
        if (AssetPath.IsEmpty())
        {
            return nullptr;
        }

        return Cast<UMaterialInterface>(StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath));
    }

    static UMaterialInterface* RI_ResolveWritableMaterialSourceInterface(UMaterialInterface* Material)
    {
        UMaterialInterface* Candidate = Material;
        while (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Candidate))
        {
            Candidate = MID->Parent;
        }

        return Candidate;
    }

    static FRIMaterialPromoteSource RI_ResolveMaterialPromoteSource(const FString& SourceTag, UMaterialInterface* RuntimeMaterial)
    {
        FRIMaterialPromoteSource Source;

        UMaterialInterface* Candidate = nullptr;
        const FString TaggedAssetPath = RI_ExtractMaterialSourceAssetPath(SourceTag);
        if (!TaggedAssetPath.IsEmpty())
        {
            Candidate = RI_LoadMaterialSourceAsset(TaggedAssetPath);
            if (!Candidate)
            {
                Source.Message = FString::Printf(
                    TEXT("Material source asset could not be loaded: %s"),
                    *TaggedAssetPath);
                return Source;
            }
        }
        else
        {
            Candidate = RuntimeMaterial;
        }

        Candidate = RI_ResolveWritableMaterialSourceInterface(Candidate);
        if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Candidate))
        {
            Source.Kind = ERIMaterialPromoteSourceKind::MIC;
            Source.MIC = MIC;
            Source.AssetPath = MIC->GetPathName();
            return Source;
        }

        if (UMaterial* Material = Cast<UMaterial>(Candidate))
        {
            Source.Kind = ERIMaterialPromoteSourceKind::Material;
            Source.Material = Material;
            Source.AssetPath = Material->GetPathName();
            return Source;
        }

        Source.Message = TEXT("Material slot does not resolve to a writable Material or MIC source");
        return Source;
    }

#if WITH_EDITOR
    static UMaterialExpressionScalarParameter* RI_FindUniqueScalarParameterExpression(UMaterial* Material, const FName ParamName, FString& OutError)
    {
        OutError.Reset();
        if (!Material)
        {
            OutError = TEXT("Source Material is invalid");
            return nullptr;
        }

        UMaterialExpressionScalarParameter* Match = nullptr;
        int32 MatchCount = 0;
        for (UMaterialExpression* Expression : Material->GetExpressions())
        {
            if (UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>(Expression))
            {
                if (ScalarParameter->ParameterName == ParamName)
                {
                    Match = ScalarParameter;
                    ++MatchCount;
                }
            }
        }

        if (MatchCount == 1)
        {
            return Match;
        }

        OutError = MatchCount <= 0
            ? TEXT("Scalar parameter not found on source Material")
            : FString::Printf(TEXT("Multiple scalar parameters named %s found on source Material"), *ParamName.ToString());
        return nullptr;
    }

    static UMaterialExpressionVectorParameter* RI_FindUniqueVectorParameterExpression(UMaterial* Material, const FName ParamName, FString& OutError)
    {
        OutError.Reset();
        if (!Material)
        {
            OutError = TEXT("Source Material is invalid");
            return nullptr;
        }

        UMaterialExpressionVectorParameter* Match = nullptr;
        int32 MatchCount = 0;
        for (UMaterialExpression* Expression : Material->GetExpressions())
        {
            if (UMaterialExpressionVectorParameter* VectorParameter = Cast<UMaterialExpressionVectorParameter>(Expression))
            {
                if (VectorParameter->ParameterName == ParamName)
                {
                    Match = VectorParameter;
                    ++MatchCount;
                }
            }
        }

        if (MatchCount == 1)
        {
            return Match;
        }

        OutError = MatchCount <= 0
            ? TEXT("Vector parameter not found on source Material")
            : FString::Printf(TEXT("Multiple vector parameters named %s found on source Material"), *ParamName.ToString());
        return nullptr;
    }

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

    UMaterialInterface* RuntimeMaterial = nullptr;
    AActor* RuntimeActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (RuntimeActor)
    {
        UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
            RuntimeActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent))
        {
            RuntimeMaterial = MeshComponent->GetMaterial(Operation.Target.MaterialSlotIndex);
        }
        else if (RI_ExtractMaterialSourceAssetPath(Operation.SourceTag).IsEmpty())
        {
            OutPreview.Message = TEXT("Target component is not a mesh component");
            return true;
        }
    }
    else if (RI_ExtractMaterialSourceAssetPath(Operation.SourceTag).IsEmpty())
    {
        OutPreview.Message = TEXT("Runtime actor not found");
        return true;
    }

    const FRIMaterialPromoteSource Source = RI_ResolveMaterialPromoteSource(Operation.SourceTag, RuntimeMaterial);
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][PromoteMaterial] Preview SourceTag=%s Runtime=%s Resolved=%s Asset=%s Param=%s FieldKind=%d"),
        *Operation.SourceTag,
        *GetNameSafe(RuntimeMaterial),
        RI_MaterialPromoteSourceKindToString(Source.Kind),
        *Source.AssetPath,
        *Operation.Field.FieldPath,
        static_cast<int32>(Operation.Field.FieldKind));
    if (Source.Kind == ERIMaterialPromoteSourceKind::Unsupported)
    {
        OutPreview.Message = Source.Message;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Preview unsupported: %s"), *OutPreview.Message);
        return true;
    }

    const FName ParamName(*Operation.Field.FieldPath);
    const FMaterialParameterInfo Info(ParamName);
    OutPreview.AssetPath = Source.AssetPath;

    if (Operation.Field.FieldKind == ERIPatchFieldKind::MaterialScalar)
    {
        if (Source.Kind == ERIMaterialPromoteSourceKind::MIC)
        {
            float CurrentValue = 0.0f;
            if (!Source.MIC->GetScalarParameterValue(Info, CurrentValue))
            {
                OutPreview.Message = TEXT("Scalar parameter not found on source MIC");
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Preview not found on MIC: %s"), *Operation.Field.FieldPath);
                return true;
            }

            OutPreview.CurrentSourceValue = FString::SanitizeFloat(CurrentValue);
        }
#if WITH_EDITOR
        else
        {
            FString FindError;
            UMaterialExpressionScalarParameter* ScalarParameter = RI_FindUniqueScalarParameterExpression(Source.Material, ParamName, FindError);
            if (!ScalarParameter)
            {
                OutPreview.Message = FindError;
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Preview material lookup failed: %s"), *FindError);
                return true;
            }

            OutPreview.CurrentSourceValue = FString::SanitizeFloat(ScalarParameter->DefaultValue);
        }
#else
        else
        {
            OutPreview.Message = TEXT("Promote-to-source requires WITH_EDITOR");
            return true;
        }
#endif
    }
    else
    {
        if (Source.Kind == ERIMaterialPromoteSourceKind::MIC)
        {
            FLinearColor CurrentColor = FLinearColor::Black;
            if (!Source.MIC->GetVectorParameterValue(Info, CurrentColor))
            {
                OutPreview.Message = TEXT("Vector parameter not found on source MIC");
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Preview not found on MIC: %s"), *Operation.Field.FieldPath);
                return true;
            }

            OutPreview.CurrentSourceValue = RI_FormatMaterialPromoteColor(CurrentColor);
        }
#if WITH_EDITOR
        else
        {
            FString FindError;
            UMaterialExpressionVectorParameter* VectorParameter = RI_FindUniqueVectorParameterExpression(Source.Material, ParamName, FindError);
            if (!VectorParameter)
            {
                OutPreview.Message = FindError;
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Preview material lookup failed: %s"), *FindError);
                return true;
            }

            OutPreview.CurrentSourceValue = RI_FormatMaterialPromoteColor(VectorParameter->DefaultValue);
        }
#else
        else
        {
            OutPreview.Message = TEXT("Promote-to-source requires WITH_EDITOR");
            return true;
        }
#endif
    }

    OutPreview.bSupported = true;
    OutPreview.Message = Source.Kind == ERIMaterialPromoteSourceKind::MIC
        ? TEXT("Ready to promote to MIC source")
        : TEXT("Ready to promote to Material source");
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
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][PromoteMaterial] Apply start SourceTag=%s Asset=%s Param=%s PreviewSupported=%d PreviewMessage=%s"),
        *Operation.SourceTag,
        *OutResult.AssetPath,
        *Operation.Field.FieldPath,
        Preview.bSupported ? 1 : 0,
        *Preview.Message);

    if (!Preview.bSupported)
    {
        OutResult.Status = Preview.Message.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
            ? ERIPromoteOperationStatus::NotFound
            : ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = Preview.Message;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Apply blocked: %s"), *OutResult.Message);
        return false;
    }

#if !WITH_EDITOR
    OutResult.Status = ERIPromoteOperationStatus::Unsupported;
    OutResult.Message = TEXT("Promote-to-source requires WITH_EDITOR");
    return false;
#else
    UMaterialInterface* RuntimeMaterial = nullptr;
    AActor* RuntimeActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (RuntimeActor)
    {
        UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
            RuntimeActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent))
        {
            RuntimeMaterial = MeshComponent->GetMaterial(Operation.Target.MaterialSlotIndex);
        }
        else if (RI_ExtractMaterialSourceAssetPath(Operation.SourceTag).IsEmpty())
        {
            OutResult.Status = ERIPromoteOperationStatus::Unsupported;
            OutResult.Message = TEXT("Target component is not a mesh component");
            return false;
        }
    }
    else if (RI_ExtractMaterialSourceAssetPath(Operation.SourceTag).IsEmpty())
    {
        OutResult.Status = ERIPromoteOperationStatus::NotFound;
        OutResult.Message = TEXT("Runtime actor not found");
        return false;
    }

    const FRIMaterialPromoteSource Source = RI_ResolveMaterialPromoteSource(Operation.SourceTag, RuntimeMaterial);
    if (Source.Kind == ERIMaterialPromoteSourceKind::Unsupported)
    {
        OutResult.Status = Source.Message.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
            ? ERIPromoteOperationStatus::NotFound
            : ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = Source.Message;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Apply unsupported after resolve: %s"), *OutResult.Message);
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

        if (Source.Kind == ERIMaterialPromoteSourceKind::MIC)
        {
            Source.MIC->Modify();
            Source.MIC->PreEditChange(nullptr);
            Source.MIC->SetScalarParameterValueEditorOnly(Info, NewValue);
            Source.MIC->PostEditChange();
        }
        else
        {
            FString FindError;
            UMaterialExpressionScalarParameter* ScalarParameter = RI_FindUniqueScalarParameterExpression(Source.Material, ParamName, FindError);
            if (!ScalarParameter)
            {
                OutResult.Status = FindError.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
                    ? ERIPromoteOperationStatus::NotFound
                    : ERIPromoteOperationStatus::Unsupported;
                OutResult.Message = FindError;
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Apply scalar material lookup failed: %s"), *OutResult.Message);
                return false;
            }

            Source.Material->Modify();
            Source.Material->PreEditChange(nullptr);
            ScalarParameter->Modify();
            ScalarParameter->DefaultValue = NewValue;
            Source.Material->PostEditChange();
        }

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

        if (Source.Kind == ERIMaterialPromoteSourceKind::MIC)
        {
            Source.MIC->Modify();
            Source.MIC->PreEditChange(nullptr);
            Source.MIC->SetVectorParameterValueEditorOnly(Info, NewColor);
            Source.MIC->PostEditChange();
        }
        else
        {
            FString FindError;
            UMaterialExpressionVectorParameter* VectorParameter = RI_FindUniqueVectorParameterExpression(Source.Material, ParamName, FindError);
            if (!VectorParameter)
            {
                OutResult.Status = FindError.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
                    ? ERIPromoteOperationStatus::NotFound
                    : ERIPromoteOperationStatus::Unsupported;
                OutResult.Message = FindError;
                UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Apply vector material lookup failed: %s"), *OutResult.Message);
                return false;
            }

            Source.Material->Modify();
            Source.Material->PreEditChange(nullptr);
            VectorParameter->Modify();
            VectorParameter->DefaultValue = NewColor;
            Source.Material->PostEditChange();
        }

        OutResult.ValueWritten = RI_FormatMaterialPromoteColor(NewColor);
    }

    FString SaveError;
    UObject* SourceAsset = Source.Kind == ERIMaterialPromoteSourceKind::MIC
        ? static_cast<UObject*>(Source.MIC)
        : static_cast<UObject*>(Source.Material);
    if (!RI_SaveAssetPackage(SourceAsset, SaveError))
    {
        OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
        OutResult.Message = SaveError;
        UE_LOG(LogRuntimeInspector, Warning, TEXT("[RI][PromoteMaterial] Save failed Asset=%s Error=%s"), *GetNameSafe(SourceAsset), *SaveError);
        return false;
    }

    OutResult.Status = ERIPromoteOperationStatus::Promoted;
    OutResult.Message = Source.Kind == ERIMaterialPromoteSourceKind::MIC
        ? TEXT("Promoted to MIC source")
        : TEXT("Promoted to Material source");
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][PromoteMaterial] Apply success Resolved=%s Asset=%s Param=%s Value=%s"),
        RI_MaterialPromoteSourceKindToString(Source.Kind),
        *GetNameSafe(SourceAsset),
        *Operation.Field.FieldPath,
        *OutResult.ValueWritten);
    return true;
#endif
}
