#pragma once


#include "CoreMinimal.h"
#include "InspectorDefines.h"
#include "Components/SlateWrapperTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"
#include "InspectorGroupItem.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Slate/WidgetTransform.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorTypes.h"
#include "InspectorPatchTypes.h"
#include "InspectorNetworkCompareTypes.h"
#include "InspectorRemoteSessionTypes.h"
#include "RuntimeInspectorSettings.h"
#include "TimerManager.h"


#include "InspectorWorldSubsystem.generated.h"

//USTRUCT(BlueprintType)
//struct FInspectorChange
//{
//    GENERATED_BODY()
//
//    UPROPERTY() TWeakObjectPtr<UObject> Target;
//    UPROPERTY() FName PropertyName;
//
//    UPROPERTY() FString OldValueText;
//    UPROPERTY() FString NewValueText;
//
//    UPROPERTY() FString DebugObjectName; // ��ѡ��������־
//};
class UCameraComponent;
class UBorder;
class UButton;
class UInputComponent;
class UInspectorColorPickerWidget;
class UInspectorDockRootWidget;
class UInspectorFilePageWidget;
class UInspectorFunctionItem;
class UInspectorFunctionsSectionWidget;
class UInspectorGroupsSectionWidget;
class UInspectorModalBlockerWidget;
class UInspectorPropertiesSectionWidget;
class UInspectorSettingsPageWidget;
class UInspectorTestPageWidget;
class UPanelWidget;
class UScrollBox;
class USizeBox;
class UHorizontalBox;
class UVerticalBox;
class UWidget;
class UWidgetSwitcher;
class UUserWidget;
class UEditableTextBox;
class UTextBlock;
class UInspectorPropertyItem;
class UInspectorGroupButtonProxy;
class UInspectorPinnedItemButtonProxy;
class URuntimeInspectorController;
class FRuntimeInspectorInputProcessor;
struct FPointerEvent;
class FSlateRect;

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
UENUM()
enum class EInspectorChangeType : uint8
{
    Property,
    MaterialScalar,
    MaterialVector,
};

UENUM()
enum class ERIPropertyViewMode : uint8
{
    Full,           // Ĭ�ϣ�Actor + Components + Materials��������������
    MaterialOnly,   // ֻ��ʾĳ������� Materials ����
};

UENUM(BlueprintType)
enum class EInspectorRefreshReason : uint8
{
    ValuesChanged UMETA(DisplayName = "Values Changed"),      // ֻˢ����ʾֵ
    UIStateChanged UMETA(DisplayName = "UI State Changed"),   // �۵�/������
    UndoRedo UMETA(DisplayName = "Undo/Redo"),                // ��ҪӲˢ�� Entry
    StructureChanged UMETA(DisplayName = "Structure Changed"),// �����ɾ/ѡ�б仯
    TargetInvalid UMETA(DisplayName = "Target Invalid"),      // Actor ʧЧ
};

UENUM(BlueprintType)
enum class ERIToastType : uint8
{
    Info,
    Success,
    Warning,
    Error
};

struct FRIHostLegacyChildState
{
    TWeakObjectPtr<UWidget> Widget;
    ESlateVisibility OriginalVisibility = ESlateVisibility::Visible;
};

struct FRIHostPanelMountState
{
    TWeakObjectPtr<UPanelWidget> HostPanel;
    TWeakObjectPtr<UWidget> MountedPageWidget;
    TArray<FRIHostLegacyChildState> LegacyChildren;
};

struct FRIActivityLogEntry
{
    FDateTime TimestampUtc;
    ERIToastType Severity = ERIToastType::Info;
    FString Category;
    FString Message;
};


USTRUCT()
struct FInspectorChange
{
    GENERATED_BODY()

    // ====== �������ֶΣ�ʾ����======
    UPROPERTY() TWeakObjectPtr<UObject> Target;
    UPROPERTY() FName PropertyName = NAME_None;
    UPROPERTY() FString OldValueText;
    UPROPERTY() FString NewValueText;
    UPROPERTY() FString DebugObjectName;

    // ====== ���������� ======
    UPROPERTY() EInspectorChangeType ChangeType = EInspectorChangeType::Property;

    // ====== ���������ʲ����ط�������Ϣ ======
    UPROPERTY() TWeakObjectPtr<UPrimitiveComponent> TargetComponent;
    UPROPERTY() int32 MaterialIndex = INDEX_NONE;
    UPROPERTY() FName ParamName = NAME_None;

    UPROPERTY() float OldScalar = 0.f;
    UPROPERTY() float NewScalar = 0.f;

    UPROPERTY() FLinearColor OldVector = FLinearColor::Black;
    UPROPERTY() FLinearColor NewVector = FLinearColor::Black;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRIOnToast, ERIToastType, Type, const FString&, Message, float, Duration);

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
	//Modified ���
