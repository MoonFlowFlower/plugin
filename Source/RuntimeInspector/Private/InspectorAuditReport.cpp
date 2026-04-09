#include "InspectorWorldSubsystem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorSnapshotItem.h"

#include "Components/MeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    static FString RI_AuditModeToString(ERIAuditComparisonMode Mode)
    {
        switch (Mode)
        {
        case ERIAuditComparisonMode::BaselineVsCurrent: return TEXT("BaselineVsCurrent");
        case ERIAuditComparisonMode::CurrentVsPatch: return TEXT("CurrentVsPatch");
        case ERIAuditComparisonMode::PatchVsSource: return TEXT("PatchVsSource");
        case ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote: return TEXT("AppliedPatchVsSourceAfterPromote");
        default: return TEXT("Unknown");
        }
    }

    static FString RI_AuditModeLabel(ERIAuditComparisonMode Mode)
    {
        switch (Mode)
        {
        case ERIAuditComparisonMode::BaselineVsCurrent: return TEXT("Baseline vs Current");
        case ERIAuditComparisonMode::CurrentVsPatch: return TEXT("Current vs Patch");
        case ERIAuditComparisonMode::PatchVsSource: return TEXT("Patch vs Source");
        case ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote: return TEXT("Applied Patch vs Source");
        default: return TEXT("Unknown");
        }
    }

    static FString RI_AuditTargetLabel(const FRIPatchTarget& Target)
    {
        FString Label = Target.ActorBaseName.IsEmpty() ? Target.ActorPath : Target.ActorBaseName;
        if (Target.TargetKind == ERIPatchTargetKind::Component)
        {
            Label += FString::Printf(TEXT(" / %s"), Target.ComponentName.IsEmpty() ? *Target.ComponentPath : *Target.ComponentName);
        }
        else if (Target.TargetKind == ERIPatchTargetKind::MaterialSlot)
        {
            Label += FString::Printf(
                TEXT(" / %s / Slot %d"),
                Target.ComponentName.IsEmpty() ? *Target.ComponentPath : *Target.ComponentName,
                Target.MaterialSlotIndex);
        }
        return Label;
    }

    static void RI_AddUniqueText(TArray<FString>& InOutValues, const FString& Value)
    {
        if (!Value.IsEmpty() && !InOutValues.Contains(Value))
        {
            InOutValues.Add(Value);
        }
    }

    static void RI_ListFilesSorted(const FString& Directory, const FString& Pattern, TArray<FString>& OutFiles)
    {
        TArray<FString> FileNames;
        if (IFileManager::Get().DirectoryExists(*Directory))
        {
            IFileManager::Get().FindFiles(FileNames, *(FPaths::Combine(Directory, Pattern)), true, false);
            FileNames.Sort([](const FString& A, const FString& B)
            {
                return A > B;
            });
        }

        for (const FString& FileName : FileNames)
        {
            OutFiles.Add(FPaths::Combine(Directory, FileName));
        }
    }

    static bool RI_TryParseBoolText(const FString& InValue, bool& OutValue)
    {
        const FString Trimmed = InValue.TrimStartAndEnd().ToLower();
        if (Trimmed == TEXT("true") || Trimmed == TEXT("1"))
        {
            OutValue = true;
            return true;
        }
        if (Trimmed == TEXT("false") || Trimmed == TEXT("0"))
        {
            OutValue = false;
            return true;
        }
        return false;
    }

    static bool RI_AreAuditValuesEquivalent(const FString& A, const FString& B)
    {
        const FString TrimmedA = A.TrimStartAndEnd();
        const FString TrimmedB = B.TrimStartAndEnd();

        if (TrimmedA.Equals(TrimmedB, ESearchCase::IgnoreCase))
        {
            return true;
        }

        const FString CompactA = TrimmedA.Replace(TEXT(" "), TEXT(""));
        const FString CompactB = TrimmedB.Replace(TEXT(" "), TEXT(""));
        if (CompactA.Equals(CompactB, ESearchCase::IgnoreCase))
        {
            return true;
        }

        bool BoolA = false;
        bool BoolB = false;
        if (RI_TryParseBoolText(TrimmedA, BoolA) && RI_TryParseBoolText(TrimmedB, BoolB))
        {
            return BoolA == BoolB;
        }

        double NumberA = 0.0;
        double NumberB = 0.0;
        if (FDefaultValueHelper::ParseDouble(TrimmedA, NumberA) && FDefaultValueHelper::ParseDouble(TrimmedB, NumberB))
        {
            return FMath::IsNearlyEqual(NumberA, NumberB, 0.0001);
        }

        return false;
    }
}

