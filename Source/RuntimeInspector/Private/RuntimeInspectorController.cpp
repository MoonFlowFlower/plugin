#include "RuntimeInspectorController.h"

#include "InspectorFunctionItem.h"
#include "InspectorGroupItem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorPropertyItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorWorldSubsystem.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/DateTime.h"

namespace
{
    static FText RI_TextFromString(const FString& Value)
    {
        return Value.IsEmpty() ? FText::GetEmpty() : FText::FromString(Value);
    }

    static FString RI_FormatDouble(double Value)
    {
        return FString::SanitizeFloat(Value);
    }

    static FString RI_FormatVectorImportText(const FVector& Value)
    {
        return FString::Printf(
            TEXT("(X=%s,Y=%s,Z=%s)"),
            *RI_FormatDouble(Value.X),
            *RI_FormatDouble(Value.Y),
            *RI_FormatDouble(Value.Z));
    }

    static FString RI_FormatRotatorImportText(const FRotator& Value)
    {
        return FString::Printf(
            TEXT("(Pitch=%s,Yaw=%s,Roll=%s)"),
            *RI_FormatDouble(Value.Pitch),
            *RI_FormatDouble(Value.Yaw),
            *RI_FormatDouble(Value.Roll));
    }

    static FString RI_GetActorDisplayName(const AActor* Actor)
    {
        if (!Actor)
        {
            return TEXT("No Actor Selected");
        }
#if WITH_EDITOR
        return Actor->GetActorLabel();
#else
        return Actor->GetName();
#endif
    }

    static FString RI_BuildPatchTargetLabel(const FRIPatchTarget& Target)
    {
        if (!Target.ComponentName.IsEmpty())
        {
            return FString::Printf(TEXT("%s / %s"), *Target.ActorBaseName, *Target.ComponentName);
        }
        return Target.ActorBaseName.IsEmpty() ? Target.ActorPath : Target.ActorBaseName;
    }

    static bool RI_IsSamePatchField(const FRIPatchOperation& Left, const FRIPatchOperation& Right)
    {
        return Left.Target.ActorPath == Right.Target.ActorPath
            && Left.Target.ComponentPath == Right.Target.ComponentPath
            && Left.Target.MaterialSlotIndex == Right.Target.MaterialSlotIndex
            && Left.Field.FieldKind == Right.Field.FieldKind
            && Left.Field.FieldPath == Right.Field.FieldPath;
    }

    static FGuid RI_MakePatchViewId(int32 OperationIndex)
    {
        return FGuid(0u, 0u, 0u, static_cast<uint32>(OperationIndex + 1));
    }

    static int32 RI_GetPatchIndexFromId(const FGuid& PatchId)
    {
        if (PatchId.A != 0u || PatchId.B != 0u || PatchId.C != 0u || PatchId.D == 0u)
        {
            return INDEX_NONE;
        }
        return static_cast<int32>(PatchId.D) - 1;
    }

    static bool RI_DockGroupCanExpand(const UInspectorGroupItem* Item)
    {
        if (!Item || Item->IsMaterialSlot())
        {
            return false;
        }

        if (Item->StableKey == TEXT("ROOT_COMPONENTS") || Item->IsMaterialsRoot())
        {
            return true;
        }

        return Cast<UStaticMeshComponent>(Item->TargetObject) != nullptr;
    }

    static bool RI_IsDockRenderableGroup(const UInspectorGroupItem* Item)
    {
        return Item
            && Item->StableKey != TEXT("ROOT_COMPONENTS")
            && Item->StableKey != TEXT("ROOT_ACTOR")
            && Item->StableKey != TEXT("PINNED_ROOT");
    }

    static ERIComponentNodeKind RI_DockComponentNodeKind(const UInspectorGroupItem* Item)
    {
        if (Item && Item->IsMaterialSlot())
        {
            return ERIComponentNodeKind::MaterialSlot;
        }
        if (Item && Item->IsMaterialsRoot())
        {
            return ERIComponentNodeKind::MaterialsRoot;
        }
        return ERIComponentNodeKind::Component;
    }

