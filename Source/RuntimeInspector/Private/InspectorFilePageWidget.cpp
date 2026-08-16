#include "InspectorFilePageWidget.h"

#include "InspectorCompactWidgetUtils.h"
#include "InspectorTouchScrollBox.h"
#include "InspectorWorldSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"
#include "InspectorSnapshotItem.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace
{
    static FLinearColor RI_FileSectionColor() { return RICompactUI::GetSectionSurfaceBackgroundColor(); }
    static FLinearColor RI_FileRowColor() { return RICompactUI::GetRowSurfaceBackgroundColor(); }
    static FLinearColor RI_FileCellColor() { return RICompactUI::GetCellSurfaceBackgroundColor(); }
    static FLinearColor RI_FileTextColor() { return RICompactUI::GetStrongTextColor(); }
    static FLinearColor RI_FileMutedColor() { return RICompactUI::GetMutedTextColor(); }
    static FLinearColor RI_FileErrorColor() { return RICompactUI::GetErrorTextColor(); }

    static UTextBlock* RI_MakeText(UWidgetTree* WidgetTree, const FString& InText, int32 Size, bool bBold, const FLinearColor& Color, bool bWrap = false)
    {
        return RICompactUI::MakeText(WidgetTree, InText, Size, bBold, Color, bWrap);
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

    static void RI_GetAuditPairLabels(const FRIAuditReport& Report, FString& OutLeft, FString& OutRight)
    {
        OutLeft.Reset();
        OutRight.Reset();

        if (Report.Lines.Num() > 0)
        {
            const FRIAuditLine& FirstLine = Report.Lines[0];
            if (!FirstLine.LeftTag.IsEmpty() || !FirstLine.RightTag.IsEmpty())
            {
                OutLeft = FirstLine.LeftTag;
                OutRight = FirstLine.RightTag;
                return;
            }
        }

        switch (Report.Mode)
        {
        case ERIAuditComparisonMode::BaselineVsCurrent:
            OutLeft = TEXT("Baseline");
            OutRight = TEXT("Current");
            break;
        case ERIAuditComparisonMode::CurrentVsPatch:
            OutLeft = TEXT("Current");
            OutRight = TEXT("Patch");
            break;
        case ERIAuditComparisonMode::PatchVsSource:
            OutLeft = TEXT("Patch");
            OutRight = TEXT("SourcePreview");
            break;
        case ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote:
            OutLeft = TEXT("Patch");
            OutRight = TEXT("SourceApplied");
            break;
        default:
            break;
        }
    }

    static FString RI_BuildAuditPairLabel(const FRIAuditReport& Report)
    {
        FString LeftLabel;
        FString RightLabel;
        RI_GetAuditPairLabels(Report, LeftLabel, RightLabel);

        if (LeftLabel.IsEmpty() && RightLabel.IsEmpty())
        {
            return TEXT("No compare pair");
        }

        return FString::Printf(TEXT("%s -> %s"), *LeftLabel, *RightLabel);
    }

    static FString RI_FindFirstDifferentField(const FRIAuditReport& Report)
    {
        for (const FRIAuditLine& Line : Report.Lines)
        {
            if (Line.bDifferent)
            {
                return Line.Field.FieldPath;
            }
        }

        if (Report.Lines.Num() > 0)
        {
            return Report.Lines[0].Field.FieldPath;
        }

        return TEXT("No compare fields");
    }

    static FString RI_BuildAuditPreviewText(const FRIAuditReport& Report)
    {
        if (Report.Lines.Num() <= 0)
        {
            return TEXT("No audit lines");
        }

        TArray<FString> Lines;
        int32 Added = 0;
        for (const FRIAuditLine& Line : Report.Lines)
        {
            if (!Line.bDifferent)
            {
                continue;
            }

            Lines.Add(FString::Printf(
                TEXT("%s :: %s | %s=%s | %s=%s"),
                *Line.GroupLabel,
                *Line.Field.FieldPath,
                *Line.LeftTag,
                *Line.LeftValue,
                *Line.RightTag,
                *Line.RightValue));

            ++Added;
            if (Added >= 4)
            {
                break;
            }
        }

        if (Lines.Num() <= 0)
        {
            for (int32 Index = 0; Index < FMath::Min(Report.Lines.Num(), 2); ++Index)
            {
                const FRIAuditLine& Line = Report.Lines[Index];
                Lines.Add(FString::Printf(
                    TEXT("%s :: %s | %s=%s | %s=%s"),
                    *Line.GroupLabel,
                    *Line.Field.FieldPath,
                    *Line.LeftTag,
                    *Line.LeftValue,
                    *Line.RightTag,
                    *Line.RightValue));
            }
        }

        return FString::Join(Lines, TEXT("\n"));
    }

    static FString RI_TruncateAuditValue(const FString& InValue, int32 MaxLen = 64)
    {
        if (InValue.Len() <= MaxLen)
        {
            return InValue;
        }

        return InValue.Left(MaxLen - 3) + TEXT("...");
    }

    static FString RI_BuildSelectedActorClassLabel(const AActor* Actor)
    {
        return (Actor && Actor->GetClass()) ? Actor->GetClass()->GetName() : TEXT("No actor class");
    }

    static FString RI_BuildSelectedActorSourceLabel(const AActor* Actor)
    {
        if (!Actor || !Actor->GetClass())
        {
            return TEXT("No source asset");
        }

        const FString ClassPath = Actor->GetClass()->GetPathName();
        return ClassPath.IsEmpty() ? TEXT("No source asset") : ClassPath;
    }

    static FString RI_BuildRoleCompareStatsText(const FRIRuntimeRoleCompareReport& Report)
    {
        return FString::Printf(
            TEXT("Lines=%d | Mismatch=%d | MissingRoles=%d | Verify=%d"),
            Report.ComparedLineCount,
            Report.MismatchCount,
            Report.MissingRoleCount,
            Report.VerificationMismatchCount);
    }

    static FString RI_BuildRoleComparePreviewText(const FRIRuntimeRoleCompareReport& Report)
    {
        if (Report.Lines.Num() <= 0)
        {
            return TEXT("No role compare lines");
        }

        TArray<FString> Lines;
        const int32 MaxLines = FMath::Min(Report.Lines.Num(), 3);
        for (int32 Index = 0; Index < MaxLines; ++Index)
        {
            const FRIRuntimeRoleCompareLine& Line = Report.Lines[Index];
            Lines.Add(Line.Summary.IsEmpty()
                ? FString::Printf(TEXT("%s :: no summary"), *Line.Field.FieldPath)
                : Line.Summary);
        }
        return FString::Join(Lines, TEXT("\n"));
    }

    static FString RI_BuildRemoteSessionCompareStatsText(const FRIRuntimeSessionTargetSetCompareReport& Report)
    {
        FString Stats = FString::Printf(
            TEXT("Lines=%d | Shared=%d | LeftOnly=%d | RightOnly=%d | Mismatch=%d"),
            Report.LineCount,
            Report.SharedTargetCount,
            Report.LeftOnlyCount,
            Report.RightOnlyCount,
            Report.MismatchCount);

        if (!Report.NameFilter.IsEmpty())
        {
            Stats += FString::Printf(TEXT(" | Name=%s"), *Report.NameFilter);
        }

        if (!Report.ClassFilter.IsEmpty())
        {
            Stats += FString::Printf(TEXT(" | Class=%s"), *Report.ClassFilter);
        }

        return Stats;
    }

    static FString RI_BuildRemoteSessionComparePreviewText(const FRIRuntimeSessionTargetSetCompareReport& Report)
    {
        if (Report.Lines.Num() <= 0)
        {
            return TEXT("No remote session compare lines");
        }

        TArray<FString> Lines;
        int32 Added = 0;
        for (const FRIRuntimeSessionTargetSetCompareLine& Line : Report.Lines)
        {
            if (!Line.bHasMismatch)
            {
                continue;
            }

            Lines.Add(FString::Printf(
                TEXT("%s :: L=%d R=%d | %s"),
                *Line.DisplayLabel,
                Line.LeftCount,
                Line.RightCount,
                *Line.Message));
            ++Added;
            if (Added >= 3)
            {
                break;
            }
        }

        if (Lines.Num() <= 0)
        {
            const int32 MaxLines = FMath::Min(Report.Lines.Num(), 2);
            for (int32 Index = 0; Index < MaxLines; ++Index)
            {
                const FRIRuntimeSessionTargetSetCompareLine& Line = Report.Lines[Index];
                Lines.Add(FString::Printf(
                    TEXT("%s :: L=%d R=%d | %s"),
                    *Line.DisplayLabel,
                    Line.LeftCount,
                    Line.RightCount,
                    *Line.Message));
            }
        }

        return FString::Join(Lines, TEXT("\n"));
    }

    static FString RI_BuildRemoteSessionOriginLabel(ERIRuntimeSessionOrigin Origin)
    {
        if (const UEnum* Enum = StaticEnum<ERIRuntimeSessionOrigin>())
        {
            return Enum->GetAuthoredNameStringByValue((int64)Origin);
        }

        return TEXT("Unknown");
    }

    static FString RI_BuildRemoteSessionConnectionLabel(ERIRuntimeSessionConnectionState State)
    {
        if (const UEnum* Enum = StaticEnum<ERIRuntimeSessionConnectionState>())
        {
            return Enum->GetAuthoredNameStringByValue((int64)State);
        }

        return TEXT("Unknown");
    }

    static FString RI_BuildRemoteSessionOptionLabel(const FRIRuntimeSessionInfo& Session)
    {
        FString Label = Session.DisplayName.IsEmpty() ? Session.SessionId : Session.DisplayName;
        if (Label.IsEmpty())
        {
            Label = TEXT("Unnamed Session");
        }

        if (!Session.SessionId.IsEmpty() && Session.SessionId != Label)
        {
            Label += FString::Printf(TEXT(" [%s]"), *Session.SessionId);
        }

        if (!Session.Host.IsEmpty() && Session.Port > 0)
        {
            Label += FString::Printf(TEXT(" @ %s:%d"), *Session.Host, Session.Port);
        }

        if (Session.bIsExternal)
        {
            Label += TEXT(" (External)");
        }

        return Label;
    }

    static FString RI_BuildRemoteSessionSummaryText(const FRIRuntimeSessionInfo& Session)
    {
        FString Summary = FString::Printf(
            TEXT("%s | Origin=%s | State=%s"),
            *RI_BuildRemoteSessionOptionLabel(Session),
            *RI_BuildRemoteSessionOriginLabel(Session.SessionOrigin),
            *RI_BuildRemoteSessionConnectionLabel(Session.ConnectionState));

        if (!Session.BuildConfiguration.IsEmpty())
        {
            Summary += FString::Printf(TEXT(" | Build=%s"), *Session.BuildConfiguration);
        }

        if (!Session.WorldTypeLabel.IsEmpty())
        {
            Summary += FString::Printf(TEXT(" | World=%s"), *Session.WorldTypeLabel);
        }

        if (!Session.NetModeLabel.IsEmpty())
        {
            Summary += FString::Printf(TEXT(" | Net=%s"), *Session.NetModeLabel);
        }

        if (Session.CapabilityTags.Num() > 0)
        {
            Summary += FString::Printf(TEXT(" | Caps=%s"), *FString::Join(Session.CapabilityTags, TEXT(", ")));
        }

        if (!Session.LastError.IsEmpty())
        {
            Summary += FString::Printf(TEXT(" | LastError=%s"), *Session.LastError);
        }

        return Summary;
    }

    static FString RI_BuildRemoteSessionDiscoverySummary(const TArray<FRIRuntimeSessionInfo>& Sessions)
    {
        int32 ExternalCount = 0;
        int32 ConnectedCount = 0;
        for (const FRIRuntimeSessionInfo& Session : Sessions)
        {
            if (Session.bIsExternal)
            {
                ++ExternalCount;
            }
            if (Session.ConnectionState == ERIRuntimeSessionConnectionState::Connected)
            {
                ++ConnectedCount;
            }
        }

        return FString::Printf(
            TEXT("Available=%d | External=%d | Connected=%d"),
            Sessions.Num(),
            ExternalCount,
            ConnectedCount);
    }

    static const FRIRuntimeSessionInfo* RI_FindRemoteSessionById(const TArray<FRIRuntimeSessionInfo>& Sessions, const FString& SessionId)
    {
        if (SessionId.IsEmpty())
        {
            return nullptr;
        }

        return Sessions.FindByPredicate([&SessionId](const FRIRuntimeSessionInfo& Session)
        {
            return Session.SessionId == SessionId;
        });
    }

    static const FRIRuntimeSessionInfo* RI_FindRemoteSessionByLabel(const TArray<FRIRuntimeSessionInfo>& Sessions, const FString& OptionLabel)
    {
        if (OptionLabel.IsEmpty())
        {
            return nullptr;
        }

        return Sessions.FindByPredicate([&OptionLabel](const FRIRuntimeSessionInfo& Session)
        {
            return RI_BuildRemoteSessionOptionLabel(Session) == OptionLabel;
        });
    }

    static FString RI_BuildTargetQuerySummaryText(const FString& QueryText, const TArray<FRIRuntimeTargetInfo>& Targets)
    {
        const FString FirstTarget = Targets.Num() > 0 ? Targets[0].Summary : TEXT("No targets");
        return FString::Printf(TEXT("Query=%s | Targets=%d | %s"), *QueryText, Targets.Num(), *FirstTarget);
    }

    static void RI_ConfigureNamedScrollBoxTouchSupport(UWidgetTree* WidgetTree, const FName& ScrollBoxName)
    {
        if (!WidgetTree)
        {
            return;
        }

        RIInspectorTouchScroll::Configure(Cast<UScrollBox>(WidgetTree->FindWidget(ScrollBoxName)));
    }
}

UInspectorFilePageWidget::UInspectorFilePageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(false);
}