FString UInspectorWorldSubsystem::GetAuditReportsDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Reports"), TEXT("Audit"));
}

FString UInspectorWorldSubsystem::GetAuditReportsDirectory() const
{
    return GetAuditReportsDir();
}

void UInspectorWorldSubsystem::ListPatchBundleFiles(TArray<FString>& OutFiles) const
{
    OutFiles.Reset();
    RI_ListFilesSorted(GetPatchesDirectory(), TEXT("*.json"), OutFiles);
}

void UInspectorWorldSubsystem::ListAuditReportFiles(TArray<FString>& OutFiles) const
{
    OutFiles.Reset();
    RI_ListFilesSorted(GetAuditReportsDir(), TEXT("*.txt"), OutFiles);
    RI_ListFilesSorted(GetAuditReportsDir(), TEXT("*.json"), OutFiles);
}

FRIAuditReport* UInspectorWorldSubsystem::GetMutableCachedAuditReport(ERIAuditComparisonMode InMode)
{
    switch (InMode)
    {
    case ERIAuditComparisonMode::BaselineVsCurrent: return &CachedBaselineAuditReport;
    case ERIAuditComparisonMode::CurrentVsPatch: return &CachedCurrentVsPatchAuditReport;
    case ERIAuditComparisonMode::PatchVsSource: return &CachedPatchVsSourceAuditReport;
    case ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote: return &CachedAppliedPatchVsSourceAuditReport;
    default: return nullptr;
    }
}

const FRIAuditReport* UInspectorWorldSubsystem::GetCachedAuditReportPtr(ERIAuditComparisonMode InMode) const
{
    switch (InMode)
    {
    case ERIAuditComparisonMode::BaselineVsCurrent: return &CachedBaselineAuditReport;
    case ERIAuditComparisonMode::CurrentVsPatch: return &CachedCurrentVsPatchAuditReport;
    case ERIAuditComparisonMode::PatchVsSource: return &CachedPatchVsSourceAuditReport;
    case ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote: return &CachedAppliedPatchVsSourceAuditReport;
    default: return nullptr;
    }
}

bool UInspectorWorldSubsystem::GetCachedAuditReport(ERIAuditComparisonMode InMode, FRIAuditReport& OutReport) const
{
    OutReport = FRIAuditReport();
    const FRIAuditReport* CachedReport = GetCachedAuditReportPtr(InMode);
    if (!CachedReport)
    {
        return false;
    }

    const bool bHasReport = !CachedReport->Summary.IsEmpty() || CachedReport->Lines.Num() > 0;
    if (!bHasReport)
    {
        return false;
    }

    OutReport = *CachedReport;
    return true;
}

TArray<ERIAuditComparisonMode> UInspectorWorldSubsystem::GetAvailableCachedAuditModes() const
{
    TArray<ERIAuditComparisonMode> Modes;

    auto AddIfPresent = [this, &Modes](ERIAuditComparisonMode Mode)
    {
        FRIAuditReport Report;
        if (GetCachedAuditReport(Mode, Report))
        {
            Modes.Add(Mode);
        }
    };

    AddIfPresent(ERIAuditComparisonMode::BaselineVsCurrent);
    AddIfPresent(ERIAuditComparisonMode::CurrentVsPatch);
    AddIfPresent(ERIAuditComparisonMode::PatchVsSource);
    AddIfPresent(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote);
    return Modes;
}