    static void RI_AppendDockComponentGroup(
        UInspectorWorldSubsystem* InspectorSubsystem,
        UInspectorGroupItem* GroupItem,
        const FString& SearchText,
        const FString& SelectedStableKey,
        FRIInspectorViewModel& ViewModel,
        int32 ParentIndex)
    {
        if (!InspectorSubsystem || !GroupItem)
        {
            return;
        }

        int32 ThisIndex = ParentIndex;
        if (RI_IsDockRenderableGroup(GroupItem))
        {
            FRIComponentNodeViewModel Node;
            Node.StableKey = GroupItem->StableKey;
            Node.Depth = FMath::Max(0, GroupItem->Depth - 1);
            Node.ParentIndex = ParentIndex;
            Node.Kind = RI_DockComponentNodeKind(GroupItem);
            Node.bExpanded = GroupItem->bExpanded;
            Node.bCanExpand = RI_DockGroupCanExpand(GroupItem);
            Node.MaterialSlotIndex = GroupItem->MaterialSlotIndex;
            Node.bSelected = !SelectedStableKey.IsEmpty() && GroupItem->StableKey == SelectedStableKey;

            if (UActorComponent* Component = Cast<UActorComponent>(GroupItem->TargetObject))
            {
                Node.ComponentName = Component->GetName();
                Node.Path = RI_TextFromString(Component->GetPathName());
                if (Node.Kind == ERIComponentNodeKind::Component)
                {
                    Node.DisplayName = RI_TextFromString(Component->GetName());
                    Node.ClassName = RI_TextFromString(GetNameSafe(Component->GetClass()));
                }
                else if (Node.Kind == ERIComponentNodeKind::MaterialsRoot)
                {
                    Node.DisplayName = RI_TextFromString(GroupItem->DisplayName.IsEmpty() ? TEXT("Materials") : GroupItem->DisplayName);
                    Node.ClassName = FText::GetEmpty();
                }
                else
                {
                    Node.DisplayName = RI_TextFromString(GroupItem->DisplayName);
                    Node.ClassName = FText::FromString(TEXT("Material"));
                }
            }
            else
            {
                Node.DisplayName = RI_TextFromString(GroupItem->DisplayName);
                Node.ClassName = RI_TextFromString(GetNameSafe(GroupItem->TargetObject));
                Node.Path = RI_TextFromString(GetPathNameSafe(GroupItem->TargetObject));
            }

            ThisIndex = ViewModel.Components.Num();
            ViewModel.Components.Add(Node);
        }

        const bool bShouldVisitChildren = GroupItem->StableKey == TEXT("ROOT_COMPONENTS")
            || !SearchText.IsEmpty()
            || (GroupItem->bExpanded && RI_DockGroupCanExpand(GroupItem));
        if (!bShouldVisitChildren)
        {
            return;
        }

        TArray<UObject*> ChildObjects;
        InspectorSubsystem->GetGroupTreeChildrenForItem(GroupItem, SearchText, ChildObjects);
        for (UObject* ChildObject : ChildObjects)
        {
            RI_AppendDockComponentGroup(
                InspectorSubsystem,
                Cast<UInspectorGroupItem>(ChildObject),
                SearchText,
                SelectedStableKey,
                ViewModel,
                ThisIndex);
        }
    }
}

void URuntimeInspectorController::Initialize(UInspectorWorldSubsystem* InSubsystem)
{
    Subsystem = InSubsystem;
}

void URuntimeInspectorController::SetLastIntentLog(const FString& InMessage)
{
    LastIntentLog = FString::Printf(TEXT("%s | %s"), *FDateTime::UtcNow().ToIso8601(), *InMessage);
}

