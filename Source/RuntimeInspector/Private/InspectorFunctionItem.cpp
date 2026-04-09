#include "InspectorFunctionItem.h"

#include "InspectorWorldSubsystem.h"

namespace
{
    static FRIFunctionParameterSpec RI_MakeFunctionSpec(const FRIInspectorFunctionParameterDefinition& InDefinition)
    {
        FRIFunctionParameterSpec Spec;
        Spec.Name = InDefinition.Name;
        Spec.DisplayName = InDefinition.DisplayName;
        Spec.TypeLabel = InDefinition.TypeLabel;
        Spec.bIsEnum = InDefinition.ValueType == EInspectorValueType::Enum;
        Spec.bIsSupported = InDefinition.ValueType != EInspectorValueType::Unsupported;
        Spec.DefaultText = InDefinition.DefaultValueText;
        Spec.EnumOptions = InDefinition.EnumOptions;
        return Spec;
    }
}

void UInspectorFunctionItem::Init(UObject* InTarget, FName InFunctionName)
{
    Target = InTarget;
    FunctionName = InFunctionName;
    DisplayName.Reset();
    OwnerLabel.Reset();
    SignatureText.Reset();
    TooltipText.Reset();
    ParameterDefinitions.Reset();
    ParameterSpecs.Reset();
}

void UInspectorFunctionItem::SetDisplayMetadata(
    const FString& InDisplayName,
    const FString& InOwnerLabel,
    const FString& InSignatureText,
    const FString& InTooltipText)
{
    DisplayName = InDisplayName;
    OwnerLabel = InOwnerLabel;
    SignatureText = InSignatureText;
    TooltipText = InTooltipText;
}

void UInspectorFunctionItem::SetParameterDefinitions(const TArray<FRIInspectorFunctionParameterDefinition>& InDefinitions)
{
    ParameterDefinitions = InDefinitions;
    ParameterSpecs.Reset();
    ParameterSpecs.Reserve(ParameterDefinitions.Num());
    for (const FRIInspectorFunctionParameterDefinition& Definition : ParameterDefinitions)
    {
        ParameterSpecs.Add(RI_MakeFunctionSpec(Definition));
    }
}

FString UInspectorFunctionItem::GetFunctionName() const
{
    return FunctionName.ToString();
}

const FRIInspectorFunctionParameterDefinition* UInspectorFunctionItem::GetParameterDefinition(int32 Index) const
{
    return ParameterDefinitions.IsValidIndex(Index) ? &ParameterDefinitions[Index] : nullptr;
}

FString UInspectorFunctionItem::GetQualifiedDisplayName() const
{
    if (DisplayName.IsEmpty())
    {
        return GetFunctionName();
    }

    if (OwnerLabel.IsEmpty())
    {
        return DisplayName;
    }

    return FString::Printf(TEXT("%s [%s]"), *DisplayName, *OwnerLabel);
}

bool UInspectorFunctionItem::Invoke(const TArray<FString>& ParameterValues, FString& OutError)
{
    OutError.Reset();

    UObject* TargetObject = Target.Get();
    if (!TargetObject)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    if (UInspectorWorldSubsystem* Subsystem = Cast<UInspectorWorldSubsystem>(GetOuter()))
    {
        return Subsystem->InvokeFunctionItem(this, ParameterValues, OutError);
    }

    OutError = TEXT("Inspector subsystem unavailable");
    return false;
}