void UInspectorFilePageWidget::SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem)
{
    CancelDeferredRefresh();
    Subsystem = InSubsystem;
}

bool UInspectorFilePageWidget::HasTouchScrollSupportForAutomation() const
{
    if (!WidgetTree)
    {
        return false;
    }

    const TArray<FName> ScrollNames = {
        TEXT("RI_FilePageScroll"),
        TEXT("RI_FileAuditLinesScroll"),
        TEXT("RI_FileRoleCompareLinesScroll"),
        TEXT("RI_FileRemoteSessionCompareLinesScroll")
    };

    bool bFoundAny = false;
    for (const FName& ScrollName : ScrollNames)
    {
        if (UScrollBox* Scroll = Cast<UScrollBox>(WidgetTree->FindWidget(ScrollName)))
        {
            bFoundAny = true;
            if (!RIInspectorTouchScroll::HasTouchSupport(Scroll))
            {
                return false;
            }
        }
    }

    return bFoundAny;
}

FString UInspectorFilePageWidget::GetCompareDebugSummary() const
{
    return FString::Printf(
        TEXT("Mode=%s Pair=%s Rendered=%d Diff=%d FirstField=%s Preview=%s"),
        AuditModeText ? *AuditModeText->GetText().ToString() : TEXT("None"),
        AuditPairText ? *AuditPairText->GetText().ToString() : TEXT("None"),
        RenderedAuditLineCount,
        RenderedAuditDifferentCount,
        RenderedFirstAuditField.IsEmpty() ? TEXT("-") : *RenderedFirstAuditField,
        AuditPreviewText ? *AuditPreviewText->GetText().ToString() : TEXT("None"));
}

TSharedRef<SWidget> UInspectorFilePageWidget::RebuildWidget()
{
    const bool bHasUsableBlueprintLayout = PageScrollBox
        || (WidgetTree && WidgetTree->FindWidget(TEXT("RI_FilePageScroll")) != nullptr);

    if (WidgetTree && (!WidgetTree->RootWidget || !bHasUsableBlueprintLayout))
    {
        BuildWidgetTree();
    }

    return Super::RebuildWidget();
}

void UInspectorFilePageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree)
    {
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_FilePageScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_FileAuditLinesScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_FileRoleCompareLinesScroll"));
        RI_ConfigureNamedScrollBoxTouchSupport(WidgetTree, TEXT("RI_FileRemoteSessionCompareLinesScroll"));
    }
}

void UInspectorFilePageWidget::NativeDestruct()
{
    CancelDeferredRefresh();
    Super::NativeDestruct();
}

void UInspectorFilePageWidget::RefreshFastFromSubsystem()
{
    const double StartSeconds = FPlatformTime::Seconds();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    const AActor* SelectedActor = InspectorSubsystem->GetSelectedActor();
    SetValueText(SelectedActorSummaryText, SelectedActor ? SelectedActor->GetName() : TEXT("No selected actor"), SelectedActor == nullptr);
    SetValueText(SelectedActorClassText, RI_BuildSelectedActorClassLabel(SelectedActor), SelectedActor == nullptr);
    SetValueText(SelectedActorSourcePathText, RI_BuildSelectedActorSourceLabel(SelectedActor), false);
    const double ActorContextEndSeconds = FPlatformTime::Seconds();

    FRIFileManagementSummary CachedSummary;
    FString CachedSummaryError;
    const bool bHasCachedSummary = InspectorSubsystem->TryGetCachedFileManagementSummary(CachedSummary, CachedSummaryError);
    ApplyFileSummaryToWidgets(bHasCachedSummary ? &CachedSummary : nullptr);
    ApplyPrimaryActionGuidance(SelectedActor, bHasCachedSummary ? &CachedSummary : nullptr);
    ApplyPrimaryActionButtonStates(SelectedActor, bHasCachedSummary ? &CachedSummary : nullptr);
    const double SummaryEndSeconds = FPlatformTime::Seconds();

    FRIAuditReport AuditReport;
    const bool bHasCachedActiveReport = InspectorSubsystem->GetCachedAuditReport(InspectorSubsystem->GetActiveFileAuditViewMode(), AuditReport);
    if (!bHasCachedActiveReport)
    {
        AuditReport = InspectorSubsystem->GetLastAuditReport();
    }
    const bool bHasAuditReport = !AuditReport.Summary.IsEmpty() || AuditReport.Lines.Num() > 0;
    SetValueText(AuditModeText, bHasAuditReport ? RI_AuditModeLabel(AuditReport.Mode) : TEXT("No audit report"));
    SetValueText(AuditPairText, bHasAuditReport ? RI_BuildAuditPairLabel(AuditReport) : TEXT("No compare pair"));
    if (bHasAuditReport)
    {
        int32 DifferentCount = 0;
        for (const FRIAuditLine& Line : AuditReport.Lines)
        {
            if (Line.bDifferent)
            {
                ++DifferentCount;
            }
        }
        SetValueText(
            AuditStatsText,
            FString::Printf(TEXT("Lines=%d | Different=%d | Same=%d"), AuditReport.Lines.Num(), DifferentCount, AuditReport.Lines.Num() - DifferentCount));
    }
    else
    {
        SetValueText(AuditStatsText, TEXT("No audit stats"));
    }
    SetValueText(AuditFirstFieldText, bHasAuditReport ? RI_FindFirstDifferentField(AuditReport) : TEXT("No compare fields"));
    SetValueText(AuditPreviewText, bHasAuditReport ? RI_BuildAuditPreviewText(AuditReport) : TEXT("No audit preview"));
    const bool bAuditsExpanded = IsAuditsSectionExpanded();
    if (bAuditsExpanded)
    {
        RebuildAuditLineCards(AuditReport);
    }
    const double AuditEndSeconds = FPlatformTime::Seconds();

    const FRIRuntimeRoleCompareReport RoleCompareReport = InspectorSubsystem->GetLastRuntimeRoleCompareReport();
    const bool bHasRoleCompare = !RoleCompareReport.Summary.IsEmpty() || RoleCompareReport.Lines.Num() > 0;
    SetValueText(RoleCompareSummaryText, bHasRoleCompare ? RoleCompareReport.Summary : TEXT("No runtime role compare"));
    SetValueText(RoleCompareAvailableRoleText, bHasRoleCompare ? RoleCompareReport.AvailableRoleLabel : TEXT("No available role"));
    SetValueText(RoleCompareStatsText, bHasRoleCompare ? RI_BuildRoleCompareStatsText(RoleCompareReport) : TEXT("No role compare stats"));
    SetValueText(RoleComparePreviewText, bHasRoleCompare ? RI_BuildRoleComparePreviewText(RoleCompareReport) : TEXT("No runtime role compare preview"));

    const FRIRuntimeSessionTargetSetCompareReport RemoteSessionCompareReport = InspectorSubsystem->GetLastRuntimeSessionTargetSetCompareReport();
    const bool bHasRemoteSessionCompare = !RemoteSessionCompareReport.Summary.IsEmpty() || RemoteSessionCompareReport.Lines.Num() > 0;
    SetValueText(RemoteSessionCompareSummaryText, bHasRemoteSessionCompare ? RemoteSessionCompareReport.Summary : TEXT("No remote session compare"));
    SetValueText(
        RemoteSessionCompareSessionsText,
        bHasRemoteSessionCompare
            ? FString::Printf(TEXT("%s -> %s"), *RemoteSessionCompareReport.LeftSessionId, *RemoteSessionCompareReport.RightSessionId)
            : TEXT("No session pair"));
    SetValueText(
        RemoteSessionCompareStatsText,
        bHasRemoteSessionCompare ? RI_BuildRemoteSessionCompareStatsText(RemoteSessionCompareReport) : TEXT("No remote session compare stats"));
    SetValueText(
        RemoteSessionComparePreviewText,
        bHasRemoteSessionCompare ? RI_BuildRemoteSessionComparePreviewText(RemoteSessionCompareReport) : TEXT("No remote session compare preview"));
    const bool bDiagnosticsExpanded = IsDiagnosticsSectionExpanded();
    if (bDiagnosticsExpanded)
    {
        RebuildRoleCompareLineCards(RoleCompareReport);
        RebuildRemoteSessionCompareLineCards(RemoteSessionCompareReport);
    }
    const double DiagnosticsEndSeconds = FPlatformTime::Seconds();

    if (!bHasCachedSummary)
    {
        SetValueText(StatusText, TEXT("Showing cached data | Refreshing..."));
    }
    else
    {
        SetValueText(StatusText, TEXT("Showing cached data"));
    }
    const double StatusEndSeconds = FPlatformTime::Seconds();

    InspectorSubsystem->RefreshSharedContextStrip();
    const double EndSeconds = FPlatformTime::Seconds();
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] File FastRefresh %.2f ms"), (EndSeconds - StartSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefresh"), (EndSeconds - StartSeconds) * 1000.0, TEXT("Changes"));
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] File FastRefresh Detail %.2f ms | Actor=%.2f Summary=%.2f Audit=%.2f Diagnostics=%.2f Status=%.2f Shared=%.2f | Expanded=A%d D%d"),
        (EndSeconds - StartSeconds) * 1000.0,
        (ActorContextEndSeconds - StartSeconds) * 1000.0,
        (SummaryEndSeconds - ActorContextEndSeconds) * 1000.0,
        (AuditEndSeconds - SummaryEndSeconds) * 1000.0,
        (DiagnosticsEndSeconds - AuditEndSeconds) * 1000.0,
        (StatusEndSeconds - DiagnosticsEndSeconds) * 1000.0,
        (EndSeconds - StatusEndSeconds) * 1000.0,
        bAuditsExpanded ? 1 : 0,
        bDiagnosticsExpanded ? 1 : 0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefreshDetail.Actor"), (ActorContextEndSeconds - StartSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefreshDetail.Summary"), (SummaryEndSeconds - ActorContextEndSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefreshDetail.Audit"), (AuditEndSeconds - SummaryEndSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefreshDetail.Diagnostics"), (DiagnosticsEndSeconds - AuditEndSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(TEXT("FileFastRefreshDetail.Status"), (StatusEndSeconds - DiagnosticsEndSeconds) * 1000.0);
    InspectorSubsystem->RecordValidationCaptureMetric(
        TEXT("FileFastRefreshDetail.Shared"),
        (EndSeconds - StatusEndSeconds) * 1000.0,
        FString::Printf(TEXT("Expanded=A%d D%d"), bAuditsExpanded ? 1 : 0, bDiagnosticsExpanded ? 1 : 0));
}

