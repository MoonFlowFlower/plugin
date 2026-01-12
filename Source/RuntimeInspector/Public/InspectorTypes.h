#pragma once

#include "CoreMinimal.h"


#include "InspectorTypes.generated.h"

UENUM(BlueprintType)
enum class EInspectorValueType : uint8
{
    Unsupported UMETA(DisplayName = "Unsupported"),
    Bool        UMETA(DisplayName = "Bool"),
    Int         UMETA(DisplayName = "Int"),
    Float       UMETA(DisplayName = "Float"),
    Double      UMETA(DisplayName = "Double"),
    String      UMETA(DisplayName = "String"),
    Name        UMETA(DisplayName = "Name"),
    Enum        UMETA(DisplayName = "Enum"),
};

// ===== Snapshot Import Report (UI-facing) =====
USTRUCT(BlueprintType)
struct FRIImportReport
{
    GENERATED_BODY()

    // Final outcome: true only when missing==0 && hardFail==0
    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    int32 AppliedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    int32 SkippedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    int32 MissingCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    int32 HardFailCount = 0;

    // Short single-line summary for UI
    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    FString Summary;

    // Multi-line details (combines Missing/Hard/Warnings)
    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    FString Details;

    // Optional structured lists (already trimmed, one item per line)
    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    TArray<FString> MissingErrors;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    TArray<FString> HardErrors;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Snapshot")
    TArray<FString> Warnings;
};


