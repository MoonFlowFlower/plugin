#include "InspectorWorldSubsystem.h"
#include "InspectorPropertyUtils.h"
#include "RuntimeInspectorSettings.h"

namespace
{
    static bool RI_ParseSourceTagPrefix(const FString& SourceTag, FString& OutPrefix, FString& OutPayload)
    {
        OutPrefix.Reset();
        OutPayload.Reset();

        FString Trimmed = SourceTag.TrimStartAndEnd();
        int32 ColonIndex = INDEX_NONE;
        if (!Trimmed.FindChar(TEXT(':'), ColonIndex) || ColonIndex <= 0)
        {
            return false;
        }

        OutPrefix = Trimmed.Left(ColonIndex);
        OutPayload = Trimmed.Mid(ColonIndex + 1).TrimStartAndEnd();
        return true;
    }
}

bool UInspectorWorldSubsystem::BuildDataPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const
{
    OutPreview = FRIPromoteOperationPreview();
    OutPreview.Operation = Operation;
    OutPreview.DesiredSourceValue = Operation.PatchedValue;

    FString Prefix;
    FString Payload;
    if (!RI_ParseSourceTagPrefix(Operation.SourceTag, Prefix, Payload))
    {
        OutPreview.Message = TEXT("Invalid source mapping tag");
        return true;
    }

    if (Prefix.Equals(TEXT("Config"), ESearchCase::IgnoreCase))
    {
        if (!Payload.Equals(TEXT("/Script/RuntimeInspector.RuntimeInspectorSettings"), ESearchCase::IgnoreCase))
        {
            OutPreview.Message = TEXT("Unsupported config target");
            return true;
        }

        const URuntimeInspectorSettings* Settings = GetDefault<URuntimeInspectorSettings>();
        if (!Settings)
        {
            OutPreview.Message = TEXT("RuntimeInspector settings object is unavailable");
            return true;
        }
        UObject* SettingsObject = const_cast<URuntimeInspectorSettings*>(Settings);

        const FName PropertyName(*Operation.Field.FieldPath);
        FProperty* Property = Settings->GetClass()->FindPropertyByName(PropertyName);
        if (!Property)
        {
            OutPreview.Message = TEXT("Config property not found");
            return true;
        }

        if (!Property->HasAnyPropertyFlags(CPF_Config))
        {
            OutPreview.Message = TEXT("Property is not config-backed");
            return true;
        }

        if (!InspectorPropertyUtils::CanSetFromText(SettingsObject, Property))
        {
            OutPreview.Message = TEXT("Config property is not writable from text");
            return true;
        }

        if (!InspectorPropertyUtils::GetValueAsText(SettingsObject, PropertyName, OutPreview.CurrentSourceValue))
        {
            OutPreview.Message = TEXT("Failed to read config source value");
            return true;
        }

        OutPreview.AssetPath = Payload;
        OutPreview.bSupported = true;
        OutPreview.Message = TEXT("Ready to promote to config-backed settings source");
        OutPreview.DiffText = FString::Printf(
            TEXT("%s :: %s %s -> %s"),
            *OutPreview.AssetPath,
            *Operation.Field.FieldPath,
            *OutPreview.CurrentSourceValue,
            *OutPreview.DesiredSourceValue);
        return true;
    }

    if (Prefix.Equals(TEXT("DataAsset"), ESearchCase::IgnoreCase))
    {
        OutPreview.AssetPath = Payload;
        OutPreview.Message = TEXT("Explicit DataAsset mapping recognized, but DataAsset promote is not implemented yet");
        return true;
    }

    if (Prefix.Equals(TEXT("DataTable"), ESearchCase::IgnoreCase))
    {
        OutPreview.AssetPath = Payload;
        OutPreview.Message = TEXT("Explicit DataTable mapping recognized, but DataTable promote is not implemented yet");
        return true;
    }

    OutPreview.Message = TEXT("Unsupported source mapping prefix");
    return true;
}

bool UInspectorWorldSubsystem::PromoteDataOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult)
{
    OutResult = FRIPromoteOperationResult();
    OutResult.Operation = Operation;

    FRIPromoteOperationPreview Preview;
    BuildDataPromotePreview(Operation, Preview);
    OutResult.AssetPath = Preview.AssetPath;

    if (!Preview.bSupported)
    {
        OutResult.Status = Preview.Message.Contains(TEXT("not found"), ESearchCase::IgnoreCase)
            ? ERIPromoteOperationStatus::NotFound
            : ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = Preview.Message;
        return false;
    }

    FString Prefix;
    FString Payload;
    if (!RI_ParseSourceTagPrefix(Operation.SourceTag, Prefix, Payload))
    {
        OutResult.Status = ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = TEXT("Invalid source mapping tag");
        return false;
    }

    if (!Prefix.Equals(TEXT("Config"), ESearchCase::IgnoreCase)
        || !Payload.Equals(TEXT("/Script/RuntimeInspector.RuntimeInspectorSettings"), ESearchCase::IgnoreCase))
    {
        OutResult.Status = ERIPromoteOperationStatus::Unsupported;
        OutResult.Message = TEXT("Only RuntimeInspector config promotion is supported in v1");
        return false;
    }

    URuntimeInspectorSettings* Settings = GetMutableDefault<URuntimeInspectorSettings>();
    if (!Settings)
    {
        OutResult.Status = ERIPromoteOperationStatus::NotFound;
        OutResult.Message = TEXT("RuntimeInspector settings object is unavailable");
        return false;
    }

    const FName PropertyName(*Operation.Field.FieldPath);
    FString SetError;
    Settings->Modify();
    if (!InspectorPropertyUtils::SetValueFromText(Settings, PropertyName, Operation.PatchedValue, &SetError))
    {
        OutResult.Status = ERIPromoteOperationStatus::WriteFailed;
        OutResult.Message = SetError;
        return false;
    }

    Settings->SaveConfig();

    FString FinalValue;
    InspectorPropertyUtils::GetValueAsText(Settings, PropertyName, FinalValue);
    OutResult.Status = ERIPromoteOperationStatus::Promoted;
    OutResult.Message = TEXT("Promoted to config-backed settings source");
    OutResult.ValueWritten = FinalValue;

    LastSavedSettingsSnapshot = GetEditableSettings();
    bSettingsDirty = false;
    return true;
}
