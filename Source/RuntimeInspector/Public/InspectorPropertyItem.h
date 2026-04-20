#pragma once


#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InspectorTypes.h"
#include "InspectorPropertyItem.generated.h"

class AActor;
class USceneComponent;


UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorPropertyItem : public UObject
{
    GENERATED_BODY()

public:
    enum class ESyntheticKind : uint8
    {
        None,
        ActorWorldLocation,
        ActorWorldRotation,
        ActorWorldScale,
    };

    void Init(UObject* InTarget, FName InPropertyName);
    void InitSyntheticActorWorld(
        AActor* InActor,
        FName InSyntheticPropertyName,
        USceneComponent* InTrackingRootComponent,
        FName InTrackingPropertyName,
        ESyntheticKind InSyntheticKind);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetPropertyName() const;

    FString GetPropertyNameWithoutOwnerPrefix() const;
    FString GetCategoryPath() const;
    FString GetPrimaryCategoryName() const;
    FString GetSubcategoryPath() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetValueText();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
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

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsValidItem() const { return Target.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsEditable() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsReadOnly() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetTypeLabel() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    FString GetReadOnlyReason() const;

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

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    UObject* GetTrackingTargetObject() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FName GetTrackingPropertyFName() const;

    bool IsSyntheticItem() const { return SyntheticKind != ESyntheticKind::None; }
    bool IsSyntheticActorWorldTransform() const;
private:
    TWeakObjectPtr<UObject> Target;
    FName PropertyName;
    TWeakObjectPtr<UObject> TrackingTarget;
    FName TrackingPropertyName;
    ESyntheticKind SyntheticKind = ESyntheticKind::None;
};
