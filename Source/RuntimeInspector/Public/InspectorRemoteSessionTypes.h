#pragma once

#include "CoreMinimal.h"

#include "InspectorRemoteSessionTypes.generated.h"

UENUM(BlueprintType)
enum class ERIRuntimeSessionConnectionState : uint8
{
    Disconnected UMETA(DisplayName = "Disconnected"),
    Connected UMETA(DisplayName = "Connected"),
    Error UMETA(DisplayName = "Error")
};

UENUM(BlueprintType)
enum class ERIRuntimeSessionOrigin : uint8
{
    LocalEditor UMETA(DisplayName = "Local Editor"),
    LocalPIE UMETA(DisplayName = "Local PIE"),
    ExternalPackaged UMETA(DisplayName = "External Packaged")
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString SessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString SessionType;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Host;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 Port = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 ProtocolVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    ERIRuntimeSessionOrigin SessionOrigin = ERIRuntimeSessionOrigin::LocalEditor;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString BuildConfiguration;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bIsExternal = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bLoopbackOnly = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString WorldPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString MapName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString WorldTypeLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString NetModeLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    ERIRuntimeSessionConnectionState ConnectionState = ERIRuntimeSessionConnectionState::Disconnected;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bSessionAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bRequiresExplicitConnect = true;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bRuntimeEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bUnlockRequired = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bUnlocked = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bSupportsTargetListing = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bSupportsPatchApply = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bSupportsVerification = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    TArray<FString> CapabilityTags;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LastError;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionHandshake
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString SessionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Host;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    int32 Port = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    int32 ProtocolVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    ERIRuntimeSessionOrigin SessionOrigin = ERIRuntimeSessionOrigin::ExternalPackaged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString BuildConfiguration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bLoopbackOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bHandshakeSuccessful = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    TArray<FString> CapabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString LastError;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRIRuntimeTargetInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString SessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorClass;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorClassPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bSelected = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bHasAuthority = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bReplicates = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bReplicateMovement = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LocalRoleLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RemoteRoleLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString OwnerPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionCompareField
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString FieldName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftValue;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightValue;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bDifferent = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Message;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetCompareReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString GeneratedAtUtc;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftSessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightSessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString TargetQuery;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bLeftTargetFound = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bRightTargetFound = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftTargetPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightTargetPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftTargetLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightTargetLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftTargetClass;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightTargetClass;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 FieldCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 DifferenceCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Summary;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Details;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionCompareField> Fields;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareLine
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString CompareKey;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString DisplayLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ActorClass;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bPresentInLeft = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bPresentInRight = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 LeftCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 RightCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    bool bHasMismatch = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftPrimaryPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightPrimaryPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Message;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString GeneratedAtUtc;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString LeftSessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString RightSessionId;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString NameFilter;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString ClassFilter;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 LeftMatchCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 RightMatchCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 SharedTargetCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 LeftOnlyCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 RightOnlyCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 MismatchCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    int32 LineCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Summary;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    FString Details;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionTargetSetCompareLine> Lines;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString LeftSessionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString RightSessionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString NameFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString ClassFilter;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareMatrixEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FName EntryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetSetCompareRequest Request;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareMatrixDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FName MatrixId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bRequiresPIE = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionTargetSetCompareMatrixEntry> Entries;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareMatrixEntryResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FName EntryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetSetCompareRequest Request;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetSetCompareReport Report;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionTargetSetCompareMatrixRunResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FName MatrixId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    int32 PassedEntryCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    int32 FailedEntryCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionTargetSetCompareMatrixEntryResult> EntryResults;
};