void UInspectorWorldSubsystem::SetActiveFileAuditViewMode(ERIAuditComparisonMode InMode)
{
    FRIAuditReport CachedReport;
    if (GetCachedAuditReport(InMode, CachedReport))
    {
        ActiveFileAuditMode = InMode;
        LastAuditReport = CachedReport;
        InvalidateFileManagementSummaryCache();
        return;
    }

    ActiveFileAuditMode = InMode;
    InvalidateFileManagementSummaryCache();
}

bool UInspectorWorldSubsystem::TryGetCachedFileManagementSummary(FRIFileManagementSummary& OutSummary, FString& OutError) const
{
    OutSummary = FRIFileManagementSummary();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    if (!bHasCachedFileManagementSummary)
    {
        OutError = TEXT("File summary cache unavailable");
        return false;
    }

    OutSummary = CachedFileManagementSummary;
    OutError = CachedFileManagementSummaryError;
    return CachedFileManagementSummaryError.IsEmpty();
#endif
}

bool UInspectorWorldSubsystem::RebuildFileManagementSummaryCache(FRIFileManagementSummary& OutSummary, FString& OutError) const
{
    OutSummary = FRIFileManagementSummary();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    TArray<UObject*> SnapshotItems;
    GetSnapshotList(SnapshotItems);
    OutSummary.SnapshotCount = SnapshotItems.Num();
    if (SnapshotItems.Num() > 0)
    {
        if (const UInspectorSnapshotItem* SnapshotItem = Cast<UInspectorSnapshotItem>(SnapshotItems[0]))
        {
            OutSummary.LatestSnapshotLabel = SnapshotItem->FileName;
        }
    }

    TArray<FString> PatchBundleFiles;
    ListPatchBundleFiles(PatchBundleFiles);
    OutSummary.PatchBundleCount = PatchBundleFiles.Num();
    if (PatchBundleFiles.Num() > 0)
    {
        OutSummary.LatestPatchBundleLabel = FPaths::GetCleanFilename(PatchBundleFiles[0]);
    }

    TArray<FRIPatchPresetMetadata> Presets;
    FString PresetError;
    if (!ListPatchPresets(Presets, PresetError))
    {
        OutError = PresetError;
        CachedFileManagementSummaryError = OutError;
        return false;
    }
    OutSummary.PresetCount = Presets.Num();
    if (Presets.Num() > 0)
    {
        const FRIPatchPresetMetadata& LatestPreset = Presets[0];
        const FString ScopeText = StaticEnum<ERIPatchPresetApplicabilityScope>()
            ? StaticEnum<ERIPatchPresetApplicabilityScope>()->GetAuthoredNameStringByValue((int64)LatestPreset.ApplicabilityScope)
            : TEXT("Unknown");
        OutSummary.LatestPresetLabel = FString::Printf(TEXT("%s (%s)"), *LatestPreset.DisplayName, *ScopeText);
    }

    TArray<FString> AuditFiles;
    ListAuditReportFiles(AuditFiles);
    OutSummary.AuditReportCount = AuditFiles.Num();
    if (AuditFiles.Num() > 0)
    {
        OutSummary.LatestAuditReportLabel = FPaths::GetCleanFilename(AuditFiles[0]);
    }

    OutSummary.bHasStagedPatch = HasStagedPatch();
    OutSummary.StagedPatchOperationCount = OutSummary.bHasStagedPatch ? StagedPatchBundle.Operations.Num() : 0;
    OutSummary.StagedPatchLabel = OutSummary.bHasStagedPatch
        ? (!StagedPatchBundle.DisplayName.IsEmpty() ? StagedPatchBundle.DisplayName : StagedPatchBundle.BundleId)
        : TEXT("No staged patch");
    OutSummary.LastPatchApplySummary = LastPatchApplyResult.Summary.IsEmpty()
        ? TEXT("No patch apply result")
        : LastPatchApplyResult.Summary;
    OutSummary.LastAuditSummary = LastAuditReport.Summary.IsEmpty()
        ? TEXT("No audit report")
        : LastAuditReport.Summary;
    {
        const TArray<ERIAuditComparisonMode> CachedModes = GetAvailableCachedAuditModes();
        OutSummary.CachedAuditViewCount = CachedModes.Num();
        TArray<FString> Labels;
        for (ERIAuditComparisonMode Mode : CachedModes)
        {
            Labels.Add(RI_AuditModeLabel(Mode));
        }
        OutSummary.CachedAuditViewsSummary = Labels.Num() > 0
            ? FString::Join(Labels, TEXT(" | "))
            : TEXT("No cached compare views");
        OutSummary.ActiveAuditViewLabel = RI_AuditModeLabel(ActiveFileAuditMode);
    }
    OutSummary.bHasLastPromoteResult = !LastPromoteResult.Summary.IsEmpty()
        || LastPromoteResult.PromotedCount > 0
        || LastPromoteResult.FailedCount > 0
        || LastPromoteResult.SkippedCount > 0;
    OutSummary.LastPromotePromotedCount = LastPromoteResult.PromotedCount;
    OutSummary.LastPromoteFailedCount = LastPromoteResult.FailedCount;
    OutSummary.LastPromoteSkippedCount = LastPromoteResult.SkippedCount;
    OutSummary.LastPromoteSummary = OutSummary.bHasLastPromoteResult
        ? LastPromoteResult.Summary
        : TEXT("No promote result");
    OutSummary.bHasRoleCompareReport = !LastRuntimeRoleCompareReport.Summary.IsEmpty() || LastRuntimeRoleCompareReport.Lines.Num() > 0;
    OutSummary.LastRoleCompareSummary = OutSummary.bHasRoleCompareReport
        ? LastRuntimeRoleCompareReport.Summary
        : TEXT("No runtime role compare");
    OutSummary.LastRoleCompareAvailableRole = OutSummary.bHasRoleCompareReport
        ? (LastRuntimeRoleCompareReport.AvailableRoleLabel.IsEmpty() ? TEXT("None") : LastRuntimeRoleCompareReport.AvailableRoleLabel)
        : TEXT("None");
    OutSummary.LastRoleCompareLineCount = LastRuntimeRoleCompareReport.ComparedLineCount;
    OutSummary.LastRoleCompareMismatchCount = LastRuntimeRoleCompareReport.MismatchCount;
    OutSummary.LastRoleCompareMissingRoleCount = LastRuntimeRoleCompareReport.MissingRoleCount;
    OutSummary.LastRoleCompareVerificationMismatchCount = LastRuntimeRoleCompareReport.VerificationMismatchCount;
    OutSummary.bHasRemoteSessionCompareReport = !LastRuntimeSessionTargetSetCompareReport.Summary.IsEmpty()
        || LastRuntimeSessionTargetSetCompareReport.Lines.Num() > 0;
    OutSummary.LastRemoteSessionCompareSummary = OutSummary.bHasRemoteSessionCompareReport
        ? LastRuntimeSessionTargetSetCompareReport.Summary
        : TEXT("No remote session compare");
    OutSummary.LastRemoteSessionCompareSessionPair = OutSummary.bHasRemoteSessionCompareReport
        ? FString::Printf(
            TEXT("%s -> %s"),
            *LastRuntimeSessionTargetSetCompareReport.LeftSessionId,
            *LastRuntimeSessionTargetSetCompareReport.RightSessionId)
        : TEXT("No session pair");
    OutSummary.LastRemoteSessionCompareLineCount = LastRuntimeSessionTargetSetCompareReport.LineCount;
    OutSummary.LastRemoteSessionCompareSharedCount = LastRuntimeSessionTargetSetCompareReport.SharedTargetCount;
    OutSummary.LastRemoteSessionCompareLeftOnlyCount = LastRuntimeSessionTargetSetCompareReport.LeftOnlyCount;
    OutSummary.LastRemoteSessionCompareRightOnlyCount = LastRuntimeSessionTargetSetCompareReport.RightOnlyCount;
    OutSummary.LastRemoteSessionCompareMismatchCount = LastRuntimeSessionTargetSetCompareReport.MismatchCount;
    OutSummary.bHasRemotePatchPullResult = !LastRemotePatchPullSummary.IsEmpty();
    OutSummary.LastRemotePatchPullSummary = OutSummary.bHasRemotePatchPullResult
        ? LastRemotePatchPullSummary
        : TEXT("No remote patch pull");
    OutSummary.LastRemoteSessionSelectionSummary = LastRemoteSessionSelectionSummary.IsEmpty()
        ? TEXT("No remote session selected")
        : LastRemoteSessionSelectionSummary;
    OutSummary.LastRemoteSessionTargetQuery = LastRemoteSessionTargetQuery.IsEmpty()
        ? TEXT("No target query")
        : LastRemoteSessionTargetQuery;
    OutSummary.LastRemoteSessionWorkflowId = LastRemoteSessionWorkflowId.IsEmpty()
        ? TEXT("No remote workflow")
        : LastRemoteSessionWorkflowId;

    if (OutSummary.bHasStagedPatch)
    {
        FRIPromotePreview Preview;
        FString PromoteError;
        if (CreatePromotePreview(StagedPatchBundle, Preview, PromoteError))
        {
            OutSummary.bHasPromotePreview = true;
            OutSummary.PromoteSupportedCount = Preview.SupportedOperationCount;
            OutSummary.PromoteUnsupportedCount = Preview.UnsupportedOperationCount;
            OutSummary.PromotePreviewSummary = Preview.Summary.IsEmpty() ? TEXT("Promote preview ready") : Preview.Summary;
        }
        else
        {
            OutSummary.PromotePreviewSummary = PromoteError.IsEmpty() ? TEXT("Promote preview unavailable") : PromoteError;
        }
    }
    else
    {
        OutSummary.PromotePreviewSummary = TEXT("No staged patch");
    }

    CachedFileManagementSummary = OutSummary;
    CachedFileManagementSummaryError = OutError;
    bHasCachedFileManagementSummary = true;
    return true;
#endif
}

