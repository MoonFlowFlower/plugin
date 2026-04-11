#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InspectorGroupItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInspectorGroupExpandedChanged, bool, bIsExpanded);

UENUM(BlueprintType)
enum class EInspectorGroupKind : uint8
{
	RootActor        UMETA(DisplayName = "ActorRoot"),
	RootComponents   UMETA(DisplayName = "ComponentsRoot"),
	Component        UMETA(DisplayName = "Component"),
	MaterialsRoot    UMETA(DisplayName = "MaterialsRoot"),
	MaterialSlot     UMETA(DisplayName = "MaterialSlot")
};

/**
 * Collapsible group row model used by the navigator ListView.
 * - RootActor / RootComponents are logical roots.
 * - Component rows represent a whitelisted component group.
 */
UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorGroupItem : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Tree")
	int32 Depth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	EInspectorGroupKind Kind = EInspectorGroupKind::RootActor;

	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	FString DisplayName;

	/** Stable key used to persist expand/collapse state. */
	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	FString StableKey;

	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	bool bExpanded = true;

	/** Backing object for component groups, typically a UActorComponent. */
	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	TObjectPtr<UObject> TargetObject = nullptr;

public:

	UPROPERTY(BlueprintAssignable, Category = "RuntimeInspector|Group")
	FOnInspectorGroupExpandedChanged OnExpandedChanged;

	UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Group")
	bool GetIsExpanded() const { return bExpanded; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Group")
	void SetIsExpanded(bool bInExpanded)
	{
		if (bExpanded == bInExpanded) return;
		bExpanded = bInExpanded;
		OnExpandedChanged.Broadcast(bExpanded);
	}

	UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Group")
	void ToggleExpanded()
	{
		SetIsExpanded(!bExpanded);
	}

public:
	UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector")
	int32 MaterialSlotIndex = INDEX_NONE;

	UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
	bool IsMaterialsRoot() const
	{
		return StableKey.EndsWith(TEXT(":MATERIALS"));
	}

	UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
	bool IsMaterialSlot() const
	{
		return StableKey.Contains(TEXT(":MATERIALS:MAT:"));
	}

	UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
	bool IsComponentGroup() const { return Kind == EInspectorGroupKind::Component && TargetObject != nullptr; }

	UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
	bool IsMaterialsNode() const
	{
		return StableKey.EndsWith(TEXT(":MATERIALS"));
	}

	UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
	bool IsMaterialSlotNode() const
	{
		return StableKey.Contains(TEXT(":MAT:"));
	}

};