void UInspectorFilePageWidget::ScheduleDeferredRefresh(bool bForceSessionRefresh, bool bForceTargetQueryRefresh)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bDeferredHeavyRefreshScheduled = true;
    bDeferredHeavyRefreshForceSessionRefresh = bDeferredHeavyRefreshForceSessionRefresh || bForceSessionRefresh;
    bDeferredHeavyRefreshForceTargetQueryRefresh = bDeferredHeavyRefreshForceTargetQueryRefresh || bForceTargetQueryRefresh;
    World->GetTimerManager().ClearTimer(DeferredHeavyRefreshTimerHandle);
    World->GetTimerManager().SetTimer(
        DeferredHeavyRefreshTimerHandle,
        this,
        &UInspectorFilePageWidget::HandleDeferredRefreshTimerElapsed,
        0.12f,
        false);
}

void UInspectorFilePageWidget::CancelDeferredRefresh()
{
    bDeferredHeavyRefreshScheduled = false;
    bDeferredHeavyRefreshForceSessionRefresh = false;
    bDeferredHeavyRefreshForceTargetQueryRefresh = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeferredHeavyRefreshTimerHandle);
    }
}

void UInspectorFilePageWidget::HandleDeferredRefreshTimerElapsed()
{
    const bool bForceSessionRefresh = bDeferredHeavyRefreshForceSessionRefresh;
    const bool bForceTargetQueryRefresh = bDeferredHeavyRefreshForceTargetQueryRefresh;
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(bForceSessionRefresh, bForceTargetQueryRefresh);
}

void UInspectorFilePageWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_FileRoot"));
    RootBorder->SetPadding(RICompactUI::GetPanelPadding());
    RootBorder->SetBrushColor(RICompactUI::GetPageBackgroundColor());

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileRootBox"));
    RootBorder->SetContent(RootBox);

    PageScrollBox = WidgetTree->ConstructWidget<UInspectorTouchScrollBox>(UInspectorTouchScrollBox::StaticClass(), TEXT("RI_FilePageScroll"));
    RIInspectorTouchScroll::Configure(PageScrollBox);
    if (UVerticalBoxSlot* VBoxSlot = RootBox->AddChildToVerticalBox(PageScrollBox))
    {
        VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileMainBox"));
    PageScrollBox->AddChild(MainBox);

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSectionTitle(TEXT("Changes Workspace"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetInlineGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(RI_MakeText(
        WidgetTree,
        TEXT("See runtime edits, compare the result, then apply or export when it is ready."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_FileMutedColor(),
        true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateMainActionsSection()))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateStatusSection()))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateSelectionContextSection()))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreateAuditsSection()))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    if (UVerticalBoxSlot* VBoxSlot = MainBox->AddChildToVerticalBox(CreatePresetsSection()))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RICompactUI::GetSectionGap()));
    }

    UBorder* FooterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_FileFooterBorder"));
    FooterBorder->SetPadding(RICompactUI::GetSurfaceCardPadding());
    FooterBorder->SetBrushColor(RICompactUI::GetFooterBackgroundColor());
    if (UVerticalBoxSlot* VBoxSlot = RootBox->AddChildToVerticalBox(FooterBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
    }

    UVerticalBox* FooterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileFooterBox"));
    FooterBorder->SetContent(FooterBox);

    StatusText = RI_MakeText(WidgetTree, TEXT("Changes ready"), RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true);
    if (UVerticalBoxSlot* VBoxSlot = FooterBox->AddChildToVerticalBox(StatusText))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    RefreshButton = RICompactUI::MakeLabeledButton(
        WidgetTree,
        TEXT("BTN_FileRefresh"),
        TEXT("Refresh"),
        RICompactUI::ERIButtonVisualStyle::Secondary,
        0.0f);
    RefreshButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleRefreshClicked);
    if (UVerticalBoxSlot* VBoxSlot = FooterBox->AddChildToVerticalBox(RefreshButton))
    {
        VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    WidgetTree->RootWidget = RootBorder;

    SetSectionExpanded(AuditsSectionBody, AuditsSectionToggleText, bAuditsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(PresetsSectionBody, PresetsSectionToggleText, bPresetsSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

UWidget* UInspectorFilePageWidget::CreateSectionTitle(const FString& InTitle, bool bEmphasis)
{
    return RICompactUI::MakeSectionTitle(
        WidgetTree,
        InTitle,
        bEmphasis ? RICompactUI::ERISectionVisualStyle::Emphasis : RICompactUI::ERISectionVisualStyle::Standard);
}

UWidget* UInspectorFilePageWidget::CreateSelectionContextSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateSectionTitle(TEXT("Changes Context"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Selected Actor"), SelectedActorSummaryText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Actor Class"), SelectedActorClassText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Source Asset"), SelectedActorSourcePathText, true));
    return Outer;
}

UButton* UInspectorFilePageWidget::CreateActionButton(const FString& Label, const FName& Name, float Width)
{
    return RICompactUI::MakeLabeledButton(
        WidgetTree,
        Name,
        Label,
        RICompactUI::ERIButtonVisualStyle::Secondary,
        Width);
}

UButton* UInspectorFilePageWidget::GetNamedButton(const FName& Name) const
{
    if (Name == TEXT("BTN_FileStagePatch"))
    {
        return StagePatchButton;
    }
    if (Name == TEXT("BTN_FilePreviewPromote"))
    {
        return PreviewPromoteButton;
    }
    if (Name == TEXT("BTN_FilePromoteApply"))
    {
        return PromoteApplyButton;
    }
    if (Name == TEXT("BTN_FileClearStaged"))
    {
        return ClearStagedButton;
    }
    if (Name == TEXT("BTN_FileExportPatch"))
    {
        return ExportPatchButton;
    }
    if (Name == TEXT("BTN_FileSavePreset"))
    {
        return SavePresetButton;
    }
    if (Name == TEXT("BTN_FileApplyLatestPreset"))
    {
        return ApplyLatestPresetButton;
    }
    if (Name == TEXT("BTN_FileBuildBaselineAudit"))
    {
        return BuildBaselineAuditButton;
    }
    if (Name == TEXT("BTN_FileBuildAudit"))
    {
        return BuildAuditButton;
    }
    if (Name == TEXT("BTN_FileBuildPatchVsSource"))
    {
        return BuildPatchVsSourceAuditButton;
    }
    if (Name == TEXT("BTN_FileBuildAppliedAudit"))
    {
        return BuildAppliedAuditButton;
    }
    if (Name == TEXT("BTN_FileBuildRoleCompare"))
    {
        return BuildRuntimeRoleCompareButton;
    }
    if (Name == TEXT("BTN_FileBuildRemoteSessionCompare"))
    {
        return BuildRemoteSessionCompareButton;
    }
    if (Name == TEXT("BTN_FileRefresh"))
    {
        return RefreshButton;
    }
    return nullptr;
}

UWidget* UInspectorFilePageWidget::CreateCollapsibleSectionHeader(const FString& InTitle, TObjectPtr<UTextBlock>& OutToggleText, const FName& ButtonName)
{
    UButton* HeaderButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
    RICompactUI::ConfigureButton(HeaderButton, RICompactUI::ERIButtonVisualStyle::Header, false);
    UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    HeaderBorder->SetPadding(FMargin(6.f, 3.f));
    HeaderBorder->SetBrushColor(RICompactUI::GetButtonFillColor(RICompactUI::ERIButtonVisualStyle::Header));
    HeaderButton->AddChild(HeaderBorder);

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    HeaderBorder->SetContent(HeaderRow);

    UTextBlock* TitleText = RI_MakeText(WidgetTree, InTitle, RICompactUI::GetSectionTitleFontSize(), true, RICompactUI::GetButtonTextColor(RICompactUI::ERIButtonVisualStyle::Header));
    if (UHorizontalBoxSlot* HBoxSlot = HeaderRow->AddChildToHorizontalBox(TitleText))
    {
        HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HBoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutToggleText = RI_MakeText(WidgetTree, TEXT("Show"), RICompactUI::GetMutedFontSize(), true, RICompactUI::GetButtonTextColor(RICompactUI::ERIButtonVisualStyle::Header));
    HeaderRow->AddChildToHorizontalBox(OutToggleText);
    return HeaderButton;
}

void UInspectorFilePageWidget::SetSectionExpanded(UVerticalBox* SectionBody, UTextBlock* ToggleText, bool bExpanded, const FString& ExpandedLabel, const FString& CollapsedLabel)
{
    if (SectionBody)
    {
        SectionBody->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (ToggleText)
    {
        ToggleText->SetText(FText::FromString(bExpanded ? ExpandedLabel : CollapsedLabel));
    }
}

UWidget* UInspectorFilePageWidget::CreateMainActionsSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateSectionTitle(TEXT("Mainline Path"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UBorder* IntroBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RI_FileActionIntro"));
    IntroBorder->SetPadding(FMargin(5.f, 4.f));
    IntroBorder->SetBrushColor(RI_FileRowColor());
    IntroBorder->SetContent(RI_MakeText(
        WidgetTree,
        TEXT("1) Stage runtime edits  2) review the compare result  3) apply or export."),
        RICompactUI::GetMutedFontSize(),
        false,
        RI_FileMutedColor(),
        true));
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(IntroBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    auto AddCard = [this](
        UVerticalBox* Parent,
        const FString& Step,
        const FString& Title,
        const FString& Description,
        const FName& ButtonName,
        const FName& CardName,
        TObjectPtr<UButton>& OutButton,
        const FLinearColor& CardColor,
        const FLinearColor& TitleColor,
        RICompactUI::ERIButtonVisualStyle ButtonStyle,
        FSimpleDelegate Bind)
    {
        if (!Parent)
        {
            return;
        }

        UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), CardName);
        CardBorder->SetPadding(FMargin(6.f, 5.f));
        CardBorder->SetBrushColor(CardColor);

        UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        CardBorder->SetContent(CardBox);

        const FString StepTitle = FString::Printf(TEXT("%s  %s"), *Step, *Title);
        if (UVerticalBoxSlot* VBoxSlot = CardBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, StepTitle, RICompactUI::GetLabelFontSize(), true, TitleColor, true)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
        }

        if (UVerticalBoxSlot* VBoxSlot = CardBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, Description, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }

        OutButton = RICompactUI::MakeLabeledButton(WidgetTree, ButtonName, Title, ButtonStyle, 0.f);
        if (OutButton)
        {
            Bind.ExecuteIfBound();
        }
        if (UVerticalBoxSlot* ButtonSlot = CardBox->AddChildToVerticalBox(OutButton))
        {
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        }

        if (UVerticalBoxSlot* VBoxSlot = Parent->AddChildToVerticalBox(CardBorder))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
        }
    };

    AddCard(
        Outer,
        TEXT("Step 1"),
        TEXT("Stage Runtime Edit"),
        TEXT("Capture the current runtime edits into a staged patch."),
        TEXT("BTN_FileStagePatch"),
        TEXT("RI_FileActionCard_Stage"),
        StagePatchButton,
        FLinearColor(0.06f, 0.10f, 0.16f, 0.92f),
        FLinearColor(0.78f, 0.88f, 1.0f, 1.0f),
        RICompactUI::ERIButtonVisualStyle::Primary,
        FSimpleDelegate::CreateLambda([this]()
        {
            StagePatchButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleStagePatchClicked);
        }));

    AddCard(
        Outer,
        TEXT("Step 2"),
        TEXT("Review Source Diff"),
        TEXT("Preview source-side impact before writing anything back."),
        TEXT("BTN_FilePreviewPromote"),
        TEXT("RI_FileActionCard_Preview"),
        PreviewPromoteButton,
        FLinearColor(0.11f, 0.10f, 0.05f, 0.92f),
        FLinearColor(1.0f, 0.90f, 0.60f, 1.0f),
        RICompactUI::ERIButtonVisualStyle::Secondary,
        FSimpleDelegate::CreateLambda([this]()
        {
            PreviewPromoteButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandlePreviewPromoteClicked);
        }));

    AddCard(
        Outer,
        TEXT("Step 3"),
        TEXT("Apply To Source"),
        TEXT("Write the staged patch back to source when review is clear."),
        TEXT("BTN_FilePromoteApply"),
        TEXT("RI_FileActionCard_Apply"),
        PromoteApplyButton,
        FLinearColor(0.05f, 0.11f, 0.08f, 0.92f),
        FLinearColor(0.76f, 1.0f, 0.80f, 1.0f),
        RICompactUI::ERIButtonVisualStyle::Primary,
        FSimpleDelegate::CreateLambda([this]()
        {
            PromoteApplyButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandlePromoteApplyClicked);
        }));

    AddCard(
        Outer,
        TEXT("Optional"),
        TEXT("Discard Staged Patch"),
        TEXT("Clear the staged patch without touching source content."),
        TEXT("BTN_FileClearStaged"),
        TEXT("RI_FileActionCard_Clear"),
        ClearStagedButton,
        FLinearColor(0.12f, 0.06f, 0.06f, 0.92f),
        FLinearColor(1.0f, 0.78f, 0.78f, 1.0f),
        RICompactUI::ERIButtonVisualStyle::Danger,
        FSimpleDelegate::CreateLambda([this]()
        {
            ClearStagedButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleClearStagedClicked);
        }));

    return Outer;
}

