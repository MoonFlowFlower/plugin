#include "InspectorWorldSubsystem.h"
#include "RuntimeInspectorInputProcessor.h"
#include "InspectorGroupItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorDefines.h"
#include "InspectorPropertyItem.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorSnapshotItem.h"


#include "HAL/PlatformFilemanager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"

#include "Dom/JsonObject.h"

#include "Blueprint/UserWidget.h"

#include "Engine/World.h"
#include "EngineUtils.h"

#include "Kismet/GameplayStatics.h"

#include "Framework/Application/SlateApplication.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialDomain.h"   


#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// =======================
// Property Whitelists (MVP)
// - Only used when SearchText is empty.
// - Search mode: show matched supported props even if not in whitelist.
// =======================


static TAutoConsoleVariable<int32> CVarRIEnable(
    TEXT("ri.Enable"),
#if UE_BUILD_SHIPPING
    0,
#else
    1,
#endif
    TEXT("Enable RuntimeInspector. 0=disabled, 1=enabled"),
    ECVF_Default
);
bool UInspectorWorldSubsystem::IsRIEnabled() const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    return CVarRIEnable.GetValueOnGameThread() != 0;
#endif
}

FString UInspectorWorldSubsystem::GetRIDisabledReason() const
{
#if UE_BUILD_SHIPPING
    return TEXT("Not available in Shipping");
#else
    return TEXT("Disabled (ri.Enable=0)");
#endif
}
static const TSet<FName>& GetStaticMeshComponentWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // USceneComponent
        Set.Add(TEXT("Mobility"));                 // Enum
        Set.Add(TEXT("bVisible"));                 // Bool (有些版本可能是 Visible 相关)
        Set.Add(TEXT("bHiddenInGame"));            // Bool

        // UPrimitiveComponent
        Set.Add(TEXT("CastShadow"));               // Bool
        Set.Add(TEXT("bRenderCustomDepth"));       // Bool
        Set.Add(TEXT("CustomDepthStencilValue"));  // Int
        Set.Add(TEXT("BoundsScale"));              // Float
        Set.Add(TEXT("TranslucencySortPriority")); // Int

        // UStaticMeshComponent
        Set.Add(TEXT("ForcedLodModel"));           // Int

        bInit = true;
    }
    return Set;
}

static const TSet<FName>& GetLightComponentWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // UActorComponent / USceneComponent
        Set.Add(TEXT("Mobility"));                 // Enum
        Set.Add(TEXT("bVisible"));                 // Bool
        Set.Add(TEXT("bHiddenInGame"));            // Bool

        // ULightComponent
        Set.Add(TEXT("Intensity"));                // Float
        Set.Add(TEXT("IndirectLightingIntensity"));// Float
        Set.Add(TEXT("bAffectsWorld"));            // Bool
        Set.Add(TEXT("CastShadows"));              // Bool (有些版本叫 CastShadows)
        Set.Add(TEXT("bUseInverseSquaredFalloff"));// Bool (点光/聚光常见)

        // UPointLightComponent / USpotLightComponent 等
        Set.Add(TEXT("AttenuationRadius"));        // Float

        bInit = true;
    }
    return Set;
}

static const TSet<FName>& GetCharacterMovementWhitelist()
{
    static TSet<FName> Set;
    static bool bInit = false;
    if (!bInit)
    {
        // UCharacterMovementComponent
        Set.Add(TEXT("MaxWalkSpeed"));                 // Float
        Set.Add(TEXT("MaxAcceleration"));              // Float
        Set.Add(TEXT("BrakingDecelerationWalking"));   // Float
        Set.Add(TEXT("JumpZVelocity"));                // Float
        Set.Add(TEXT("GravityScale"));                 // Float
        Set.Add(TEXT("AirControl"));                   // Float

        bInit = true;
    }
    return Set;
}

static const TSet<FName>* GetWhitelistForWhitelistedComponent(const UActorComponent* Comp)
{
    if (!Comp) return nullptr;

    if (Comp->IsA<UStaticMeshComponent>())          return &GetStaticMeshComponentWhitelist();
    if (Comp->IsA<ULightComponent>())               return &GetLightComponentWhitelist();
    if (Comp->IsA<UCharacterMovementComponent>())   return &GetCharacterMovementWhitelist();

    return nullptr;
}

void UInspectorWorldSubsystem::PushToast(ERIToastType Type, const FString& Message, float Duration)
{
#if !UE_BUILD_SHIPPING
    // 也可以在这里统一写 UE_LOG，方便排查
	UE_LOG(LogTemp, Log, TEXT("RI Toast: [%d] %s"), (int32)Type, *Message);
    OnToast.Broadcast(Type, Message, Duration);
#endif
}

static FString RI_ExtractTailAfterLastDot(const FString& PathLike)
{
    int32 Dot = INDEX_NONE;
    if (PathLike.FindLastChar(TEXT('.'), Dot))
    {
        return PathLike.Mid(Dot + 1);
    }
    return PathLike;
}

// 从 Actor 实例名里提取 baseName：
// BP_TestVarsActor_C_UAID_xxx -> BP_TestVarsActor
// BP_TestVarsActor_C_0        -> BP_TestVarsActor
static FString RI_ExtractActorBaseName(const FString& ActorInstanceName)
{
    FString N = ActorInstanceName;

    // 常见：..._C_UAID_... 或 ..._C_0
    int32 CPos = N.Find(TEXT("_C"));
    if (CPos != INDEX_NONE)
    {
        return N.Left(CPos);
    }

    // 兜底：取第一个 '_' 前
    int32 Under = INDEX_NONE;
    if (N.FindChar(TEXT('_'), Under))
    {
        return N.Left(Under);
    }

    return N;
}

// 从 snapshot 里的 class 字符串提取 “类短名”：
// "/RuntimeInspector/Test/BP_TestVarsActor.BP_TestVarsActor_C" -> "BP_TestVarsActor_C"
static FString RI_ExtractShortClassName(const FString& ClassPath)
{
    FString Tail = RI_ExtractTailAfterLastDot(ClassPath);

    // 有些路径可能是 /Script/.. 这种，取最后一个 '/' 后
    int32 Slash = INDEX_NONE;
    if (Tail.FindLastChar(TEXT('/'), Slash))
    {
        Tail = Tail.Mid(Slash + 1);
    }
    return Tail;
}

static bool RI_ClassMatches(AActor* Actor, const FString& SnapshotClassPathOrName)
{
    if (!Actor || SnapshotClassPathOrName.IsEmpty()) return false;

    // 1) 完整路径名直接比（最稳，如果你导出存的是 GetClass()->GetPathName()）
    const FString RuntimeClassPath = Actor->GetClass()->GetPathName();
    if (RuntimeClassPath == SnapshotClassPathOrName)
    {
        return true;
    }

    // 2) 用短名比：BP_TestVarsActor_C
    const FString WantShort = RI_ExtractShortClassName(SnapshotClassPathOrName);
    if (Actor->GetClass()->GetName() == WantShort)
    {
        return true;
    }

    // 3) 兜底：有些 snapshot 存的是 "/Pkg/..BP_xxx_C"，runtime path 不同，但都包含 short
    if (RuntimeClassPath.Contains(WantShort))
    {
        return true;
    }

    return false;
}

#if RUNTIME_INSPECTOR_ENABLED
static const TCHAR* DefaultPanelPath = TEXT("/RuntimeInspector/UI/WBP_InspectorPanel.WBP_InspectorPanel_C");
#endif

void UInspectorWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

#if RUNTIME_INSPECTOR_ENABLED
    PanelWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(DefaultPanelPath));
#endif

#if RUNTIME_INSPECTOR_ENABLED
    LoadFavorites();
#endif

}

void UInspectorWorldSubsystem::Deinitialize()
{
#if RUNTIME_INSPECTOR_ENABLED
   
    Close();

#endif
   

    Super::Deinitialize();
}

void UInspectorWorldSubsystem::Tick(float DeltaTime)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!bInputsBound)
    {
        TryBindInputs();
    }

    // 0.2s 检查一次够用
    ValidateAccum += DeltaTime;
    if (ValidateAccum >= 0.2f)
    {
        ValidateAccum = 0.f;
        ValidateSelection();
    }
#endif
}

void UInspectorWorldSubsystem::ValidateSelection()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (SelectedActor.IsValid())
    {
        return;
    }

    // 弱引用无效：清空一切，避免 UI 继续引用旧 Items
    if (SelectedActor.Get() != nullptr || ItemPool.Num() > 0)
    {
        SelectedActor = nullptr;
        ClearItemPool();
        RefreshPanel(EInspectorRefreshReason::TargetInvalid);
    }
#endif
}


APlayerController* UInspectorWorldSubsystem::GetLocalPC() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    return World->GetFirstPlayerController();
}

void UInspectorWorldSubsystem::TryBindInputs()
{
   
#if RUNTIME_INSPECTOR_ENABLED
    APlayerController* PC = GetLocalPC();
    
    

    if (!PC) return;

    // 绑定到现有 InputComponent（大多数模板都有），否则创建一个
    if (!PC->InputComponent)
    {
        PC->InputComponent = NewObject<UInputComponent>(PC, TEXT("RuntimeInspectorInputComponent"));
        PC->InputComponent->RegisterComponent();
    }

    PC->InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &UInspectorWorldSubsystem::OnUndoKeyPressed);
    PC->InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &UInspectorWorldSubsystem::OnRedoKeyPressed);
    // F1 Toggle
    PC->InputComponent->BindKey(EKeys::O, IE_Pressed, this, &UInspectorWorldSubsystem::Toggle);

    // F2 Pick
    PC->InputComponent->BindKey(EKeys::P, IE_Pressed, this, &UInspectorWorldSubsystem::PickActorInView);

    bInputsBound = true;
#endif
}

void UInspectorWorldSubsystem::Toggle()
{
    UE_LOG(LogTemp, Log, TEXT("Test:::::::Toggle"));
#if RUNTIME_INSPECTOR_ENABLED
    if (bOpen) Close();
    else Open();
#endif
}

void UInspectorWorldSubsystem::Open()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (bOpen) return;
    bOpen = true;


    RegisterInputProcessor();

    EnsurePanelWidget();

    if (UUserWidget* W = PanelWidget.Get())
    {
        W->AddToViewport(9999);
    }

    // 让鼠标可用（也可以后续做成设置项）
    if (APlayerController* PC = GetLocalPC())
    {
        PC->bShowMouseCursor = true;
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }

    RefreshPanel();
#endif
}

void UInspectorWorldSubsystem::Close()
{
#if RUNTIME_INSPECTOR_ENABLED
    UnregisterInputProcessor();

    if (!bOpen) return;
    bOpen = false;

    if (UUserWidget* W = PanelWidget.Get())
    {
        W->RemoveFromParent();
    }

    if (APlayerController* PC = GetLocalPC())
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
#endif
}

void UInspectorWorldSubsystem::EnsurePanelWidget()
{
#if RUNTIME_INSPECTOR_ENABLED
    if (PanelWidget.IsValid()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UClass* WidgetCls = PanelWidgetClass.LoadSynchronous();
    if (!WidgetCls) return;

    UUserWidget* W = CreateWidget<UUserWidget>(World, WidgetCls);
    PanelWidget = W;
#endif
}

void UInspectorWorldSubsystem::PickActorInView()
{
    UE_LOG(LogTemp, Log, TEXT("Test:::::::PickActorInView"));
#if RUNTIME_INSPECTOR_ENABLED
    if (!bOpen) return;

    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector Start = CamLoc;
    const FVector End = Start + CamRot.Vector() * 100000.f;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RuntimeInspectorPick), true);
    Params.bReturnPhysicalMaterial = false;

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            SetSelectedActor(HitActor);
        }
    }
#endif
}

void UInspectorWorldSubsystem::SetSelectedActor(AActor* NewActor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (SelectedActor.Get() == NewActor)
    {
        return;
    }

    UnbindFromSelectedActor();
    SelectedActor = NewActor;
    BindToSelectedActor(NewActor);

    // 切换选中对象：清理 ItemPool，避免旧 Items 残留
    ClearItemPool();
    ClearModified();
    RefreshPanel(EInspectorRefreshReason::StructureChanged);
#endif
}

//void UInspectorWorldSubsystem::RefreshPanel()
//{
//#if RUNTIME_INSPECTOR_ENABLED
//    // 这里我们不直接依赖你面板的具体类，先用“BlueprintImplementableEvent”方式解耦
//    // 让 WBP_InspectorPanel 自己去拉取 SelectedActor 并刷新列表
//    if (UUserWidget* W = PanelWidget.Get())
//    {
//        // 约定：WBP_InspectorPanel 里实现一个名为 "OnInspectorRefresh" 的事件
//        static const FName RefreshFuncName(TEXT("OnInspectorRefresh"));
//        if (W->GetClass()->FindFunctionByName(RefreshFuncName))
//        {
//            W->ProcessEvent(W->FindFunction(RefreshFuncName), nullptr);
//        }
//    }
//#endif
//}
void UInspectorWorldSubsystem::RefreshPanel()
{
#if RUNTIME_INSPECTOR_ENABLED
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);
#endif
}

void UInspectorWorldSubsystem::RefreshPanel(EInspectorRefreshReason Reason)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (UUserWidget* W = PanelWidget.Get())
    {
        // 推荐：WBP 实现 OnInspectorRefreshEx(Reason)
        static const FName RefreshExName(TEXT("OnInspectorRefreshEx"));
        if (UFunction* Fn = W->GetClass()->FindFunctionByName(RefreshExName))
        {
            struct FParams
            {
                EInspectorRefreshReason Reason;
            };
            FParams Params{ Reason };
            W->ProcessEvent(Fn, &Params);
            return;
        }

        // 兼容旧：OnInspectorRefresh()
        static const FName RefreshName(TEXT("OnInspectorRefresh"));
        if (UFunction* FnOld = W->GetClass()->FindFunctionByName(RefreshName))
        {
            W->ProcessEvent(FnOld, nullptr);
        }
    }
#endif
}