FRIInspectorViewModel URuntimeInspectorController::BuildEmptyViewModel() const
{
    FRIInspectorViewModel ViewModel;
    ViewModel.ActiveTab = ActiveTab;
    ViewModel.bOnlyModify = bOnlyModify;
    ViewModel.SelectedActor.ActorDisplayName = FText::FromString(TEXT("No Actor Selected"));
    ViewModel.SelectedActor.ActorClassName = FText::FromString(TEXT("None"));
    ViewModel.SelectedActor.SelectionState = ERISelectionState::None;
    return ViewModel;
}

FRIInspectorViewModel URuntimeInspectorController::GetCurrentViewModel() const
{
    return GetCurrentViewModel(ERIViewModelHydrationMode::Full);
}

FRIInspectorViewModel URuntimeInspectorController::GetCurrentViewModel(ERIViewModelHydrationMode HydrationMode) const
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        return BuildEmptyViewModel();
    }

    FRIInspectorViewModel ViewModel;
    ViewModel.ActiveTab = ActiveTab;
    ViewModel.bOnlyModify = bOnlyModify;
    ViewModel.bCanUndo = InspectorSubsystem->CanUndo();
    ViewModel.bCanRedo = InspectorSubsystem->CanRedo();

    const FString EffectiveSearch = InspectorSubsystem->GetCurrentActorSearchText();

    AActor* Actor = InspectorSubsystem->GetSelectedActor();
    if (!Actor)
    {
        ViewModel.SelectedActor = BuildEmptyViewModel().SelectedActor;
    }
    else
    {
        ViewModel.SelectedActor.ActorDisplayName = RI_TextFromString(RI_GetActorDisplayName(Actor));
        ViewModel.SelectedActor.ActorClassName = RI_TextFromString(GetNameSafe(Actor->GetClass()));
        ViewModel.SelectedActor.ActorPath = RI_TextFromString(Actor->GetPathName());
        ViewModel.SelectedActor.ShaderStatusText = FText::FromString(InspectorSubsystem->HasStagedPatch() ? TEXT("Staged changes pending") : TEXT("Ready"));
        ViewModel.SelectedActor.SelectionState = Actor->IsPendingKillPending() ? ERISelectionState::PendingRefresh : ERISelectionState::Selected;

        if (const USceneComponent* RootComponent = Actor->GetRootComponent())
        {
            ViewModel.Transform.Location = RootComponent->GetRelativeLocation();
            ViewModel.Transform.Rotation = RootComponent->GetRelativeRotation();
            ViewModel.Transform.Scale = RootComponent->GetRelativeScale3D();
            ViewModel.Transform.bReadOnly = false;
            ViewModel.Transform.bHasStagedChange = InspectorSubsystem->HasStagedPatch();
        }
        else
        {
            ViewModel.Transform.bReadOnly = true;
        }

        TArray<UObject*> RootObjects;
        InspectorSubsystem->GetGroupTreeRootsForSelected(EffectiveSearch, RootObjects);
        const FString SelectedStableKey = InspectorSubsystem->GetSelectedGroupKeyForAutomation();
        for (UObject* RootObject : RootObjects)
        {
            UInspectorGroupItem* RootItem = Cast<UInspectorGroupItem>(RootObject);
            if (!RootItem || RootItem->StableKey == TEXT("PINNED_ROOT"))
            {
                continue;
            }

            RI_AppendDockComponentGroup(
                InspectorSubsystem,
                RootItem,
                EffectiveSearch,
                SelectedStableKey,
                ViewModel,
                INDEX_NONE);
        }
    }

    if (HydrationMode == ERIViewModelHydrationMode::Full)
    {
        TArray<UInspectorFunctionItem*> FunctionItems;
        InspectorSubsystem->GetFunctionItemsForSelected(EffectiveSearch, FunctionItems);
        for (UInspectorFunctionItem* Item : FunctionItems)
        {
            if (!Item || !Item->IsValidItem())
            {
                continue;
            }

            FRIFunctionViewModel FunctionViewModel;
            FunctionViewModel.FunctionName = Item->GetFunctionFName();
            FunctionViewModel.DisplayName = RI_TextFromString(Item->GetDisplayName().IsEmpty() ? Item->GetFunctionName() : Item->GetDisplayName());
            FunctionViewModel.Description = RI_TextFromString(Item->GetSignatureText().IsEmpty() ? Item->GetTooltipText() : Item->GetSignatureText());

            bool bAllParamsSupported = true;
            for (const FRIFunctionParameterSpec& Spec : Item->GetParameterSpecs())
            {
                FunctionViewModel.ParameterSummaries.Add(FString::Printf(TEXT("%s: %s"), *Spec.DisplayName, *Spec.TypeLabel));
                bAllParamsSupported = bAllParamsSupported && Spec.bIsSupported;
            }

            FunctionViewModel.bCallable = bAllParamsSupported;
            FunctionViewModel.bDeprecated = Item->GetFunctionName().Contains(TEXT("Deprecated"), ESearchCase::IgnoreCase);
            FunctionViewModel.RiskLevel = FunctionViewModel.bDeprecated ? ERIFunctionRiskLevel::Medium : ERIFunctionRiskLevel::Low;
            ViewModel.Functions.Add(FunctionViewModel);

            if (ViewModel.Functions.Num() >= 64)
            {
                break;
            }
        }
    }

    if (HydrationMode != ERIViewModelHydrationMode::ShellOnly)
    {
        TArray<UObject*> PinnedItems;
        InspectorSubsystem->GetPinnedItemsForSelected(EffectiveSearch, PinnedItems);
        for (UObject* PinnedItem : PinnedItems)
        {
            FRIFavoriteViewModel FavoriteViewModel;
            FavoriteViewModel.SourceItem = PinnedItem;
            if (UInspectorPropertyItem* PropertyItem = Cast<UInspectorPropertyItem>(PinnedItem))
            {
                FavoriteViewModel.DisplayName = RI_TextFromString(PropertyItem->GetPropertyName());
                FavoriteViewModel.OwnerLabel = RI_TextFromString(PropertyItem->OwnerPrefix);
                FavoriteViewModel.ValueText = RI_TextFromString(PropertyItem->GetValueText());
            }
            else if (UInspectorMaterialParamItem* MaterialItem = Cast<UInspectorMaterialParamItem>(PinnedItem))
            {
                FavoriteViewModel.DisplayName = RI_TextFromString(MaterialItem->GetPropertyName());
                FavoriteViewModel.OwnerLabel = FText::FromString(TEXT("Material"));
                FavoriteViewModel.ValueText = RI_TextFromString(MaterialItem->GetValueText());
            }
            else if (UInspectorFunctionItem* FunctionItem = Cast<UInspectorFunctionItem>(PinnedItem))
            {
                FavoriteViewModel.DisplayName = RI_TextFromString(FunctionItem->GetDisplayName().IsEmpty() ? FunctionItem->GetFunctionName() : FunctionItem->GetDisplayName());
                FavoriteViewModel.OwnerLabel = FText::FromString(TEXT("Function"));
                FavoriteViewModel.ValueText = RI_TextFromString(FunctionItem->GetSignatureText());
            }
            else if (PinnedItem)
            {
                FavoriteViewModel.DisplayName = RI_TextFromString(PinnedItem->GetName());
                FavoriteViewModel.OwnerLabel = RI_TextFromString(GetNameSafe(PinnedItem->GetClass()));
            }

            if (!FavoriteViewModel.DisplayName.IsEmpty())
            {
                ViewModel.Favorites.Add(FavoriteViewModel);
            }

            if (ViewModel.Favorites.Num() >= 16)
            {
                break;
            }
        }
    }

    if (InspectorSubsystem->HasStagedPatch())
    {
        const FRIPatchBundle Bundle = InspectorSubsystem->GetStagedPatch();
        for (int32 OperationIndex = 0; OperationIndex < Bundle.Operations.Num(); ++OperationIndex)
        {
            const FRIPatchOperation& Operation = Bundle.Operations[OperationIndex];
            FRIPatchViewModel PatchViewModel;
            PatchViewModel.PatchId = RI_MakePatchViewId(OperationIndex);
            PatchViewModel.TargetPath = RI_TextFromString(RI_BuildPatchTargetLabel(Operation.Target));
            PatchViewModel.PropertyName = RI_TextFromString(Operation.Field.DisplayName.IsEmpty() ? Operation.Field.FieldPath : Operation.Field.DisplayName);
            PatchViewModel.OldValueText = RI_TextFromString(Operation.BaselineValue);
            PatchViewModel.NewValueText = RI_TextFromString(Operation.PatchedValue);
            PatchViewModel.RiskLevel = ERIPatchRiskLevel::Low;
            PatchViewModel.bCanApply = true;
            PatchViewModel.bCanRevert = true;
            ViewModel.StagedPatches.Add(PatchViewModel);
        }
    }

    return ViewModel;
}