UWidget* UInspectorFilePageWidget::CreateCountCell(const FString& Label, TObjectPtr<UTextBlock>& OutValueText)
{
    UBorder* Cell = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Cell->SetPadding(FMargin(4.f, 2.f));
    Cell->SetBrushColor(RI_FileCellColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Cell->SetContent(Box);

    Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, Label, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor()));
    OutValueText = RI_MakeText(WidgetTree, TEXT("0"), RICompactUI::GetValueFontSize(), true, RI_FileTextColor());
    Box->AddChildToVerticalBox(OutValueText);
    return Cell;
}

UWidget* UInspectorFilePageWidget::CreateInfoRow(const FString& Label, TObjectPtr<UTextBlock>& OutValueText, bool bWrapValue, const FName& WidgetName)
{
    return RICompactUI::MakeStackedValueRow(
        WidgetTree,
        Label,
        OutValueText,
        RI_FileRowColor(),
        RI_FileMutedColor(),
        RI_FileTextColor(),
        bWrapValue,
        NAME_None,
        WidgetName);
}

UWidget* UInspectorFilePageWidget::CreateStatusSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateSectionTitle(TEXT("Staged Status"), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Next Step"), NextStepText, true, TEXT("RI_FileNextStepRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Action Guide"), ActionGuideText, true, TEXT("RI_FileActionGuideRow"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    UVerticalBox* CountsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileCountsBox"));
    auto AddCountCell = [this, CountsBox](const FString& Label, TObjectPtr<UTextBlock>& OutValueText)
    {
        if (UVerticalBoxSlot* VBoxSlot = CountsBox->AddChildToVerticalBox(CreateCountCell(Label, OutValueText)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
        }
    };
    AddCountCell(TEXT("Snapshots"), SnapshotCountText);
    AddCountCell(TEXT("Patches"), PatchCountText);
    AddCountCell(TEXT("Presets"), PresetCountText);
    AddCountCell(TEXT("Audits"), AuditCountText);
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CountsBox))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Staged Patch"), StagedPatchText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Preview Source Changes"), PromotePreviewText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Apply To Source"), PromoteResultText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Last Audit"), AuditSummaryText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    Outer->AddChildToVerticalBox(CreateInfoRow(TEXT("Snapshot Patch Apply"), PatchApplyText, true));
    return Outer;
}

UWidget* UInspectorFilePageWidget::CreateAuditsSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UWidget* Header = CreateCollapsibleSectionHeader(TEXT("Compare & Audit"), AuditsSectionToggleText, TEXT("BTN_FileToggleAudits"));
    if (UButton* HeaderButton = Cast<UButton>(Header))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleToggleAuditsSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(Header))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    AuditsSectionBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileAuditsSectionBody"));
    Outer->AddChildToVerticalBox(AuditsSectionBody);

    UVerticalBox* ActionRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileAuditActionRow"));
    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(ActionRow))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    auto AddActionButton = [this, ActionRow](UButton* Button)
    {
        if (!ActionRow || !Button)
        {
            return;
        }
        if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(Button))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    };

    BuildBaselineAuditButton = CreateActionButton(TEXT("Compare Baseline"), TEXT("BTN_FileBuildBaselineAudit"), 0.f);
    BuildBaselineAuditButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleBuildBaselineAuditClicked);
    AddActionButton(BuildBaselineAuditButton);

    BuildAuditButton = CreateActionButton(TEXT("Compare Patch"), TEXT("BTN_FileBuildAudit"), 0.f);
    BuildAuditButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleBuildAuditClicked);
    AddActionButton(BuildAuditButton);

    BuildPatchVsSourceAuditButton = CreateActionButton(TEXT("Preview Source Audit"), TEXT("BTN_FileBuildPatchVsSource"), 0.f);
    BuildPatchVsSourceAuditButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleBuildPatchVsSourceAuditClicked);
    AddActionButton(BuildPatchVsSourceAuditButton);

    BuildAppliedAuditButton = CreateActionButton(TEXT("Verify Applied Source"), TEXT("BTN_FileBuildAppliedAudit"), 0.f);
    BuildAppliedAuditButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleBuildAppliedAuditClicked);
    AddActionButton(BuildAppliedAuditButton);

    UBorder* ViewButtonsBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    ViewButtonsBorder->SetPadding(FMargin(4.f, 3.f));
    ViewButtonsBorder->SetBrushColor(RI_FileRowColor());

    UVerticalBox* ViewButtonsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileCompareViewsRow"));
    ViewButtonsBorder->SetContent(ViewButtonsBox);

    auto AddViewButton = [this, ViewButtonsBox](const FString& Label, const FName& Name, TObjectPtr<UButton>& OutButton)
    {
        OutButton = CreateActionButton(Label, Name, 0.0f);
        if (UVerticalBoxSlot* VBoxSlot = ViewButtonsBox->AddChildToVerticalBox(OutButton))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    };

    AddViewButton(TEXT("View B/C"), TEXT("BTN_FileViewBaseline"), ViewBaselineButton);
    AddViewButton(TEXT("View C/P"), TEXT("BTN_FileViewCurrentPatch"), ViewCurrentPatchButton);
    AddViewButton(TEXT("View P/S"), TEXT("BTN_FileViewPatchSource"), ViewPatchSourceButton);
    AddViewButton(TEXT("View Applied"), TEXT("BTN_FileViewApplied"), ViewAppliedButton);

    if (ViewBaselineButton)
    {
        ViewBaselineButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleViewBaselineClicked);
    }
    if (ViewCurrentPatchButton)
    {
        ViewCurrentPatchButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleViewCurrentPatchClicked);
    }
    if (ViewPatchSourceButton)
    {
        ViewPatchSourceButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleViewPatchSourceClicked);
    }
    if (ViewAppliedButton)
    {
        ViewAppliedButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleViewAppliedClicked);
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(ViewButtonsBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Cached Views"), AuditCacheText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Audit Mode"), AuditModeText, false)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Compare Pair"), AuditPairText, false)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Audit Stats"), AuditStatsText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("First Compare Field"), AuditFirstFieldText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    UBorder* PreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    PreviewBorder->SetPadding(FMargin(5.f, 4.f));
    PreviewBorder->SetBrushColor(RI_FileRowColor());
    AuditPreviewText = RI_MakeText(WidgetTree, TEXT("No audit preview"), RICompactUI::GetMutedFontSize(), false, RI_FileTextColor(), true);
    PreviewBorder->SetContent(AuditPreviewText);
    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(PreviewBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    UBorder* LinesBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    LinesBorder->SetPadding(FMargin(4.f, 3.f));
    LinesBorder->SetBrushColor(RI_FileRowColor());

    AuditLinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FileAuditLinesBox"));
    LinesBorder->SetContent(AuditLinesBox);
    if (UVerticalBoxSlot* VBoxSlot = AuditsSectionBody->AddChildToVerticalBox(LinesBorder))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    return Outer;
}

UWidget* UInspectorFilePageWidget::CreatePresetsSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UWidget* Header = CreateCollapsibleSectionHeader(TEXT("Advanced & Remote"), PresetsSectionToggleText, TEXT("BTN_FileTogglePresets"));
    if (UButton* HeaderButton = Cast<UButton>(Header))
    {
        HeaderButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleTogglePresetsSectionClicked);
    }
    if (UVerticalBoxSlot* VBoxSlot = Outer->AddChildToVerticalBox(Header))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    }

    PresetsSectionBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FilePresetsSectionBody"));
    Outer->AddChildToVerticalBox(PresetsSectionBody);

    UVerticalBox* ActionRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RI_FilePresetActionRow"));
    if (UVerticalBoxSlot* VBoxSlot = PresetsSectionBody->AddChildToVerticalBox(ActionRow))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }

    auto AddButton = [ActionRow](UButton* Button)
    {
        if (!ActionRow || !Button)
        {
            return;
        }
        if (UVerticalBoxSlot* VBoxSlot = ActionRow->AddChildToVerticalBox(Button))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    };

    ExportPatchButton = CreateActionButton(TEXT("Export Patch"), TEXT("BTN_FileExportPatch"), 0.0f);
    ExportPatchButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleExportPatchClicked);
    AddButton(ExportPatchButton);

    SavePresetButton = CreateActionButton(TEXT("Save Preset"), TEXT("BTN_FileSavePreset"), 0.0f);
    SavePresetButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleSavePresetClicked);
    AddButton(SavePresetButton);

    ApplyLatestPresetButton = CreateActionButton(TEXT("Apply Preset"), TEXT("BTN_FileApplyLatestPreset"), 0.0f);
    ApplyLatestPresetButton->OnClicked.AddDynamic(this, &UInspectorFilePageWidget::HandleApplyLatestPresetClicked);
    AddButton(ApplyLatestPresetButton);

    if (UVerticalBoxSlot* VBoxSlot = PresetsSectionBody->AddChildToVerticalBox(CreateSectionTitle(TEXT("Latest Artifacts"))))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = PresetsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Snapshot"), LatestSnapshotText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = PresetsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Patch Bundle"), LatestPatchFileText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    if (UVerticalBoxSlot* VBoxSlot = PresetsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Preset"), LatestPresetText, true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
    }
    PresetsSectionBody->AddChildToVerticalBox(CreateInfoRow(TEXT("Audit Report"), LatestAuditFileText, true));
    return Outer;
}

