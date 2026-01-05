#pragma once


#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InspectorTypes.h"
#include "InspectorPropertyItem.generated.h"


UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorPropertyItem : public UObject
{
    GENERATED_BODY()

public:
    void Init(UObject* InTarget, FName InPropertyName);

    UFUNCTION(BlueprintCallable)
    FString GetPropertyName() const;

    UFUNCTION(BlueprintCallable)
    FString GetValueText();

    UFUNCTION(BlueprintCallable)
    bool ApplyFromText(const FString& NewText, FString& OutError);

    UFUNCTION(BlueprintCallable)
    bool IsValidItem() const { return Target.IsValid(); }

    UFUNCTION(BlueprintCallable)
    bool IsEditable() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    EInspectorValueType GetValueType() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsEnum() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetEnumOptions(TArray<FString>& OutOptions) const;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
    FString OwnerPrefix;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    UObject* GetTargetObject() const { return Target.Get(); }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FName GetPropertyFName() const { return PropertyName; }
private:
    TWeakObjectPtr<UObject> Target;
    FName PropertyName;
};

