#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "InspectorRemoteSessionTypes.h"

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

    // Struct types (commonly tuned at runtime)
    Vector2     UMETA(DisplayName = "Vector2"),
    Vector3     UMETA(DisplayName = "Vector"),
    Vector4     UMETA(DisplayName = "Vector4"),
    Rotator     UMETA(DisplayName = "Rotator"),
    Transform   UMETA(DisplayName = "Transform"),
    LinearColor UMETA(DisplayName = "LinearColor"),
    Color       UMETA(DisplayName = "Color"),
};

USTRUCT(BlueprintType)
struct FRIInspectorFunctionParameterDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    FName Name = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    EInspectorValueType ValueType = EInspectorValueType::Unsupported;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    FString TypeLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    FString DefaultValueText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    bool bHasDefaultValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Function")
    TArray<FString> EnumOptions;
};

USTRUCT(BlueprintType)
struct FRIFunctionParameterSpec
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    FName Name = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    FString TypeLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    bool bIsEnum = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    bool bIsSupported = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    FString DefaultText;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Function")
    TArray<FString> EnumOptions;
};

// ===== Snapshot Import Report (UI-facing) =====
USTRUCT(BlueprintType)
struct FRIEditableSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    FKey ToggleKey = EKeys::O;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    FKey PickKey = EKeys::P;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bPickKeyRequiresCtrl = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bPickKeyRequiresShift = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bEnableRightMousePick = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bRightMousePickRequiresCtrl = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bRightMousePickRequiresShift = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bEnableOutlinePP = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    float OutlinePPWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bEnableApplyDebounce = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    float ApplyDebounceSeconds = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bRequireUnlock = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Settings")
    bool bAutoLockOnClose = true;
};

USTRUCT(BlueprintType)
struct FRISettingsDiagnostics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bRuntimeEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    FString DisabledReason;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bUnlockRequired = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bUnlocked = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bHasUnlockCode = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bOutlineMaterialAssigned = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    FString OutlineMaterialPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Settings")
    bool bCustomDepthStencilReady = false;
};

USTRUCT(BlueprintType)
struct FRIRuntimeSessionSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bSessionAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bIsGameWorld = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bIsPIEWorld = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString WorldTypeLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString NetModeLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString MapName;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bHasLocalPlayerController = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString LocalPlayerControllerPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRIRuntimeActorRoleSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bHasActor = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString ActorPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString ActorClass;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bHasAuthority = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString LocalRoleLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString RemoteRoleLabel;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bReplicates = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    bool bReplicateMovement = false;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString OwnerPath;

    UPROPERTY(BlueprintReadOnly, Category = "RuntimeInspector|Session")
    FString Summary;
};

USTRUCT(BlueprintType)
struct FRISelfTestDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FName Id = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    bool bRequiresPIE = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    bool bMutatesRuntime = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct FRISelfTestResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FName Id = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    FString StartedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|SelfTest")
    int32 DurationMs = 0;
};

USTRUCT(BlueprintType)
struct FRIVerificationProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FName ProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString PatchCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    TArray<FName> RequiredSelfTestIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    bool bRequiresPIE = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    bool bMutatesRuntime = false;
};

USTRUCT(BlueprintType)
struct FRIVerificationRunResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FName ProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString PatchCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Verification")
    TArray<FRISelfTestResult> TestResults;
};

USTRUCT(BlueprintType)
struct FRIWorkflowDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FName WorkflowId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bRequiresPIE = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bRequiresSelectedActor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bMutatesRuntime = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bMutatesSource = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FString> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FName> ChildWorkflowIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FName> VerificationProfileIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FName> SelfTestIds;
};

USTRUCT(BlueprintType)
struct FRIWorkflowRunResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FName WorkflowId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bMutatedRuntime = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    bool bMutatedSource = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString SelectedActorPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FString> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FName> ExecutedChildWorkflowIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FString> ExecutedChildWorkflowSummaries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    int32 PassedStepCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    int32 FailedStepCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FRIVerificationRunResult> VerificationResults;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Workflow")
    TArray<FRISelfTestResult> SelfTestResults;
};

USTRUCT(BlueprintType)
struct FRIWorkflowMatrixEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName EntryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName WorkflowId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString ActorQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FRIRuntimeSessionTargetSetCompareRequest RemoteSessionCompareRequest;
};

USTRUCT(BlueprintType)
struct FRIWorkflowMatrixDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName MatrixId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    bool bRequiresPIE = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    TArray<FRIWorkflowMatrixEntry> Entries;
};

USTRUCT(BlueprintType)
struct FRIWorkflowMatrixEntryRunResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName EntryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName WorkflowId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString RequestedActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FRIRuntimeSessionTargetSetCompareRequest RemoteSessionCompareRequest;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FRIWorkflowRunResult WorkflowRunResult;
};

USTRUCT(BlueprintType)
struct FRIWorkflowMatrixRunResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FName MatrixId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    int32 PassedEntryCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    int32 FailedEntryCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    FString FullReport;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|WorkflowMatrix")
    TArray<FRIWorkflowMatrixEntryRunResult> EntryResults;
};

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
