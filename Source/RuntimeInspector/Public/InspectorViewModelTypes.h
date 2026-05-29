#pragma once

#include "CoreMinimal.h"
#include "InspectorViewModelTypes.generated.h"

UENUM(BlueprintType)
enum class ERIInspectorTab : uint8
{
    Actor,
    Changes,
    Settings,
    Tools
};

UENUM(BlueprintType)
enum class ERISelectionState : uint8
{
    None,
    Selected,
    Missing,
    PendingRefresh,
    Invalid
};

UENUM(BlueprintType)
enum class ERITransformField : uint8
{
    Location,
    Rotation,
    Scale
};

UENUM(BlueprintType)
enum class ERIAxis : uint8
{
    X,
    Y,
    Z
};

UENUM(BlueprintType)
enum class ERIFunctionRiskLevel : uint8
{
    ReadOnly,
    Low,
    Medium,
    High,
    Dangerous
};

UENUM(BlueprintType)
enum class ERIPatchRiskLevel : uint8
{
    None,
    Low,
    Medium,
    High,
    Dangerous
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIActorSummaryViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ActorDisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ActorClassName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ActorPath;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ShaderStatusText;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    ERISelectionState SelectionState = ERISelectionState::None;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIComponentNodeViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ClassName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText Path;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FString ComponentName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    int32 ParentIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bSelected = false;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRITransformViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FVector Scale = FVector::OneVector;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bReadOnly = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bHasStagedChange = false;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIFunctionViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FName FunctionName = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText Description;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    TArray<FString> ParameterSummaries;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bDeprecated = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bCallable = true;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    ERIFunctionRiskLevel RiskLevel = ERIFunctionRiskLevel::Low;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIFavoriteViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText OwnerLabel;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText ValueText;

    UPROPERTY(Transient)
    TObjectPtr<UObject> SourceItem = nullptr;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIPatchViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FGuid PatchId;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText TargetPath;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText PropertyName;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText OldValueText;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FText NewValueText;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    ERIPatchRiskLevel RiskLevel = ERIPatchRiskLevel::Low;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bCanApply = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bCanRevert = false;
};

USTRUCT(BlueprintType)
struct RUNTIMEINSPECTOR_API FRIInspectorViewModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FRIActorSummaryViewModel SelectedActor;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    TArray<FRIComponentNodeViewModel> Components;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    FRITransformViewModel Transform;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    TArray<FRIFunctionViewModel> Functions;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    TArray<FRIPatchViewModel> StagedPatches;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    TArray<FRIFavoriteViewModel> Favorites;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    ERIInspectorTab ActiveTab = ERIInspectorTab::Actor;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bOnlyModify = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bCanUndo = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bCanRedo = false;

    UPROPERTY(BlueprintReadWrite, Category = "RuntimeInspector|ViewModel")
    bool bIsRefreshing = false;
};