void UInspectorWorldSubsystem::GetGroupItemsForSelected(const FString& SearchText, TArray<UObject*>& OutGroups)
{
#if !UE_BUILD_SHIPPING
    OutGroups.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    const bool bSearchMode = !SearchText.IsEmpty();

    // Actor 根组
    UInspectorGroupItem* ActorGroup = GetOrCreateGroupItem(TEXT("ROOT_ACTOR"));
    ActorGroup->Kind = EInspectorGroupKind::RootActor;
    ActorGroup->DisplayName = TEXT("Actor");
    ActorGroup->StableKey = TEXT("ROOT_ACTOR");
    ActorGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(ActorGroup->StableKey, true);
    OutGroups.Add(ActorGroup);

    // Components 根组
    UInspectorGroupItem* CompRoot = GetOrCreateGroupItem(TEXT("ROOT_COMPONENTS"));
    CompRoot->Kind = EInspectorGroupKind::RootComponents;
    CompRoot->DisplayName = TEXT("Components");
    CompRoot->StableKey = TEXT("ROOT_COMPONENTS");
    CompRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(CompRoot->StableKey, true);
    OutGroups.Add(CompRoot);

    if (!CompRoot->bExpanded) return;

    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);

    for (UActorComponent* Comp : Components)
    {
        if (!IsWhitelistedComponent(Comp)) continue;

        const FString CompKey = MakeComponentKey(ActorPtr, Comp);

        UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
        CompGroup->Kind = EInspectorGroupKind::Component;
        CompGroup->TargetObject = Comp;
        CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
        CompGroup->StableKey = CompKey;
        CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);
        OutGroups.Add(CompGroup);

        // Materials 结构（只加 Group，不加参数）
        if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
        {
            const FString MatRootKey = CompKey + TEXT(":MATERIALS");

            UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
            MatRoot->Kind = EInspectorGroupKind::Component;
            MatRoot->TargetObject = SMC; // ✅ 关键：让 UI 点击能拿到 InComp
            MatRoot->DisplayName = TEXT("Materials");
            MatRoot->StableKey = MatRootKey;
            MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRootKey, false);
            OutGroups.Add(MatRoot);

            if (MatRoot->bExpanded)
            {
                const int32 NumMats = SMC->GetNumMaterials();
                for (int32 Slot = 0; Slot < NumMats; ++Slot)
                {
                    UMaterialInterface* Mat = SMC->GetMaterial(Slot);
                    if (!Mat) continue;

                    const FString SlotKey = MatRootKey + FString::Printf(TEXT(":MAT:%d"), Slot);

                    
                    UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
                    SlotGroup->Kind = EInspectorGroupKind::Component;
                    SlotGroup->TargetObject = SMC;
                    SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
                    SlotGroup->StableKey = SlotKey;
                    SlotGroup->MaterialSlotIndex = Slot;
                    // Slot 索引你如果还没加字段，就先靠 StableKey 解析（见下）
                    SlotGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(SlotKey, false);
                    OutGroups.Add(SlotGroup);
                }
            }
        }
    }
#endif
}


void UInspectorWorldSubsystem::GetPropertyItemsForSelected(const FString& SearchText, TArray<UObject*>& OutItems)
{

#if !UE_BUILD_SHIPPING


    OutItems.Reset();

   
    AActor* ActorPtr = nullptr;
    {
        ActorPtr = SelectedActor.Get(); // <<< 如果你这里编译报错，就换成 SelectedActor.Get()
    }

    if (!ActorPtr)
    {
        return;
    }

    const bool bSearchMode = !SearchText.IsEmpty();


    if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
    {
        UMeshComponent* MC = ViewMeshComp.Get();
        if (!MC || ViewMaterialSlot == INDEX_NONE)
        {
            return;
        }

        // 可选：加一个小标题组，让右侧有层级（也可以不加）
        UInspectorGroupItem* MatOnlyGroup = GetOrCreateGroupItem(TEXT("VIEW_MATERIAL_ONLY"));
        MatOnlyGroup->Kind = EInspectorGroupKind::Component;
        MatOnlyGroup->DisplayName = TEXT("Material Parameters");
        MatOnlyGroup->StableKey = TEXT("VIEW_MATERIAL_ONLY");
        MatOnlyGroup->bExpanded = true;
        OutItems.Add(MatOnlyGroup);

        // 只支持 StaticMeshComponent 先（你当前就是这样做的）
        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MC);
        if (!SMC) return;

        UMaterialInterface* Mat = SMC->GetMaterial(ViewMaterialSlot);
        if (!Mat) return;

        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;

        // Scalar
        Infos.Reset(); Ids.Reset();
        Mat->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Scalar);
            OutItems.Add(Item);
        }

        // Vector
        Infos.Reset(); Ids.Reset();
        Mat->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Vector);
            OutItems.Add(Item);
        }
        UE_LOG(LogTemp, Log, TEXT("Mode=MaterialOnly, Items=X"));
        return; // ✅ 关键：直接 return，阻止后面 Append 父级组件属性
    }

    // ---------- 1) Actor 根组 ----------
    {

        UInspectorGroupItem* ActorGroup = GetOrCreateGroupItem(TEXT("ROOT_ACTOR"));
        ActorGroup->Kind = EInspectorGroupKind::RootActor;
        ActorGroup->DisplayName = TEXT("Actor");
        ActorGroup->StableKey = TEXT("ROOT_ACTOR");
        ActorGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(ActorGroup->StableKey, true);
        OutItems.Add(ActorGroup);


        if (ActorGroup->bExpanded)
        {
            AppendPropertiesForObject(ActorPtr, SearchText, OutItems, TEXT(""), bSearchMode);
        }
    }

    // ---------- 2) Components 根组 ----------
    {

        UInspectorGroupItem* CompRoot = GetOrCreateGroupItem(TEXT("ROOT_COMPONENTS"));
        CompRoot->Kind = EInspectorGroupKind::RootComponents;
        CompRoot->DisplayName = TEXT("Components");
        CompRoot->StableKey = TEXT("ROOT_COMPONENTS");
        CompRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(CompRoot->StableKey, true);
        OutItems.Add(CompRoot);



        if (!CompRoot->bExpanded)
        {
            return;
        }

        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        for (UActorComponent* Comp : Components)
        {
            if (!IsWhitelistedComponent(Comp))
            {
                continue;
            }

            // 组件组 Key
            const FString CompKey = MakeComponentKey(ActorPtr, Comp);

            // 搜索模式：只显示“有命中属性”的组件，并强制展开（但不写回折叠状态）
            if (bSearchMode)
            {
                TArray<UObject*> TempProps;
                AppendPropertiesForObject(Comp, SearchText, TempProps, Comp->GetName(), true);

                if (TempProps.Num() == 0)
                {
                    continue;
                }

                UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
                CompGroup->Kind = EInspectorGroupKind::Component;
                CompGroup->TargetObject = Comp;
                CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
                CompGroup->StableKey = CompKey;
                CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);
                OutItems.Add(CompGroup);

              

                OutItems.Append(TempProps);
                continue;
            }

            // 非搜索模式：显示组件组 + 按折叠状态决定是否展开
            UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
            CompGroup->Kind = EInspectorGroupKind::Component;
            CompGroup->TargetObject = Comp;
            CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
            CompGroup->StableKey = CompKey;
            CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);
            OutItems.Add(CompGroup);

   

            if (CompGroup->bExpanded)
            {
                AppendPropertiesForObject(Comp, SearchText, OutItems, Comp->GetName(), false);


                if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
                {
                    // Materials 子组

                    // Materials Key
                    const FString MatRootKey = MakeComponentKey(ActorPtr, Comp) + TEXT(":MATERIALS");

                    UInspectorGroupItem* MatRoot = GetOrCreateGroupItem(MatRootKey);
                    MatRoot->Kind = EInspectorGroupKind::MaterialsRoot;
                    MatRoot->DisplayName = TEXT("Materials");
                    MatRoot->TargetObject = SMC;         // 关键：把 MeshComp 挂上，右侧需要它
                    MatRoot->MaterialSlotIndex = INDEX_NONE; // 新增字段（int32）
                    MatRoot->StableKey = MatRootKey;
                    MatRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(MatRoot->StableKey, false);
                    OutItems.Add(MatRoot);
                   
                    if (MatRoot->bExpanded)
                    {
                        const int32 NumMats = SMC->GetNumMaterials();
                        for (int32 Slot = 0; Slot < NumMats; ++Slot)
                        {
                            UMaterialInterface* Mat = SMC->GetMaterial(Slot);
                            if (!Mat) continue;

                            // ✅ Slot 节点
                            const FString SlotKey = MatRootKey + FString::Printf(TEXT(":MAT:%d"), Slot);

                            UInspectorGroupItem* SlotGroup = GetOrCreateGroupItem(SlotKey);
                            SlotGroup->Kind = EInspectorGroupKind::MaterialSlot; // 先不改 enum 也能跑
                            SlotGroup->TargetObject = SMC;
                            SlotGroup->DisplayName = FString::Printf(TEXT("Element %d: %s"), Slot, *GetNameSafe(Mat));
                            SlotGroup->StableKey = SlotKey;
                            SlotGroup->MaterialSlotIndex = Slot;          // ✅ 新字段
                            SlotGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(SlotKey, false);

                      
                            OutItems.Add(SlotGroup);

                            if (!SlotGroup->bExpanded) continue;

                            // ✅ Slot 展开后再塞参数
                            TArray<FMaterialParameterInfo> Infos;
                            TArray<FGuid> Ids;

                            Infos.Reset(); Ids.Reset();
                            Mat->GetAllScalarParameterInfo(Infos, Ids);
                            for (const FMaterialParameterInfo& Info : Infos)
                            {
                                UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, Slot, Info.Name, EInspectorMatParamType::Scalar);
                                OutItems.Add(Item); // ❗注意：别再 Item->Init(...) 了
                            }

                            Infos.Reset(); Ids.Reset();
                            Mat->GetAllVectorParameterInfo(Infos, Ids);
                            for (const FMaterialParameterInfo& Info : Infos)
                            {
                                UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, Slot, Info.Name, EInspectorMatParamType::Vector);
                                OutItems.Add(Item);
                            }
                        }
                       
                    }
                }
            }
        }
    }

#endif

}

void UInspectorWorldSubsystem::GetPinnedItemsForSelected(const FString& SearchText, TArray<UObject*>& OutPinnedItems)
{
#if !UE_BUILD_SHIPPING
    OutPinnedItems.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    // ✅ 先确认调用
    UE_LOG(LogTemp, Warning, TEXT("[RI] GetPinnedItemsForSelected CALLED. Keys=%d"), FavoriteKeys.Num());

    // 预取组件
    TArray<UActorComponent*> Components;
    ActorPtr->GetComponents(Components);

    // 稳定顺序（避免 UI 抖动）
    TArray<FString> Keys = FavoriteKeys.Array();
    Keys.Sort();

    if (Keys.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RI] PinnedKey0=%s"), *Keys[0]);
        UE_LOG(LogTemp, Warning, TEXT("[RI] SelectedActorPath=%s"), *ActorPtr->GetPathName());
    }

    const bool bSearchMode = !SearchText.IsEmpty();

    for (const FString& K : Keys)
    {
        if (K.IsEmpty()) continue;

        // 如果你不希望 pinned 被搜索框影响，可以直接注释掉这一段
        if (bSearchMode && !K.Contains(SearchText)) continue;

       
        // ---------- 新格式：M|ActorPath|CompPath|Slot|TypeInt|ParamName ----------
        if (K.Contains(TEXT("|")))
        {
            TArray<FString> Parts;
            K.ParseIntoArray(Parts, TEXT("|"), false);

            if (Parts.Num() > 0 && Parts[0] == TEXT("M"))
            {
                if (Parts.Num() < 6)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[RI] M key malformed: %s"), *K);
                    continue;
                }

                const FString& ActorPath = Parts[1];
                const FString& CompPath = Parts[2];
                const int32 Slot = FCString::Atoi(*Parts[3]);
                const int32 TypeInt = FCString::Atoi(*Parts[4]);
                const FName  ParamName(*Parts[5]);

                if (ParamName.IsNone())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[RI] M key ParamName none: %s"), *K);
                    continue;
                }

                // 只接受当前选中 Actor 的 pinned
                if (ActorPtr->GetPathName() != ActorPath)
                {
                    continue;
                }

                // 1) 先用完整 PathName 在当前 Actor 组件里匹配（最准确）
                UStaticMeshComponent* SMC = nullptr;
                for (UActorComponent* Comp : Components)
                {
                    if (!Comp) continue;

                    // ⚠️ 建议这里不要白名单过滤，避免材质 pin 被误杀
                    // if (!IsWhitelistedComponent(Comp)) continue;

                    if (Comp->GetPathName() == CompPath)
                    {
                        SMC = Cast<UStaticMeshComponent>(Comp);
                        break;
                    }
                }

                // 2) PathName 匹配不到就退化：按组件对象名匹配（解决 PIE/Undo 导致 Path 变化）
                if (!SMC)
                {
                    FString CompName = CompPath;
                    int32 DotPos = INDEX_NONE;
                    if (CompName.FindLastChar(TEXT('.'), DotPos))
                    {
                        CompName = CompName.Mid(DotPos + 1);
                    }

                    for (UActorComponent* Comp : Components)
                    {
                        if (!Comp) continue;
                        if (Comp->GetName() == CompName)
                        {
                            SMC = Cast<UStaticMeshComponent>(Comp);
                            if (SMC) break;
                        }
                    }
                }

                if (!SMC)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[RI] M resolve FAILED. CompPath=%s  Key=%s"), *CompPath, *K);
                    continue;
                }

                // ✅ 复用池对象 + 再 Init 一次，确保绑定正确
                UInspectorMaterialParamItem* MatItem =
                    GetOrCreateMaterialItem(SMC, Slot, ParamName, (EInspectorMatParamType)TypeInt);

                if (!MatItem)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[RI] M GetOrCreateMaterialItem null. SMC=%s Slot=%d Param=%s Type=%d"),
                        *GetNameSafe(SMC), Slot, *ParamName.ToString(), TypeInt);
                    continue;
                }

                MatItem->Init(SMC, Slot, ParamName, (EInspectorMatParamType)TypeInt);
                OutPinnedItems.Add(MatItem);

                UE_LOG(LogTemp, Warning, TEXT("[RI] M pinned OK: %s"), *GetNameSafe(MatItem));
                continue;
            }

            // 其它带 '|' 的 key（未来扩展用），先跳过
            continue;
        }

        // ---------- 旧格式：C:/Script/Engine.StaticMeshComponent:BoundsScale ----------
        int32 FirstColon = INDEX_NONE;
        if (!K.FindChar(TEXT(':'), FirstColon)) continue;

        const FString Prefix = K.Left(FirstColon);    // "A" or "C"
        const FString Rest = K.Mid(FirstColon + 1); // "/Script/...:Prop"

        int32 SecondColon = INDEX_NONE;
        if (!Rest.FindChar(TEXT(':'), SecondColon)) continue;

        const FString ClassPath = Rest.Left(SecondColon);   // "/Script/Engine.StaticMeshComponent"
        const FString PropStr = Rest.Mid(SecondColon + 1);// "BoundsScale"

        const FName PropName(*PropStr);
        if (PropName.IsNone()) continue;

        if (Prefix == TEXT("A"))
        {
            // 旧格式 A:ClassPath:Prop （如果你有这种 key）
            if (ActorPtr->GetClass()->GetPathName() == ClassPath)
            {
                if (UInspectorPropertyItem* Item = GetOrCreatePropertyItem(ActorPtr, PropName))
                {
                    OutPinnedItems.Add(Item);
                }
            }
            continue;
        }

        if (Prefix == TEXT("C"))
        {
            // ⚠️ 旧 key 没有组件实例信息，只能按“类”找第一个匹配的组件
            UActorComponent* FoundComp = nullptr;

            for (UActorComponent* Comp : Components)
            {
                if (!Comp) continue;
                if (!IsWhitelistedComponent(Comp)) continue;

                if (Comp->GetClass()->GetPathName() == ClassPath)
                {
                    FoundComp = Comp;
                    break;
                }
            }

            if (FoundComp)
            {
                if (UInspectorPropertyItem* Item = GetOrCreatePropertyItem(FoundComp, PropName))
                {
                    OutPinnedItems.Add(Item);
                }
            }
            continue;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[RI] PinnedItems Out=%d"), OutPinnedItems.Num());
#endif
}

