#pragma once

#include "CoreMinimal.h"

#include "InspectorPatchTypes.generated.h"

UENUM(BlueprintType)
enum class ERIPatchTargetKind : uint8
{
    Actor,
    Component,
    MaterialSlot
};

UENUM(BlueprintType)
enum class ERIPatchFieldKind : uint8
{
    Property,
    MaterialScalar,
    MaterialVector
};

UENUM(BlueprintType)
enum class ERIPatchValueKind : uint8
{
    ImportText
};

UENUM(BlueprintType)
enum class ERIPatchPresetApplicabilityScope : uint8
{
    CurrentSelection,
    ActorClass,
    ComponentClass
};

UENUM(BlueprintType)
enum class ERIApplyOperationStatus : uint8
{
    Applied,
    NotFound,
    TypeMismatch,
    WriteFailed,
    Skipped
};

USTRUCT(BlueprintType)
struct FRIPatchTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    ERIPatchTargetKind TargetKind = ERIPatchTargetKind::Actor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ActorPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ActorBaseName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ComponentPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ComponentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ComponentClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    int32 MaterialSlotIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString MaterialSlotName;
};

USTRUCT(BlueprintType)
struct FRIPatchFieldRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    ERIPatchFieldKind FieldKind = ERIPatchFieldKind::Property;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString FieldPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString DisplayName;
};

USTRUCT(BlueprintType)
struct FRIPatchOperation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FRIPatchTarget Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FRIPatchFieldRef Field;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    ERIPatchValueKind ValueKind = ERIPatchValueKind::ImportText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString BaselineValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString PatchedValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString SourceTag;
};

USTRUCT(BlueprintType)
struct FRIPatchBundle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    int32 Version = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString BundleId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString CapturedAtUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString CapturedFromSelection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    TArray<FRIPatchOperation> Operations;
};

USTRUCT(BlueprintType)
struct FRIPatchOperationResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FRIPatchTarget Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FRIPatchFieldRef Field;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    ERIApplyOperationStatus Status = ERIApplyOperationStatus::Skipped;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString ValueApplied;
};

USTRUCT(BlueprintType)
struct FRIApplyResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    int32 AppliedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    int32 FailedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    int32 SkippedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    TArray<FRIPatchOperationResult> OperationResults;
};

USTRUCT(BlueprintType)
struct FRIPatchPresetMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString PresetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString CreatedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Patch")
    ERIPatchPresetApplicabilityScope ApplicabilityScope = ERIPatchPresetApplicabilityScope::CurrentSelection;
};

UENUM(BlueprintType)
enum class ERIPromoteOperationStatus : uint8
{
    Promoted,
    PreviewSupported,
    Unsupported,
    NotFound,
    WriteFailed,
    Skipped
};

USTRUCT(BlueprintType)
struct FRIPromoteOperationPreview
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FRIPatchOperation Operation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString AssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    bool bSupported = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString CurrentSourceValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString DesiredSourceValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString DiffText;
};

USTRUCT(BlueprintType)
struct FRIPromotePreview
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    bool bCanPromote = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    int32 SupportedOperationCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    int32 UnsupportedOperationCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    TArray<FString> TargetAssetPaths;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    TArray<FRIPromoteOperationPreview> Operations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString DiffText;
};

USTRUCT(BlueprintType)
struct FRIPromoteOperationResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FRIPatchOperation Operation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString AssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    ERIPromoteOperationStatus Status = ERIPromoteOperationStatus::Skipped;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString ValueWritten;
};

USTRUCT(BlueprintType)
struct FRIPromoteResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    int32 PromotedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    int32 FailedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    int32 SkippedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    TArray<FString> TargetAssetPaths;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    TArray<FRIPromoteOperationResult> OperationResults;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Promote")
    FString ReportText;
};

UENUM(BlueprintType)
enum class ERIAuditComparisonMode : uint8
{
    BaselineVsCurrent,
    CurrentVsPatch,
    PatchVsSource,
    AppliedPatchVsSourceAfterPromote
};

USTRUCT(BlueprintType)
struct FRIAuditLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FRIPatchTarget Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FRIPatchFieldRef Field;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString GroupLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString LeftTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString LeftValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString RightTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString RightValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    bool bDifferent = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString Message;
};

USTRUCT(BlueprintType)
struct FRIAuditReport
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    ERIAuditComparisonMode Mode = ERIAuditComparisonMode::CurrentVsPatch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString GeneratedAtUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    FString Details;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|Audit")
    TArray<FRIAuditLine> Lines;
};

USTRUCT(BlueprintType)
struct FRIFileManagementSummary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 SnapshotCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 PatchBundleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 PresetCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 AuditReportCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LatestSnapshotLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LatestPatchBundleLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LatestPresetLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LatestAuditReportLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasStagedPatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 StagedPatchOperationCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString StagedPatchLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastPatchApplySummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastAuditSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 CachedAuditViewCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString CachedAuditViewsSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString ActiveAuditViewLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasPromotePreview = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 PromoteSupportedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 PromoteUnsupportedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString PromotePreviewSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasLastPromoteResult = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastPromotePromotedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastPromoteFailedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastPromoteSkippedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastPromoteSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasRoleCompareReport = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRoleCompareSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRoleCompareAvailableRole;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRoleCompareLineCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRoleCompareMismatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRoleCompareMissingRoleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRoleCompareVerificationMismatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasRemoteSessionCompareReport = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemoteSessionCompareSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemoteSessionCompareSessionPair;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRemoteSessionCompareLineCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRemoteSessionCompareSharedCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRemoteSessionCompareLeftOnlyCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRemoteSessionCompareRightOnlyCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    int32 LastRemoteSessionCompareMismatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    bool bHasRemotePatchPullResult = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemotePatchPullSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemoteSessionSelectionSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemoteSessionTargetQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeInspector|File")
    FString LastRemoteSessionWorkflowId;
};
