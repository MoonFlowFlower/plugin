#pragma once

#include "CoreMinimal.h"
#include "InspectorPatchTypes.h"
#include "InspectorViewModelTypes.h"
#include "UObject/Object.h"
#include "RuntimeInspectorController.generated.h"

class AActor;
class UInspectorWorldSubsystem;

UCLASS(BlueprintType)
class RUNTIMEINSPECTOR_API URuntimeInspectorController : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UInspectorWorldSubsystem* InSubsystem);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    FRIInspectorViewModel GetCurrentViewModel() const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    void SelectActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    void RequestRefresh();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestStageTransformChange(ERITransformField Field, ERIAxis Axis, double NewValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestRunFunction(FName FunctionName, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestFocusComponent(const FString& ComponentName, FString& OutError);
    bool RequestFocusComponentWithRefreshPolicy(const FString& ComponentName, FString& OutError, bool bRefreshPanel);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestNavigateToPinnedItem(UObject* Item, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestToggleFavorite(UObject* Item, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestApplyStagedPatches(FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestRevertPatch(FGuid PatchId, FRIApplyResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestReset(FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestUndo();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    bool RequestRedo();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    void SetActiveTab(ERIInspectorTab InTab);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Controller")
    ERIInspectorTab GetActiveTab() const { return ActiveTab; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    void SetOnlyModify(bool bInOnlyModify);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Controller")
    bool IsOnlyModifyEnabled() const { return bOnlyModify; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Controller")
    void SetSearchText(const FText& InText);

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Controller")
    FString GetSearchText() const { return SearchText; }

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Controller")
    FString GetLastIntentLog() const { return LastIntentLog; }

    UInspectorWorldSubsystem* GetSubsystem() const { return Subsystem.Get(); }

private:
    void SetLastIntentLog(const FString& InMessage);
    FRIInspectorViewModel BuildEmptyViewModel() const;
    bool BuildTransformPatch(ERITransformField Field, ERIAxis Axis, double NewValue, FRIPatchOperation& OutOperation, FString& OutError) const;

    UPROPERTY(Transient)
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    UPROPERTY(Transient)
    ERIInspectorTab ActiveTab = ERIInspectorTab::Actor;

    UPROPERTY(Transient)
    bool bOnlyModify = false;

    UPROPERTY(Transient)
    FString SearchText;

    UPROPERTY(Transient)
    FString LastIntentLog;
};