bool UInspectorWorldSubsystem::CanUndo() const
{
#if !UE_BUILD_SHIPPING
    return UndoStack.Num() > 0;
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::CanRedo() const
{
#if !UE_BUILD_SHIPPING
    return RedoStack.Num() > 0;
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::RecordChange(const FInspectorChange& Change)
{
#if !UE_BUILD_SHIPPING
    if (bApplyingHistory) return;

    // Track modified state (Only Modified/Snapshot)
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
    {
        const FString Key = MakePropertySnapshotKey(Change.Target.Get(), Change.PropertyName);
        if (!Key.IsEmpty())
        {
            TrackModifiedForKey(Key, Change.OldValueText, Change.NewValueText);
        }
        break;
    }
    case EInspectorChangeType::MaterialScalar:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        const FString Key = MakeMaterialSnapshotKey(Comp, Change.MaterialIndex, EInspectorMatParamType::Scalar, Change.ParamName);
        if (!Key.IsEmpty())
        {
            TrackModifiedForKey(Key, FString::SanitizeFloat(Change.OldScalar), FString::SanitizeFloat(Change.NewScalar));
        }
        break;
    }
    case EInspectorChangeType::MaterialVector:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        const FString Key = MakeMaterialSnapshotKey(Comp, Change.MaterialIndex, EInspectorMatParamType::Vector, Change.ParamName);
        if (!Key.IsEmpty())
        {
            const FString OldText = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Change.OldVector.R, Change.OldVector.G, Change.OldVector.B, Change.OldVector.A);
            const FString NewText = FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), Change.NewVector.R, Change.NewVector.G, Change.NewVector.B, Change.NewVector.A);
            TrackModifiedForKey(Key, OldText, NewText);
        }
        break;
    }
    default:
        break;
    }

    UndoStack.Add(Change);
    RedoStack.Reset();

    // 可选：打印一条日志
    UE_LOG(LogTemp, Log, TEXT("[RI] Record: %s.%s %s -> %s"),
        *Change.DebugObjectName,
        *Change.PropertyName.ToString(),
        *Change.OldValueText,
        *Change.NewValueText);
#endif
}

bool UInspectorWorldSubsystem::ApplyChangeValue(UObject* Target, FName PropName, const FString& TextValue)
{
#if !UE_BUILD_SHIPPING
    FString Err;
    const bool bOK = InspectorPropertyUtils::SetValueFromText(Target, PropName, TextValue, &Err);
    if (!bOK)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RI] ApplyChangeValue failed: %s"), *Err);
    }
    return bOK;
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::Undo()
{
#if !UE_BUILD_SHIPPING
    if (UndoStack.Num() == 0) return false;

    FInspectorChange Change = UndoStack.Pop();

    // 按类型校验目标，而不是统一校验 Change.Target
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid() || Change.PropertyName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RI] Undo property target invalid: %s.%s"),
                *GetNameSafe(Change.Target.Get()), *Change.PropertyName.ToString());
            return false;
        }
        break;

    case EInspectorChangeType::MaterialScalar:
    case EInspectorChangeType::MaterialVector:
        if (!Change.TargetComponent.IsValid() || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RI] Undo material target invalid: %s slot=%d param=%s"),
                *GetNameSafe(Change.TargetComponent.Get()), Change.MaterialIndex, *Change.ParamName.ToString());
            return false;
        }
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("[RI] Undo unknown change type"));
        return false;
    }

    bApplyingHistory = true;
    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ false);
    bApplyingHistory = false;

    if (bOK)
    {
        RedoStack.Add(Change);

        // Keep "Only Modified" state in sync after Undo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }
        RefreshPanel(EInspectorRefreshReason::UndoRedo);
    }
    return bOK;
//    if (!CanUndo()) return false;
//
//    FInspectorChange Change = UndoStack.Pop();
//
//   /* if (!Change.Target.IsValid())
//    {
//        UE_LOG(LogTemp, Warning, TEXT("[RI] Undo target invalid"));
//        return false;
//    }*/
//
//    bApplyingHistory = true;
// //   const bool bOK = ApplyChangeValue(Change.Target.Get(), Change.PropertyName, Change.OldValueText);
//    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ false);
//    bApplyingHistory = false;
//
//    if (bOK)
//    {
//        RedoStack.Add(Change);
//      //  RefreshPanel(); // 先简单粗暴刷新（后面可做更细的行刷新）
//    }
//    return bOK;
//#else
//    return false;
#endif
}

bool UInspectorWorldSubsystem::Redo()
{
#if !UE_BUILD_SHIPPING

    if (RedoStack.Num() == 0) return false;

    FInspectorChange Change = RedoStack.Pop();

    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid() || Change.PropertyName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RI] Redo property target invalid: %s.%s"),
                *GetNameSafe(Change.Target.Get()), *Change.PropertyName.ToString());
            return false;
        }
        break;

    case EInspectorChangeType::MaterialScalar:
    case EInspectorChangeType::MaterialVector:
        if (!Change.TargetComponent.IsValid() || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RI] Redo material target invalid: %s slot=%d param=%s"),
                *GetNameSafe(Change.TargetComponent.Get()), Change.MaterialIndex, *Change.ParamName.ToString());
            return false;
        }
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("[RI] Redo unknown change type"));
        return false;
    }

    bApplyingHistory = true;
    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ true);
    bApplyingHistory = false;

    if (bOK)
    {
        UndoStack.Add(Change);

        // Keep "Only Modified" state in sync after Redo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }

        // Keep "Only Modified" state in sync after Redo
        if (Change.ChangeType == EInspectorChangeType::Property)
        {
            UpdateModifiedStateFromCurrentValue(Change.Target.Get(), Change.PropertyName);
        }
        else if (Change.ChangeType == EInspectorChangeType::MaterialScalar || Change.ChangeType == EInspectorChangeType::MaterialVector)
        {
            UpdateModifiedStateFromCurrentMaterial(Change.TargetComponent.Get(), Change.MaterialIndex, Change.ChangeType, Change.ParamName);
        }
        RefreshPanel(EInspectorRefreshReason::UndoRedo);
    }
    return bOK;
//    if (!CanRedo()) return false;
//
//    FInspectorChange Change = RedoStack.Pop();
//
// /*   if (!Change.Target.IsValid())
//    {
//        UE_LOG(LogTemp, Warning, TEXT("[RI] Redo target invalid"));
//        return false;
//    }*/
//
//    bApplyingHistory = true;
//    //const bool bOK = ApplyChangeValue(Change.Target.Get(), Change.PropertyName, Change.NewValueText);
//    const bool bOK = ApplyChange(Change, /*bUseNewValue*/ true);
//    bApplyingHistory = false;
//
//    if (bOK)
//    {
//        UndoStack.Add(Change);
//     //   RefreshPanel();
//    }
//    return bOK;
//#else
//    return false;
#endif
}

void UInspectorWorldSubsystem::OnUndoKeyPressed()
{
#if !UE_BUILD_SHIPPING
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    const bool bCtrl =
        PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);

    const bool bShift =
        PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);

    if (!bCtrl) return;

    // Ctrl+Z 或 Ctrl+Shift+Z 都做 Undo（你也可以把 Ctrl+Shift+Z 设为 Redo）
    Undo();
#endif
}

void UInspectorWorldSubsystem::OnRedoKeyPressed()
{
#if !UE_BUILD_SHIPPING
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    const bool bCtrl =
        PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);

    if (!bCtrl) return;

    // Ctrl+Y -> Redo
    Redo();
#endif
}

void UInspectorWorldSubsystem::RegisterInputProcessor()
{
#if !UE_BUILD_SHIPPING
    if (InputProcessor.IsValid()) return;
    if (!FSlateApplication::IsInitialized()) return;

    // 可选：只在 PIE/Game world 里启用，避免编辑器世界误注册
    if (UWorld* W = GetWorld())
    {
        if (!(W->WorldType == EWorldType::PIE || W->WorldType == EWorldType::Game))
        {
            return;
        }
    }

    InputProcessor = MakeShared<FRuntimeInspectorInputProcessor>();
    InputProcessor->Subsystem = this;

    FInputPreprocessorRegistrationKey Info;
    Info.Type = EInputPreProcessorType::PreGame;   // ✅ 比 Game 更高一档
    Info.Priority = INT32_MAX;                     // ✅ 同 bucket 内尽量靠前


    FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, EInputPreProcessorType::PreGame);
    //// 第二个参数是 priority，0 通常够用；数字越小越早拿到（视版本实现）
    //FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
    //FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor.ToSharedRef(), INT32_MAX);
#endif
}

void UInspectorWorldSubsystem::UnregisterInputProcessor()
{
#if !UE_BUILD_SHIPPING
    if (!InputProcessor.IsValid()) return;
    if (!FSlateApplication::IsInitialized())
    {
        InputProcessor.Reset();
       
        return;
    }

    FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
    InputProcessor.Reset();
#endif
}

FString UInspectorWorldSubsystem::MakeComponentKey(const AActor* Actor, const UActorComponent* Comp)
{
    if (!Actor || !Comp)
    {
        return TEXT("InvalidComponentKey");
    }

    return FString::Printf(TEXT("%s:%s:%s"),
        *Actor->GetPathName(),
        *Comp->GetFName().ToString(),
        *Comp->GetClass()->GetName());
}

bool UInspectorWorldSubsystem::IsWhitelistedComponent(const UActorComponent* Comp)
{
    if (!Comp) return false;

    // 白名单：StaticMesh / Light(含子类) / CharacterMovement
    if (Comp->IsA<UStaticMeshComponent>()) return true;
    if (Comp->IsA<ULightComponent>()) return true;
    if (Comp->IsA<UCharacterMovementComponent>()) return true;

    return false;
}

bool UInspectorWorldSubsystem::GetGroupExpanded(const FString& GroupKey, bool bDefault) const
{
    if (const bool* Found = GroupExpandedMap.Find(GroupKey))
    {
        return *Found;
    }
    return bDefault;
}

void UInspectorWorldSubsystem::ToggleGroupExpanded(const FString& GroupKey, bool bDefault)
{
    const bool Current = GetGroupExpanded(GroupKey, bDefault);
    GroupExpandedMap.Add(GroupKey, !Current);

    // 可选：调试用
    // UE_LOG(LogTemp, Log, TEXT("ToggleGroupExpanded Key=%s Current=%d New=%d"),
    //     *GroupKey, Current, !Current);
}

//void UInspectorWorldSubsystem::ToggleGroupExpanded(const FString& GroupKey)
//{
//    bool& b = GroupExpandedMap.FindOrAdd(GroupKey);
//    b = !b;
//	
//    // 这里不主动“推 UI 刷新”，因为你的 UI 刷新逻辑在 BP 里：
//    // 组标题点击后：ToggleGroupExpanded -> 再调用 GetPropertyItemsForSelected -> SetListItems
//}