bool UInspectorWorldSubsystem::GetFileManagementSummary(FRIFileManagementSummary& OutSummary, FString& OutError) const
{
    if (TryGetCachedFileManagementSummary(OutSummary, OutError))
    {
        return true;
    }

    return RebuildFileManagementSummaryCache(OutSummary, OutError);
}

void UInspectorWorldSubsystem::InvalidateFileManagementSummaryCache()
{
    bHasCachedFileManagementSummary = false;
    CachedFileManagementSummary = FRIFileManagementSummary();
    CachedFileManagementSummaryError.Reset();
}

bool UInspectorWorldSubsystem::TryReadRuntimePatchOperationValue(const FRIPatchOperation& Operation, FString& OutValue, FString& OutError) const
{
    OutValue.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    AActor* TargetActor = ResolveRuntimeActorTarget(Operation.Target.ActorPath, Operation.Target.ActorClass, Operation.Target.ActorBaseName);
    if (!TargetActor)
    {
        OutError = TEXT("Actor not found");
        return false;
    }

    UObject* TargetObject = TargetActor;
    if (Operation.Field.FieldKind == ERIPatchFieldKind::Property && Operation.Target.TargetKind == ERIPatchTargetKind::Component)
    {
        UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
            TargetActor,
            Operation.Target.ActorPath,
            Operation.Target.ComponentPath,
            Operation.Target.ComponentName,
            Operation.Target.ComponentClass);
        if (!TargetComponent)
        {
            OutError = TEXT("Component not found");
            return false;
        }
        TargetObject = TargetComponent;
    }

    if (Operation.Field.FieldKind == ERIPatchFieldKind::Property)
    {
        if (!InspectorPropertyUtils::GetValueAsText(TargetObject, FName(*Operation.Field.FieldPath), OutValue))
        {
            OutError = TEXT("Failed to read property value");
            return false;
        }
        return true;
    }

    UActorComponent* TargetComponent = ResolveRuntimeComponentTarget(
        TargetActor,
        Operation.Target.ActorPath,
        Operation.Target.ComponentPath,
        Operation.Target.ComponentName,
        Operation.Target.ComponentClass);
    UMeshComponent* MeshComp = Cast<UMeshComponent>(TargetComponent);
    if (!MeshComp)
    {
        OutError = TEXT("Target component is not a mesh component");
        return false;
    }

    const EInspectorMatParamType ParamType = (Operation.Field.FieldKind == ERIPatchFieldKind::MaterialScalar)
        ? EInspectorMatParamType::Scalar
        : EInspectorMatParamType::Vector;

    UInspectorMaterialParamItem* Item = NewObject<UInspectorMaterialParamItem>(const_cast<UInspectorWorldSubsystem*>(this));
    Item->Init(MeshComp, Operation.Target.MaterialSlotIndex, FName(*Operation.Field.FieldPath), ParamType);
    OutValue = Item->GetValueText();
    return !OutValue.IsEmpty();
