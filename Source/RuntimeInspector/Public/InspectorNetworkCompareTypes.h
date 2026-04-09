#pragma once

#include "CoreMinimal.h"
#include "InspectorPatchTypes.h"

#include "InspectorNetworkCompareTypes.generated.h"

UENUM(BlueprintType)
enum class ERIRuntimeCompareRole : uint8
{
    Authority,
    AutonomousProxy,
    SimulatedProxy
};

USTRUCT(BlueprintType)
struct FRIRuntimeRoleFieldState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    ERIRuntimeCompareRole Role = ERIRuntimeCompareRole::Authority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString RoleLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bRoleAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bTargetFound = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bHasValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString ValueText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Message;
};

USTRUCT(BlueprintType)
struct FRIRuntimeRoleCompareLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FRIPatchTarget Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FRIPatchFieldRef Field;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString CompareKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 AvailableRoleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 MissingRoleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bHasMismatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    TArray<FRIRuntimeRoleFieldState> RoleStates;
};

USTRUCT(BlueprintType)
struct FRIRuntimeRoleVerificationState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    ERIRuntimeCompareRole Role = ERIRuntimeCompareRole::Authority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString RoleLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bRoleAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bExecuted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRIRuntimeRoleVerificationLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FName ProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    bool bHasMismatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    TArray<FRIRuntimeRoleVerificationState> RoleStates;
};

USTRUCT(BlueprintType)
struct FRIRuntimeRoleCompareReport
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString GeneratedAtUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString ActorPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString AvailableRoleLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString BundleId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 OperationCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 ComparedLineCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 MismatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 MissingRoleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    int32 VerificationMismatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    FString Details;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    TArray<FRIRuntimeRoleCompareLine> Lines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|RoleCompare")
    TArray<FRIRuntimeRoleVerificationLine> VerificationLines;
};

RUNTIMEINSPECTOR_API FString RI_RuntimeCompareRoleLabel(ERIRuntimeCompareRole InRole);
RUNTIMEINSPECTOR_API const TArray<ERIRuntimeCompareRole>& RI_GetRuntimeCompareRoles();