void URuntimeInspectorController::SelectActor(AActor* Actor)
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        SetLastIntentLog(FString::Printf(TEXT("SelectActor %s"), *GetNameSafe(Actor)));
        InspectorSubsystem->SetSelectedActor(Actor);
    }
}

void URuntimeInspectorController::RequestRefresh()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        SetLastIntentLog(TEXT("Refresh requested"));
        InspectorSubsystem->RequestActorPageRefresh();
    }
}

bool URuntimeInspectorController::BuildTransformPatch(ERITransformField Field, ERIAxis Axis, double NewValue, FRIPatchOperation& OutOperation, FString& OutError) const
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    AActor* Actor = InspectorSubsystem->GetSelectedActor();
    if (!Actor)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    USceneComponent* RootComponent = Actor->GetRootComponent();
    if (!RootComponent)
    {
        OutError = TEXT("Selected actor has no root scene component");
        return false;
    }

    FName PropertyName = NAME_None;
    FString PatchedValue;
    switch (Field)
    {
    case ERITransformField::Location:
    {
        PropertyName = TEXT("RelativeLocation");
        FVector Location = RootComponent->GetRelativeLocation();
        if (Axis == ERIAxis::X) { Location.X = NewValue; }
        else if (Axis == ERIAxis::Y) { Location.Y = NewValue; }
        else { Location.Z = NewValue; }
        PatchedValue = RI_FormatVectorImportText(Location);
        break;
    }
    case ERITransformField::Rotation:
    {
        PropertyName = TEXT("RelativeRotation");
        FRotator Rotation = RootComponent->GetRelativeRotation();
        if (Axis == ERIAxis::X) { Rotation.Roll = NewValue; }
        else if (Axis == ERIAxis::Y) { Rotation.Pitch = NewValue; }
        else { Rotation.Yaw = NewValue; }
        PatchedValue = RI_FormatRotatorImportText(Rotation);
        break;
    }
    case ERITransformField::Scale:
    {
        PropertyName = TEXT("RelativeScale3D");
        FVector Scale = RootComponent->GetRelativeScale3D();
        if (Axis == ERIAxis::X) { Scale.X = NewValue; }
        else if (Axis == ERIAxis::Y) { Scale.Y = NewValue; }
        else { Scale.Z = NewValue; }
        PatchedValue = RI_FormatVectorImportText(Scale);
        break;
    }
    default:
        OutError = TEXT("Unsupported transform field");
        return false;
    }

    FString BaselineValue;
    if (!InspectorPropertyUtils::GetValueAsText(RootComponent, PropertyName, BaselineValue))
    {
        OutError = FString::Printf(TEXT("Failed to read %s"), *PropertyName.ToString());
        return false;
    }

    OutOperation = FRIPatchOperation();
    OutOperation.Target.TargetKind = ERIPatchTargetKind::Component;
    OutOperation.Target.ActorPath = Actor->GetPathName();
    OutOperation.Target.ActorClass = GetNameSafe(Actor->GetClass());
    OutOperation.Target.ActorBaseName = Actor->GetName();
    OutOperation.Target.ComponentPath = RootComponent->GetPathName();
    OutOperation.Target.ComponentName = RootComponent->GetName();
    OutOperation.Target.ComponentClass = GetNameSafe(RootComponent->GetClass());
    OutOperation.Field.FieldKind = ERIPatchFieldKind::Property;
    OutOperation.Field.FieldPath = PropertyName.ToString();
    OutOperation.Field.DisplayName = PropertyName.ToString();
    OutOperation.ValueKind = ERIPatchValueKind::ImportText;
    OutOperation.BaselineValue = BaselineValue;
    OutOperation.PatchedValue = PatchedValue;
    OutOperation.SourceTag = TEXT("RuntimeInspectorDockTransform");
    return true;
}