bool UInspectorWorldSubsystem::DoesPropertyNameMatchSearch(const FString& PropertyName, const FString& SearchText) const
{
    if (SearchText.IsEmpty())
    {
        return true;
    }

    return PropertyName.Contains(SearchText, ESearchCase::IgnoreCase, ESearchDir::FromStart)
        || PropertyName.Contains(SearchText, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
}

void UInspectorWorldSubsystem::AppendPropertiesForObject(
    UObject* TargetObject,
    const FString& SearchText,
    TArray<UObject*>& OutItems,
    const FString& OwnerPrefixForUI,
    bool bSearchMode
)
{
    if (!TargetObject) return;

    // 只在“非搜索模式 + 白名单组件”时启用白名单过滤
    const TSet<FName>* Whitelist = nullptr;
    const bool bUseWhitelist =
        (!bSearchMode) &&
        SearchText.IsEmpty() &&
        (Cast<UActorComponent>(TargetObject) != nullptr) &&
        IsWhitelistedComponent(Cast<UActorComponent>(TargetObject));

    if (bUseWhitelist)
    {
        Whitelist = GetWhitelistForWhitelistedComponent(Cast<UActorComponent>(TargetObject));
    }

    UClass* Cls = TargetObject->GetClass();
    if (!Cls) return;

    for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        FProperty* Prop = *It;
        if (!Prop) continue;

        if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

        const bool bVisible =
            Prop->HasAnyPropertyFlags(CPF_Edit) ||
            Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
        if (!bVisible) continue;

        if (!IsSupportedByInspector(Prop)) continue;

        // ✅ 白名单过滤：只在非搜索模式启用
        if (Whitelist && !Whitelist->Contains(Prop->GetFName()))
        {
            continue;
        }

        const FString PropNameStr = Prop->GetName();
        if (!NameMatchesSearch(PropNameStr, SearchText)) continue;

       /* UInspectorPropertyItem* Item = NewObject<UInspectorPropertyItem>(this);
        Item->Init(TargetObject, Prop->GetFName());
        OutItems.Add(Item);*/
        UInspectorPropertyItem* Item = GetOrCreatePropertyItem(TargetObject, Prop->GetFName());
        OutItems.Add(Item);
    }
	//// 旧版参考v0.2（注释掉以免误导）：
    //if (!TargetObject) return;

    //UClass* Cls = TargetObject->GetClass();
    //if (!Cls) return;

    //for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    //{
    //    FProperty* Prop = *It;
    //    if (!Prop) continue;

    //    if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

    //    // 只列“编辑/可见”的，避免噪音（与你现有规则一致）
    //    const bool bVisible =
    //        Prop->HasAnyPropertyFlags(CPF_Edit) ||
    //        Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);

    //    if (!bVisible) continue;

    //    if (!IsSupportedByInspector(Prop)) continue;

    //    const FString PropNameStr = Prop->GetName();
    //    if (!NameMatchesSearch(PropNameStr, SearchText)) continue;

    //    UInspectorPropertyItem* Item = NewObject<UInspectorPropertyItem>(this);
    //    Item->Init(TargetObject, Prop->GetFName());    
    //    OutItems.Add(Item);
    //}
	//// 旧版参考v0.1（注释掉以免误导）：
    //if (!TargetObject)
    //{
    //    return;
    //}

    //UClass* Cls = TargetObject->GetClass();
    //if (!Cls)
    //{
    //    return;
    //}

    //// 只列“常见可展示”的基础属性类型，避免复杂容器/对象指针导致 UI 不可控
    //auto IsSupportedPropertyType = [](const FProperty* P) -> bool
    //    {
    //        if (!P) return false;

    //        if (P->IsA<FBoolProperty>()) return true;
    //        if (P->IsA<FEnumProperty>()) return true;
    //        if (const FByteProperty* BP = CastField<FByteProperty>(P)) { return BP->Enum != nullptr; }
    //        if (P->IsA<FNumericProperty>()) return true;
    //        if (P->IsA<FStrProperty>()) return true;
    //        if (P->IsA<FNameProperty>()) return true;
    //        if (P->IsA<FTextProperty>()) return true;

    //        return false;
    //    };

    //for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
    //{
    //    FProperty* Prop = *It;
    //    if (!Prop) continue;

    //    // 过滤掉明显不该显示的
    //    if (Prop->HasAnyPropertyFlags(CPF_Deprecated))
    //    {
    //        continue;
    //    }

    //    // 只展示“Edit 或 BlueprintVisible”的（更贴近你现有逻辑）
    //    const bool bVisible =
    //        Prop->HasAnyPropertyFlags(CPF_Edit) ||
    //        Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);

    //    if (!bVisible)
    //    {
    //        continue;
    //    }

    //    if (!IsSupportedPropertyType(Prop))
    //    {
    //        continue;
    //    }

    //    const FString PropName = Prop->GetName();
    //    if (!DoesPropertyNameMatchSearch(PropName, SearchText))
    //    {
    //        continue;
    //    }

    //    // 这里用你已有的 UInspectorPropertyItem（交接包里它提供 ApplyFromText/GetValueType/IsEditable）
    //    UInspectorPropertyItem* Item = NewObject<UInspectorPropertyItem>(this);
    //    Item->Init(TargetObject, Prop->GetFName());
    //    // 假设你现有 Item 有类似字段/初始化逻辑；
    //    // 交接包写的是“持有 Target(weak)、PropertyName”，所以我们按最通用的方式赋值：
    //    // 1) 如果你 Item 里是公开 UPROPERTY，直接赋值即可；
    //    // 2) 如果你 Item 里是私有 + Initialize(...)，用你工程现有 Initialize。
    //    //
    //    // 为了保证“直接能编译”，这里采用反射安全赋值：要求你 Item 里确实存在名为 Target / PropertyName 的 UPROPERTY。
    //    // 如果你命名不同（例如 TargetObject / Property），按下方“常见命名对照”改两行即可。

    //    // ---- 常见命名：Target (TWeakObjectPtr<UObject>) + PropertyName (FName) ----
    //    // 你如果就是这套，完全不用改：
    //    {
    //        FProperty* TargetProp = Item->GetClass()->FindPropertyByName(TEXT("Target"));
    //        FProperty* NameProp = Item->GetClass()->FindPropertyByName(TEXT("PropertyName"));
    //        if (TargetProp && NameProp)
    //        {
    //            void* TargetAddr = TargetProp->ContainerPtrToValuePtr<void>(Item);
    //            void* NameAddr = NameProp->ContainerPtrToValuePtr<void>(Item);

    //            // 写 Target
    //            if (FObjectPropertyBase* ObjP = CastField<FObjectPropertyBase>(TargetProp))
    //            {
    //                ObjP->SetObjectPropertyValue(TargetAddr, TargetObject);
    //            }
    //            else if (FWeakObjectProperty* WeakObjP = CastField<FWeakObjectProperty>(TargetProp))
    //            {
    //                WeakObjP->SetObjectPropertyValue(TargetAddr, TargetObject);
    //            }

    //            // 写 PropertyName
    //            if (FNameProperty* NP = CastField<FNameProperty>(NameProp))
    //            {
    //                NP->SetPropertyValue(NameAddr, FName(*PropName));
    //            }
    //        }
    //    }

    //    // 可选：如果你想在 UI 显示 “OwnerPrefixForUI”（比如组件名），你可以在 Item 里加一个 UPROPERTY FString OwnerPrefix;
    //    // 这里同样用反射写入，不会破坏编译：
    //    if (!OwnerPrefixForUI.IsEmpty())
    //    {
    //        if (FProperty* OwnerProp = Item->GetClass()->FindPropertyByName(TEXT("OwnerPrefix")))
    //        {
    //            if (FStrProperty* SP = CastField<FStrProperty>(OwnerProp))
    //            {
    //                void* OwnerAddr = OwnerProp->ContainerPtrToValuePtr<void>(Item);
    //                SP->SetPropertyValue(OwnerAddr, OwnerPrefixForUI);
    //            }
    //        }
    //    }

    //    OutItems.Add(Item);
    //}
}

bool UInspectorWorldSubsystem::IsSupportedByInspector(const FProperty* Prop)
{
    if (!Prop) return false;

    if (Prop->IsA<FBoolProperty>())   return true;
    if (Prop->IsA<FIntProperty>())    return true;
    if (Prop->IsA<FFloatProperty>())  return true;
    if (Prop->IsA<FDoubleProperty>()) return true;
    if (Prop->IsA<FStrProperty>())    return true;
    if (Prop->IsA<FNameProperty>())   return true;

    if (Prop->IsA<FEnumProperty>())   return true;
    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        return ByteProp->Enum != nullptr;
    }

    return false;
}

bool UInspectorWorldSubsystem::NameMatchesSearch(const FString& Name, const FString& SearchText)
{
    if (SearchText.IsEmpty()) return true;
    return Name.Contains(SearchText, ESearchCase::IgnoreCase);
}

UMaterialInstanceDynamic* UInspectorWorldSubsystem::GetOrCreateMID(UPrimitiveComponent* Comp, int32 MaterialIndex)
{
#if !UE_BUILD_SHIPPING
    if (!Comp || MaterialIndex < 0) return nullptr;

    UMaterialInterface* Current = Comp->GetMaterial(MaterialIndex);
    if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Current))
    {
        return Existing;
    }

    UMaterialInterface* Parent = Current;
    if (!Parent)
    {
        Parent = UMaterial::GetDefaultMaterial(MD_Surface);
    }

    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Comp);
    if (!MID) return nullptr;

    Comp->SetMaterial(MaterialIndex, MID);
    return MID;
#else
    return nullptr;
#endif
}

bool UInspectorWorldSubsystem::ApplyChange(const FInspectorChange& Change, bool bUseNewValue)
{
#if !UE_BUILD_SHIPPING
    switch (Change.ChangeType)
    {
    case EInspectorChangeType::Property:
        if (!Change.Target.IsValid()) return false;
        return ApplyChangeValue(Change.Target.Get(), Change.PropertyName, bUseNewValue ? Change.NewValueText : Change.OldValueText);

    case EInspectorChangeType::MaterialScalar:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        if (!Comp || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone()) return false;
        UMaterialInstanceDynamic* MID = GetOrCreateMID(Comp, Change.MaterialIndex);
        if (!MID) return false;

        MID->SetScalarParameterValue(Change.ParamName, bUseNewValue ? Change.NewScalar : Change.OldScalar);
        Comp->MarkRenderStateDirty();
        return true;
    }

    case EInspectorChangeType::MaterialVector:
    {
        UPrimitiveComponent* Comp = Change.TargetComponent.Get();
        if (!Comp || Change.MaterialIndex == INDEX_NONE || Change.ParamName.IsNone()) return false;
        UMaterialInstanceDynamic* MID = GetOrCreateMID(Comp, Change.MaterialIndex);
        if (!MID) return false;

        MID->SetVectorParameterValue(Change.ParamName, bUseNewValue ? Change.NewVector : Change.OldVector);
        Comp->MarkRenderStateDirty();
        return true;
    }

    default:
        return false;
    }
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::BindToSelectedActor(AActor* Actor)
{
    if (!Actor) return;
    Actor->OnDestroyed.AddDynamic(this, &UInspectorWorldSubsystem::HandleSelectedActorDestroyed);
}

void UInspectorWorldSubsystem::UnbindFromSelectedActor()
{
    if (AActor* Actor = SelectedActor.Get())
    {
        Actor->OnDestroyed.RemoveDynamic(this, &UInspectorWorldSubsystem::HandleSelectedActorDestroyed);
    }
}

void UInspectorWorldSubsystem::HandleSelectedActorDestroyed(AActor* DestroyedActor)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (DestroyedActor && DestroyedActor == SelectedActor.Get())
    {
        SelectedActor = nullptr;
        ClearItemPool();
        RefreshPanel(EInspectorRefreshReason::TargetInvalid);
    }
#endif
}

void UInspectorWorldSubsystem::ClearItemPool()
{
    ItemPool.Reset();
}

UInspectorGroupItem* UInspectorWorldSubsystem::GetOrCreateGroupItem(const FString& Key)
{
    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorGroupItem>(*Found);
    }

    UInspectorGroupItem* NewItem = NewObject<UInspectorGroupItem>(this);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

UInspectorPropertyItem* UInspectorWorldSubsystem::GetOrCreatePropertyItem(UObject* TargetObject, FName PropertyName)
{
    const FString Key = FString::Printf(TEXT("PROP:%s:%s"),
        *GetNameSafe(TargetObject),
        *PropertyName.ToString());

    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorPropertyItem>(*Found);
    }

    UInspectorPropertyItem* NewItem = NewObject<UInspectorPropertyItem>(this);
    NewItem->Init(TargetObject, PropertyName);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

UInspectorMaterialParamItem* UInspectorWorldSubsystem::GetOrCreateMaterialItem(
    UMeshComponent* Comp, int32 Slot, FName ParamName, EInspectorMatParamType Type)
{
    const FString Key = FString::Printf(TEXT("MAT:%s:%d:%d:%s"),
        *GetNameSafe(Comp),
        Slot,
        (int32)Type,
        *ParamName.ToString());

    if (TObjectPtr<UObject>* Found = ItemPool.Find(Key))
    {
        return Cast<UInspectorMaterialParamItem>(*Found);
    }

    UInspectorMaterialParamItem* NewItem = NewObject<UInspectorMaterialParamItem>(this);
    NewItem->Init(Comp, Slot, ParamName, Type);
    ItemPool.Add(Key, NewItem);
    return NewItem;
}

FString UInspectorWorldSubsystem::GetFavoritesFilePath() const
{
    // Saved/RuntimeInspector/Favorites.json
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Favorites.json"));
}