UWidget* UInspectorFilePageWidget::CreateDiagnosticsSection()
{
    UVerticalBox* Outer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    DiagnosticsSectionBody = nullptr;
    return Outer;
}

void UInspectorFilePageWidget::UpdateRemoteSessionSelectionState()
{
    const UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    const FString QueryText = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FString WorkflowText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSessionById(AvailableRemoteSessions, SelectedRemoteSessionId);

    FString SummaryText = RI_BuildRemoteSessionDiscoverySummary(AvailableRemoteSessions);
    if (SelectedSession)
    {
        SummaryText += FString::Printf(TEXT(" | Selected=%s"), *SelectedSession->SessionId);
    }
    else if (!SelectedRemoteSessionId.IsEmpty())
    {
        SummaryText += FString::Printf(TEXT(" | Selected=%s (not found)"), *SelectedRemoteSessionId);
    }
    SetValueText(RemoteSessionSummaryText, SummaryText, AvailableRemoteSessions.Num() <= 0);

    if (SelectedSession)
    {
        SetValueText(RemoteSessionSelectionText, RI_BuildRemoteSessionSummaryText(*SelectedSession), SelectedSession->ConnectionState == ERIRuntimeSessionConnectionState::Error);
    }
    else
    {
        SetValueText(
            RemoteSessionSelectionText,
            SelectedRemoteSessionId.IsEmpty()
                ? TEXT("No remote session selected")
                : FString::Printf(TEXT("Selected session %s is not available"), *SelectedRemoteSessionId),
            !SelectedRemoteSessionId.IsEmpty());
    }

    if (RemoteSessionTargetQueryBox)
    {
        if (QueryText.IsEmpty())
        {
            SetValueText(RemoteSessionTargetQueryText, TEXT("Target query not set"));
        }
        else if (InspectorSubsystem && SelectedSession && SelectedSession->ConnectionState == ERIRuntimeSessionConnectionState::Connected)
        {
            TArray<FRIRuntimeTargetInfo> Targets;
            FString Error;
            bool bCachedSuccess = false;
            const bool bHasCachedQuery = InspectorSubsystem->TryGetCachedRuntimeTargetQueryResult(
                SelectedSession->SessionId,
                QueryText,
                FString(),
                12,
                Targets,
                Error,
                bCachedSuccess);
            if (bHasCachedQuery && bCachedSuccess)
            {
                SetValueText(
                    RemoteSessionTargetQueryText,
                    RI_BuildTargetQuerySummaryText(QueryText, Targets),
                    false);
            }
            else if (bHasCachedQuery)
            {
                SetValueText(
                    RemoteSessionTargetQueryText,
                    FString::Printf(TEXT("Query=%s | %s"), *QueryText, *Error),
                    true);
            }
            else
            {
                SetValueText(
                    RemoteSessionTargetQueryText,
                    FString::Printf(TEXT("Query=%s | Pending query refresh"), *QueryText),
                    false);
            }
        }
        else
        {
            SetValueText(
                RemoteSessionTargetQueryText,
                FString::Printf(TEXT("Query=%s | Connect the session to list targets"), *QueryText),
                false);
        }
    }
    else
    {
        SetValueText(RemoteSessionTargetQueryText, TEXT("Target query unavailable"), true);
    }

    if (WorkflowText.IsEmpty())
    {
        SetValueText(RemoteSessionWorkflowText, TEXT("Workflow id not set"));
    }
    else if (SelectedSession && SelectedSession->bIsExternal)
    {
        SetValueText(RemoteSessionWorkflowText, FString::Printf(TEXT("Workflow=%s | External packaged session ready"), *WorkflowText), false);
    }
    else
    {
        SetValueText(
            RemoteSessionWorkflowText,
            FString::Printf(TEXT("Workflow=%s | Local workflow registry"), *WorkflowText),
            false);
    }

    if (RemoteSessionRefreshButton)
    {
        RemoteSessionRefreshButton->SetIsEnabled(true);
    }
    if (RemoteSessionConnectButton)
    {
        RemoteSessionConnectButton->SetIsEnabled(SelectedSession != nullptr);
    }
    if (RemoteSessionPullPatchButton)
    {
        RemoteSessionPullPatchButton->SetIsEnabled(SelectedSession != nullptr && SelectedSession->bIsExternal);
    }
    if (RemoteSessionRunWorkflowButton)
    {
        RemoteSessionRunWorkflowButton->SetIsEnabled(SelectedSession != nullptr && !WorkflowText.IsEmpty());
    }

    PushRemoteSessionContextToSubsystem();
}

void UInspectorFilePageWidget::SetValueText(UTextBlock* TextWidget, const FString& InText, bool bIsError) const
{
    if (!TextWidget)
    {
        return;
    }

    TextWidget->SetText(FText::FromString(InText.IsEmpty() ? TEXT("-") : InText));
    RICompactUI::ApplyTextStyle(TextWidget, RICompactUI::GetValueFontSize(), false, bIsError ? RI_FileErrorColor() : RI_FileTextColor());
}

UWidget* UInspectorFilePageWidget::CreateAuditLineCard(const FRIAuditLine& InLine, int32 DisplayIndex)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("RI_FileAuditLine_%d"), DisplayIndex));
    Border->SetPadding(FMargin(4.f, 3.f));
    Border->SetBrushColor(InLine.bDifferent ? RICompactUI::GetSelectedRowSurfaceBackgroundColor() : RI_FileCellColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Border->SetContent(Box);

    const FString Header = FString::Printf(TEXT("%s :: %s"), *InLine.GroupLabel, *InLine.Field.FieldPath);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, Header, RICompactUI::GetLabelFontSize(), true, RI_FileTextColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    const FString Left = FString::Printf(TEXT("%s: %s"), *InLine.LeftTag, *RI_TruncateAuditValue(InLine.LeftValue));
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, Left, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    const FString Right = FString::Printf(TEXT("%s: %s"), *InLine.RightTag, *RI_TruncateAuditValue(InLine.RightValue));
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(
        WidgetTree,
        Right,
        RICompactUI::GetMutedFontSize(),
        false,
        InLine.bDifferent ? RICompactUI::GetSuccessTextColor() : RI_FileMutedColor(),
        true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InLine.Message.IsEmpty() ? 0.f : 1.f));
    }

    if (!InLine.Message.IsEmpty())
    {
        Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, InLine.Message, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true));
    }

    return Border;
}

UWidget* UInspectorFilePageWidget::CreateRoleCompareLineCard(const FRIRuntimeRoleCompareLine& InLine, int32 DisplayIndex)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("RI_FileRoleCompareLine_%d"), DisplayIndex));
    Border->SetPadding(FMargin(4.f, 3.f));
    Border->SetBrushColor(InLine.bHasMismatch ? RICompactUI::GetSelectedRowSurfaceBackgroundColor() : RI_FileCellColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Border->SetContent(Box);

    const FString Header = FString::Printf(TEXT("%s"), *InLine.Field.FieldPath);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, Header, RICompactUI::GetLabelFontSize(), true, RI_FileTextColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    for (const FRIRuntimeRoleFieldState& State : InLine.RoleStates)
    {
        const FString StateText = FString::Printf(
            TEXT("%s: %s"),
            *State.RoleLabel,
            State.bHasValue ? *RI_TruncateAuditValue(State.ValueText) : *State.Message);
        if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(
            WidgetTree,
            StateText,
            RICompactUI::GetMutedFontSize(),
            false,
            State.bRoleAvailable
                ? (State.bHasValue ? RICompactUI::GetSuccessTextColor() : RI_FileErrorColor())
                : RI_FileMutedColor(),
            true)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
        }
    }

    if (!InLine.Summary.IsEmpty())
    {
        Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, InLine.Summary, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true));
    }

    return Border;
}

UWidget* UInspectorFilePageWidget::CreateRemoteSessionCompareLineCard(const FRIRuntimeSessionTargetSetCompareLine& InLine, int32 DisplayIndex)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("RI_FileRemoteSessionCompareLine_%d"), DisplayIndex));
    Border->SetPadding(FMargin(4.f, 3.f));
    Border->SetBrushColor(InLine.bHasMismatch ? RICompactUI::GetSelectedRowSurfaceBackgroundColor() : RI_FileCellColor());

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Border->SetContent(Box);

    const FString Header = FString::Printf(TEXT("%s (%s)"), *InLine.DisplayLabel, *InLine.ActorClass);
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, Header, RICompactUI::GetLabelFontSize(), true, RI_FileTextColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    const FString LeftText = FString::Printf(
        TEXT("Left: %s | Count=%d | %s"),
        InLine.bPresentInLeft ? TEXT("present") : TEXT("missing"),
        InLine.LeftCount,
        *RI_TruncateAuditValue(InLine.LeftPrimaryPath.IsEmpty() ? TEXT("-") : InLine.LeftPrimaryPath));
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, LeftText, RICompactUI::GetMutedFontSize(), false, InLine.bPresentInLeft ? RI_FileMutedColor() : RI_FileErrorColor(), true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
    }

    const FString RightText = FString::Printf(
        TEXT("Right: %s | Count=%d | %s"),
        InLine.bPresentInRight ? TEXT("present") : TEXT("missing"),
        InLine.RightCount,
        *RI_TruncateAuditValue(InLine.RightPrimaryPath.IsEmpty() ? TEXT("-") : InLine.RightPrimaryPath));
    if (UVerticalBoxSlot* VBoxSlot = Box->AddChildToVerticalBox(RI_MakeText(
        WidgetTree,
        RightText,
        RICompactUI::GetMutedFontSize(),
        false,
        InLine.bPresentInRight ? (InLine.bHasMismatch ? RICompactUI::GetSuccessTextColor() : RI_FileMutedColor()) : RI_FileErrorColor(),
        true)))
    {
        VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InLine.Message.IsEmpty() ? 0.f : 1.f));
    }

    if (!InLine.Message.IsEmpty())
    {
        Box->AddChildToVerticalBox(RI_MakeText(WidgetTree, InLine.Message, RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor(), true));
    }

    return Border;
}

void UInspectorFilePageWidget::RebuildAuditLineCards(const FRIAuditReport& InReport)
{
    RenderedAuditLineCount = 0;
    RenderedAuditDifferentCount = 0;
    RenderedFirstAuditField.Reset();

    if (!AuditLinesBox)
    {
        return;
    }

    AuditLinesBox->ClearChildren();

    if (InReport.Lines.Num() <= 0)
    {
        AuditLinesBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, TEXT("No audit lines"), RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor()));
        return;
    }

    TArray<const FRIAuditLine*> PreferredLines;
    for (const FRIAuditLine& Line : InReport.Lines)
    {
        if (Line.bDifferent)
        {
            PreferredLines.Add(&Line);
            ++RenderedAuditDifferentCount;
        }
    }

    if (PreferredLines.Num() <= 0)
    {
        for (const FRIAuditLine& Line : InReport.Lines)
        {
            PreferredLines.Add(&Line);
        }
    }

    const int32 MaxLines = FMath::Min(PreferredLines.Num(), 4);
    for (int32 Index = 0; Index < MaxLines; ++Index)
    {
        const FRIAuditLine* Line = PreferredLines[Index];
        if (!Line)
        {
            continue;
        }

        if (RenderedFirstAuditField.IsEmpty())
        {
            RenderedFirstAuditField = Line->Field.FieldPath;
        }

        if (UVerticalBoxSlot* VBoxSlot = AuditLinesBox->AddChildToVerticalBox(CreateAuditLineCard(*Line, Index)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Index + 1 < MaxLines ? 3.f : 0.f));
        }
        ++RenderedAuditLineCount;
    }
}

