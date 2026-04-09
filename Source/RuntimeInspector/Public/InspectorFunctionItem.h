#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InspectorTypes.h"

#include "InspectorFunctionItem.generated.h"

UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorFunctionItem : public UObject
{
    GENERATED_BODY()

public:
    void Init(UObject* InTarget, FName InFunctionName);
    void SetDisplayMetadata(const FString& InDisplayName, const FString& InOwnerLabel, const FString& InSignatureText, const FString& InTooltipText);
    void SetParameterDefinitions(const TArray<FRIInspectorFunctionParameterDefinition>& InDefinitions);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    bool IsValidItem() const { return Target.IsValid() && FunctionName != NAME_None; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    UObject* GetTargetObject() const { return Target.Get(); }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FName GetFunctionFName() const { return FunctionName; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetFunctionName() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetOwnerLabel() const { return OwnerLabel; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetSignatureText() const { return SignatureText; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Function")
    FString GetTooltipText() const { return TooltipText; }

    int32 GetParameterCount() const { return ParameterDefinitions.Num(); }
    const TArray<FRIInspectorFunctionParameterDefinition>& GetParameterDefinitions() const { return ParameterDefinitions; }
    const TArray<FRIFunctionParameterSpec>& GetParameterSpecs() const { return ParameterSpecs; }
    const FRIInspectorFunctionParameterDefinition* GetParameterDefinition(int32 Index) const;
    FString GetOwnerPrefix() const { return GetOwnerLabel(); }
    FString GetQualifiedDisplayName() const;

    bool Invoke(const TArray<FString>& ParameterValues, FString& OutError);

private:
    TWeakObjectPtr<UObject> Target;
    FName FunctionName = NAME_None;
    FString DisplayName;
    FString OwnerLabel;
    FString SignatureText;
    FString TooltipText;

    UPROPERTY()
    TArray<FRIInspectorFunctionParameterDefinition> ParameterDefinitions;

    UPROPERTY()
    TArray<FRIFunctionParameterSpec> ParameterSpecs;
};