void UInspectorWorldSubsystem::LoadFavorites()
{
    FavoriteKeys.Reset();

    const FString Path = GetFavoritesFilePath();
    FString Content;
    if (!FPaths::FileExists(Path) || !FFileHelper::LoadFileToString(Content, *Path))
    {
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
    if (Root->TryGetArrayField(TEXT("favorites"), Arr) && Arr)
    {
        for (const TSharedPtr<FJsonValue>& V : *Arr)
        {
            FString Key;
            if (V.IsValid() && V->TryGetString(Key))
            {
                FavoriteKeys.Add(Key);
            }
        }
    }
}

void UInspectorWorldSubsystem::SaveFavorites() const
{
    const FString Path = GetFavoritesFilePath();
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> Arr;
    Arr.Reserve(FavoriteKeys.Num());
    for (const FString& Key : FavoriteKeys)
    {
        Arr.Add(MakeShared<FJsonValueString>(Key));
    }
    Root->SetArrayField(TEXT("favorites"), Arr);

    FString Out;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(Out, *Path);
}

FString UInspectorWorldSubsystem::MakeFavoriteKeyForProperty(UObject* TargetObject, FName PropertyName) const
{
    if (!TargetObject) return FString();

    // Actor vs Component 分开，避免同名属性误撞
    if (TargetObject->IsA(AActor::StaticClass()))
    {
        return FString::Printf(TEXT("A:%s:%s"),
            *TargetObject->GetClass()->GetPathName(),
            *PropertyName.ToString());
    }

    // 其他情况按 Component / UObject 也一样处理：用类路径
    return FString::Printf(TEXT("C:%s:%s"),
        *TargetObject->GetClass()->GetPathName(),
        *PropertyName.ToString());
}

bool UInspectorWorldSubsystem::IsFavoriteForItem(UInspectorPropertyItem* Item) const
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return false;
    UObject* TargetObj = Item->GetTargetObject();     // 如果你没有这个 getter，就用 Item->Target.Get() 自己取
    const FName PropName = Item->GetPropertyFName();  // 同理：没有就加一个 getter（推荐）
    const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);
    return !Key.IsEmpty() && FavoriteKeys.Contains(Key);
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::ToggleFavoriteForItem(UInspectorPropertyItem* Item)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (!Item) return;


    

    UObject* TargetObj = Item->GetTargetObject();
    const FName PropName = Item->GetPropertyFName();
    const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);

    UE_LOG(LogTemp, Warning, TEXT("[RI] ToggleFavoriteForItem: Item=%s  Key=%s  Before=%d"),
        *GetNameSafe(Item),
        *Key,
        FavoriteKeys.Num());

    if (Key.IsEmpty()) return;

    if (FavoriteKeys.Contains(Key))
    {
        FavoriteKeys.Remove(Key);
    }
    else
    {
        FavoriteKeys.Add(Key);
    }

    SaveFavorites();



    

    UE_LOG(LogTemp, Warning, TEXT("[RI] After=%d  Contains=%d"),
        FavoriteKeys.Num(),
        FavoriteKeys.Contains(Key));

    // 这里用你现有刷新机制：建议当作 UIStateChanged
    RefreshPanel(/*EInspectorRefreshReason::UIStateChanged*/);
#endif
}

void UInspectorWorldSubsystem::InsertPinnedGroupIfNeeded(TArray<UObject*>& OutItems)
{
#if RUNTIME_INSPECTOR_ENABLED
    if (FavoriteKeys.Num() == 0) return;

    TArray<UObject*> PinnedProps;
    TSet<UObject*> PinnedSet;

    // 1) 找出当前列表里已收藏的 PropertyItem
    for (UObject* Obj : OutItems)
    {
        if (!Obj) continue;

        UInspectorPropertyItem* PropItem = Cast<UInspectorPropertyItem>(Obj);
        if (!PropItem) continue;

        UObject* TargetObj = PropItem->GetTargetObject();
        const FName PropName = PropItem->GetPropertyFName();
        const FString Key = MakeFavoriteKeyForProperty(TargetObj, PropName);

        if (!Key.IsEmpty() && FavoriteKeys.Contains(Key))
        {
            PinnedProps.Add(PropItem);
            PinnedSet.Add(PropItem);
        }
    }

    if (PinnedProps.Num() == 0) return;

    // 2) Pinned 组（你也可以改成从 ItemPool 复用一个 group，避免频繁 new）
    UInspectorGroupItem* PinnedGroup = NewObject<UInspectorGroupItem>(this);
    PinnedGroup->DisplayName = TEXT("⭐ Pinned");
    PinnedGroup->StableKey = TEXT("PINNED_ROOT");
    PinnedGroup->bExpanded = true;

    // 3) 生成新列表：PinnedGroup + PinnedProps + 原列表(去掉重复的 pinned)
    TArray<UObject*> NewList;
    NewList.Reserve(OutItems.Num() + 1);

    NewList.Add(PinnedGroup);
    for (UObject* P : PinnedProps)
    {
        if (P) NewList.Add(P);
    }

    for (UObject* Obj : OutItems)
    {
        if (!Obj) continue;
        if (PinnedSet.Contains(Obj)) continue; // ✅ 关键：去重，避免 ListView 断言
        NewList.Add(Obj);
    }

    OutItems = MoveTemp(NewList);
#endif
}

static FString MakeMaterialFavoriteKey(UInspectorMaterialParamItem* M)
{
    if (!M) return FString();

    UMeshComponent* Comp = M->GetMeshComponent(); // 下面我会告诉你怎么加
    if (!Comp) return FString();

    AActor* Owner = Comp->GetOwner();
    const FString ActorPath = Owner ? Owner->GetPathName() : TEXT("None");
    const FString CompPath = Comp->GetPathName();

    const int32 Slot = M->GetSlotIndex();
    const int32 TypeInt = (int32)M->GetParamType();
    const FName ParamName = M->GetParamName();

    return FString::Printf(TEXT("M|%s|%s|%d|%d|%s"),
        *ActorPath,
        *CompPath,
        Slot,
        TypeInt,
        *ParamName.ToString());
}

void UInspectorWorldSubsystem::ToggleFavoriteForAnyItem(UObject* Item)
{
#if !UE_BUILD_SHIPPING
    if (!Item) return;

    // 1) 普通属性
    if (UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Item))
    {
        // ✅ 复用你现有的逻辑（你之前已经有 ToggleFavoriteForItem / MakeFavoriteKey）
        ToggleFavoriteForItem(P); // <- 如果你函数名不是这个，改成你自己的
        return;
    }

    // 2) 材质参数
    if (UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Item))
    {
        const FString Key = MakeMaterialFavoriteKey(M);

        UE_LOG(LogTemp, Warning, TEXT("[RI] MaterialKey=%s"), *Key);

        if (Key.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[RI] ToggleFavoriteForAnyItem(Material): key empty. Item=%s"), *GetNameSafe(Item));
            return;
        }

        const int32 Before = FavoriteKeys.Num();
        if (FavoriteKeys.Contains(Key))
        {
            FavoriteKeys.Remove(Key);
        }
        else
        {
            FavoriteKeys.Add(Key);
        }

        UE_LOG(LogTemp, Warning, TEXT("[RI] ToggleFavoriteForAnyItem(Material): %s  Before=%d After=%d"),
            *Key, Before, FavoriteKeys.Num());

        // 触发你现有的刷新（按你项目里已经用的那套）
        RefreshPanel(EInspectorRefreshReason::StructureChanged); // <- 如果你叫 OnInspectorRefreshEx/RefreshPanel/RequestRefresh，改成你实际函数名
        return;
    }

    
#endif
}

// =======================
// Snapshot / Only-Modified (v0.2)
// =======================

void UInspectorWorldSubsystem::ClearModified()
{
#if !UE_BUILD_SHIPPING
    BaselineValueByKey.Reset();
    ModifiedValueByKey.Reset();
#endif
}

FString UInspectorWorldSubsystem::GetSnapshotsDir() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
}

FString UInspectorWorldSubsystem::MakePropertySnapshotKey(UObject* TargetObject, FName PropertyName) const
{
#if !UE_BUILD_SHIPPING
    if (!TargetObject || PropertyName.IsNone()) return FString();

    FString ActorPath;
    FString CompPath;

    if (AActor* A = Cast<AActor>(TargetObject))
    {
        ActorPath = A->GetPathName();
    }
    else if (UActorComponent* C = Cast<UActorComponent>(TargetObject))
    {
        CompPath = C->GetPathName();
        if (AActor* Owner = C->GetOwner())
        {
            ActorPath = Owner->GetPathName();
        }
    }
    else
    {
        // Fallback: treat it as "component"-like object
        CompPath = TargetObject->GetPathName();
    }

    const FString ClassPath = TargetObject->GetClass() ? TargetObject->GetClass()->GetPathName() : TEXT("");

    // P|ActorPath|CompPath|ClassPath|PropName
    return FString::Printf(TEXT("P|%s|%s|%s|%s"),
        *ActorPath,
        *CompPath,
        *ClassPath,
        *PropertyName.ToString());
#else
    return FString();
#endif
}

FString UInspectorWorldSubsystem::MakeMaterialSnapshotKey(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorMatParamType Type, FName ParamName) const
{
#if !UE_BUILD_SHIPPING
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return FString();

    AActor* Owner = Comp->GetOwner();
    const FString ActorPath = Owner ? Owner->GetPathName() : TEXT("");
    const FString CompPath = Comp->GetPathName();

    // M|ActorPath|CompPath|Slot|TypeInt|ParamName
    return FString::Printf(TEXT("M|%s|%s|%d|%d|%s"),
        *ActorPath,
        *CompPath,
        SlotIndex,
        (int32)Type,
        *ParamName.ToString());
#else
    return FString();
#endif
}

void UInspectorWorldSubsystem::TrackModifiedForKey(const FString& Key, const FString& OldText, const FString& NewText)
{
#if !UE_BUILD_SHIPPING
    if (Key.IsEmpty()) return;

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, OldText);
    }

    const FString& Baseline = BaselineValueByKey[Key];

    if (NewText == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, NewText);
    }
#endif
}

void UInspectorWorldSubsystem::UpdateModifiedStateFromCurrentValue(UObject* TargetObject, FName PropertyName)
{
#if !UE_BUILD_SHIPPING
    if (!TargetObject || PropertyName.IsNone()) return;

    const FString Key = MakePropertySnapshotKey(TargetObject, PropertyName);
    if (Key.IsEmpty()) return;

    FString Current;
    if (!InspectorPropertyUtils::GetValueAsText(TargetObject, PropertyName, Current))
    {
        return;
    }

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, Current);
        ModifiedValueByKey.Remove(Key);
        return;
    }

    const FString& Baseline = BaselineValueByKey[Key];
    if (Current == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, Current);
    }
#endif
}