#endif
}

void UInspectorWorldSubsystem::FinalizeAuditReport(FRIAuditReport& OutReport) const
{
    OutReport.GeneratedAtUtc = FDateTime::UtcNow().ToIso8601();

    OutReport.Lines.Sort([](const FRIAuditLine& A, const FRIAuditLine& B)
    {
        if (A.GroupLabel != B.GroupLabel)
        {
            return A.GroupLabel < B.GroupLabel;
        }
        if (A.Field.FieldPath != B.Field.FieldPath)
        {
            return A.Field.FieldPath < B.Field.FieldPath;
        }
        return A.Message < B.Message;
    });

    int32 DifferentCount = 0;
    TArray<FString> DetailLines;
    for (const FRIAuditLine& Line : OutReport.Lines)
    {
        if (Line.bDifferent)
        {
            ++DifferentCount;
        }

        const FString MessageSuffix = Line.Message.IsEmpty()
            ? FString()
            : FString::Printf(TEXT(" | %s"), *Line.Message);
        DetailLines.Add(FString::Printf(
            TEXT("[%s] %s :: %s | %s=%s | %s=%s%s"),
            Line.bDifferent ? TEXT("DIFF") : TEXT("SAME"),
            *Line.GroupLabel,
            *Line.Field.FieldPath,
            *Line.LeftTag,
            *Line.LeftValue,
            *Line.RightTag,
            *Line.RightValue,
            *MessageSuffix));
    }

    OutReport.Summary = FString::Printf(
        TEXT("Audit %s Lines=%d Different=%d Same=%d"),
        *RI_AuditModeToString(OutReport.Mode),
        OutReport.Lines.Num(),
        DifferentCount,
        OutReport.Lines.Num() - DifferentCount);
    OutReport.Details = FString::Join(DetailLines, TEXT("\n"));
}

