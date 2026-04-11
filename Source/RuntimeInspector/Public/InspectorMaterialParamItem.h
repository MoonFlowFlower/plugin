#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "UObject/Object.h"

#include "InspectorTypes.h"

#include "InspectorMaterialParamItem.generated.h"

UENUM(BlueprintType)
enum class EInspectorMatParamType : uint8
{
    Scalar,
    Vector
};

UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorMaterialParamItem : public UObject
{
    GENERATED_BODY()

public:
    void Init(class UMeshComponent* InComp, int32 InSlotIndex, FName InParamName, EInspectorMatParamType InType);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetPropertyName() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetValueText();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsEditable() const { return true; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool ApplyFromText(const FString& NewText, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsEnum() const { return false; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    EInspectorValueType GetValueType() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool GetScalar(float& OutValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool SetScalar(float NewValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool GetVector(FLinearColor& OutValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool SetVector(const FLinearColor& NewValue, FString& OutError);

private:
    UPROPERTY()
    TWeakObjectPtr<UMeshComponent> TargetComp;

    UPROPERTY()
    int32 SlotIndex = 0;

    UPROPERTY()
    FName ParamName = NAME_None;

    UPROPERTY()
    EInspectorMatParamType ParamType = EInspectorMatParamType::Scalar;

public:
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    UMeshComponent* GetMeshComponent() const { return TargetComp.Get(); }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FName GetParamName() const { return ParamName; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    EInspectorMatParamType GetParamType() const { return ParamType; }
};