static FString RI_GetMaterialParamValueText(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName)
{
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return TEXT("");

    UMaterialInterface* Mat = Comp->GetMaterial(SlotIndex);
    if (!Mat) return TEXT("");

    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat))
    {
        if (ChangeType == EInspectorChangeType::MaterialScalar)
        {
            const float V = MID->K2_GetScalarParameterValue(ParamName);
            return FString::SanitizeFloat(V);
        }
        else
        {
            const FLinearColor C = MID->K2_GetVectorParameterValue(ParamName);
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
    }

    const FMaterialParameterInfo Info(ParamName);
    if (ChangeType == EInspectorChangeType::MaterialScalar)
    {
        float V = 0.f;
        if (Mat->GetScalarParameterValue(Info, V))
        {
            return FString::SanitizeFloat(V);
        }
        return TEXT("");
    }
    else
    {
        FLinearColor C;
        if (Mat->GetVectorParameterValue(Info, C))
        {
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
        return TEXT("");
    }
}

void UInspectorWorldSubsystem::UpdateModifiedStateFromCurrentMaterial(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName)
{
#if !UE_BUILD_SHIPPING
    if (!Comp || SlotIndex == INDEX_NONE || ParamName.IsNone()) return;

    const EInspectorMatParamType Type = (ChangeType == EInspectorChangeType::MaterialScalar)
        ? EInspectorMatParamType::Scalar
        : EInspectorMatParamType::Vector;

    const FString Key = MakeMaterialSnapshotKey(Comp, SlotIndex, Type, ParamName);
    if (Key.IsEmpty()) return;

    const FString Current = RI_GetMaterialParamValueText(Comp, SlotIndex, ChangeType, ParamName);

    if (!BaselineValueByKey.Contains(Key))
    {
        BaselineValueByKey.Add(Key, Current);
        ModifiedValueByKey.Remove(Key);
        return;
    }

    const FString& Baseline = BaselineValueByKey[Key];
    if (Current == Baseline)
    {
        ModifiedValueByKey.Remove(Key);
    }
    else
    {
        ModifiedValueByKey.Add(Key, Current);
    }
#endif
}

bool UInspectorWorldSubsystem::ExportSnapshot(bool bOnlyModified, FString& OutFilePath, FString& OutError)
{
#if !UE_BUILD_SHIPPING

    if (!IsRIEnabled())
    {
        OutError = TEXT("RuntimeInspector disabled (ri.Enable=0)");
        return false;
    }

    OutError.Reset();
    OutFilePath.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    const FString SelectedActorPath = ActorPtr->GetPathName();
    const FString SelectedActorClass = ActorPtr->GetClass()->GetPathName();
    const FString SelectedActorBaseName = RI_ExtractActorBaseName(ActorPtr->GetName());

    Root->SetNumberField(TEXT("schemaVersion"), 2);
    Root->SetStringField(TEXT("createdAtUtc"), FDateTime::UtcNow().ToIso8601());

    if (UWorld* World = GetWorld())
    {
        Root->SetStringField(TEXT("map"), World->GetMapName());
    }

    Root->SetStringField(TEXT("selectedActorPath"), SelectedActorPath);

    // v2 关键字段（用于抗 UAID 导入）
    Root->SetStringField(TEXT("selectedActorClass"), SelectedActorClass);
    Root->SetStringField(TEXT("selectedActorBaseName"), SelectedActorBaseName);


    TArray<TSharedPtr<FJsonValue>> Entries;

    auto AddPropertyEntry = [&](const FString& ActorPath,
        const FString& CompPath,
        const FString& ObjClassPath,
        const FString& ActorClassPath,
        const FString& ActorBaseName,
        const FString& PropName,
        const FString& ValueText,
        bool bHasValueInt=false,
        int64 ValueInt=0)
        {
            TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
            E->SetStringField(TEXT("kind"), TEXT("property"));
            E->SetStringField(TEXT("actorPath"), ActorPath);
            E->SetStringField(TEXT("componentPath"), CompPath);

            // 旧字段：对象 class（Actor 或 Component）
            E->SetStringField(TEXT("class"), ObjClassPath);

            // v2：用于定位 actor
            E->SetStringField(TEXT("actorClass"), ActorClassPath);
            E->SetStringField(TEXT("actorBaseName"), ActorBaseName);

            E->SetStringField(TEXT("property"), PropName);
            E->SetStringField(TEXT("value"), ValueText);

            // ✅ 新增：Enum/ByteEnum 等，用整数更稳
            if (bHasValueInt)
            {
                E->SetNumberField(TEXT("valueInt"), (double)ValueInt);
            }

            Entries.Add(MakeShared<FJsonValueObject>(E));
        };
    auto AddMaterialEntry = [&](const FString& ActorPath,
        const FString& CompPath,
        const FString& ActorClassPath,
        const FString& ActorBaseName,
        int32 SlotIndex,
        EInspectorMatParamType Type,
        const FString& ParamName,
        const FString& ValueText)
        {
            TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
            E->SetStringField(TEXT("kind"), (Type == EInspectorMatParamType::Scalar) ? TEXT("materialScalar") : TEXT("materialVector"));
            E->SetStringField(TEXT("actorPath"), ActorPath);
            E->SetStringField(TEXT("componentPath"), CompPath);

            // v2 新字段：用于导入定位 Actor
            E->SetStringField(TEXT("actorClass"), ActorClassPath);
            E->SetStringField(TEXT("actorBaseName"), ActorBaseName);

            E->SetNumberField(TEXT("slot"), SlotIndex);
            E->SetNumberField(TEXT("paramType"), (int32)Type);
            E->SetStringField(TEXT("param"), ParamName);
            E->SetStringField(TEXT("value"), ValueText);
            Entries.Add(MakeShared<FJsonValueObject>(E));
        };


    if (bOnlyModified)
    {
        // Only export keys in ModifiedValueByKey
        for (const TPair<FString, FString>& KV : ModifiedValueByKey)
        {
            const FString& Key = KV.Key;
            const FString& Val = KV.Value;

            if (Key.StartsWith(TEXT("P|")))
            {
                TArray<FString> Parts;
                Key.ParseIntoArray(Parts, TEXT("|"), false);
                // P|ActorPath|CompPath|ClassPath|PropName
                if (Parts.Num() >= 5)
                {
                    const FString ActorPath = Parts[1];
                    const FString ActorBase = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));

                    AddPropertyEntry(Parts[1], Parts[2], Parts[3], SelectedActorClass, ActorBase, Parts[4], Val);

                }
            }
            else if (Key.StartsWith(TEXT("M|")))
            {
                TArray<FString> Parts;
                Key.ParseIntoArray(Parts, TEXT("|"), false);
                // M|ActorPath|CompPath|Slot|TypeInt|ParamName
                if (Parts.Num() >= 6)
                {
                 
                    const FString ActorPath = Parts[1];
                    const FString ActorBase = RI_ExtractActorBaseName(RI_ExtractTailAfterLastDot(ActorPath));

                    const int32 SlotIndex = FCString::Atoi(*Parts[3]);
                    const int32 TypeInt = FCString::Atoi(*Parts[4]);
                    const EInspectorMatParamType Type = (EInspectorMatParamType)TypeInt;

                    AddMaterialEntry(Parts[1], Parts[2], SelectedActorClass, ActorBase, SlotIndex, Type, Parts[5], Val);

                }
            }
        }
    }
    else
    {
        // Export Actor + whitelisted components (matching what the inspector can display by default).
        auto ExportObjectProps = [&](UObject* Obj, const TSet<FName>* Whitelist)
        {
            if (!Obj) return;
            UClass* Cls = Obj->GetClass();
            if (!Cls) return;

            /*const FString ActorPath = ActorPtr->GetPathName();
            const FString CompPath = Obj->IsA(AActor::StaticClass()) ? TEXT("") : Obj->GetPathName();
            const FString ClassPath = Cls->GetPathName();*/

            const FString ActorPath = SelectedActorPath;
            const FString CompPath = Obj->IsA(AActor::StaticClass()) ? TEXT("") : Obj->GetPathName();
            const FString ObjClassPath = Cls->GetPathName();

            for (TFieldIterator<FProperty> It(Cls, EFieldIteratorFlags::IncludeSuper); It; ++It)
            {
                FProperty* Prop = *It;
                if (!Prop) continue;
                if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) continue;

                const bool bVisible = Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_BlueprintVisible);
                if (!bVisible) continue;

                if (!IsSupportedByInspector(Prop)) continue;
                if (Whitelist && !Whitelist->Contains(Prop->GetFName())) continue;

               

                FString ValText;
                if (!InspectorPropertyUtils::GetValueAsText(Obj, Prop->GetFName(), ValText))
                {
                    continue;
                }

                bool bHasValueInt = false;
                int64 ValueInt = 0;

                // 1) FEnumProperty（强类型 enum class / enum）
                if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
                {
                    if (const FNumericProperty* Under = EnumProp->GetUnderlyingProperty())
                    {
                        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
                        ValueInt = Under->GetSignedIntPropertyValue(ValuePtr);
                        bHasValueInt = true;
                    }
                }
                // 2) Byte + Enum（UE 很多 enum 是这种）
                else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
                {
                    if (ByteProp->Enum)
                    {
                        ValueInt = (int64)ByteProp->GetPropertyValue_InContainer(Obj);
                        bHasValueInt = true;
                    }
                }

                if (!InspectorPropertyUtils::CanSetFromText(Obj, Prop)) 
                {
                    continue;
                }

                AddPropertyEntry(ActorPath, CompPath, ObjClassPath, SelectedActorClass, SelectedActorBaseName,
                    Prop->GetName(), ValText,
                    bHasValueInt, ValueInt);

                //AddPropertyEntry(ActorPath, CompPath, ObjClassPath, SelectedActorClass, SelectedActorBaseName, Prop->GetName(), ValText);
            }
        };

        ExportObjectProps(ActorPtr, nullptr);

        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        for (UActorComponent* Comp : Components)
        {
            if (!IsWhitelistedComponent(Comp)) continue;

            const TSet<FName>* WL = GetWhitelistForWhitelistedComponent(Comp);
            ExportObjectProps(Comp, WL);
        }

        // If user is in MaterialOnly view, also export that slot parameters.
        if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
        {
            UMeshComponent* MC = ViewMeshComp.Get();
            if (MC && ViewMaterialSlot != INDEX_NONE)
            {
                UMaterialInterface* Mat = MC->GetMaterial(ViewMaterialSlot);
                if (Mat)
                {
                    const FString APath = ActorPtr->GetPathName();
                    const FString CPath = MC->GetPathName();

                    TArray<FMaterialParameterInfo> Infos;
                    TArray<FGuid> Ids;

                    Infos.Reset(); Ids.Reset();
                    Mat->GetAllScalarParameterInfo(Infos, Ids);
                    for (const FMaterialParameterInfo& Info : Infos)
                    {
                        const FString V = RI_GetMaterialParamValueText(Cast<UPrimitiveComponent>(MC), ViewMaterialSlot, EInspectorChangeType::MaterialScalar, Info.Name);
                        AddMaterialEntry(APath, CPath, SelectedActorClass, SelectedActorBaseName, ViewMaterialSlot, EInspectorMatParamType::Scalar, Info.Name.ToString(), V);
                    }

                    Infos.Reset(); Ids.Reset();
                    Mat->GetAllVectorParameterInfo(Infos, Ids);
                    for (const FMaterialParameterInfo& Info : Infos)
                    {
                        const FString V = RI_GetMaterialParamValueText(Cast<UPrimitiveComponent>(MC), ViewMaterialSlot, EInspectorChangeType::MaterialVector, Info.Name);
                        AddMaterialEntry(APath, CPath, SelectedActorClass, SelectedActorBaseName, ViewMaterialSlot, EInspectorMatParamType::Vector, Info.Name.ToString(), V);
                    }
                }
            }
        }
    }

    Root->SetArrayField(TEXT("entries"), Entries);

    FString OutJson;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
    {
        OutError = TEXT("JSON serialize failed");
        return false;
    }

    const FString Dir = GetSnapshotsDir();
    IFileManager::Get().MakeDirectory(*Dir, true);

    const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    const FString SafeActorName = SelectedActorBaseName; // 不带 UAID
    const FString FileName = FString::Printf(TEXT("Snapshot_%s_%s.json"), *SafeActorName, *Timestamp);
    OutFilePath = FPaths::Combine(Dir, FileName);

    if (!FFileHelper::SaveStringToFile(OutJson, *OutFilePath))
    {
        OutError = TEXT("Save failed");
        OutFilePath.Reset();
        PushToast(ERIToastType::Error, OutError, 3.0f);
        return false;
    }
    PushToast(ERIToastType::Success, FString::Printf(TEXT("Exported (%d changes)"), Entries.Num()), 1.5f);
    UE_LOG(LogTemp, Log, TEXT("[RI] Snapshot exported: %s (entries=%d, onlyModified=%d)"), *OutFilePath, Entries.Num(), bOnlyModified ? 1 : 0);
    return true;
#else
    OutError = TEXT("Not available in Shipping");
    PushToast(ERIToastType::Error, OutError, 3.0f);
    return false;