public:
    enum class ERIVisiblePage : uint8
    {
        Actor,
        Changes,
        Settings,
        Tools
    };

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Modified")
    bool RevertModifiedForSelection(int32& OutRevertedCount, int32& OutFailedCount, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    bool IsItemModified(UObject* Item) const;

	// Toast ֪ͨ
public:
    UPROPERTY(BlueprintAssignable, Category = "RuntimeInspector|Toast")
    FRIOnToast OnToast;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Toast")
    void PushToast(ERIToastType Type, const FString& Message, float Duration = 1.5f);

public:
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|SelfTest")
    TArray<FRISelfTestDefinition> GetAvailableSelfTests() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|SelfTest")
    bool RunSelfTestById(FName TestId, FRISelfTestResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|SelfTest")
    void RunAllSelfTests(TArray<FRISelfTestResult>& OutResults);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|SelfTest")
    TArray<FRISelfTestResult> GetLastSelfTestResults() const { return LastSelfTestResults; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Verification")
    TArray<FRIVerificationProfile> GetAvailableVerificationProfiles() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Verification")
    bool RunVerificationProfile(FName ProfileId, FRIVerificationRunResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Verification")
    FRIVerificationRunResult GetLastVerificationRunResult() const { return LastVerificationRunResult; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Workflow")
    TArray<FRIWorkflowDefinition> GetAvailableWorkflows() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Workflow")
    bool RunWorkflowById(FName WorkflowId, FRIWorkflowRunResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Workflow")
    FRIWorkflowRunResult GetLastWorkflowRunResult() const { return LastWorkflowRunResult; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|WorkflowMatrix")
    TArray<FRIWorkflowMatrixDefinition> GetAvailableWorkflowMatrices() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|WorkflowMatrix")
    bool RunWorkflowMatrixById(FName MatrixId, FRIWorkflowMatrixRunResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|WorkflowMatrix")
    FRIWorkflowMatrixRunResult GetLastWorkflowMatrixRunResult() const { return LastWorkflowMatrixRunResult; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    bool RunValidationCaptureScenario(FName ScenarioId, FRIValidationCaptureReport& OutReport);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Validation")
    FRIValidationCaptureReport GetLastValidationCaptureReport() const { return LastValidationCaptureReport; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    bool ExportLastValidationCaptureReport(FString& OutFilePath, FString& OutError) const;

    void RecordValidationCaptureMetric(const FString& MetricName, double ValueMs, const FString& Details = FString());
    void AppendValidationCaptureLogLine(const FString& LogLine);
    bool IsValidationCaptureActive() const { return bValidationCaptureActive; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    bool RunTransformSourcePersistencePrepareSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    FString RunTransformSourcePersistencePrepareSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    bool RunTransformSourcePersistenceVerifyRestoreSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Validation")
    FString RunTransformSourcePersistenceVerifyRestoreSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunConfirmDialogColorInputSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunColorPickerUIContractSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunConfirmDialogColorInputSelfTestSimple();

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Debug")
    FString GetLastConfirmDialogSelfTestReport() const { return LastConfirmDialogSelfTestReport; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Debug")
    bool GetLastConfirmDialogSelfTestPassed() const { return bLastConfirmDialogSelfTestPassed; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunSettingsPreviewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunSettingsPreviewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunSettingsSavePersistenceSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunSettingsSavePersistenceSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunStarLiveEditAndRunSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunStarLiveEditAndRunSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunStarPreciseNavigationSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunStarPreciseNavigationSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunSettingsHotkeyRebindSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunSettingsHotkeyRebindSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunSettingsPageLayoutSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunSettingsPageLayoutSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunThemePresetPreviewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunThemePresetPreviewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPatchPresetRoundtripSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPatchPresetRoundtripSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPromotePreviewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPromotePreviewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPromoteConfigSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPromoteConfigSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPromoteBlueprintApplySelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPromoteBlueprintApplySelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPromoteMaterialApplySelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPromoteMaterialApplySelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunAuditReportSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunAuditReportSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFilePageInjectionSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFilePageInjectionSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunContextStripSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunContextStripSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunWorkflowPageViewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunWorkflowPageViewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFileWorkflowSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFileWorkflowSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFilePromoteWorkflowSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFilePromoteWorkflowSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunActorPromoteFileWorkflowSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunActorPromoteFileWorkflowSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunActorApplyFileWorkflowSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunActorApplyFileWorkflowSelfTestSimple();

    bool RunFileCompareViewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFileRoleCompareViewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFileRoleCompareViewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFileRemoteSessionCompareViewSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFileRemoteSessionCompareViewSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunTestPageLayoutSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunPanelInteractionSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunPanelInteractionSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRuntimeSessionRoleSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRuntimeSessionRoleSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRuntimeRoleCompareSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRuntimeRoleCompareSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemoteRuntimeFoundationSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemoteRuntimeFoundationSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemoteSessionCompareSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemoteSessionCompareSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemoteSessionTargetSetCompareSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemoteSessionTargetSetCompareSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemoteSessionTargetSetCompareMatrixSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemoteSessionTargetSetCompareMatrixSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemoteSessionContextUISelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemoteSessionContextUISelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemotePackagedFoundationSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemotePackagedFoundationSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemotePackagedPatchPullSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemotePackagedPatchPullSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunRemotePackagedToSourceClosureSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunRemotePackagedToSourceClosureSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotFoundationSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotFoundationSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotActorPageSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotActorPageSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotSettingsPageSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotSettingsPageSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotToolsPageSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotToolsPageSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotRemoteSessionSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotRemoteSessionSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunFabScreenshotPromoteOrAuditSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunFabScreenshotPromoteOrAuditSelfTestSimple();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool ApplyFabScreenshotStateByName(const FString& InShotName, FString& OutSummary, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|UI")
    bool SetVisiblePageByName(const FString& InPageName, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    bool RunWorkflowMatrixSelfTest(FString& OutReport);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Debug")
    FString RunWorkflowMatrixSelfTestSimple();

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Session")
    FRIRuntimeSessionSummary GetRuntimeSessionSummary() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Session")
    FRIRuntimeActorRoleSummary GetSelectedActorRoleSummary() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Session")
    bool CompareRuntimeRoles(FRIRuntimeRoleCompareReport& OutReport, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Session")
    FRIRuntimeRoleCompareReport GetLastRuntimeRoleCompareReport() const { return LastRuntimeRoleCompareReport; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionInfo> GetAvailableRuntimeSessions() const;

    TArray<FRIRuntimeSessionInfo> QueryAvailableRuntimeSessions(bool bForceRefresh, bool bAllowExternalProbe) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool ConnectRemoteRuntimeSession(const FString& SessionId, FRIRuntimeSessionInfo& OutSession, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool ListRuntimeTargetsForSession(const FString& SessionId, TArray<FRIRuntimeTargetInfo>& OutTargets, FString& OutError, const FString& NameFilter, const FString& ClassFilter, int32 Limit) const;

    bool TryGetCachedRuntimeTargetQueryResult(
        const FString& SessionId,
        const FString& NameFilter,
        const FString& ClassFilter,
        int32 Limit,
        TArray<FRIRuntimeTargetInfo>& OutTargets,
        FString& OutError,
        bool& bOutSuccess) const;

    bool RefreshRuntimeTargetQueryResult(
        const FString& SessionId,
        const FString& NameFilter,
        const FString& ClassFilter,
        int32 Limit,
        TArray<FRIRuntimeTargetInfo>& OutTargets,
        FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool CompareRuntimeTargetsAcrossSessions(const FString& LeftSessionId, const FString& RightSessionId, const FString& TargetQuery, FRIRuntimeSessionTargetCompareReport& OutReport, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool CompareRuntimeTargetSetsAcrossSessions(const FString& LeftSessionId, const FString& RightSessionId, const FString& NameFilter, const FString& ClassFilter, FRIRuntimeSessionTargetSetCompareReport& OutReport, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool PullPatchBundleFromRuntimeSession(const FString& SessionId, const FString& ActorQuery, FRIPatchBundle& OutBundle, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool RunWorkflowOnRuntimeSession(const FString& SessionId, FName WorkflowId, const FString& ActorQuery, FRIWorkflowRunResult& OutResult, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    TArray<FRIRuntimeSessionTargetSetCompareMatrixDefinition> GetAvailableRuntimeSessionTargetSetCompareMatrices() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    bool RunRuntimeSessionTargetSetCompareMatrixById(FName MatrixId, FRIRuntimeSessionTargetSetCompareMatrixRunResult& OutResult);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetConnectedRuntimeSessionId() const { return ConnectedRuntimeSessionId; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetPreferredRemoteSessionId() const { return PreferredRemoteSessionId; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetLastRemoteRuntimeSessionError() const { return LastRemoteRuntimeSessionError; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetLastRemoteSessionSelectionSummary() const { return LastRemoteSessionSelectionSummary; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetLastRemoteSessionTargetQuery() const { return LastRemoteSessionTargetQuery; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FString GetLastRemoteSessionWorkflowId() const { return LastRemoteSessionWorkflowId; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Remote")
    void SetRemoteSessionUIContext(const FString& SessionId, const FString& SelectionSummary, const FString& TargetQuery, const FString& WorkflowId);

    bool TryResolvePackagedRuntimeValidationSession(FRIRuntimeSessionInfo& OutSession, FString& OutError, bool bForceRefresh = false) const;
    bool EnsurePackagedRuntimeValidationSession(FRIRuntimeSessionInfo& OutSession, FString& OutError);
    bool StopPackagedRuntimeValidationSession(FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetCompareReport GetLastRuntimeSessionTargetCompareReport() const { return LastRuntimeSessionTargetCompareReport; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetSetCompareReport GetLastRuntimeSessionTargetSetCompareReport() const { return LastRuntimeSessionTargetSetCompareReport; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Remote")
    FRIRuntimeSessionTargetSetCompareMatrixRunResult GetLastRuntimeSessionTargetSetCompareMatrixRunResult() const { return LastRuntimeSessionTargetSetCompareMatrixRunResult; }

    void SetActiveRemoteSessionTargetSetCompareRequest(const FRIRuntimeSessionTargetSetCompareRequest& InRequest);
    void ClearActiveRemoteSessionTargetSetCompareRequest();
    FRIRuntimeSessionTargetSetCompareRequest GetActiveRemoteSessionTargetSetCompareRequest() const { return ActiveRemoteSessionTargetSetCompareRequest; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Settings")
    FRIEditableSettings GetEditableSettings() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Settings")
    FRISettingsDiagnostics GetSettingsDiagnostics() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    bool PreviewApplySettings(const FRIEditableSettings& InSettings, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Settings")
    ERuntimeInspectorThemePreset GetThemePreset() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    bool PreviewApplyThemePreset(ERuntimeInspectorThemePreset InPreset, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    bool SaveSettings(FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    void ReloadSettingsFromConfig();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Settings")
    bool ValidateHotkeyCandidate(FKey InKey, FString& OutError) const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Settings")
    bool HasUnsavedSettingsChanges() const { return bSettingsDirty; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    bool IsRIEnabled() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    FString GetRIDisabledReason() const;

    // ===== Security: optional unlock gate =====
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    bool IsRIUnlocked() const;

    // Unlock via code (can be empty if settings UnlockCode is empty). Returns false with OutError on failure.
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Security")
    bool UnlockRI(const FString& Code, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Security")
    void LockRI();

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    FString GetRIUnlockHint() const;

    // UWorldSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Tickable
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UInspectorWorldSubsystem, STATGROUP_Tickables); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void Toggle();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void Open();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void Close();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|UI")
    void RefreshSharedContextStrip();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsOpen() const { return bOpen; }

    const TArray<FRIActivityLogEntry>& GetActivityLogEntries() const { return ActivityLogEntries; }
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    int32 GetActivityLogEntryCountForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetLastActivityLogSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    int32 GetActorGroupsEntryCountForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    int32 GetActorPinnedEntryCountForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    int32 GetActorPropertyRowCountForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    int32 GetActorFunctionRowCountForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetActorPropertyHostDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    bool HasActorGroupsTouchScrollSupportForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetActorPropertyAnchorChainForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetInspectBodyChildrenDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetPanelPresentationDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetPageRoutingDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetActorTopContextValueDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetActorFooterDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetContextLabelAnchorChainsForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetPanelHostWindowDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetDockLayoutDebugSummaryForAutomation() const;
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Automation")
    FString GetLastPickDebugSummaryForAutomation() const;
    void ClearActivityLog();
    void AppendActivityLog(ERIToastType Severity, const FString& Category, const FString& Message);
    bool HandlePanelMouseButtonDown(const FPointerEvent& MouseEvent);
    bool HandlePanelMouseMove(const FPointerEvent& MouseEvent);
    bool HandlePanelMouseButtonUp(const FPointerEvent& MouseEvent);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void PickActorInView(); // Legacy wrapper: current behavior is cursor-based actor pick.


    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    AActor* GetSelectedActor() const { return SelectedActor.Get(); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void SetSelectedActor(AActor* NewActor);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetPropertyItemsForSelected(const FString& SearchText, TArray<UObject*>& OutItems);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetGroupItemsForSelected(const FString& SearchText, TArray<UObject*>& OutGroups);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetPinnedItemsForSelected(const FString& SearchText, TArray<UObject*>& OutPinnedItems);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool CanUndo() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool CanRedo() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool Undo();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool Redo();

    // �� PropertyItem ����
    void RecordChange(const FInspectorChange& Change);

    void OnUndoKeyPressed();
    void OnRedoKeyPressed();

    bool IsInspectorOpen() const { return bInspectorOpen; }


    // >>> ADD���۵�״̬���ƣ�BP ���������ã�
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Groups")
    void ToggleGroupExpanded(const FString& GroupKey, bool bDefault);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Groups")
    bool GetGroupExpanded(const FString& GroupKey, bool bDefault = true) const;

    bool GetPropertyCategoryExpanded(const UObject* TargetObject, const FString& PrimaryCategory, bool bDefault = true) const;
    void SetPropertyCategoryExpanded(const UObject* TargetObject, const FString& PrimaryCategory, bool bExpanded);
    FString BuildPropertyCategoryStateKey(const UObject* TargetObject, const FString& PrimaryCategory) const;
    void GetActorWorldTransformPropertyItems(TArray<UInspectorPropertyItem*>& OutItems);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|UI")
    void RequestActorPageRefresh();

    bool IsApplyingHistory() const { return bApplyingHistory; }

    UMaterialInstanceDynamic* GetOrCreateMID(UPrimitiveComponent* Comp, int32 MaterialIndex);


    // ===== Favorites (Pin) =====
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Favorites")
    bool IsFavoriteForItem(UInspectorPropertyItem* Item) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Favorites")
    bool IsFavoriteForAnyItem(UObject* Item) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Favorites")
    void ToggleFavoriteForItem(UInspectorPropertyItem* Item);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|UI")
    bool OpenColorEditorForAnyItem(UObject* ItemObject);

    // ===== Snapshot (Modified / Export / Import) =====
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    int32 GetModifiedCount() const { return ModifiedValueByKey.Num(); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    void ClearModified();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool ExportSnapshot(bool bOnlyModified, FString& OutFilePath, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool ImportSnapshot(const FString& InFilePath, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool CaptureSelectionAsPatch(FRIPatchBundle& OutBundle, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool StageSelectionAsPatch(FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    void ClearStagedPatch();

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Patch")
    bool HasStagedPatch() const { return bHasStagedPatch && StagedPatchBundle.Operations.Num() > 0; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Patch")
    FRIPatchBundle GetStagedPatch() const { return StagedPatchBundle; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool StagePatchBundle(const FRIPatchBundle& InBundle, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ApplyStagedPatch(FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool RollbackStagedPatch(FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool SavePatchPreset(const FRIPatchPresetMetadata& InMetadata, FString& OutFilePath, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ListPatchPresets(TArray<FRIPatchPresetMetadata>& OutPresets, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool LoadPatchPreset(const FString& InPresetIdOrPath, FRIPatchPresetMetadata& OutMetadata, FRIPatchBundle& OutBundle, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool DeletePatchPreset(const FString& InPresetIdOrPath, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ApplyPatchPreset(const FString& InPresetIdOrPath, FRIApplyResult& OutResult, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ExportPatchBundle(const FRIPatchBundle& InBundle, FString& OutFilePath, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ImportPatchBundle(const FString& InFilePath, FRIPatchBundle& OutBundle, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool ApplyPatchBundle(const FRIPatchBundle& InBundle, FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Patch")
    bool RollbackPatchBundle(const FRIPatchBundle& InBundle, FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Promote")
    bool CreatePromotePreview(const FRIPatchBundle& InBundle, FRIPromotePreview& OutPreview, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Promote")
    bool PromotePatchToSource(const FRIPatchBundle& InBundle, FRIPromoteResult& OutResult, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Audit")
    bool BuildAuditReport(ERIAuditComparisonMode InMode, const FRIPatchBundle& InBundle, FRIAuditReport& OutReport, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    FRIAuditReport GetLastAuditReport() const { return LastAuditReport; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    bool GetCachedAuditReport(ERIAuditComparisonMode InMode, FRIAuditReport& OutReport) const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    TArray<ERIAuditComparisonMode> GetAvailableCachedAuditModes() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Audit")
    void SetActiveFileAuditViewMode(ERIAuditComparisonMode InMode);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    ERIAuditComparisonMode GetActiveFileAuditViewMode() const { return ActiveFileAuditMode; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    FString GetLastAuditReportAsText() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Audit")
    bool ExportLastAuditReportToFile(bool bAsJson, FString& OutFilePath, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool GetFileManagementSummary(FRIFileManagementSummary& OutSummary, FString& OutError) const;

    bool TryGetCachedFileManagementSummary(FRIFileManagementSummary& OutSummary, FString& OutError) const;
    bool RebuildFileManagementSummaryCache(FRIFileManagementSummary& OutSummary, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    void ListPatchBundleFiles(TArray<FString>& OutFiles) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    void ListAuditReportFiles(TArray<FString>& OutFiles) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileStagePatchAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileExportPatchAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileSavePresetAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileApplyLatestPresetAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildBaselineAuditAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildAuditAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildPatchVsSourceAuditAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildAppliedPatchVsSourceAuditAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildRuntimeRoleCompareAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileBuildRemoteSessionTargetSetCompareAction(FString& OutSummary, FString& OutDetails);

    bool ExecuteFileBuildRemoteSessionTargetSetCompareAction(const FRIRuntimeSessionTargetSetCompareRequest& InRequest, FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFilePullPatchFromRemoteSessionAction(const FString& SessionId, const FString& ActorQuery, FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileRunRemoteWorkflowAction(const FString& SessionId, FName WorkflowId, const FString& ActorQuery, FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFilePromotePreviewAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFilePromoteApplyAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|File")
    bool ExecuteFileClearStagedAction(FString& OutSummary, FString& OutDetails);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Patch")
    FString GetPatchesDirectory() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Patch")
    FString GetPresetsDirectory() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Audit")
    FString GetAuditReportsDirectory() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Patch")
    FRIApplyResult GetLastPatchApplyResult() const { return LastPatchApplyResult; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Promote")
    FRIPromoteResult GetLastPromoteResult() const { return LastPromoteResult; }

    // Last snapshot import report (cached for UI)
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    FRIImportReport GetLastImportReport() const { return LastImportReport; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    void ClearLastImportReport();

    // Build a human-readable text block for the cached import report (useful for UI, copy, export).
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    FString GetLastImportReportAsText(bool bIncludeDetails = true, bool bIncludeLists = true) const;

    // Copy the cached import report text to clipboard.
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool CopyLastImportReportToClipboard(FString& OutCopiedText);

    // Export the cached import report to a file under Saved/RuntimeInspector/Reports.
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool ExportLastImportReportToFile(bool bAsJson, FString& OutFilePath, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    FString GetImportReportsDirectory() const;

    // Baseline (v0.2): capture current values as the session baseline.
    // This resets Modified/Baseline maps and records current values for properties/material params
    // that RuntimeInspector can write.
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Baseline")
    void CaptureBaselineForSelection(bool bIncludeMaterialParams = true);

    // Apply (v0.2): optional debounced apply to avoid spamming SetValue calls (sliders, drag, etc.)
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Apply")
    bool RequestApplyPropertyText(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Apply")
    bool ApplyPropertyTextImmediate(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    bool IsPropertyItemModified(const UInspectorPropertyItem* Item) const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    bool IsMaterialItemModified(const UInspectorMaterialParamItem* Item) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetPropertyItemsForSelectedEx(const FString& SearchText, bool bOnlyModified, TArray<UObject*>& OutItems);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    UObject* GetFocusedInspectObject() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Selection")
    bool FocusSelectedActorComponentByName(const FString& ComponentName, FString& OutError);
    bool FocusSelectedActorComponentByNameWithRefreshPolicy(const FString& ComponentName, FString& OutError, bool bRefreshPanel);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Selection")
    bool NavigateToPinnedItem(UObject* ItemObject, FString& OutError);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FString GetCurrentActorSearchText() const { return CurrentActorSearchText; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    FString GetSelectedGroupKeyForAutomation() const { return SelectedGroupKey; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector")
    ERIPropertyViewMode GetPropertyViewModeForAutomation() const { return PropertyViewMode; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetFunctionItemsForSelected(const FString& SearchText, TArray<UInspectorFunctionItem*>& OutItems);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool InvokeFunctionItem(UInspectorFunctionItem* Item, const TArray<FString>& InArgTexts, FString& OutError);

    URuntimeInspectorController* GetOrCreateRuntimeInspectorController();
    void RegisterDockHostedPages(UInspectorFilePageWidget* InFilePage, UInspectorSettingsPageWidget* InSettingsPage, UInspectorTestPageWidget* InTestPage);
    void RegisterDockHostedActorSections(UInspectorPropertiesSectionWidget* InPropertiesSection, UInspectorFunctionsSectionWidget* InFunctionsSection);

    void RefreshActorPropertyValue(UObject* TargetObject, FName PropertyName, bool bAllowSectionFallback = true);

    UFUNCTION()
    void HandleActorSearchTextChanged(const FText& InText);


private:
    APlayerController* GetLocalPC() const;
    FRIRuntimeActorRoleSummary BuildActorRoleSummary(AActor* InActor) const;
    bool ApplyChange(const FInspectorChange& Change, bool bUseNewValue);
    UInspectorFilePageWidget* CreateFilePageWidgetInstance();
    UInspectorSettingsPageWidget* CreateSettingsPageWidgetInstance();
    UInspectorTestPageWidget* CreateTestPageWidgetInstance();
    void OpenToPage(ERIVisiblePage InitialPage);
    void EnsureDockRootWidget();
    void RefreshDockRootWidget();
    bool IsDockRootActive() const;
    void EnsurePanelWidget();
    void ResetPanelWidgetRuntimeState();
    void ReleasePanelWidgetForRecreate();
    void EnsureFilePageInjected();
    void EnsureSettingsPageInjected();
    void EnsureTestPageInjected();
    void BindPanelTabButtons();
    void EnsureLegacySupplementalTabsAndHosts();
    void ShowFilePage();
    void ShowSettingsPage();
    void ShowTestPage();
    void HideSettingsPage();
    void EnsureSharedContextStripInjected();
    void HideSharedContextStripForActorPage();
    void UpdateSharedContextStrip();
    void EnsureActorTopContextStripInjected();
    void UpdateActorTopContextStrip();
    void BuildContextStripDisplayState(FString& OutActorLabel, FString& OutActorClass, FString& OutSourcePath, FString& OutStagedState, bool& bOutActorMissing, bool& bOutHasStagedPatch) const;
    UWidgetSwitcher* FindContentSwitcher() const;
    UPanelWidget* FindFileHostPanel() const;
    UPanelWidget* FindSettingsHostPanel() const;
    UPanelWidget* FindTestHostPanel() const;
    void MountPageAsExclusiveChild(UPanelWidget* HostPanel, UWidget* PageWidget);
    void RestoreMountedHostPanelVisibility();
    void SetContentSwitcherIndex(int32 InIndex);
    ERIVisiblePage GetVisiblePage() const;
    void ScheduleThemePreviewRefresh(ERIVisiblePage RestorePage);
    void CancelThemePreviewRefresh();
    void HandleThemePreviewRefreshTimerElapsed();
    void ScheduleDeferredOpenActorRefresh(bool bCaptureBaseline);
    void CancelDeferredOpenActorRefresh();
    void HandleDeferredOpenActorRefreshTimerElapsed();
    void MaybePrecreatePanelWidget();
    void MaybePrecreateSecondaryPageWidgets();
    void MaybePrecreateFilePageWidget();
    void MaybePrecreateSettingsPageWidget();
    void EnsureActorWorkbenchBodyInjected();
    void PrepareActorPageForPresentation(bool bForceSlateLayout);
    void MarkActorPageStructureDirty();
    FString BuildActorPageStructureKey() const;
    bool IsActorOnlyModifiedFilterEnabled() const;
    void PrimeActorPageForInitialOpen();
    void EnsureActorSplitSidebarPanelInjected();
    void RestoreActorSinglePanelBodyLayout();
    bool ShouldUseActorSplitPresentation(float& OutLeftWidth, float& OutRightWidth, float& OutCenterClear) const;
    void ApplyActorSplitPresentation();
    void EnsureActorGroupsSectionInjected();
    void RefreshActorGroupsSection();
    void ApplySelectedActorRootState(AActor* ActorPtr = nullptr);
    void FocusSelectedActorRoot(EInspectorRefreshReason Reason = EInspectorRefreshReason::StructureChanged);
    bool IsActorRootSelectionActive() const;
    void UpdateActorGroupsHeaderState();
    void EnsureActorWorkspaceSelectionBandInjected();
    void UpdateActorWorkspaceSelectionBand();
    void EnsureActorPropertiesSectionInjected();
    void RefreshActorPropertiesSection(bool bRebuildRows = true);
    void RefreshActorValuePresentation();
    void EnsureActorFunctionsSectionInjected();
    void RefreshActorFunctionsSection();
    void CacheActorPageSearchTextFromPanel();
    void BindActorSearchBox();
    void UpdatePanelTabButtonStyles();
    UButton* FindPanelTabButtonByText(const FString& DesiredText) const;
    UButton* FindPanelTabButtonByTexts(std::initializer_list<const TCHAR*> DesiredTexts) const;
    void SetPanelTabButtonLabel(UButton* Button, const FString& NewLabel) const;
    bool IsSelfTestPIEAvailable() const;
    FString BuildSelfTestSummary(const FString& FullReport) const;
    FRISelfTestResult MakeSelfTestResult(const FRISelfTestDefinition& Definition, bool bPassed, const FString& FullReport, int32 DurationMs) const;
    void LogToolsDefinitionIssue(const FString& Message) const;
    TArray<FRISelfTestTableRow> LoadConfiguredSelfTestRows() const;
    TArray<FRIWorkflowTableRow> LoadConfiguredWorkflowRows() const;
    FRISelfTestDefinition MakeSelfTestDefinitionFromTableRow(const FRISelfTestTableRow& Row, bool bPIEAvailable) const;
    FRIWorkflowDefinition MakeWorkflowDefinitionFromTableRow(const FRIWorkflowTableRow& Row) const;
    const FRISelfTestTableRow* FindConfiguredSelfTestRow(FName TestId, const TArray<FRISelfTestTableRow>& Rows) const;
    const FRIWorkflowTableRow* FindConfiguredWorkflowRow(FName WorkflowId, const TArray<FRIWorkflowTableRow>& Rows) const;
    bool ExecuteToolAction(
        const FRIToolActionDefinition& Action,
        const TArray<FRISelfTestTableRow>& SelfTestRows,
        const TArray<FRIWorkflowTableRow>& WorkflowRows,
        bool& bOutPassed,
        bool& bOutBlocked,
        FString& OutSummary,
        FRIWorkflowRunResult* OutWorkflowResult = nullptr,
        TArray<FRIVerificationRunResult>* OutVerificationResults = nullptr,
        TArray<FRISelfTestResult>* OutSelfTestResults = nullptr,
        TArray<FName>* OutExecutedChildWorkflowIds = nullptr,
        TArray<FString>* OutExecutedChildWorkflowSummaries = nullptr);
    bool ExecuteToolActionSequence(
        const TArray<FRIToolActionDefinition>& Actions,
        const TArray<FRISelfTestTableRow>& SelfTestRows,
        const TArray<FRIWorkflowTableRow>& WorkflowRows,
        int32& OutPassedStepCount,
        int32& OutFailedStepCount,
        bool& bOutBlocked,
        TArray<FString>& OutReportSections,
        FRIWorkflowRunResult* OutWorkflowResult = nullptr,
        TArray<FRIVerificationRunResult>* OutVerificationResults = nullptr,
        TArray<FRISelfTestResult>* OutSelfTestResults = nullptr,
        TArray<FName>* OutExecutedChildWorkflowIds = nullptr,
        TArray<FString>* OutExecutedChildWorkflowSummaries = nullptr);
    bool ExecuteLegacyToolNativeBridgeAction(FName BridgeId, FString& OutReport, bool& bOutPassed);
    bool ExecuteSelfTestByIdInternal(FName TestId, FString& OutReport, bool& bOutPassed);
    bool RunChangesFirstOpenValidationCapture(FRIValidationCaptureReport& OutReport);
    void BeginValidationCapture(FName ScenarioId);
    void CaptureValidationStateSnapshot(FRIValidationCaptureReport& OutReport) const;
    bool CaptureValidationScreenshot(const FString& ScreenshotBaseName, FString& OutPath, FString& OutError);
    bool SaveValidationCaptureReport(FRIValidationCaptureReport& InOutReport, FString& OutError) const;
    FString BuildValidationCaptureSummaryText(const FRIValidationCaptureReport& Report) const;
    FString GetValidationCaptureRootDir() const;
    FString GetValidationCaptureScenarioDir(const FString& CaptureId) const;
    FString GetTransformSourcePersistenceRootDir() const;
    FString GetTransformSourcePersistencePendingPath() const;
    FString GetTransformSourcePersistenceScenarioDir(const FString& CaptureId) const;
    bool SaveTransformSourcePersistenceCheckpoint(const FRITransformSourcePersistenceCheckpoint& InCheckpoint, FString& OutPath, FString& OutError) const;
    bool LoadTransformSourcePersistenceCheckpoint(FRITransformSourcePersistenceCheckpoint& OutCheckpoint, FString& OutPath, FString& OutError) const;
    bool SaveTransformSourcePersistenceReport(FRITransformSourcePersistenceReport& InOutReport, FString& OutError) const;
    bool ApplyFabScreenshotFoundationState(FString& OutSummary, FString& OutError);
    bool ApplyFabRemoteSessionScreenshotState(FString& OutSummary, FString& OutError);
    bool ApplyFabPromoteOrAuditScreenshotState(FString& OutSummary, FString& OutError);
    bool ApplyFabScreenshotViewportResolution(FString& OutError);
    bool ApplyFabScreenshotApplicationScale(float InScale, FString& OutError);
    void RestoreFabScreenshotApplicationScale();
    bool ApplyFabScreenshotPanelTransform(FString& OutError);
    void RestoreFabScreenshotPanelTransform();
    bool RunActorPageStructureSelfTest(FString& OutReport);
    bool RunFabScreenshotPageSelfTest(const FString& InPageName, ERIVisiblePage ExpectedPage, const FString& InTestLabel, FString& OutReport);
    AActor* ResolvePreferredFabScreenshotActor() const;
    void RefreshConfirmDialogBinding();
    void ClearConfirmDialogBinding();
    bool TryBindActiveConfirmDialog(UUserWidget* DialogWidget);
    bool TryBindActiveColorPicker(UInspectorColorPickerWidget* PickerWidget);
    void ActivateConfirmDialogModalState(UUserWidget* DialogWidget);
    void DeactivateConfirmDialogModalState();
    bool TryActivateConfirmDialogColorPage(UUserWidget* DialogWidget) const;
    bool IsConfirmDialogColorPageActive(UUserWidget* DialogWidget) const;
    bool TryGetActiveConfirmDialogColor(FLinearColor& OutColor) const;
    bool TrySetActiveConfirmDialogColor(const FLinearColor& InColor);
    bool ApplyActiveConfirmDialogColor(const FLinearColor& InColor);
    bool TryBuildActiveConfirmDialogColorFromInputs(FLinearColor& OutColor) const;
    void RefreshActiveConfirmDialogColor();
    bool TryParseConfirmDialogUnitFloat(const FText& InText, float CurrentValue, float& OutValue) const;
    bool TryParseConfirmDialogHexColor(const FText& InText, FLinearColor& OutColor) const;
    bool TryGetInspectorItemColor(UObject* ItemObject, FLinearColor& OutColor) const;
    bool ApplyInspectorItemColor(UObject* ItemObject, const FLinearColor& InColor, FString& OutError);
    bool ApplyInspectorItemColorInternal(UObject* ItemObject, const FLinearColor& InColor, FString& OutError, bool bSuppressHistory);
    bool OpenColorEditorForItemInternal(UObject* ItemObject);
    void ApplyActiveColorEditItemIfNeeded(const FLinearColor& InColor);
    void RememberRecentColorPickerColor(const FLinearColor& InColor);
    void SyncActiveConfirmDialogColorPreview();
    void FinalizeActiveColorEdit(bool bAccept);
    void ApplyActiveConfirmDialogChannels();
    void HandleConfirmDialogNumericTextChanged(UEditableTextBox* SourceTextBox, int32 ChannelIndex, const FText& InText);
    void HandleConfirmDialogHexTextChanged(const FText& InText);

    UFUNCTION()
    void HandleActorComponentsHeaderClicked();

    UFUNCTION()
    void HandleActiveConfirmDialogAccepted();

    UFUNCTION()
    void HandleActiveConfirmDialogCanceled();

    UFUNCTION()
    void HandleActiveColorPickerPreviewChanged(FLinearColor InColor);

    UFUNCTION()
    void HandleActiveColorPickerAccepted(FLinearColor InColor);
    //void RefreshPanel();

    // <<< ADD���۵�״̬����
    UPROPERTY()
    TMap<FString, bool> GroupExpandedMap;
    TMap<FString, bool> PropertyCategoryExpandedMap;

private:
    bool bOpen = false;
    bool bInputsBound = false;
    bool bInspectorOpen = false;
    bool bUnlocked = false;

    bool IsUnlockRequired() const;

    FString LastImportedSnapshotPath;

    TSharedPtr<FRuntimeInspectorInputProcessor> InputProcessor;

    void RegisterInputProcessor();
    void UnregisterInputProcessor();

    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorRecoveryPath;
    FString SelectedActorRecoveryBaseName;
    FString SelectedActorRecoveryClassPath;
    FString SelectedActorRecoveryDisplayLabel;
    FString SelectedActorRecoveryClassDisplayLabel;
    FString LastPickDebugSummary = TEXT("Source=None Hit=0 Actor=None Component=None Reason=Uninitialized");
    bool bSelectedActorRecoveryPending = false;
    float SelectedActorRecoveryWaitSeconds = 0.f;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> PanelWidgetStrong = nullptr;

    TWeakObjectPtr<UUserWidget> PanelWidget;

    UPROPERTY(Transient)
    TObjectPtr<UInspectorDockRootWidget> DockRootWidgetStrong = nullptr;

    TWeakObjectPtr<UInspectorDockRootWidget> DockRootWidget;

    UPROPERTY(Transient)
    TObjectPtr<URuntimeInspectorController> RuntimeInspectorController = nullptr;

    TWeakObjectPtr<UUserWidget> ActiveConfirmDialogWidget;
    TWeakObjectPtr<UInspectorModalBlockerWidget> ActiveConfirmDialogModalBlockerWidget;
    TWeakObjectPtr<UObject> ActiveColorEditItem;
    TWeakObjectPtr<UButton> ActiveConfirmDialogYesButton;
    TWeakObjectPtr<UButton> ActiveConfirmDialogNoButton;

    TWeakObjectPtr<UEditableTextBox> ActiveConfirmDialogInputR;
    TWeakObjectPtr<UEditableTextBox> ActiveConfirmDialogInputG;
    TWeakObjectPtr<UEditableTextBox> ActiveConfirmDialogInputB;
    TWeakObjectPtr<UEditableTextBox> ActiveConfirmDialogInputA;
    TWeakObjectPtr<UEditableTextBox> ActiveConfirmDialogInputHex;

    UPROPERTY(Transient)
    TArray<FLinearColor> RecentColorPickerColors;

    UPROPERTY()
    TSoftClassPtr<UUserWidget> ConfirmDialogWidgetClass;

    UPROPERTY()
    TSoftClassPtr<UInspectorFilePageWidget> FilePageWidgetClass;

    UPROPERTY()
    TSoftClassPtr<UInspectorSettingsPageWidget> SettingsPageWidgetClass;

    UPROPERTY()
    TSoftClassPtr<UInspectorTestPageWidget> TestPageWidgetClass;

    float ConfirmDialogBindAccum = 0.f;
    float ConfirmDialogPreviewAccum = 0.f;
    bool bUpdatingConfirmDialogText = false;
    bool bApplyingColorDialogPreview = false;
    bool bActiveColorEditPreviewDirty = false;
    bool bActiveColorEditCanceled = false;
    bool bHasActiveColorEditOriginalColor = false;
    bool bHasActiveColorEditLastPreviewColor = false;
    FLinearColor ActiveColorEditOriginalColor = FLinearColor::Black;
    FLinearColor ActiveColorEditLastPreviewColor = FLinearColor::Black;

    // ����Ժ��������������Ӳ����һ��Ĭ��·��
    UPROPERTY()
    TSoftClassPtr<UUserWidget> PanelWidgetClass;

    UPROPERTY() TArray<FInspectorChange> UndoStack;
    UPROPERTY() TArray<FInspectorChange> RedoStack;


    bool bApplyingHistory = false;

    bool ApplyChangeValue(UObject* Target, FName PropName, const FString& TextValue);



    // >>> ADD������������ Key / ������ / �ռ�����
    static FString MakeComponentKey(const AActor* Actor, const UActorComponent* Comp);
    static bool IsWhitelistedComponent(const UActorComponent* Comp);

    void AppendPropertiesForObject(
        UObject* TargetObject,
        const FString& SearchText,
        TArray<UObject*>& OutItems,
        const FString& OwnerPrefixForUI,
        bool bSearchMode
    );
    bool DoesPropertyNameMatchSearch(const FString& PropertyName, const FString& SearchText) const;

    static bool IsSupportedByInspector(const FProperty* Prop);
    static bool NameMatchesSearch(const FString& Name, const FString& SearchText);


	// Lifecycle
    private:
        // --- �������ڰ� ---
        void BindToSelectedActor(AActor* Actor);
        void UnbindFromSelectedActor();

        UFUNCTION()
        void HandleSelectedActorDestroyed(AActor* DestroyedActor);

        // --- Item ���óأ�����ÿ�� GetPropertyItemsForSelected �� NewObject ---
        UPROPERTY(Transient)
        TMap<FString, TObjectPtr<UObject>> ItemPool;

        void ClearItemPool();

        UInspectorGroupItem* GetOrCreateGroupItem(const FString& Key);
        UInspectorPropertyItem* GetOrCreatePropertyItem(UObject* TargetObject, FName PropertyName);
        UInspectorMaterialParamItem* GetOrCreateMaterialItem(UMeshComponent* Comp, int32 Slot, FName ParamName, EInspectorMatParamType Type);
        UInspectorFunctionItem* GetOrCreateFunctionItem(UObject* TargetObject, FName FunctionName);

        // --- ������������У�飨Tick���ã�---
        float ValidateAccum = 0.f;
        void ValidateSelection();

        UFUNCTION()
        void HandleConfirmDialogRChanged(const FText& InText);

        UFUNCTION()
        void HandleConfirmDialogGChanged(const FText& InText);

        UFUNCTION()
        void HandleConfirmDialogBChanged(const FText& InText);

        UFUNCTION()
        void HandleConfirmDialogAChanged(const FText& InText);

        UFUNCTION()
    void HandleConfirmDialogHexChanged(const FText& InText);

    UFUNCTION()
    void HandleActorTabClicked();

    UFUNCTION()
    void HandleFileTabClicked();

    UFUNCTION()
    void HandleSettingsTabClicked();

    UFUNCTION()
    void HandleTestTabClicked();

        UFUNCTION()
        void HandleConfirmDialogRCommitted(const FText& InText, ETextCommit::Type CommitMethod);

        UFUNCTION()
        void HandleConfirmDialogGCommitted(const FText& InText, ETextCommit::Type CommitMethod);

        UFUNCTION()
        void HandleConfirmDialogBCommitted(const FText& InText, ETextCommit::Type CommitMethod);

        UFUNCTION()
        void HandleConfirmDialogACommitted(const FText& InText, ETextCommit::Type CommitMethod);

        UFUNCTION()
        void HandleConfirmDialogHexCommitted(const FText& InText, ETextCommit::Type CommitMethod);

        // --- ˢ�£���ԭ�򣬸� BP ���� RequestRefresh / Regenerate / Rebuild ---
        void RefreshPanel(EInspectorRefreshReason Reason);
        void RefreshPanel(); // ���ݾɵ���


    // ===== Favorites persistence =====
    private:
        TSet<FString> FavoriteKeys;

        FString GetFavoritesFilePath() const;
        void LoadFavorites();
        void SaveFavorites() const;

        FString MakeFavoriteKeyForProperty(UObject* TargetObject, FName PropertyName) const;

        // �� GetPropertyItemsForSelected �ã��ѵ�ǰ OutItems ���� pin �������
        void InsertPinnedGroupIfNeeded(TArray<UObject*>& OutItems);

        // ===== Snapshot state (Only Modified / Export / Import) =====
        // Cached import result for UI
        UPROPERTY(Transient)
        FRIImportReport LastImportReport;

        FString GetImportReportsDir() const;
        FString GetAuditReportsDir() const;
        FString GetPatchesDir() const;
        FString GetPresetsDir() const;
        void CacheSharedFileReport(bool bSuccess, const FString& Summary, const FString& Details, int32 AppliedCount = 0, int32 SkippedCount = 0, int32 MissingCount = 0, int32 HardFailCount = 0);

        UPROPERTY(Transient)
        TMap<FString, FString> BaselineValueByKey;

        UPROPERTY(Transient)
        TMap<FString, FString> ModifiedValueByKey;

        UPROPERTY(Transient)
        FRIPatchBundle StagedPatchBundle;

        UPROPERTY(Transient)
        bool bHasStagedPatch = false;

        UPROPERTY(Transient)
        FRIApplyResult LastPatchApplyResult;

        UPROPERTY(Transient)
        FRIPromoteResult LastPromoteResult;

        UPROPERTY(Transient)
        FRIVerificationRunResult LastVerificationRunResult;

        UPROPERTY(Transient)
        FRIWorkflowRunResult LastWorkflowRunResult;

        UPROPERTY(Transient)
        FRIWorkflowMatrixRunResult LastWorkflowMatrixRunResult;

        UPROPERTY(Transient)
        FRIRuntimeRoleCompareReport LastRuntimeRoleCompareReport;

        UPROPERTY(Transient)
        FRIRuntimeSessionTargetCompareReport LastRuntimeSessionTargetCompareReport;

        UPROPERTY(Transient)
        FRIRuntimeSessionTargetSetCompareReport LastRuntimeSessionTargetSetCompareReport;

        UPROPERTY(Transient)
        FRIRuntimeSessionTargetSetCompareMatrixRunResult LastRuntimeSessionTargetSetCompareMatrixRunResult;

        UPROPERTY(Transient)
        FRIRuntimeSessionTargetSetCompareRequest ActiveRemoteSessionTargetSetCompareRequest;

        UPROPERTY(Transient)
        FString ConnectedRuntimeSessionId;

        UPROPERTY(Transient)
        FString PreferredRemoteSessionId;

        UPROPERTY(Transient)
        FString LastRemoteRuntimeSessionError;

        UPROPERTY(Transient)
        FString LastRemotePatchPullSummary;

        UPROPERTY(Transient)
        FString LastRemoteSessionSelectionSummary;

        UPROPERTY(Transient)
        FString LastRemoteSessionTargetQuery;

        UPROPERTY(Transient)
        FString LastRemoteSessionWorkflowId;

        mutable double CachedExternalRuntimeProbeTimestampSeconds = -1.0;
        mutable TArray<FRIRuntimeSessionInfo> CachedExternalRuntimeSessions;
        mutable FString CachedExternalRuntimeProbeError;
        mutable bool bHasCachedFileManagementSummary = false;
        mutable FRIFileManagementSummary CachedFileManagementSummary;
        mutable FString CachedFileManagementSummaryError;

        struct FRIRuntimeTargetQueryCacheEntry
        {
            FString SessionId;
            FString NameFilter;
            FString ClassFilter;
            int32 Limit = 0;
            TArray<FRIRuntimeTargetInfo> Targets;
            FString Error;
            bool bSuccess = false;
            double CachedAtSeconds = 0.0;
        };

        mutable TMap<FString, FRIRuntimeTargetQueryCacheEntry> CachedRuntimeTargetQueryResults;

        UPROPERTY(Transient)
        FRIAuditReport LastAuditReport;

        UPROPERTY(Transient)
        FRIAuditReport CachedBaselineAuditReport;

        UPROPERTY(Transient)
        FRIAuditReport CachedCurrentVsPatchAuditReport;

        UPROPERTY(Transient)
        FRIAuditReport CachedPatchVsSourceAuditReport;

        UPROPERTY(Transient)
        FRIAuditReport CachedAppliedPatchVsSourceAuditReport;

        UPROPERTY(Transient)
        ERIAuditComparisonMode ActiveFileAuditMode = ERIAuditComparisonMode::CurrentVsPatch;

        UPROPERTY(Transient)
        FString LastConfirmDialogSelfTestReport;

        UPROPERTY(Transient)
        bool bLastConfirmDialogSelfTestPassed = false;

        FString GetSnapshotsDir() const;
        FString MakePropertySnapshotKey(UObject* TargetObject, FName PropertyName) const;
        FString MakeMaterialSnapshotKey(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorMatParamType Type, FName ParamName) const;
        AActor* ResolveRuntimeActorTarget(const FString& ActorPath, const FString& ActorClass, const FString& ActorBaseName) const;
        UActorComponent* ResolveRuntimeComponentTarget(AActor* Owner, const FString& ActorPathForRemap, const FString& ComponentPath, const FString& ComponentName, const FString& ComponentClass) const;
        void RememberSelectedActorRecoveryIdentity(AActor* Actor);
        void ClearSelectedActorRecoveryState();
        void BeginSelectedActorRecovery(AActor* PreviousActor);
        bool TryRecoverSelectedActorFromIdentity();
        bool TryBuildPatchOperationFromModifiedKey(const FString& Key, const FString& PatchedValue, const FString* BaselineValuePtr, FRIPatchOperation& OutOperation) const;
        void SortPatchOperationsForApply(TArray<FRIPatchOperation>& InOutOperations) const;
        void FinalizePatchApplyResult(FRIApplyResult& OutResult, const TCHAR* SummaryPrefix) const;
        bool ApplyPatchOperationValue(const FRIPatchOperation& Operation, const FString& ValueText, FRIPatchOperationResult& OutResult);
        bool ResolvePatchPresetFile(const FString& InPresetIdOrPath, FString& OutPresetFilePath) const;
        bool SerializePatchPresetToJson(const FRIPatchPresetMetadata& Metadata, const FRIPatchBundle& Bundle, FString& OutJson, FString& OutError) const;
        bool DeserializePatchPresetFromJson(const FString& InJson, FRIPatchPresetMetadata& OutMetadata, FRIPatchBundle& OutBundle, FString& OutError) const;
        bool RetargetPatchBundleToSelection(const FRIPatchPresetMetadata& Metadata, const FRIPatchBundle& InBundle, FRIPatchBundle& OutBundle, FString& OutError) const;
        bool SerializePatchBundleToJson(const FRIPatchBundle& Bundle, FString& OutJson, FString& OutError) const;
        bool DeserializePatchBundleFromJson(const FString& InJson, FRIPatchBundle& OutBundle, FString& OutError) const;
        void FinalizePromotePreview(FRIPromotePreview& OutPreview) const;
        void FinalizePromoteResult(FRIPromoteResult& OutResult) const;
        bool BuildBlueprintPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const;
        bool BuildMaterialPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const;
        bool BuildDataPromotePreview(const FRIPatchOperation& Operation, FRIPromoteOperationPreview& OutPreview) const;
        bool PromoteBlueprintOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult);
        bool PromoteMaterialOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult);
        bool PromoteDataOperationToSource(const FRIPatchOperation& Operation, FRIPromoteOperationResult& OutResult);
        bool TryReadRuntimePatchOperationValue(const FRIPatchOperation& Operation, FString& OutValue, FString& OutError) const;
        void FinalizeAuditReport(FRIAuditReport& OutReport) const;
        void InvalidateFileManagementSummaryCache();
        void InvalidateRuntimeTargetQueryCache(const FString& SessionId = FString());
        FString MakeRuntimeTargetQueryCacheKey(const FString& SessionId, const FString& NameFilter, const FString& ClassFilter, int32 Limit) const;
        void CacheRuntimeTargetQueryResult(
            const FString& SessionId,
            const FString& NameFilter,
            const FString& ClassFilter,
            int32 Limit,
            const TArray<FRIRuntimeTargetInfo>& Targets,
            const FString& Error,
            bool bSuccess) const;
        FRIAuditReport* GetMutableCachedAuditReport(ERIAuditComparisonMode InMode);
        const FRIAuditReport* GetCachedAuditReportPtr(ERIAuditComparisonMode InMode) const;

        void TrackModifiedForKey(const FString& Key, const FString& OldText, const FString& NewText);
        void UpdateModifiedStateFromCurrentValue(UObject* TargetObject, FName PropertyName);
        void UpdateModifiedStateFromCurrentMaterial(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName);

        // ===== Debounced apply (avoid spamming SetValue on sliders etc.) =====
        struct FRIPendingPropertyApply
        {
            TWeakObjectPtr<UObject> Target;
            FName PropertyName = NAME_None;
            FString PendingText;
            double ApplyAtSeconds = 0.0;
        };

        // (Not a UPROPERTY on purpose: FRIPendingPropertyApply is a plain C++ struct (no reflection needed).
        TMap<FString, FRIPendingPropertyApply> PendingPropertyApplyByKey;

        bool ApplyPropertyTextNow(UObject* TargetObject, FName PropertyName, const FString& NewText, FString& OutError);
        void FlushPendingPropertyApplies();



	// ===== Material Slot Selection =====
    public:
        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
        void SetPropertyView_MaterialOnly(UMeshComponent* InComp, int32 InSlot)
        {
        #if RUNTIME_INSPECTOR_ENABLED
                    PropertyViewMode = ERIPropertyViewMode::MaterialOnly;
                    ViewMeshComp = InComp;
                    ViewMaterialSlot = InSlot;
                    MarkActorPageStructureDirty();
                    RefreshPanel(EInspectorRefreshReason::StructureChanged);
        #endif
                }

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
        void SetPropertyView_Full()
        {
        #if RUNTIME_INSPECTOR_ENABLED
                    PropertyViewMode = ERIPropertyViewMode::Full;
                    ViewMeshComp = nullptr;
                    ViewMaterialSlot = INDEX_NONE;
                    MarkActorPageStructureDirty();
                    RefreshPanel(EInspectorRefreshReason::StructureChanged);
        #endif
        }
        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
        void ToggleFavoriteForAnyItem(UObject* Item);

    private:
        ERIPropertyViewMode PropertyViewMode = ERIPropertyViewMode::Full;

        UPROPERTY(Transient)
        TWeakObjectPtr<UMeshComponent> ViewMeshComp;

        int32 ViewMaterialSlot = INDEX_NONE;


    private:
        FString LastPinnedSignature;

    public:
        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
        void GetSnapshotList(TArray<UObject*>& OutItems) const;

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
        bool ReadSnapshotHeader(const FString& FullPath, FString& OutCreatedAtUtc, FString& OutMap, FString& OutSelectedActorPath, int32& OutEntryCount) const;

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
        void CopySnapshotPathToClipboard(const FString& FullPath);

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
        bool DeleteSnapshotFile(const FString& FullPath, FString& OutError);

        UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
        FString GetSnapshotDirectory() const;

	// ===== Group Tree Support =====
    public:

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Tree")
        void GetGroupTreeRootsForSelected(const FString& SearchText, TArray<UObject*>& OutRoots);

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Tree")
        void GetGroupTreeChildrenForItem(UInspectorGroupItem* Parent, const FString& SearchText, TArray<UObject*>& OutChildren);

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Groups")
        void SetGroupExpanded(const FString& GroupKey, bool bExpanded);

		// ===== Selection Support =====
    public:


        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Selection")
        void SetSelectedGroupItem(class UInspectorGroupItem* Item);

        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Selection")
        void ClearSelectedGroupItem();


	private:
        UPROPERTY()
        TObjectPtr<UObject> SelectedInspectObject = nullptr;

        UPROPERTY()
        int32 SelectedMaterialSlotIndex = INDEX_NONE;

        UPROPERTY()
        FString SelectedGroupKey;

		// ===== Input Binding =====
    public:
        UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Selection")
        bool PickActorUnderCursor();
        bool HandleRightMousePickInput(bool bCtrlDown, bool bShiftDown);
    private:
        bool PickActorAtMousePositionInternal(const TCHAR* SourceTag);
        void UpdateLastPickDebugSummary(const TCHAR* SourceTag, bool bHit, const FString& ActorPath, const FString& ComponentName, const TCHAR* Reason);
        void EnsureInspectorInputComponent();
        void ReleaseInspectorInputComponent();
        void RebindInspectorKeys();
        void TryBindInputs();
        void OnPickKeyPressed(); // NEW
        bool ValidateHotkeyCandidateInternal(FKey InKey, FString& OutError) const;
        bool IsCustomDepthStencilReady() const;
        void RefreshOutlineRuntimeSettings();
        void UpdateSettingsDirtyFlag();

		// ===== Actor Outline Support =====
    private:
        TWeakObjectPtr<AActor> OutlinedActor;
        TWeakObjectPtr<UButton> ActorTabButton;
        TWeakObjectPtr<UButton> FileTabButton;
        TWeakObjectPtr<UButton> SettingsTabButton;
        TWeakObjectPtr<UButton> TestTabButton;
        TWeakObjectPtr<UWidgetSwitcher> ContentSwitcher;
        TWeakObjectPtr<UPanelWidget> FileHostPanel;
        TWeakObjectPtr<UPanelWidget> SettingsHostPanel;
        TWeakObjectPtr<UPanelWidget> TestHostPanel;
        TWeakObjectPtr<UPanelWidget> SharedContextStripHostPanel;
        TWeakObjectPtr<UBorder> SharedContextStripBorder;
        TWeakObjectPtr<UWidget> PanelTitleBarWidget;
        TWeakObjectPtr<UWidget> PanelRootContentWidget;
        TWeakObjectPtr<class UCanvasPanelSlot> PanelRootCanvasSlot;
        TWeakObjectPtr<USizeBox> PanelSizeBox;
        TWeakObjectPtr<UWidget> SharedContextActorCell;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> SharedContextActorText = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> SharedContextClassText = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> SharedContextSourceText = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> SharedContextStagedText = nullptr;
        TWeakObjectPtr<UWidget> ActorTopContextActorCell;
        UPROPERTY(Transient)
        TObjectPtr<UBorder> ActorTopContextStripStrong = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorTopContextActorTextStrong = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorTopContextClassTextStrong = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorTopContextSourceTextStrong = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorTopContextStagedTextStrong = nullptr;
        UPROPERTY(Transient)
        TObjectPtr<UInspectorFilePageWidget> FilePageWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UInspectorSettingsPageWidget> SettingsPageWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UInspectorTestPageWidget> TestPageWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UInspectorGroupsSectionWidget> ActorGroupsSectionWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<USizeBox> ActorGroupsSectionHostBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UScrollBox> ActorGroupsScrollBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorGroupsEntriesBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UScrollBox> ActorPinnedScrollBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorPinnedEntriesBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UButton> ActorGroupsHeaderButtonStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UBorder> ActorGroupsHeaderBorderStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorGroupsHeaderTextStrong = nullptr;

        UPROPERTY(Transient)
        TArray<TObjectPtr<UInspectorGroupButtonProxy>> ActorGroupsClickProxies;

        UPROPERTY(Transient)
        TArray<TObjectPtr<UInspectorPinnedItemButtonProxy>> ActorPinnedClickProxies;

        UPROPERTY(Transient)
        TObjectPtr<UInspectorPropertiesSectionWidget> ActorPropertiesSectionWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UInspectorFunctionsSectionWidget> ActorFunctionsSectionWidgetStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UBorder> ActorWorkspaceSelectionBandStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorWorkspaceSelectionActorTextStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorWorkspaceSelectionSourceTextStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UTextBlock> ActorWorkspaceSelectionStateTextStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorPropertyFunctionHostBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UHorizontalBox> ActorWorkbenchBodyHostStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorWorkbenchPageStackHostStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorWorkbenchSidebarHostStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UVerticalBox> ActorWorkbenchContentHostStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<USizeBox> ActorSplitSidebarSizeBoxStrong = nullptr;

        UPROPERTY(Transient)
        TObjectPtr<UBorder> ActorSplitSidebarBorderStrong = nullptr;

        TWeakObjectPtr<UInspectorFilePageWidget> FilePageWidget;
        TWeakObjectPtr<UInspectorSettingsPageWidget> SettingsPageWidget;
        TWeakObjectPtr<UInspectorTestPageWidget> TestPageWidget;
        TWeakObjectPtr<UInspectorGroupsSectionWidget> ActorGroupsSectionWidget;
        TWeakObjectPtr<USizeBox> ActorGroupsSectionHostBox;
        TWeakObjectPtr<UButton> ActorGroupsHeaderButton;
        TWeakObjectPtr<UBorder> ActorGroupsHeaderBorder;
        TWeakObjectPtr<UTextBlock> ActorGroupsHeaderText;
        TWeakObjectPtr<UVerticalBox> ActorPinnedEntriesBox;
        TWeakObjectPtr<UInspectorPropertiesSectionWidget> ActorPropertiesSectionWidget;
        TWeakObjectPtr<UInspectorFunctionsSectionWidget> ActorFunctionsSectionWidget;
        TWeakObjectPtr<UBorder> ActorTopContextStrip;
        TWeakObjectPtr<UTextBlock> ActorTopContextActorText;
        TWeakObjectPtr<UTextBlock> ActorTopContextClassText;
        TWeakObjectPtr<UTextBlock> ActorTopContextSourceText;
        TWeakObjectPtr<UTextBlock> ActorTopContextStagedText;
        TWeakObjectPtr<UBorder> ActorWorkspaceSelectionBand;
        TWeakObjectPtr<UTextBlock> ActorWorkspaceSelectionActorText;
        TWeakObjectPtr<UTextBlock> ActorWorkspaceSelectionSourceText;
        TWeakObjectPtr<UTextBlock> ActorWorkspaceSelectionStateText;
        TWeakObjectPtr<UVerticalBox> ActorPropertyFunctionHostBox;
        TWeakObjectPtr<UHorizontalBox> ActorWorkbenchBodyHost;
        TWeakObjectPtr<UVerticalBox> ActorWorkbenchPageStackHost;
        TWeakObjectPtr<UVerticalBox> ActorWorkbenchSidebarHost;
        TWeakObjectPtr<UVerticalBox> ActorWorkbenchContentHost;
        TWeakObjectPtr<USizeBox> ActorSplitSidebarSizeBox;
        TWeakObjectPtr<UBorder> ActorSplitSidebarBorder;
        TWeakObjectPtr<class UCanvasPanelSlot> ActorSplitSidebarCanvasSlot;
        TArray<FRIHostPanelMountState> HostPanelMountStates;
        int32 SettingsPageIndex = INDEX_NONE;
        int32 TestPageIndex = INDEX_NONE;
        int32 LastAppliedThemePresetFingerprint = 0;
        bool bDeferredOpenActorRefreshScheduled = false;
        bool bDeferredOpenActorRefreshNeedsBaseline = false;
        bool bHasCompletedInitialActorPanelRefresh = false;
        bool bActorPageStructureDirty = true;
        bool bPanelWidgetPrecreated = false;
        bool bThemePreviewRefreshScheduled = false;
        ERIVisiblePage PendingThemePreviewPage = ERIVisiblePage::Actor;
        FString CurrentActorSearchText;
        FString LastActorPageStructureKey;
        FTimerHandle DeferredOpenActorRefreshTimerHandle;
        FTimerHandle ThemePreviewRefreshTimerHandle;
        bool bFabScreenshotApplicationScaleCaptured = false;
        float SavedFabScreenshotApplicationScale = 1.0f;
        bool bFabScreenshotPanelTransformCaptured = false;
        FVector2D SavedFabScreenshotPanelTranslation = FVector2D::ZeroVector;
        bool bFabScreenshotPanelHeightCaptured = false;
        float SavedFabScreenshotPanelHeight = 0.0f;
        bool bFabScreenshotPanelWidthCaptured = false;
        float SavedFabScreenshotPanelWidth = 0.0f;
        bool bPanelInteractionInitialized = false;
        bool bDraggingPanel = false;
        bool bResizingPanelVertically = false;
        FVector2D PanelTranslation = FVector2D::ZeroVector;
        FVector2D PanelInteractionStartCursor = FVector2D::ZeroVector;
        FVector2D PanelInteractionStartTranslation = FVector2D::ZeroVector;
        float PanelWidth = 0.0f;
        float PanelDefaultWidth = 0.0f;
        float PanelHeight = 0.0f;
        float PanelDefaultHeight = 0.0f;
        float PanelInteractionStartHeight = 0.0f;
        TArray<FRIActivityLogEntry> ActivityLogEntries;

        bool bWarnedCustomDepthStencil = false;
        bool bSettingsDirty = false;
        bool EnsureCustomDepthStencilEnabled();
        void EnsurePanelInteractionInitialized();
        void ApplyPanelInteractionPresentation();
        void CacheInitialPanelWidth();
        void CacheInitialPanelHeight();
        bool HandlePanelPointerDownAt(const FVector2D& Cursor);
        bool HandlePanelPointerMoveTo(const FVector2D& Cursor);
        bool HandlePanelPointerUp();
        bool TryGetPanelWindowRect(FSlateRect& OutRect) const;

        void SetActorOutline(AActor* Actor, bool bEnable, int32 StencilValue = 1);

        UCameraComponent* FindOutlineCamera(APlayerController* PC) const;

        void EnableOutlinePP(bool bEnable);

        // 运行时创建的 MID
        UPROPERTY(Transient)
        TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

        UPROPERTY(Transient)
        TObjectPtr<UInputComponent> InspectorInputComponent = nullptr;

        UPROPERTY(Transient)
        FRIEditableSettings LastSavedSettingsSnapshot;

        UPROPERTY(Transient)
        ERuntimeInspectorThemePreset LastSavedThemePresetSnapshot = ERuntimeInspectorThemePreset::StudioSlate;

        UPROPERTY(Transient)
        TArray<FRISelfTestResult> LastSelfTestResults;
        FRIValidationCaptureReport LastValidationCaptureReport;
        bool bValidationCaptureActive = false;
        double ValidationCaptureStartedSeconds = 0.0;
        FName ActiveValidationCaptureScenarioId = NAME_None;
        FRIValidationCaptureReport ActiveValidationCaptureReport;

        TSet<FString> ActiveToolExecutionKeys;
        int32 ActiveToolExecutionDepth = 0;

        // 记录相机原本的 PostProcessBlendWeight，关闭时恢复
        float SavedCamPPBlendWeight = 0.0f;
        bool bSavedCamPPBlendWeightValid = false;

};
