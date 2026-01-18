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

    // ===== Struct helpers (Vector/Rotator/Transform/Color) =====
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetVector2D(FVector2D& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetVector(FVector& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetVector4(FVector4& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetRotator(FRotator& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetTransform(FTransform& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetLinearColor(FLinearColor& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool GetColor(FColor& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetVector2D(const FVector2D& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetVector(const FVector& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetVector4(const FVector4& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetRotator(const FRotator& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetTransform(const FTransform& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetLinearColor(const FLinearColor& InValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Struct")
    bool SetColor(const FColor& InValue, FString& OutError);

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