void UInspectorFilePageWidget::RebuildRoleCompareLineCards(const FRIRuntimeRoleCompareReport& InReport)
{
    RenderedRoleCompareLineCount = 0;
    RenderedRoleCompareMissingCount = 0;

    if (!RoleCompareLinesBox)
    {
        return;
    }

    RoleCompareLinesBox->ClearChildren();

    if (InReport.Lines.Num() <= 0)
    {
        RoleCompareLinesBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, TEXT("No runtime role compare lines"), RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor()));
        return;
    }

    const int32 MaxLines = FMath::Min(InReport.Lines.Num(), 3);
    for (int32 Index = 0; Index < MaxLines; ++Index)
    {
        const FRIRuntimeRoleCompareLine& Line = InReport.Lines[Index];
        RenderedRoleCompareMissingCount += Line.MissingRoleCount;
        if (UVerticalBoxSlot* VBoxSlot = RoleCompareLinesBox->AddChildToVerticalBox(CreateRoleCompareLineCard(Line, Index)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Index + 1 < MaxLines ? 3.f : 0.f));
        }
        ++RenderedRoleCompareLineCount;
    }
}

void UInspectorFilePageWidget::RebuildRemoteSessionCompareLineCards(const FRIRuntimeSessionTargetSetCompareReport& InReport)
{
    RenderedRemoteSessionCompareLineCount = 0;
    RenderedRemoteSessionCompareMismatchCount = 0;

    if (!RemoteSessionCompareLinesBox)
    {
        return;
    }

    RemoteSessionCompareLinesBox->ClearChildren();

    if (InReport.Lines.Num() <= 0)
    {
        RemoteSessionCompareLinesBox->AddChildToVerticalBox(RI_MakeText(WidgetTree, TEXT("No remote session compare lines"), RICompactUI::GetMutedFontSize(), false, RI_FileMutedColor()));
        return;
    }

    TArray<const FRIRuntimeSessionTargetSetCompareLine*> PreferredLines;
    for (const FRIRuntimeSessionTargetSetCompareLine& Line : InReport.Lines)
    {
        if (Line.bHasMismatch)
        {
            PreferredLines.Add(&Line);
            ++RenderedRemoteSessionCompareMismatchCount;
        }
    }

    if (PreferredLines.Num() <= 0)
    {
        for (const FRIRuntimeSessionTargetSetCompareLine& Line : InReport.Lines)
        {
            PreferredLines.Add(&Line);
        }
    }

    const int32 MaxLines = FMath::Min(PreferredLines.Num(), 4);
    for (int32 Index = 0; Index < MaxLines; ++Index)
    {
        const FRIRuntimeSessionTargetSetCompareLine* Line = PreferredLines[Index];
        if (!Line)
        {
            continue;
        }

        if (UVerticalBoxSlot* VBoxSlot = RemoteSessionCompareLinesBox->AddChildToVerticalBox(CreateRemoteSessionCompareLineCard(*Line, Index)))
        {
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Index + 1 < MaxLines ? 3.f : 0.f));
        }
        ++RenderedRemoteSessionCompareLineCount;
    }
}

void UInspectorFilePageWidget::ApplyFileSummaryToWidgets(const FRIFileManagementSummary* Summary)
{
    auto ResolvePendingText = [](const UTextBlock* TextWidget, const TCHAR* PendingText) -> FString
    {
        if (!TextWidget)
        {
            return FString(PendingText);
        }

        const FString ExistingText = TextWidget->GetText().ToString().TrimStartAndEnd();
        return (ExistingText.IsEmpty() || ExistingText == TEXT("-")) ? FString(PendingText) : ExistingText;
    };

    if (!Summary)
    {
        SetValueText(SnapshotCountText, ResolvePendingText(SnapshotCountText, TEXT("...")));
        SetValueText(PatchCountText, ResolvePendingText(PatchCountText, TEXT("...")));
        SetValueText(PresetCountText, ResolvePendingText(PresetCountText, TEXT("...")));
        SetValueText(AuditCountText, ResolvePendingText(AuditCountText, TEXT("...")));
        SetValueText(StagedPatchText, ResolvePendingText(StagedPatchText, TEXT("Refreshing...")));
        SetValueText(PatchApplyText, ResolvePendingText(PatchApplyText, TEXT("Refreshing...")));
        SetValueText(PromotePreviewText, ResolvePendingText(PromotePreviewText, TEXT("Refreshing...")));
        SetValueText(PromoteResultText, ResolvePendingText(PromoteResultText, TEXT("Refreshing...")));
        SetValueText(AuditSummaryText, ResolvePendingText(AuditSummaryText, TEXT("Refreshing...")));
        SetValueText(RemoteSessionCompareStatusText, ResolvePendingText(RemoteSessionCompareStatusText, TEXT("Refreshing...")));
        SetValueText(AuditCacheText, ResolvePendingText(AuditCacheText, TEXT("Refreshing...")));
        SetValueText(LatestSnapshotText, ResolvePendingText(LatestSnapshotText, TEXT("Refreshing...")));
        SetValueText(LatestPatchFileText, ResolvePendingText(LatestPatchFileText, TEXT("Refreshing...")));
        SetValueText(LatestPresetText, ResolvePendingText(LatestPresetText, TEXT("Refreshing...")));
        SetValueText(LatestAuditFileText, ResolvePendingText(LatestAuditFileText, TEXT("Refreshing...")));
        return;
    }

    SetValueText(SnapshotCountText, FString::FromInt(Summary->SnapshotCount));
    SetValueText(PatchCountText, FString::FromInt(Summary->PatchBundleCount));
    SetValueText(PresetCountText, FString::FromInt(Summary->PresetCount));
    SetValueText(AuditCountText, FString::FromInt(Summary->AuditReportCount));

    const FString StagedText = Summary->bHasStagedPatch
        ? FString::Printf(TEXT("%s (%d ops)"), *Summary->StagedPatchLabel, Summary->StagedPatchOperationCount)
        : TEXT("No staged snapshot patch");
    SetValueText(StagedPatchText, StagedText);
    SetValueText(PatchApplyText, Summary->LastPatchApplySummary);
    SetValueText(PromotePreviewText, Summary->PromotePreviewSummary, !Summary->bHasPromotePreview && Summary->bHasStagedPatch);
    SetValueText(
        PromoteResultText,
        Summary->bHasLastPromoteResult
            ? FString::Printf(
                TEXT("%s [P:%d F:%d S:%d]"),
                *Summary->LastPromoteSummary,
                Summary->LastPromotePromotedCount,
                Summary->LastPromoteFailedCount,
                Summary->LastPromoteSkippedCount)
            : Summary->LastPromoteSummary,
        Summary->bHasLastPromoteResult && Summary->LastPromoteFailedCount > 0);
    SetValueText(AuditSummaryText, Summary->LastAuditSummary);
    SetValueText(RemoteSessionCompareStatusText, Summary->LastRemoteSessionCompareSummary, !Summary->bHasRemoteSessionCompareReport);
    SetValueText(
        AuditCacheText,
        FString::Printf(TEXT("%s | Active=%s"), *Summary->CachedAuditViewsSummary, *Summary->ActiveAuditViewLabel),
        Summary->CachedAuditViewCount <= 0);
    SetValueText(LatestSnapshotText, Summary->LatestSnapshotLabel.IsEmpty() ? TEXT("No snapshot files") : Summary->LatestSnapshotLabel);
    SetValueText(LatestPatchFileText, Summary->LatestPatchBundleLabel.IsEmpty() ? TEXT("No patch bundle files") : Summary->LatestPatchBundleLabel);
    SetValueText(LatestPresetText, Summary->LatestPresetLabel.IsEmpty() ? TEXT("No presets") : Summary->LatestPresetLabel);
    SetValueText(LatestAuditFileText, Summary->LatestAuditReportLabel.IsEmpty() ? TEXT("No audit report files") : Summary->LatestAuditReportLabel);
}

void UInspectorFilePageWidget::ApplyPrimaryActionGuidance(const AActor* SelectedActor, const FRIFileManagementSummary* Summary)
{
    const bool bHasSelectedActor = SelectedActor != nullptr;
    const bool bHasStagedPatch = Summary && Summary->bHasStagedPatch;
    const bool bHasPreview = Summary && Summary->bHasPromotePreview;
    const bool bHasPromoteResult = Summary && (Summary->bHasLastPromoteResult || !Summary->LastPromoteSummary.TrimStartAndEnd().IsEmpty());

    const FString NextStep = !bHasSelectedActor
        ? TEXT("Select an actor in Actor, edit a property, then return to Changes.")
        : (!bHasStagedPatch
            ? TEXT("Stage Runtime Changes to capture the current runtime edits.")
            : (!bHasPreview
                ? TEXT("Preview Source Changes to inspect the staged patch before writing to source.")
                : (!bHasPromoteResult
                    ? TEXT("Review the preview, then Apply To Source when the staged patch looks correct.")
                    : TEXT("Use audits to verify the result, or discard the staged patch if you are done."))));

    const FString ActionGuide = !bHasSelectedActor
        ? TEXT("Start in Actor. Changes is for stage, preview, and apply.")
        : (!bHasStagedPatch
            ? TEXT("Recommended path: Stage Runtime Changes -> Preview Source Changes -> Apply To Source.")
            : (bHasPreview
                ? TEXT("You already have a staged patch and preview. Apply when ready, or discard it.")
                : TEXT("You already have a staged patch. Preview it next, or discard it.")));

    SetValueText(NextStepText, NextStep, !bHasSelectedActor);
    SetValueText(ActionGuideText, ActionGuide);
    if (NextStepText)
    {
        RICompactUI::ApplyStatusTone(
            NextStepText,
            !bHasSelectedActor
                ? RICompactUI::ERIStatusVisualStyle::Warning
                : (!bHasStagedPatch
                    ? RICompactUI::ERIStatusVisualStyle::Strong
                    : (!bHasPreview
                        ? RICompactUI::ERIStatusVisualStyle::Warning
                        : RICompactUI::ERIStatusVisualStyle::Success)));
    }
    if (ActionGuideText)
    {
        RICompactUI::ApplyStatusTone(
            ActionGuideText,
            !bHasSelectedActor
                ? RICompactUI::ERIStatusVisualStyle::Warning
                : (bHasStagedPatch
                    ? RICompactUI::ERIStatusVisualStyle::Strong
                    : RICompactUI::ERIStatusVisualStyle::Normal));
    }
}