bool URuntimeInspectorController::RequestStageTransformChange(ERITransformField Field, ERIAxis Axis, double NewValue, FString& OutError)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    FRIPatchOperation Operation;
    if (!BuildTransformPatch(Field, Axis, NewValue, Operation, OutError))
    {
        SetLastIntentLog(FString::Printf(TEXT("StageTransform rejected: %s"), *OutError));
        return false;
    }

    FRIPatchBundle Bundle = InspectorSubsystem->HasStagedPatch()
        ? InspectorSubsystem->GetStagedPatch()
        : FRIPatchBundle();

    Bundle.Version = 1;
    if (Bundle.BundleId.IsEmpty())
    {
        Bundle.BundleId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    }
    if (Bundle.CapturedAtUtc.IsEmpty())
    {
        Bundle.CapturedAtUtc = FDateTime::UtcNow().ToIso8601();
    }
    if (Bundle.DisplayName.IsEmpty())
    {
        Bundle.DisplayName = TEXT("RuntimeInspector staged UI changes");
    }
    if (AActor* Actor = InspectorSubsystem->GetSelectedActor())
    {
        Bundle.CapturedFromSelection = Actor->GetPathName();
    }

    for (int32 OperationIndex = Bundle.Operations.Num() - 1; OperationIndex >= 0; --OperationIndex)
    {
        if (RI_IsSamePatchField(Bundle.Operations[OperationIndex], Operation))
        {
            Bundle.Operations.RemoveAt(OperationIndex);
        }
    }
    Bundle.Operations.Add(Operation);

    if (!InspectorSubsystem->StagePatchBundle(Bundle, OutError))
    {
        SetLastIntentLog(FString::Printf(TEXT("StageTransform failed: %s"), *OutError));
        return false;
    }

    SetLastIntentLog(FString::Printf(TEXT("StageTransform %s.%s"), *Operation.Target.ComponentName, *Operation.Field.FieldPath));
    return true;
}

