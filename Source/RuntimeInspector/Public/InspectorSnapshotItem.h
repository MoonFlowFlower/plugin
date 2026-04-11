#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InspectorSnapshotItem.generated.h"

UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API UInspectorSnapshotItem : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString FileName;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString FullPath;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString CreatedAtUtc;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString MapName;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString SelectedActorPath;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    int32 EntryCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    FString ActorShortName;

    // Used for sorting (Unix seconds).
    UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
    int64 CreatedAtUnixSeconds = 0;
};