void UInspectorFilePageWidget::ApplyPrimaryActionButtonStates(const AActor* SelectedActor, const FRIFileManagementSummary* Summary)
{
    const bool bHasSelectedActor = SelectedActor != nullptr;
    const bool bHasStagedPatch = Summary && Summary->bHasStagedPatch;
    const bool bHasPreview = Summary && Summary->bHasPromotePreview;
    const bool bHasPromoteResult = Summary && (Summary->bHasLastPromoteResult || !Summary->LastPromoteSummary.TrimStartAndEnd().IsEmpty());
    const bool bHasPreset = Summary && Summary->PresetCount > 0;
    const FString StageReason = bHasSelectedActor ? FString() : TEXT("Select an actor first.");
    const FString PreviewReason = bHasStagedPatch ? FString() : TEXT("Stage runtime changes first.");
    const FString ApplyReason = !bHasStagedPatch ? TEXT("Stage runtime changes first.") : (!bHasPreview ? TEXT("Preview source changes first.") : FString());
    const FString ClearReason = bHasStagedPatch ? FString() : TEXT("Nothing is staged yet.");
    const FString ExportReason = bHasStagedPatch ? FString() : TEXT("Stage runtime changes first.");
    const FString PresetReason = bHasStagedPatch ? FString() : TEXT("Stage runtime changes first.");
    const FString ApplyPresetReason = bHasPreset ? FString() : TEXT("No presets are available yet.");
    const FString BaselineReason = bHasSelectedActor ? FString() : TEXT("Select an actor first.");
    const FString AuditReason = bHasStagedPatch ? FString() : TEXT("Stage runtime changes first.");
    const FString AppliedAuditReason = bHasPromoteResult ? FString() : TEXT("Run Apply To Source first.");

    if (StagePatchButton)
    {
        RICompactUI::SetButtonAffordance(StagePatchButton, RICompactUI::ERIButtonVisualStyle::Primary, bHasSelectedActor, StageReason, TEXT("Stage the current runtime edits as a patch."), true);
    }
    if (PreviewPromoteButton)
    {
        RICompactUI::SetButtonAffordance(PreviewPromoteButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, PreviewReason, TEXT("Preview the staged patch on the source side."), true);
    }
    if (PromoteApplyButton)
    {
        RICompactUI::SetButtonAffordance(PromoteApplyButton, RICompactUI::ERIButtonVisualStyle::Primary, bHasStagedPatch && bHasPreview, ApplyReason, TEXT("Write the staged patch back to source."), true);
    }
    if (ClearStagedButton)
    {
        RICompactUI::SetButtonAffordance(ClearStagedButton, RICompactUI::ERIButtonVisualStyle::Danger, bHasStagedPatch, ClearReason, TEXT("Discard the staged patch."), true);
    }
    if (ExportPatchButton)
    {
        RICompactUI::SetButtonAffordance(ExportPatchButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, ExportReason, TEXT("Export the staged patch bundle."), true);
    }
    if (SavePresetButton)
    {
        RICompactUI::SetButtonAffordance(SavePresetButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, PresetReason, TEXT("Save the staged patch as a preset."), true);
    }
    if (ApplyLatestPresetButton)
    {
        RICompactUI::SetButtonAffordance(ApplyLatestPresetButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasPreset, ApplyPresetReason, TEXT("Apply the newest available preset."), true);
    }
    if (BuildBaselineAuditButton)
    {
        RICompactUI::SetButtonAffordance(BuildBaselineAuditButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasSelectedActor, BaselineReason, TEXT("Compare the baseline actor state."), true);
    }
    if (BuildAuditButton)
    {
        RICompactUI::SetButtonAffordance(BuildAuditButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, AuditReason, TEXT("Compare the current runtime state against the staged patch."), true);
    }
    if (BuildPatchVsSourceAuditButton)
    {
        RICompactUI::SetButtonAffordance(BuildPatchVsSourceAuditButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, AuditReason, TEXT("Compare the staged patch against source."), true);
    }
    if (BuildAppliedAuditButton)
    {
        RICompactUI::SetButtonAffordance(BuildAppliedAuditButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasPromoteResult, AppliedAuditReason, TEXT("Verify the applied source after snapshot promote."), true);
    }
    if (BuildRuntimeRoleCompareButton)
    {
        RICompactUI::SetButtonAffordance(BuildRuntimeRoleCompareButton, RICompactUI::ERIButtonVisualStyle::Secondary, bHasStagedPatch, AuditReason, TEXT("Build a runtime role compare report."), true);
    }
    if (BuildRemoteSessionCompareButton)
    {
        RICompactUI::SetButtonAffordance(BuildRemoteSessionCompareButton, RICompactUI::ERIButtonVisualStyle::Secondary, true, FString(), TEXT("Build a remote session compare report."), true);
    }
}

void UInspectorFilePageWidget::RefreshFromSubsystem()
{
    RefreshFromSubsystemInternal(false, false);
}

void UInspectorFilePageWidget::ApplyPresentationCollapsedState(bool bCollapseAudits, bool bCollapsePresets, bool bCollapseDiagnostics)
{
    bAuditsSectionExpanded = !bCollapseAudits;
    bPresetsSectionExpanded = !bCollapsePresets;
    bDiagnosticsSectionExpanded = !bCollapseDiagnostics;

    SetSectionExpanded(AuditsSectionBody, AuditsSectionToggleText, bAuditsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(PresetsSectionBody, PresetsSectionToggleText, bPresetsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    SetSectionExpanded(DiagnosticsSectionBody, DiagnosticsSectionToggleText, bDiagnosticsSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorFilePageWidget::RefreshFromSubsystemInternal(bool bForceSessionRefresh, bool bForceTargetQueryRefresh)
{
    const double StartSeconds = FPlatformTime::Seconds();
    (void)bForceTargetQueryRefresh;
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FRIFileManagementSummary Summary;
    FString Error;
    bool bSummaryCacheHit = false;
    const bool bCanUseCachedSummary = !bForceSessionRefresh;
    if (bCanUseCachedSummary)
    {
        bSummaryCacheHit = InspectorSubsystem->TryGetCachedFileManagementSummary(Summary, Error);
    }

    if (!bSummaryCacheHit && !InspectorSubsystem->RebuildFileManagementSummaryCache(Summary, Error))
    {
        SetValueText(StatusText, Error, true);
        return;
    }
    ApplyFileSummaryToWidgets(&Summary);

    const AActor* SelectedActor = InspectorSubsystem->GetSelectedActor();
    SetValueText(SelectedActorSummaryText, SelectedActor ? SelectedActor->GetName() : TEXT("No selected actor"), SelectedActor == nullptr);
    SetValueText(SelectedActorClassText, RI_BuildSelectedActorClassLabel(SelectedActor), SelectedActor == nullptr);
    SetValueText(SelectedActorSourcePathText, RI_BuildSelectedActorSourceLabel(SelectedActor), false);
    ApplyPrimaryActionGuidance(SelectedActor, &Summary);

    FRIAuditReport AuditReport;
    const bool bHasCachedActiveReport = InspectorSubsystem->GetCachedAuditReport(InspectorSubsystem->GetActiveFileAuditViewMode(), AuditReport);
    if (!bHasCachedActiveReport)
    {
        AuditReport = InspectorSubsystem->GetLastAuditReport();
    }
    const bool bHasAuditReport = !AuditReport.Summary.IsEmpty() || AuditReport.Lines.Num() > 0;
    SetValueText(AuditModeText, bHasAuditReport ? RI_AuditModeLabel(AuditReport.Mode) : TEXT("No audit report"));
    SetValueText(AuditPairText, bHasAuditReport ? RI_BuildAuditPairLabel(AuditReport) : TEXT("No compare pair"));
    if (bHasAuditReport)
    {
        int32 DifferentCount = 0;
        for (const FRIAuditLine& Line : AuditReport.Lines)
        {
            if (Line.bDifferent)
            {
                ++DifferentCount;
            }
        }
        SetValueText(
            AuditStatsText,
            FString::Printf(TEXT("Lines=%d | Different=%d | Same=%d"), AuditReport.Lines.Num(), DifferentCount, AuditReport.Lines.Num() - DifferentCount));
    }
    else
    {
        SetValueText(AuditStatsText, TEXT("No audit stats"));
    }
    SetValueText(AuditFirstFieldText, bHasAuditReport ? RI_FindFirstDifferentField(AuditReport) : TEXT("No compare fields"));
    SetValueText(AuditPreviewText, bHasAuditReport ? RI_BuildAuditPreviewText(AuditReport) : TEXT("No audit preview"));
    RebuildAuditLineCards(AuditReport);

    const FRIRuntimeRoleCompareReport RoleCompareReport = InspectorSubsystem->GetLastRuntimeRoleCompareReport();
    const bool bHasRoleCompare = Summary.bHasRoleCompareReport || RoleCompareReport.Lines.Num() > 0 || !RoleCompareReport.Summary.IsEmpty();
    SetValueText(RoleCompareSummaryText, bHasRoleCompare ? Summary.LastRoleCompareSummary : TEXT("No runtime role compare"));
    SetValueText(RoleCompareAvailableRoleText, bHasRoleCompare ? Summary.LastRoleCompareAvailableRole : TEXT("No available role"));
    SetValueText(
        RoleCompareStatsText,
        bHasRoleCompare
            ? FString::Printf(
                TEXT("Lines=%d | Mismatch=%d | MissingRoles=%d | Verify=%d"),
                Summary.LastRoleCompareLineCount,
                Summary.LastRoleCompareMismatchCount,
                Summary.LastRoleCompareMissingRoleCount,
                Summary.LastRoleCompareVerificationMismatchCount)
            : TEXT("No role compare stats"),
        bHasRoleCompare && Summary.LastRoleCompareMismatchCount > 0);
    SetValueText(RoleComparePreviewText, bHasRoleCompare ? RI_BuildRoleComparePreviewText(RoleCompareReport) : TEXT("No runtime role compare preview"));
    RebuildRoleCompareLineCards(RoleCompareReport);

    const FRIRuntimeSessionTargetSetCompareReport RemoteSessionCompareReport = InspectorSubsystem->GetLastRuntimeSessionTargetSetCompareReport();
    const bool bHasRemoteSessionCompare = Summary.bHasRemoteSessionCompareReport
        || RemoteSessionCompareReport.Lines.Num() > 0
        || !RemoteSessionCompareReport.Summary.IsEmpty();
    SetValueText(RemoteSessionCompareSummaryText, bHasRemoteSessionCompare ? Summary.LastRemoteSessionCompareSummary : TEXT("No remote session compare"));
    SetValueText(RemoteSessionCompareSessionsText, bHasRemoteSessionCompare ? Summary.LastRemoteSessionCompareSessionPair : TEXT("No session pair"));
    SetValueText(
        RemoteSessionCompareStatsText,
        bHasRemoteSessionCompare ? RI_BuildRemoteSessionCompareStatsText(RemoteSessionCompareReport) : TEXT("No remote session compare stats"),
        bHasRemoteSessionCompare && Summary.LastRemoteSessionCompareMismatchCount > 0);
    SetValueText(
        RemoteSessionComparePreviewText,
        bHasRemoteSessionCompare ? RI_BuildRemoteSessionComparePreviewText(RemoteSessionCompareReport) : TEXT("No remote session compare preview"));
    RebuildRemoteSessionCompareLineCards(RemoteSessionCompareReport);

    ApplyPrimaryActionButtonStates(SelectedActor, &Summary);

    SetValueText(StatusText, TEXT("Ready"));
    InspectorSubsystem->RefreshSharedContextStrip();
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] File DeferredHeavyRefresh %.2f ms | SummaryCache=%d ForceSession=%d ForceTarget=%d"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0,
        bSummaryCacheHit ? 1 : 0,
        bForceSessionRefresh ? 1 : 0,
        bForceTargetQueryRefresh ? 1 : 0);
}

void UInspectorFilePageWidget::PullRemoteSessionContextFromSubsystem()
{
    const UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return;
    }

    const FString PreferredSessionId = InspectorSubsystem->GetPreferredRemoteSessionId().TrimStartAndEnd();
    if (!PreferredSessionId.IsEmpty())
    {
        SelectedRemoteSessionId = PreferredSessionId;
    }

    if (RemoteSessionTargetQueryBox)
    {
        const FString StoredQuery = InspectorSubsystem->GetLastRemoteSessionTargetQuery().TrimStartAndEnd();
        const FString CurrentQuery = RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd();
        if (!StoredQuery.IsEmpty() && !StoredQuery.Equals(CurrentQuery, ESearchCase::CaseSensitive))
        {
            RemoteSessionTargetQueryBox->SetText(FText::FromString(StoredQuery));
        }
        else if (StoredQuery.IsEmpty() && CurrentQuery.IsEmpty())
        {
            RemoteSessionTargetQueryBox->SetText(FText::FromString(TEXT("BP_TestVarsActor")));
        }
    }

    if (RemoteSessionWorkflowBox)
    {
        const FString StoredWorkflowId = InspectorSubsystem->GetLastRemoteSessionWorkflowId().TrimStartAndEnd();
        const FString CurrentWorkflowId = RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd();
        if (!StoredWorkflowId.IsEmpty() && !StoredWorkflowId.Equals(CurrentWorkflowId, ESearchCase::CaseSensitive))
        {
            RemoteSessionWorkflowBox->SetText(FText::FromString(StoredWorkflowId));
        }
    }
}

void UInspectorFilePageWidget::PushRemoteSessionContextToSubsystem()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem || !RemoteSessionTargetQueryBox || !RemoteSessionWorkflowBox)
    {
        return;
    }

    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSessionById(AvailableRemoteSessions, SelectedRemoteSessionId);
    const FString SelectionSummary = SelectedSession
        ? RI_BuildRemoteSessionSummaryText(*SelectedSession)
        : SelectedRemoteSessionId;
    const FString QueryText = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FString WorkflowText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
    InspectorSubsystem->SetRemoteSessionUIContext(SelectedRemoteSessionId, SelectionSummary, QueryText, WorkflowText);
}