bool URuntimeInspectorController::RequestRunFunction(FName FunctionName, FString& OutError)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    TArray<UInspectorFunctionItem*> FunctionItems;
    InspectorSubsystem->GetFunctionItemsForSelected(SearchText, FunctionItems);
    for (UInspectorFunctionItem* Item : FunctionItems)
    {
        if (!Item || Item->GetFunctionFName() != FunctionName)
        {
            continue;
        }

        TArray<FString> Args;
        Args.Reserve(Item->GetParameterSpecs().Num());
        for (const FRIFunctionParameterSpec& Spec : Item->GetParameterSpecs())
        {
            if (!Spec.bIsSupported)
            {
                OutError = FString::Printf(TEXT("Unsupported parameter: %s"), *Spec.DisplayName);
                SetLastIntentLog(FString::Printf(TEXT("RunFunction rejected: %s"), *OutError));
                return false;
            }
            Args.Add(Spec.DefaultText);
        }

        const bool bOk = Item->Invoke(Args, OutError);
        SetLastIntentLog(FString::Printf(TEXT("RunFunction %s -> %s"), *FunctionName.ToString(), bOk ? TEXT("ok") : *OutError));
        return bOk;
    }

    OutError = FString::Printf(TEXT("Function '%s' not found"), *FunctionName.ToString());
    SetLastIntentLog(FString::Printf(TEXT("RunFunction rejected: %s"), *OutError));
    return false;
}