#endif
}
static bool RI_SetEnumPropertyFromInt(UObject* TargetObj, const FName PropName, int64 ValueInt, FString& OutError)
{
    OutError.Reset();
    if (!TargetObj)
    {
        OutError = TEXT("TargetObj null");
        return false;
    }

    FProperty* Prop = TargetObj->GetClass()->FindPropertyByName(PropName);
    if (!Prop)
    {
        OutError = TEXT("Property not found");
        return false;
    }

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(TargetObj);

    // 1) FEnumProperty
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        FNumericProperty* Under = EnumProp->GetUnderlyingProperty();
        if (!Under)
        {
            OutError = TEXT("Enum underlying property missing");
            return false;
        }
        Under->SetIntPropertyValue(ValuePtr, ValueInt);
        return true;
    }

    // 2) Byte + Enum
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (ByteProp->Enum)
        {
            ByteProp->SetPropertyValue(ValuePtr, (uint8)ValueInt);
            return true;
        }
    }

    OutError = TEXT("Not an enum property");
    return false;
}
bool UInspectorWorldSubsystem::ImportSnapshot(const FString& InFilePath, FString& OutError)
{
#if !UE_BUILD_SHIPPING

    if (!IsRIEnabled())
    {
        OutError = TEXT("RuntimeInspector disabled (ri.Enable=0)");
        return false;
    }

    OutError.Reset();

    int32 AppliedCount = 0;
    int32 SkippedCount = 0;
    int32 HardFailCount = 0;
    FString CombinedWarnings;
    FString CombinedHardErrors;

    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *InFilePath))
    {
        OutError = TEXT("Failed to read file");
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid JSON");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
    if (!Root->TryGetArrayField(TEXT("entries"), Entries) || !Entries)
    {
        OutError = TEXT("Missing entries[]");
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutError = TEXT("World invalid");
        return false;
    }

    const int32 SchemaVersion = Root->HasField(TEXT("schemaVersion")) ? (int32)Root->GetNumberField(TEXT("schemaVersion")) : 1;

    auto ResolveActor = [&](const FString& ActorPath, const FString& ActorClass, const FString& ActorBaseName) -> AActor*
    {
        if (ActorPath.IsEmpty() && ActorBaseName.IsEmpty())
        {
            return nullptr;
        }

        // 1) 精确 path
        if (!ActorPath.IsEmpty())
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (It->GetPathName() == ActorPath)
                {
                    return *It;
                }
            }
        }

        // 2) v2：class + baseName（抗 UAID）
        if (!ActorBaseName.IsEmpty())
        {
            AActor* Best = nullptr;

            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* A = *It;
                if (!A) continue;

                const FString ThisBase = RI_ExtractActorBaseName(A->GetName());
                if (ThisBase != ActorBaseName)
                {
                    continue;
                }

                // 如果有 class，就要求 class match；没有 class 也允许
                if (!ActorClass.IsEmpty() && !RI_ClassMatches(A, ActorClass))
                {
                    continue;
                }

                Best = A;
                break;
            }

            if (Best)
            {
                return Best;
            }
        }

        // 3) 兜底：你原来的 FallbackName（path 最后段）
        if (!ActorPath.IsEmpty())
        {
            FString FallbackName = RI_ExtractTailAfterLastDot(ActorPath);

            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (It->GetName() == FallbackName)
                {
                    return *It;
                }
            }
        }

        return nullptr;
    };

    auto ResolveComponent = [&](AActor* Owner, const FString& ComponentPath) -> UActorComponent*
    {
        if (!Owner || ComponentPath.IsEmpty()) return nullptr;

        TArray<UActorComponent*> Comps;
        Owner->GetComponents(Comps);

        for (UActorComponent* C : Comps)
        {
            if (C && C->GetPathName() == ComponentPath)
            {
                return C;
            }
        }

        FString FallbackName = ComponentPath;
        int32 Dot = INDEX_NONE;
        if (ComponentPath.FindLastChar(TEXT('.'), Dot))
        {
            FallbackName = ComponentPath.Mid(Dot + 1);
        }

        for (UActorComponent* C : Comps)
        {
            if (C && C->GetName() == FallbackName)
            {
                return C;
            }
        }
        return nullptr;
    };

    // We treat the current state as baseline for keys we touch in this import.
    // (That makes "Only Modified" show exactly what this snapshot changed.)

    bool bAllOK = true;
    FString CombinedErrors;

    const bool bPrevApplying = bApplyingHistory;
    bApplyingHistory = true;

    for (const TSharedPtr<FJsonValue>& V : *Entries)
    {
        const TSharedPtr<FJsonObject> E = V.IsValid() ? V->AsObject() : nullptr;
        if (!E.IsValid()) continue;

        /*const FString Kind = E->GetStringField(TEXT("kind"));
        const FString ActorPath = E->GetStringField(TEXT("actorPath"));
        const FString CompPath = E->GetStringField(TEXT("componentPath"));

        AActor* TargetActor = ResolveActor(ActorPath);*/

        const FString Kind = E->GetStringField(TEXT("kind"));
        const FString ActorPath = E->GetStringField(TEXT("actorPath"));
        const FString CompPath = E->GetStringField(TEXT("componentPath"));

        // v2 新字段（兼容 v1：没有就从旧字段推）
        FString ActorClass;
        FString ActorBaseName;

        if (SchemaVersion >= 2)
        {
            E->TryGetStringField(TEXT("actorClass"), ActorClass);
            E->TryGetStringField(TEXT("actorBaseName"), ActorBaseName);
        }

        // v1 兼容：用 entry 的 "class" + actorPath 尾巴推 baseName
        if (ActorClass.IsEmpty())
        {
            if (SchemaVersion >= 2)
            {
                E->TryGetStringField(TEXT("actorClass"), ActorClass);
            }
            //E->TryGetStringField(TEXT("class"), ActorClass);
        }
        if (ActorBaseName.IsEmpty())
        {
            const FString FallbackInstName = RI_ExtractTailAfterLastDot(ActorPath);
            ActorBaseName = RI_ExtractActorBaseName(FallbackInstName);
        }

        AActor* TargetActor = ResolveActor(ActorPath, ActorClass, ActorBaseName);
        if (!TargetActor)
        {
            bAllOK = false;
            CombinedErrors += FString::Printf(
                TEXT("\nActor not found: path=%s base=%s class=%s"),
                *ActorPath, *ActorBaseName, *ActorClass
            );
            continue;
        }
        // --- Remap componentPath prefix if actor was resolved by fallback (UAID changed) ---
        auto RemapComponentPath = [&](const FString& InCompPath, const FString& OldActorPath, AActor* NewActor) -> FString
            {
                if (InCompPath.IsEmpty() || OldActorPath.IsEmpty() || !NewActor) return InCompPath;

                // 只有在 "旧ActorPath..." 这种情况下才做替换
                if (InCompPath.StartsWith(OldActorPath, ESearchCase::CaseSensitive))
                {
                    const FString NewActorPath = NewActor->GetPathName();
                    return NewActorPath + InCompPath.Mid(OldActorPath.Len());
                }
                return InCompPath;
            };

        const FString EffectiveCompPath = RemapComponentPath(CompPath, ActorPath, TargetActor);


        if (Kind == TEXT("property"))
        {
            const FString PropNameStr = E->GetStringField(TEXT("property"));
            const FString ValueText = E->GetStringField(TEXT("value"));

            UObject* TargetObj = TargetActor;
            if (!EffectiveCompPath.IsEmpty())
            {
                if (UActorComponent* C = ResolveComponent(TargetActor, EffectiveCompPath))
                {
                    TargetObj = C;
                }
                else
                {
                    bAllOK = false;
                    CombinedErrors += FString::Printf(TEXT("\nComponent not found: %s"), *EffectiveCompPath);
                    continue;
                }
            }

            UInspectorPropertyItem* Temp = NewObject<UInspectorPropertyItem>(this);
            Temp->Init(TargetObj, FName(*PropNameStr));

  
            const FString OldText = Temp->GetValueText();

            double ValueIntD = 0.0;
            const bool bHasValueInt = E->TryGetNumberField(TEXT("valueInt"), ValueIntD);
            const int64 ValueInt = (int64)ValueIntD;

            FString Err;

            // ✅ 优先：如果有 valueInt，就尝试写 enum
            bool bApplied = false;
            if (bHasValueInt)
            {
                if (RI_SetEnumPropertyFromInt(TargetObj, FName(*PropNameStr), ValueInt, Err))
                {
                    bApplied = true;
                }
                else
                {
                    // 如果不是 enum，或者写失败，就继续走文本（Err 会被覆盖）
                    Err.Reset();
                }
            }

            if (!bApplied)
            {
                if (!Temp->ApplyFromText(ValueText, Err))
                {
                    // ✅ 软错误：不可写/不支持 -> 跳过但不算失败
                    if (Err.Contains(TEXT("not editable"), ESearchCase::IgnoreCase) ||
                        Err.Contains(TEXT("not supported"), ESearchCase::IgnoreCase))
                    {
                        SkippedCount++;
                        CombinedWarnings += FString::Printf(
                            TEXT("\nSkipped: obj=%s prop=%s val=%s (%s)"),
                            *GetNameSafe(TargetObj), *PropNameStr, *ValueText, *Err);
                        continue;
                    }

                    // ✅ 硬错误：解析失败/其它真正异常 -> 记为失败
                    HardFailCount++;
                    CombinedHardErrors += FString::Printf(
                        TEXT("\nApply failed: obj=%s prop=%s val=%s (%s)"),
                        *GetNameSafe(TargetObj), *PropNameStr, *ValueText, *Err);
                    continue;
                }

                // 成功
                AppliedCount++;
            }

            const FString NewText = Temp->GetValueText();
            const FString Key = MakePropertySnapshotKey(TargetObj, FName(*PropNameStr));
            TrackModifiedForKey(Key, OldText, NewText);
        }
        else if (Kind == TEXT("materialScalar") || Kind == TEXT("materialVector"))
        {
            const int32 SlotIndex = (int32)E->GetNumberField(TEXT("slot"));
            const FString ParamNameStr = E->GetStringField(TEXT("param"));
            const FString ValueText = E->GetStringField(TEXT("value"));

            if (EffectiveCompPath.IsEmpty())
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nMaterial entry missing componentPath (actor=%s)"), *ActorPath);
                continue;
            }

            UActorComponent* C = ResolveComponent(TargetActor, EffectiveCompPath);
            UMeshComponent* MC = C ? Cast<UMeshComponent>(C) : nullptr;
            if (!MC)
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nMeshComponent not found: %s"), *EffectiveCompPath);
                continue;
            }

            const EInspectorMatParamType Type = (Kind == TEXT("materialScalar")) ? EInspectorMatParamType::Scalar : EInspectorMatParamType::Vector;

            UInspectorMaterialParamItem* Temp = NewObject<UInspectorMaterialParamItem>(this);
            Temp->Init(MC, SlotIndex, FName(*ParamNameStr), Type);

            const FString OldText = Temp->GetValueText();
            FString Err;
            //if (!Temp->ApplyFromText(ValueText, Err))
            //{
            //    bAllOK = false;
            //    CombinedErrors += FString::Printf(
            //        TEXT("\nApply failed: kind=%s comp=%s slot=%d param=%s val=%s (%s)"),
            //        *Kind, *GetNameSafe(MC), SlotIndex, *ParamNameStr, *ValueText, *Err);
            //    //CombinedErrors += FString::Printf(TEXT("\nApply failed: %s slot=%d %s (%s)"), *GetNameSafe(MC), SlotIndex, *ParamNameStr, *Err);
            //    continue;
            //}
            if (!Temp->ApplyFromText(ValueText, Err))
            {
                // ✅ 软错误：不可写/不支持 -> 跳过但不算失败
                if (Err.Contains(TEXT("not editable"), ESearchCase::IgnoreCase) ||
                    Err.Contains(TEXT("not supported"), ESearchCase::IgnoreCase))
                {
                    SkippedCount++;
                    CombinedWarnings += FString::Printf(
                        TEXT("\nSkipped: obj=%s prop=%s val=%s (%s)"),
                        *GetNameSafe(MC), *ParamNameStr, *ValueText, *Err);
                    continue;
                }

                // ✅ 硬错误：解析失败/其它真正异常 -> 记为失败
                HardFailCount++;
                CombinedHardErrors += FString::Printf(
                    TEXT("\nApply failed: obj=%s prop=%s val=%s (%s)"),
                    *GetNameSafe(MC), *ParamNameStr, *ValueText, *Err);
                continue;
            }

            // 成功
            AppliedCount++;
            const FString NewText = Temp->GetValueText();
            const FString Key = MakeMaterialSnapshotKey(Cast<UPrimitiveComponent>(MC), SlotIndex, Type, FName(*ParamNameStr));
            TrackModifiedForKey(Key, OldText, NewText);
        }
    }

    bApplyingHistory = bPrevApplying;

    RefreshPanel(EInspectorRefreshReason::ValuesChanged);


    const bool bOK = (HardFailCount == 0);

    if (bAllOK)
    {
        if (SkippedCount > 0)
        {
            OutError = CombinedWarnings.TrimStartAndEnd();
            PushToast(ERIToastType::Warning,
                FString::Printf(TEXT("Imported (%d applied, %d skipped — see log)"), AppliedCount, SkippedCount),
                3.0f);
            UE_LOG(LogTemp, Warning, TEXT("[RI] Import warnings:\n%s"), *OutError);
        }
        else
        {
            PushToast(ERIToastType::Success,
                FString::Printf(TEXT("Imported (%d applied)"), AppliedCount),
                1.5f);
        }
    }
    else
    {
        OutError = (CombinedHardErrors + TEXT("\n") + CombinedWarnings).TrimStartAndEnd();
        PushToast(ERIToastType::Error, TEXT("Import failed (see log)"), 3.5f);
        UE_LOG(LogTemp, Warning, TEXT("[RI] Import errors:\n%s"), *OutError);
    }

    return bAllOK;

    //if (!bAllOK)
    //{
    //    OutError = CombinedErrors.TrimStartAndEnd();
    //    PushToast(ERIToastType::Warning, TEXT("Imported with errors (see log)"), 3.0f);
    //    UE_LOG(LogTemp, Warning, TEXT("[RI] Import errors:\n%s"), *OutError);   // ✅新增
    //}
    //else {
    //    PushToast(ERIToastType::Success, TEXT("Imported"), 1.5f);
    //}
    //
    //UE_LOG(LogTemp, Log, TEXT("[RI] Snapshot imported: %s (ok=%d)"), *InFilePath, bAllOK ? 1 : 0);
    //return bAllOK;
#else
    OutError = TEXT("Not available in Shipping");
    return false;
#endif
}


static FString RI_ExtractActorShortNameFromPath(const FString& ActorPath)
{
    // ActorPath 形如：
    // /Game/...:PersistentLevel.BP_TestVarsActor_C_UAID_...
    int32 DotIdx = INDEX_NONE;
    if (ActorPath.FindLastChar(TEXT('.'), DotIdx))
    {
        FString Tail = ActorPath.Mid(DotIdx + 1); // BP_TestVarsActor_C_UAID_...
        // 去掉 _C_... 之后的部分，尽量友好
        int32 CIdx = INDEX_NONE;
        if (Tail.FindChar(TEXT('_'), CIdx))
        {
            // 先尝试截到 _C
            int32 CPos = Tail.Find(TEXT("_C"));
            if (CPos != INDEX_NONE)
            {
                return Tail.Left(CPos); // BP_TestVarsActor
            }
        }
        return Tail;
    }
    return ActorPath;
}

static int64 RI_ParseIsoUtcToUnixSeconds(const FString& IsoUtc)
{
    // "2026-01-05T22:08:26.576Z"
    FDateTime DT;
    if (FDateTime::ParseIso8601(*IsoUtc, DT))
    {
        return DT.ToUnixTimestamp();
    }
    return 0;
}

bool UInspectorWorldSubsystem::ReadSnapshotHeader(
    const FString& FullPath,
    FString& OutCreatedAtUtc,
    FString& OutMap,
    FString& OutSelectedActorPath,
    int32& OutEntryCount) const
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FullPath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    OutCreatedAtUtc = Root->GetStringField(TEXT("createdAtUtc"));
    OutMap = Root->GetStringField(TEXT("map"));
    OutSelectedActorPath = Root->GetStringField(TEXT("selectedActorPath"));

    OutEntryCount = 0;
    const TArray<TSharedPtr<FJsonValue>>* EntriesPtr = nullptr;
    if (Root->TryGetArrayField(TEXT("entries"), EntriesPtr) && EntriesPtr)
    {
        OutEntryCount = EntriesPtr->Num();
    }

    return true;
}

void UInspectorWorldSubsystem::GetSnapshotList(TArray<UObject*>& OutItems) const
{
    OutItems.Reset();

    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*Dir))
    {
        return;
    }

    TArray<FString> Files;
    PF.FindFilesRecursively(Files, *Dir, TEXT(".json"));

    TArray<UInspectorSnapshotItem*> Temp;

    for (const FString& FullPath : Files)
    {
        FString CreatedAtUtc, MapName, SelectedActorPath;
        int32 EntryCount = 0;
        if (!ReadSnapshotHeader(FullPath, CreatedAtUtc, MapName, SelectedActorPath, EntryCount))
        {
            continue;
        }

        UInspectorSnapshotItem* Item = NewObject<UInspectorSnapshotItem>(const_cast<UInspectorWorldSubsystem*>(this));
        Item->FullPath = FullPath;
        Item->FileName = FPaths::GetCleanFilename(FullPath);
        Item->CreatedAtUtc = CreatedAtUtc;
        Item->MapName = MapName;
        Item->SelectedActorPath = SelectedActorPath;
        Item->EntryCount = EntryCount;
        Item->ActorShortName = RI_ExtractActorShortNameFromPath(SelectedActorPath);
        Item->CreatedAtUnixSeconds = RI_ParseIsoUtcToUnixSeconds(CreatedAtUtc);

        Temp.Add(Item);
    }

    // 最新的排前面
    Temp.Sort([](const UInspectorSnapshotItem& A, const UInspectorSnapshotItem& B)
        {
            return A.CreatedAtUnixSeconds > B.CreatedAtUnixSeconds;
        });

    for (UInspectorSnapshotItem* It : Temp)
    {
        OutItems.Add(It);
    }
}

FString UInspectorWorldSubsystem::GetSnapshotDirectory() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("Snapshots"));
}

static bool RI_IsUnderDir(const FString& FilePath, const FString& DirPath)
{
    FString NormFile = FPaths::ConvertRelativePathToFull(FilePath);
    FString NormDir = FPaths::ConvertRelativePathToFull(DirPath);

    FPaths::NormalizeFilename(NormFile);
    FPaths::NormalizeFilename(NormDir);

    // 保证目录末尾有 /
    if (!NormDir.EndsWith(TEXT("/")))
    {
        NormDir += TEXT("/");
    }

    return NormFile.StartsWith(NormDir, ESearchCase::IgnoreCase);
}

void UInspectorWorldSubsystem::CopySnapshotPathToClipboard(const FString& FullPath)
{
    

    if (FullPath.IsEmpty())
    {
        PushToast(ERIToastType::Warning, TEXT("Empty path"), 2.0f);
    }
    else
    {
        // 允许复制任何字符串（即使不是合法路径），但你的 UI 传的一般是完整路径
        FPlatformApplicationMisc::ClipboardCopy(*FullPath);
        PushToast(ERIToastType::Info, TEXT("Copied"), 1.0f);
    }
}