bool UInspectorWorldSubsystem::BuildAuditReport(ERIAuditComparisonMode InMode, const FRIPatchBundle& InBundle, FRIAuditReport& OutReport, FString& OutError)
{
    OutError.Reset();
    OutReport = FRIAuditReport();
    OutReport.Mode = InMode;

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    auto EffectiveBundle = [&]() -> FRIPatchBundle
    {
        if (InBundle.Operations.Num() > 0)
        {
            return InBundle;
        }
        return HasStagedPatch() ? StagedPatchBundle : FRIPatchBundle();
    }();

    if (InMode == ERIAuditComparisonMode::BaselineVsCurrent)
    {
        TArray<FString> Keys;
        ModifiedValueByKey.GetKeys(Keys);
        Keys.Sort();

        for (const FString& Key : Keys)
        {
            const FString* PatchedValuePtr = ModifiedValueByKey.Find(Key);
            const FString* BaselineValuePtr = BaselineValueByKey.Find(Key);
            if (!PatchedValuePtr || !BaselineValuePtr)
            {
                continue;
            }

            FRIPatchOperation Operation;
            if (!TryBuildPatchOperationFromModifiedKey(Key, *PatchedValuePtr, BaselineValuePtr, Operation))
            {
                continue;
            }

            FString RuntimeValue;
            FString RuntimeError;
            const bool bReadOk = TryReadRuntimePatchOperationValue(Operation, RuntimeValue, RuntimeError);

            FRIAuditLine Line;
            Line.Target = Operation.Target;
            Line.Field = Operation.Field;
            Line.GroupLabel = RI_AuditTargetLabel(Operation.Target);
            Line.LeftTag = TEXT("Baseline");
            Line.LeftValue = Operation.BaselineValue;
            Line.RightTag = TEXT("Current");
            Line.RightValue = bReadOk ? RuntimeValue : Operation.PatchedValue;
            Line.bDifferent = !RI_AreAuditValuesEquivalent(Line.LeftValue, Line.RightValue);
            Line.Message = bReadOk ? TEXT("Baseline vs current runtime") : RuntimeError;
            OutReport.Lines.Add(MoveTemp(Line));
        }
    }
    else if (InMode == ERIAuditComparisonMode::CurrentVsPatch)
    {
        if (EffectiveBundle.Operations.Num() <= 0)
        {
            OutError = TEXT("No patch operations");
            return false;
        }

        for (const FRIPatchOperation& Operation : EffectiveBundle.Operations)
        {
            FString RuntimeValue;
            FString RuntimeError;
            const bool bReadOk = TryReadRuntimePatchOperationValue(Operation, RuntimeValue, RuntimeError);

            FRIAuditLine Line;
            Line.Target = Operation.Target;
            Line.Field = Operation.Field;
            Line.GroupLabel = RI_AuditTargetLabel(Operation.Target);
            Line.LeftTag = TEXT("Current");
            Line.LeftValue = bReadOk ? RuntimeValue : TEXT("<unavailable>");
            Line.RightTag = Operation.SourceTag.StartsWith(TEXT("Preset"), ESearchCase::IgnoreCase) ? TEXT("Preset") : TEXT("Patch");
            Line.RightValue = Operation.PatchedValue;
            Line.bDifferent = !bReadOk || !RI_AreAuditValuesEquivalent(Line.LeftValue, Line.RightValue);
            Line.Message = bReadOk ? TEXT("Current runtime vs patch target") : RuntimeError;
            OutReport.Lines.Add(MoveTemp(Line));
        }
    }
    else
    {
        if (EffectiveBundle.Operations.Num() <= 0)
        {
            OutError = TEXT("No patch operations");
            return false;
        }

        FRIPromotePreview Preview;
        if (!CreatePromotePreview(EffectiveBundle, Preview, OutError))
        {
            return false;
        }

        for (const FRIPromoteOperationPreview& OperationPreview : Preview.Operations)
        {
            FRIAuditLine Line;
            Line.Target = OperationPreview.Operation.Target;
            Line.Field = OperationPreview.Operation.Field;
            Line.GroupLabel = RI_AuditTargetLabel(OperationPreview.Operation.Target);
            Line.LeftTag = TEXT("Patch");
            Line.LeftValue = OperationPreview.Operation.PatchedValue;
            Line.RightTag = (InMode == ERIAuditComparisonMode::PatchVsSource) ? TEXT("SourcePreview") : TEXT("SourceApplied");
            Line.RightValue = OperationPreview.CurrentSourceValue;
            Line.bDifferent = !OperationPreview.bSupported || !RI_AreAuditValuesEquivalent(Line.LeftValue, Line.RightValue);
            Line.Message = OperationPreview.Message;
            OutReport.Lines.Add(MoveTemp(Line));
        }
    }

    FinalizeAuditReport(OutReport);
    LastAuditReport = OutReport;
    if (FRIAuditReport* CachedReport = GetMutableCachedAuditReport(InMode))
    {
        *CachedReport = OutReport;
    }
    ActiveFileAuditMode = InMode;
    InvalidateFileManagementSummaryCache();
    return true;
#endif
}