bool URuntimeInspectorController::RequestFocusComponent(const FString& ComponentName, FString& OutError)
{
    return RequestFocusComponentWithRefreshPolicy(ComponentName, OutError, true);
}

bool URuntimeInspectorController::RequestFocusComponentWithRefreshPolicy(const FString& ComponentName, FString& OutError, bool bRefreshPanel)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    const bool bOk = InspectorSubsystem->FocusSelectedActorComponentByNameWithRefreshPolicy(ComponentName, OutError, bRefreshPanel);
    ActiveTab = ERIInspectorTab::Actor;
    SetLastIntentLog(FString::Printf(
        TEXT("FocusComponent %s -> %s"),
        *ComponentName,
        bOk ? TEXT("ok") : *OutError));
    return bOk;
}

bool URuntimeInspectorController::RequestSelectComponentTreeNode(const FString& StableKey, FString& OutError)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    bool bOk = InspectorSubsystem->HandleDockGroupItemClicked(StableKey, OutError);
    if (!bOk && !StableKey.Contains(TEXT(":MATERIALS")))
    {
        FString FocusError;
        const bool bFallbackOk = InspectorSubsystem->FocusSelectedActorComponentByNameWithRefreshPolicy(StableKey, FocusError, false);
        if (bFallbackOk)
        {
            bOk = true;
            OutError.Reset();
        }
        else if (OutError.IsEmpty())
        {
            OutError = FocusError;
        }
    }

    ActiveTab = ERIInspectorTab::Actor;
    SetLastIntentLog(FString::Printf(
        TEXT("SelectComponentTreeNode %s -> %s"),
        *StableKey,
        bOk ? TEXT("ok") : *OutError));
    return bOk;
}

bool URuntimeInspectorController::RequestNavigateToPinnedItem(UObject* Item, FString& OutError)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    const bool bOk = InspectorSubsystem->NavigateToPinnedItem(Item, OutError);
    ActiveTab = ERIInspectorTab::Actor;
    if (bOk)
    {
        SearchText.Reset();
    }
    SetLastIntentLog(FString::Printf(
        TEXT("NavigatePinned %s -> %s"),
        *GetNameSafe(Item),
        bOk ? TEXT("ok") : *OutError));
    return bOk;
}

bool URuntimeInspectorController::RequestToggleFavorite(UObject* Item, FString& OutError)
{
    OutError.Reset();
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutError = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }
    if (!Item)
    {
        OutError = TEXT("Favorite item is invalid");
        SetLastIntentLog(TEXT("ToggleFavorite rejected: invalid item"));
        return false;
    }

    const bool bWasFavorite = InspectorSubsystem->IsFavoriteForAnyItem(Item);
    InspectorSubsystem->ToggleFavoriteForAnyItem(Item);
    const bool bIsFavorite = InspectorSubsystem->IsFavoriteForAnyItem(Item);
    SetLastIntentLog(FString::Printf(
        TEXT("ToggleFavorite %s %d->%d"),
        *GetNameSafe(Item),
        bWasFavorite ? 1 : 0,
        bIsFavorite ? 1 : 0));
    return bWasFavorite != bIsFavorite;
}

bool URuntimeInspectorController::RequestApplyStagedPatches(FRIApplyResult& OutResult)
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        const bool bOk = InspectorSubsystem->ApplyStagedPatch(OutResult);
        SetLastIntentLog(FString::Printf(TEXT("ApplyStagedPatches -> %s"), bOk ? TEXT("ok") : *OutResult.Summary));
        return bOk;
    }

    OutResult = FRIApplyResult();
    OutResult.Summary = TEXT("RuntimeInspector subsystem unavailable");
    return false;
}