bool UInspectorWorldSubsystem::DeleteSnapshotFile(const FString& FullPath, FString& OutError) 
{
    OutError.Reset();

    if (!IsRIEnabled())
    {
        OutError = TEXT("RuntimeInspector disabled (ri.Enable=0)");
        return false;
    }

    if (FullPath.IsEmpty())
    {
        OutError = TEXT("Empty path.");
        return false;
    }

    const FString SnapDir = GetSnapshotDirectory();

    // 安全：只允许删除 Snapshots 目录下的文件，避免误删用户任意路径
    if (!RI_IsUnderDir(FullPath, SnapDir))
    {
        OutError = FString::Printf(TEXT("Refuse to delete file outside snapshot dir: %s"), *SnapDir);
        return false;
    }

    // 只允许删 .json（你当前就是 json）
    if (!FullPath.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("Only .json files can be deleted.");
        return false;
    }

    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

    if (!PF.FileExists(*FullPath))
    {
        OutError = TEXT("File not found.");
        return false;
    }

    if (!PF.DeleteFile(*FullPath))
    {
        OutError = TEXT("Delete failed (platform file returned false).");
        return false;
    }
    PushToast(ERIToastType::Success, FString::Printf(TEXT("Deleted")), 1.2f);
    return true;
}

bool UInspectorWorldSubsystem::IsPropertyItemModified(const UInspectorPropertyItem* Item) const
{
#if !UE_BUILD_SHIPPING
    if (!Item) return false;
    UObject* Obj = Item->GetTargetObject();
    const FName PropName = Item->GetPropertyFName();
    const FString Key = MakePropertySnapshotKey(Obj, PropName);
    return !Key.IsEmpty() && ModifiedValueByKey.Contains(Key);
#else
    return false;
#endif
}

bool UInspectorWorldSubsystem::IsMaterialItemModified(const UInspectorMaterialParamItem* Item) const
{
#if !UE_BUILD_SHIPPING
    if (!Item) return false;

    UMeshComponent* MeshComp = Item->GetMeshComponent(); // 你如果没暴露 Getter，就在 MaterialParamItem 里加 BlueprintPure/C++ Getter
    if (!MeshComp) return false;

    UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(MeshComp);
    if (!Prim) return false;

    const FString Key = MakeMaterialSnapshotKey(
        Prim,
        Item->GetSlotIndex(),
        Item->GetParamType(),
        Item->GetParamName()
    );

    return !Key.IsEmpty() && ModifiedValueByKey.Contains(Key);
#else
    return false;
#endif
}

void UInspectorWorldSubsystem::GetPropertyItemsForSelectedEx(const FString& SearchText, bool bOnlyModified, TArray<UObject*>& OutItems)
{
#if !UE_BUILD_SHIPPING
    if (!bOnlyModified)
    {
        GetPropertyItemsForSelected(SearchText, OutItems);
        return;
    }

    OutItems.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr) return;

    const bool bSearchMode = !SearchText.IsEmpty();

    auto FilterOnlyModified = [&](TArray<UObject*>& Items)
        {
            Items.RemoveAll([&](UObject* Obj)
                {
                    if (UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Obj))
                    {
                        return !IsPropertyItemModified(P);
                    }
                    if (UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Obj))
                    {
                        return !IsMaterialItemModified(M);
                    }
                    return false; // GroupItem 不在这里删（组的删留由外层决定）
                });
        };

    // ----- MaterialOnly：右侧显示当前选中的 Material Slot 参数 -----
    if (PropertyViewMode == ERIPropertyViewMode::MaterialOnly)
    {
        UMeshComponent* MC = ViewMeshComp.Get();
        if (!MC || ViewMaterialSlot == INDEX_NONE)
        {
            return;
        }

        UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(MC);
        if (!SMC) return;

        UMaterialInterface* Mat = SMC->GetMaterial(ViewMaterialSlot);
        if (!Mat) return;

        TArray<UObject*> TempItems;

        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;

        // Scalar
        Infos.Reset(); Ids.Reset();
        Mat->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Scalar);

            if (!bOnlyModified || IsMaterialItemModified(Item))
            {
                TempItems.Add(Item);
            }
        }

        // Vector
        Infos.Reset(); Ids.Reset();
        Mat->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UInspectorMaterialParamItem* Item = GetOrCreateMaterialItem(SMC, ViewMaterialSlot, Info.Name, EInspectorMatParamType::Vector);

            if (!bOnlyModified || IsMaterialItemModified(Item))
            {
                TempItems.Add(Item);
            }
        }

        // OnlyModify 且没有任何 modified 参数 → 列表保持空（这是正确的）
        if (bOnlyModified && TempItems.Num() == 0)
        {
            return;
        }

        // 标题组（可选，但体验更好）
        UInspectorGroupItem* MatOnlyGroup = GetOrCreateGroupItem(TEXT("VIEW_MATERIAL_ONLY"));
        MatOnlyGroup->Kind = EInspectorGroupKind::Component;
        MatOnlyGroup->DisplayName = TEXT("Material Parameters");
        MatOnlyGroup->StableKey = TEXT("VIEW_MATERIAL_ONLY");
        MatOnlyGroup->bExpanded = true;

        OutItems.Add(MatOnlyGroup);
        OutItems.Append(TempItems);

        return; // ✅ 必须 return，阻止走 Actor/Components 分支
    }
    // ===== 1) Actor 组 =====
    {
        TArray<UObject*> ActorProps;
        AppendPropertiesForObject(ActorPtr, SearchText, ActorProps, TEXT(""), bSearchMode);
        FilterOnlyModified(ActorProps);

        if (ActorProps.Num() > 0)
        {
            UInspectorGroupItem* ActorGroup = GetOrCreateGroupItem(TEXT("ROOT_ACTOR"));
            ActorGroup->Kind = EInspectorGroupKind::RootActor;
            ActorGroup->DisplayName = TEXT("Actor");
            ActorGroup->StableKey = TEXT("ROOT_ACTOR");
            ActorGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(ActorGroup->StableKey, true);

            OutItems.Add(ActorGroup);
            if (ActorGroup->bExpanded)
            {
                OutItems.Append(ActorProps);
            }
        }
    }

    // ===== 2) Components 根组 + 每个组件组 =====
    {
        TArray<UActorComponent*> Components;
        ActorPtr->GetComponents(Components);

        // 先构建每个组件的 “是否有 modified 属性”
        struct FCompBlock { UInspectorGroupItem* Group = nullptr; TArray<UObject*> Props; };
        TArray<FCompBlock> Blocks;

        for (UActorComponent* Comp : Components)
        {
            if (!IsWhitelistedComponent(Comp)) continue;

            TArray<UObject*> Props;
            AppendPropertiesForObject(Comp, SearchText, Props, Comp->GetName(), bSearchMode);
            FilterOnlyModified(Props);

            if (Props.Num() == 0) continue; // 这个组件没有 modified，直接跳过

            const FString CompKey = MakeComponentKey(ActorPtr, Comp);
            UInspectorGroupItem* CompGroup = GetOrCreateGroupItem(CompKey);
            CompGroup->Kind = EInspectorGroupKind::Component;
            CompGroup->TargetObject = Comp;
            CompGroup->DisplayName = FString::Printf(TEXT("%s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
            CompGroup->StableKey = CompKey;
            CompGroup->bExpanded = bSearchMode ? true : GetGroupExpanded(CompKey, false);

            FCompBlock B;
            B.Group = CompGroup;
            B.Props = MoveTemp(Props);
            Blocks.Add(MoveTemp(B));
        }

        if (Blocks.Num() == 0)
        {
            return; // components 没有任何 modified
        }

        UInspectorGroupItem* CompRoot = GetOrCreateGroupItem(TEXT("ROOT_COMPONENTS"));
        CompRoot->Kind = EInspectorGroupKind::RootComponents;
        CompRoot->DisplayName = TEXT("Components");
        CompRoot->StableKey = TEXT("ROOT_COMPONENTS");
        CompRoot->bExpanded = bSearchMode ? true : GetGroupExpanded(CompRoot->StableKey, true);

        OutItems.Add(CompRoot);
        if (!CompRoot->bExpanded) return;

        for (FCompBlock& B : Blocks)
        {
            OutItems.Add(B.Group);
            if (B.Group->bExpanded)
            {
                OutItems.Append(B.Props);
            }
        }
    }
#endif
}

bool UInspectorWorldSubsystem::RevertModifiedForSelection(int32& OutRevertedCount, int32& OutFailedCount, FString& OutError)
{
#if !UE_BUILD_SHIPPING
    OutRevertedCount = 0;
    OutFailedCount = 0;
    OutError.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    const FString SelectedActorPath = ActorPtr->GetPathName();

    // 先拷贝 keys（遍历过程中 ModifiedValueByKey 会被移除）
    TArray<FString> Keys;
    ModifiedValueByKey.GetKeys(Keys);

    // 用于 ResolveComponent（复用你 import 那套也行，这里给一个轻量版）
    auto ResolveComponent = [&](AActor* Owner, const FString& ComponentPath) -> UActorComponent*
        {
            if (!Owner || ComponentPath.IsEmpty()) return nullptr;

            TArray<UActorComponent*> Comps;
            Owner->GetComponents(Comps);

            for (UActorComponent* C : Comps)
            {
                if (C && C->GetPathName() == ComponentPath)
                {
                    return C;
                }
            }

            // fallback by name
            FString Tail = ComponentPath;
            int32 Dot = INDEX_NONE;
            if (ComponentPath.FindLastChar(TEXT('.'), Dot))
            {
                Tail = ComponentPath.Mid(Dot + 1);
            }

            for (UActorComponent* C : Comps)
            {
                if (C && C->GetName() == Tail)
                {
                    return C;
                }
            }
            return nullptr;
        };

    FString CombinedErr;

    for (const FString& Key : Keys)
    {
        // 只处理当前选中的 actor
        if (Key.StartsWith(TEXT("P|")))
        {
            TArray<FString> Parts;
            Key.ParseIntoArray(Parts, TEXT("|"), false);
            // P|ActorPath|CompPath|ClassPath|PropName
            if (Parts.Num() < 5) continue;

            const FString& ActorPath = Parts[1];
            const FString& CompPath = Parts[2];
            const FString& PropName = Parts[4];

            if (ActorPath != SelectedActorPath) continue;

            const FString* BaselinePtr = BaselineValueByKey.Find(Key);
            if (!BaselinePtr) continue;

            UObject* TargetObj = ActorPtr;
            if (!CompPath.IsEmpty())
            {
                if (UActorComponent* C = ResolveComponent(ActorPtr, CompPath))
                {
                    TargetObj = C;
                }
                else
                {
                    OutFailedCount++;
                    CombinedErr += FString::Printf(TEXT("\nComponent not found: %s"), *CompPath);
                    continue;
                }
            }

            UInspectorPropertyItem* Temp = NewObject<UInspectorPropertyItem>(this);
            Temp->Init(TargetObj, FName(*PropName));

            FString Err;
            if (Temp->ApplyFromText(*BaselinePtr, Err))
            {
                OutRevertedCount++;
            }
            else
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nRevert failed: %s.%s (%s)"),
                    *GetNameSafe(TargetObj), *PropName, *Err);
            }
        }
        else if (Key.StartsWith(TEXT("M|")))
        {
            TArray<FString> Parts;
            Key.ParseIntoArray(Parts, TEXT("|"), false);
            // M|ActorPath|CompPath|Slot|TypeInt|ParamName
            if (Parts.Num() < 6) continue;

            const FString& ActorPath = Parts[1];
            const FString& CompPath = Parts[2];
            const int32 SlotIndex = FCString::Atoi(*Parts[3]);
            const EInspectorMatParamType Type = (EInspectorMatParamType)FCString::Atoi(*Parts[4]);
            const FString& ParamName = Parts[5];

            if (ActorPath != SelectedActorPath) continue;

            const FString* BaselinePtr = BaselineValueByKey.Find(Key);
            if (!BaselinePtr) continue;

            UActorComponent* C = ResolveComponent(ActorPtr, CompPath);
            UMeshComponent* MC = C ? Cast<UMeshComponent>(C) : nullptr;
            if (!MC)
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nMeshComponent not found: %s"), *CompPath);
                continue;
            }

            UInspectorMaterialParamItem* Temp = NewObject<UInspectorMaterialParamItem>(this);
            Temp->Init(MC, SlotIndex, FName(*ParamName), Type);

            FString Err;
            if (Temp->ApplyFromText(*BaselinePtr, Err))
            {
                OutRevertedCount++;
            }
            else
            {
                OutFailedCount++;
                CombinedErr += FString::Printf(TEXT("\nRevert failed: %s slot=%d %s (%s)"),
                    *GetNameSafe(MC), SlotIndex, *ParamName, *Err);
            }
        }
    }

    // 统一刷新一次（避免每条都刷）
    RefreshPanel(EInspectorRefreshReason::ValuesChanged);

    if (!CombinedErr.IsEmpty())
    {
        OutError = CombinedErr.TrimStartAndEnd();
    }

    // Toast（如果你有 PushToast）
    if (OutFailedCount == 0)
    {
        PushToast(ERIToastType::Success, FString::Printf(TEXT("Reset %d changes"), OutRevertedCount), 1.5f);
    }
    else
    {
        PushToast(ERIToastType::Warning, FString::Printf(TEXT("Reset %d, failed %d (see log)"), OutRevertedCount, OutFailedCount), 3.0f);
    }

    return OutRevertedCount > 0 && OutFailedCount == 0;

#else
    OutError = TEXT("Not available in Shipping");
    return false;
#endif
}
bool UInspectorWorldSubsystem::IsItemModified(UObject* Item) const
{
#if !UE_BUILD_SHIPPING
    if (!Item) return false;

    if (const UInspectorPropertyItem* P = Cast<UInspectorPropertyItem>(Item))
    {
        return IsPropertyItemModified(P);
    }
    if (const UInspectorMaterialParamItem* M = Cast<UInspectorMaterialParamItem>(Item))
    {
        return IsMaterialItemModified(M);
    }

    // GroupItem / 其它：不算“modified item”
    return false;
#else
    return false;
#endif
}