FString UInspectorWorldSubsystem::GetLastAuditReportAsText() const
{
    if (LastAuditReport.Summary.IsEmpty() && LastAuditReport.Details.IsEmpty())
    {
        return TEXT("No audit report available.");
    }

    FString Output = LastAuditReport.Summary;
    if (!LastAuditReport.Details.IsEmpty())
    {
        Output += TEXT("\n");
        Output += LastAuditReport.Details;
    }
    return Output;
}

bool UInspectorWorldSubsystem::ExportLastAuditReportToFile(bool bAsJson, FString& OutFilePath, FString& OutError) const
{
    OutFilePath.Reset();
    OutError.Reset();

    if (LastAuditReport.Lines.Num() <= 0 && LastAuditReport.Summary.IsEmpty())
    {
        OutError = TEXT("No audit report available");
        return false;
    }

    const FString Dir = GetAuditReportsDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
    const FString Ext = bAsJson ? TEXT(".json") : TEXT(".txt");
    OutFilePath = FPaths::Combine(Dir, FString::Printf(TEXT("audit-%s%s"), *Stamp, *Ext));

    FString Payload;
    if (bAsJson)
    {
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("mode"), RI_AuditModeToString(LastAuditReport.Mode));
        Root->SetStringField(TEXT("generatedAtUtc"), LastAuditReport.GeneratedAtUtc);
        Root->SetStringField(TEXT("summary"), LastAuditReport.Summary);
        Root->SetStringField(TEXT("details"), LastAuditReport.Details);

        TArray<TSharedPtr<FJsonValue>> Lines;
        for (const FRIAuditLine& Line : LastAuditReport.Lines)
        {
            TSharedRef<FJsonObject> JsonLine = MakeShared<FJsonObject>();
            JsonLine->SetStringField(TEXT("groupLabel"), Line.GroupLabel);
            JsonLine->SetStringField(TEXT("fieldPath"), Line.Field.FieldPath);
            JsonLine->SetStringField(TEXT("leftTag"), Line.LeftTag);
            JsonLine->SetStringField(TEXT("leftValue"), Line.LeftValue);
            JsonLine->SetStringField(TEXT("rightTag"), Line.RightTag);
            JsonLine->SetStringField(TEXT("rightValue"), Line.RightValue);
            JsonLine->SetBoolField(TEXT("different"), Line.bDifferent);
            JsonLine->SetStringField(TEXT("message"), Line.Message);
            Lines.Add(MakeShared<FJsonValueObject>(JsonLine));
        }
        Root->SetArrayField(TEXT("lines"), Lines);

        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
        if (!FJsonSerializer::Serialize(Root, Writer))
        {
            OutError = TEXT("Failed to serialize audit report JSON");
            return false;
        }
    }
    else
    {
        Payload = GetLastAuditReportAsText();
    }

    if (!FFileHelper::SaveStringToFile(Payload, *OutFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutError = FString::Printf(TEXT("Failed to save audit report: %s"), *OutFilePath);
        return false;
    }

    return true;
}