void UInspectorFilePageWidget::HandleRefreshClicked()
{
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(true, true);
}

void UInspectorFilePageWidget::HandleToggleAuditsSectionClicked()
{
    bAuditsSectionExpanded = !bAuditsSectionExpanded;
    SetSectionExpanded(AuditsSectionBody, AuditsSectionToggleText, bAuditsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    if (bAuditsSectionExpanded)
    {
        RefreshFastFromSubsystem();
    }
}

void UInspectorFilePageWidget::HandleTogglePresetsSectionClicked()
{
    bPresetsSectionExpanded = !bPresetsSectionExpanded;
    SetSectionExpanded(PresetsSectionBody, PresetsSectionToggleText, bPresetsSectionExpanded, TEXT("Hide"), TEXT("Show"));
}

void UInspectorFilePageWidget::HandleToggleDiagnosticsSectionClicked()
{
    bDiagnosticsSectionExpanded = !bDiagnosticsSectionExpanded;
    SetSectionExpanded(DiagnosticsSectionBody, DiagnosticsSectionToggleText, bDiagnosticsSectionExpanded, TEXT("Hide"), TEXT("Show"));
    if (bDiagnosticsSectionExpanded)
    {
        RefreshFastFromSubsystem();
    }
}

void UInspectorFilePageWidget::HandleRemoteSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    if (bUpdatingRemoteSessionWidgets)
    {
        return;
    }

    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSessionByLabel(AvailableRemoteSessions, SelectedItem);
    if (SelectedSession)
    {
        SelectedRemoteSessionId = SelectedSession->SessionId;
    }
    else
    {
        SelectedRemoteSessionId = SelectedItem;
    }

    UpdateRemoteSessionSelectionState();
    ScheduleDeferredRefresh(false, true);
    SetValueText(StatusText, FString::Printf(TEXT("Selected remote session %s"), *SelectedRemoteSessionId));
}

void UInspectorFilePageWidget::HandleRemoteSessionRefreshClicked()
{
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(true, true);
    SetValueText(StatusText, TEXT("Remote sessions refreshed"));
}

void UInspectorFilePageWidget::HandleRemoteSessionConnectClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    if (SelectedRemoteSessionId.IsEmpty())
    {
        SetValueText(StatusText, TEXT("No remote session selected"), true);
        return;
    }

    FRIRuntimeSessionInfo ConnectedSession;
    FString Error;
    const bool bOk = InspectorSubsystem->ConnectRemoteRuntimeSession(SelectedRemoteSessionId, ConnectedSession, Error);
    if (bOk)
    {
        SelectedRemoteSessionId = ConnectedSession.SessionId.IsEmpty() ? SelectedRemoteSessionId : ConnectedSession.SessionId;
        CancelDeferredRefresh();
        RefreshFromSubsystemInternal(false, true);
        SetValueText(
            StatusText,
            FString::Printf(TEXT("Connected to %s"), *ConnectedSession.DisplayName),
            false);
    }
    else
    {
        RefreshFastFromSubsystem();
        SetValueText(StatusText, Error.IsEmpty() ? TEXT("Failed to connect to remote session") : Error, true);
    }
}

void UInspectorFilePageWidget::HandleRemoteSessionPullPatchClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    if (SelectedRemoteSessionId.IsEmpty())
    {
        SetValueText(StatusText, TEXT("No remote session selected"), true);
        return;
    }

    const FRIRuntimeSessionInfo* SelectedSession = RI_FindRemoteSessionById(AvailableRemoteSessions, SelectedRemoteSessionId);
    if (!SelectedSession || !SelectedSession->bIsExternal)
    {
        SetValueText(StatusText, TEXT("Pull Patch requires an external packaged session"), true);
        return;
    }

    const FString ActorQuery = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    if (ActorQuery.IsEmpty())
    {
        SetValueText(StatusText, TEXT("Target query not set"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFilePullPatchFromRemoteSessionAction(SelectedRemoteSessionId, ActorQuery, Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, true);
    SetValueText(StatusText, Summary.IsEmpty() ? (bOk ? TEXT("Remote pull patch completed") : TEXT("Remote pull patch failed")) : Summary, !bOk);
}

void UInspectorFilePageWidget::HandleRemoteSessionRunWorkflowClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    if (SelectedRemoteSessionId.IsEmpty())
    {
        SetValueText(StatusText, TEXT("No remote session selected"), true);
        return;
    }

    const FString WorkflowIdText = RemoteSessionWorkflowBox ? RemoteSessionWorkflowBox->GetText().ToString().TrimStartAndEnd() : FString();
    if (WorkflowIdText.IsEmpty())
    {
        SetValueText(StatusText, TEXT("Workflow id not set"), true);
        return;
    }

    const FString ActorQuery = RemoteSessionTargetQueryBox ? RemoteSessionTargetQueryBox->GetText().ToString().TrimStartAndEnd() : FString();
    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileRunRemoteWorkflowAction(SelectedRemoteSessionId, FName(*WorkflowIdText), ActorQuery, Summary, Details);
    UpdateRemoteSessionSelectionState();
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, true);

    const FString StatusMessage = Summary.IsEmpty()
        ? FString::Printf(TEXT("Workflow %s finished"), *WorkflowIdText)
        : FString::Printf(TEXT("Workflow %s: %s"), *WorkflowIdText, *Summary);
    SetValueText(StatusText, StatusMessage, !bOk);
    SetValueText(RemoteSessionWorkflowText, StatusMessage, !bOk);
}

void UInspectorFilePageWidget::HandleRemoteSessionTargetQueryCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    (void)CommitMethod;

    if (bUpdatingRemoteSessionWidgets)
    {
        return;
    }

    const FString QueryText = InText.ToString().TrimStartAndEnd();
    if (QueryText.IsEmpty())
    {
        SetValueText(RemoteSessionTargetQueryText, TEXT("Target query not set"));
    }
    else
    {
        UpdateRemoteSessionSelectionState();
        ScheduleDeferredRefresh(false, true);
    }
}

void UInspectorFilePageWidget::HandleRemoteSessionWorkflowCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
    (void)CommitMethod;

    if (bUpdatingRemoteSessionWidgets)
    {
        return;
    }

    const FString WorkflowText = InText.ToString().TrimStartAndEnd();
    if (WorkflowText.IsEmpty())
    {
        SetValueText(RemoteSessionWorkflowText, TEXT("Workflow id not set"));
    }
    else
    {
        UpdateRemoteSessionSelectionState();
    }
}

void UInspectorFilePageWidget::HandleStagePatchClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileStagePatchAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}

void UInspectorFilePageWidget::HandleExportPatchClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileExportPatchAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}

void UInspectorFilePageWidget::HandleSavePresetClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileSavePresetAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}

void UInspectorFilePageWidget::HandleApplyLatestPresetClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFileApplyLatestPresetAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}

void UInspectorFilePageWidget::HandleBuildBaselineAuditClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildBaselineAuditAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleBuildAuditClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildAuditAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleBuildPatchVsSourceAuditClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildPatchVsSourceAuditAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleBuildAppliedAuditClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildAppliedPatchVsSourceAuditAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleBuildRuntimeRoleCompareClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildRuntimeRoleCompareAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleBuildRemoteSessionCompareClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bBuilt = InspectorSubsystem->ExecuteFileBuildRemoteSessionTargetSetCompareAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bBuilt);
}

void UInspectorFilePageWidget::HandleViewBaselineClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FRIAuditReport Report;
    if (!InspectorSubsystem->GetCachedAuditReport(ERIAuditComparisonMode::BaselineVsCurrent, Report))
    {
        SetValueText(StatusText, TEXT("No cached baseline/current compare"), true);
        return;
    }

    InspectorSubsystem->SetActiveFileAuditViewMode(ERIAuditComparisonMode::BaselineVsCurrent);
    RefreshFastFromSubsystem();
    SetValueText(StatusText, TEXT("Viewing baseline/current compare"));
}

void UInspectorFilePageWidget::HandleViewCurrentPatchClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FRIAuditReport Report;
    if (!InspectorSubsystem->GetCachedAuditReport(ERIAuditComparisonMode::CurrentVsPatch, Report))
    {
        SetValueText(StatusText, TEXT("No cached current/patch compare"), true);
        return;
    }

    InspectorSubsystem->SetActiveFileAuditViewMode(ERIAuditComparisonMode::CurrentVsPatch);
    RefreshFastFromSubsystem();
    SetValueText(StatusText, TEXT("Viewing current/patch compare"));
}

void UInspectorFilePageWidget::HandleViewPatchSourceClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FRIAuditReport Report;
    if (!InspectorSubsystem->GetCachedAuditReport(ERIAuditComparisonMode::PatchVsSource, Report))
    {
        SetValueText(StatusText, TEXT("No cached patch/source compare"), true);
        return;
    }

    InspectorSubsystem->SetActiveFileAuditViewMode(ERIAuditComparisonMode::PatchVsSource);
    RefreshFastFromSubsystem();
    SetValueText(StatusText, TEXT("Viewing patch/source compare"));
}

void UInspectorFilePageWidget::HandleViewAppliedClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FRIAuditReport Report;
    if (!InspectorSubsystem->GetCachedAuditReport(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote, Report))
    {
        SetValueText(StatusText, TEXT("No cached applied/source compare"), true);
        return;
    }

    InspectorSubsystem->SetActiveFileAuditViewMode(ERIAuditComparisonMode::AppliedPatchVsSourceAfterPromote);
    RefreshFastFromSubsystem();
    SetValueText(StatusText, TEXT("Viewing applied/source compare"));
}

void UInspectorFilePageWidget::HandlePreviewPromoteClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFilePromotePreviewAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}

void UInspectorFilePageWidget::HandleClearStagedClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    InspectorSubsystem->ExecuteFileClearStagedAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary);
}

void UInspectorFilePageWidget::HandlePromoteApplyClicked()
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        SetValueText(StatusText, TEXT("Subsystem unavailable"), true);
        return;
    }

    FString Summary;
    FString Details;
    const bool bOk = InspectorSubsystem->ExecuteFilePromoteApplyAction(Summary, Details);
    CancelDeferredRefresh();
    RefreshFromSubsystemInternal(false, false);
    SetValueText(StatusText, Summary, !bOk);
}
