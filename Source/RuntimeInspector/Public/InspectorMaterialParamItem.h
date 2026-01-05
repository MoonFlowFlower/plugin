#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"


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
    UMeshComponent* GetMeshComponent() const { return TargetComp.Get(); }  // 下面变量名按你自己的改

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FName GetParamName() const { return ParamName; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    EInspectorMatParamType GetParamType() const { return ParamType; }
};