bool URuntimeInspectorController::RequestRevertPatch(FGuid PatchId, FRIApplyResult& OutResult)
{
    UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get();
    if (!InspectorSubsystem)
    {
        OutResult = FRIApplyResult();
        OutResult.Summary = TEXT("RuntimeInspector subsystem unavailable");
        return false;
    }

    if (!InspectorSubsystem->HasStagedPatch())
    {
        OutResult = FRIApplyResult();
        OutResult.Summary = TEXT("No staged patch");
        return false;
    }

    FRIPatchBundle Bundle = InspectorSubsystem->GetStagedPatch();
    const int32 OperationIndex = RI_GetPatchIndexFromId(PatchId);
    if (!Bundle.Operations.IsValidIndex(OperationIndex))
    {
        OutResult = FRIApplyResult();
        OutResult.Summary = TEXT("Patch row no longer exists");
        return false;
    }

    FRIPatchBundle SingleOperationBundle;
    SingleOperationBundle.Version = Bundle.Version;
    SingleOperationBundle.BundleId = Bundle.BundleId;
    SingleOperationBundle.DisplayName = Bundle.DisplayName;
    SingleOperationBundle.CapturedAtUtc = Bundle.CapturedAtUtc;
    SingleOperationBundle.CapturedFromSelection = Bundle.CapturedFromSelection;
    SingleOperationBundle.Operations.Add(Bundle.Operations[OperationIndex]);

    InspectorSubsystem->RollbackPatchBundle(SingleOperationBundle, OutResult);
    Bundle.Operations.RemoveAt(OperationIndex);

    if (Bundle.Operations.Num() == 0)
    {
        InspectorSubsystem->ClearStagedPatch();
    }
    else
    {
        FString StageError;
        InspectorSubsystem->StagePatchBundle(Bundle, StageError);
    }

    SetLastIntentLog(FString::Printf(TEXT("RevertPatch row=%d"), OperationIndex));
    return true;
}

bool URuntimeInspectorController::RequestReset(FString& OutError)
{
    OutError.Reset();
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        InspectorSubsystem->ClearStagedPatch();
        SetLastIntentLog(TEXT("Reset staged patches"));
        return true;
    }

    OutError = TEXT("RuntimeInspector subsystem unavailable");
    return false;
}

bool URuntimeInspectorController::RequestUndo()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        const bool bOk = InspectorSubsystem->Undo();
        SetLastIntentLog(FString::Printf(TEXT("Undo -> %s"), bOk ? TEXT("ok") : TEXT("empty")));
        return bOk;
    }
    return false;
}

bool URuntimeInspectorController::RequestRedo()
{
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        const bool bOk = InspectorSubsystem->Redo();
        SetLastIntentLog(FString::Printf(TEXT("Redo -> %s"), bOk ? TEXT("ok") : TEXT("empty")));
        return bOk;
    }
    return false;
}

void URuntimeInspectorController::SetActiveTab(ERIInspectorTab InTab)
{
    ActiveTab = InTab;
    SetLastIntentLog(FString::Printf(TEXT("SetActiveTab %d"), static_cast<int32>(InTab)));
}

void URuntimeInspectorController::SetOnlyModify(bool bInOnlyModify)
{
    bOnlyModify = bInOnlyModify;
    SetLastIntentLog(FString::Printf(TEXT("OnlyModify %d"), bOnlyModify ? 1 : 0));
}

void URuntimeInspectorController::SetSearchText(const FText& InText)
{
    SearchText = InText.ToString();
    if (UInspectorWorldSubsystem* InspectorSubsystem = Subsystem.Get())
    {
        InspectorSubsystem->HandleActorSearchTextChanged(InText);
    }
    SetLastIntentLog(FString::Printf(TEXT("SearchText len=%d"), SearchText.Len()));
}
