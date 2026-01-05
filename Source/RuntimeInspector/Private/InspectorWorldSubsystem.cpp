#include "InspectorWorldSubsystem.h"
#include "RuntimeInspectorInputProcessor.h"
#include "InspectorGroupItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorDefines.h"
#include "InspectorPropertyItem.h"
#include "InspectorMaterialParamItem.h"



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
    OutError.Reset();
    OutFilePath.Reset();

    AActor* ActorPtr = SelectedActor.Get();
    if (!ActorPtr)
    {
        OutError = TEXT("No selected actor");
        return false;
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), 1);
    Root->SetStringField(TEXT("createdAtUtc"), FDateTime::UtcNow().ToIso8601());

    if (UWorld* World = GetWorld())
    {
        Root->SetStringField(TEXT("map"), World->GetMapName());
    }

    Root->SetStringField(TEXT("selectedActorPath"), ActorPtr->GetPathName());

    TArray<TSharedPtr<FJsonValue>> Entries;

    auto AddPropertyEntry = [&](const FString& ActorPath, const FString& CompPath, const FString& ClassPath, const FString& PropName, const FString& ValueText)
    {
        TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
        E->SetStringField(TEXT("kind"), TEXT("property"));
        E->SetStringField(TEXT("actorPath"), ActorPath);
        E->SetStringField(TEXT("componentPath"), CompPath);
        E->SetStringField(TEXT("class"), ClassPath);
        E->SetStringField(TEXT("property"), PropName);
        E->SetStringField(TEXT("value"), ValueText);
        Entries.Add(MakeShared<FJsonValueObject>(E));
    };

    auto AddMaterialEntry = [&](const FString& ActorPath, const FString& CompPath, int32 SlotIndex, EInspectorMatParamType Type, const FString& ParamName, const FString& ValueText)
    {
        TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
        E->SetStringField(TEXT("kind"), (Type == EInspectorMatParamType::Scalar) ? TEXT("materialScalar") : TEXT("materialVector"));
        E->SetStringField(TEXT("actorPath"), ActorPath);
        E->SetStringField(TEXT("componentPath"), CompPath);
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
                    AddPropertyEntry(Parts[1], Parts[2], Parts[3], Parts[4], Val);
                }
            }
            else if (Key.StartsWith(TEXT("M|")))
            {
                TArray<FString> Parts;
                Key.ParseIntoArray(Parts, TEXT("|"), false);
                // M|ActorPath|CompPath|Slot|TypeInt|ParamName
                if (Parts.Num() >= 6)
                {
                    const int32 SlotIndex = FCString::Atoi(*Parts[3]);
                    const int32 TypeInt = FCString::Atoi(*Parts[4]);
                    const EInspectorMatParamType Type = (EInspectorMatParamType)TypeInt;
                    AddMaterialEntry(Parts[1], Parts[2], SlotIndex, Type, Parts[5], Val);
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

            const FString ActorPath = ActorPtr->GetPathName();
            const FString CompPath = Obj->IsA(AActor::StaticClass()) ? TEXT("") : Obj->GetPathName();
            const FString ClassPath = Cls->GetPathName();

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
                AddPropertyEntry(ActorPath, CompPath, ClassPath, Prop->GetName(), ValText);
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
                        AddMaterialEntry(APath, CPath, ViewMaterialSlot, EInspectorMatParamType::Scalar, Info.Name.ToString(), V);
                    }

                    Infos.Reset(); Ids.Reset();
                    Mat->GetAllVectorParameterInfo(Infos, Ids);
                    for (const FMaterialParameterInfo& Info : Infos)
                    {
                        const FString V = RI_GetMaterialParamValueText(Cast<UPrimitiveComponent>(MC), ViewMaterialSlot, EInspectorChangeType::MaterialVector, Info.Name);
                        AddMaterialEntry(APath, CPath, ViewMaterialSlot, EInspectorMatParamType::Vector, Info.Name.ToString(), V);
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
    const FString FileName = FString::Printf(TEXT("Snapshot_%s_%s.json"), *ActorPtr->GetName(), *Timestamp);
    OutFilePath = FPaths::Combine(Dir, FileName);

    if (!FFileHelper::SaveStringToFile(OutJson, *OutFilePath))
    {
        OutError = TEXT("Save failed");
        OutFilePath.Reset();
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[RI] Snapshot exported: %s (entries=%d, onlyModified=%d)"), *OutFilePath, Entries.Num(), bOnlyModified ? 1 : 0);
    return true;
#else
    OutError = TEXT("Not available in Shipping");
    return false;
#endif
}

bool UInspectorWorldSubsystem::ImportSnapshot(const FString& InFilePath, FString& OutError)
{
#if !UE_BUILD_SHIPPING
    OutError.Reset();

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

    auto ResolveActor = [&](const FString& ActorPath) -> AActor*
    {
        if (ActorPath.IsEmpty()) return nullptr;

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetPathName() == ActorPath)
            {
                return *It;
            }
        }

        FString FallbackName = ActorPath;
        int32 Dot = INDEX_NONE;
        if (ActorPath.FindLastChar(TEXT('.'), Dot))
        {
            FallbackName = ActorPath.Mid(Dot + 1);
        }

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == FallbackName)
            {
                return *It;
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

        const FString Kind = E->GetStringField(TEXT("kind"));
        const FString ActorPath = E->GetStringField(TEXT("actorPath"));
        const FString CompPath = E->GetStringField(TEXT("componentPath"));

        AActor* TargetActor = ResolveActor(ActorPath);
        if (!TargetActor)
        {
            bAllOK = false;
            CombinedErrors += FString::Printf(TEXT("\nActor not found: %s"), *ActorPath);
            continue;
        }

        if (Kind == TEXT("property"))
        {
            const FString PropNameStr = E->GetStringField(TEXT("property"));
            const FString ValueText = E->GetStringField(TEXT("value"));

            UObject* TargetObj = TargetActor;
            if (!CompPath.IsEmpty())
            {
                if (UActorComponent* C = ResolveComponent(TargetActor, CompPath))
                {
                    TargetObj = C;
                }
                else
                {
                    bAllOK = false;
                    CombinedErrors += FString::Printf(TEXT("\nComponent not found: %s"), *CompPath);
                    continue;
                }
            }

            UInspectorPropertyItem* Temp = NewObject<UInspectorPropertyItem>(this);
            Temp->Init(TargetObj, FName(*PropNameStr));

            const FString OldText = Temp->GetValueText();
            FString Err;
            if (!Temp->ApplyFromText(ValueText, Err))
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nApply failed: %s.%s (%s)"), *GetNameSafe(TargetObj), *PropNameStr, *Err);
                continue;
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

            if (CompPath.IsEmpty())
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nMaterial entry missing componentPath (actor=%s)"), *ActorPath);
                continue;
            }

            UActorComponent* C = ResolveComponent(TargetActor, CompPath);
            UMeshComponent* MC = C ? Cast<UMeshComponent>(C) : nullptr;
            if (!MC)
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nMeshComponent not found: %s"), *CompPath);
                continue;
            }

            const EInspectorMatParamType Type = (Kind == TEXT("materialScalar")) ? EInspectorMatParamType::Scalar : EInspectorMatParamType::Vector;

            UInspectorMaterialParamItem* Temp = NewObject<UInspectorMaterialParamItem>(this);
            Temp->Init(MC, SlotIndex, FName(*ParamNameStr), Type);

            const FString OldText = Temp->GetValueText();
            FString Err;
            if (!Temp->ApplyFromText(ValueText, Err))
            {
                bAllOK = false;
                CombinedErrors += FString::Printf(TEXT("\nApply failed: %s slot=%d %s (%s)"), *GetNameSafe(MC), SlotIndex, *ParamNameStr, *Err);
                continue;
            }

            const FString NewText = Temp->GetValueText();
            const FString Key = MakeMaterialSnapshotKey(Cast<UPrimitiveComponent>(MC), SlotIndex, Type, FName(*ParamNameStr));
            TrackModifiedForKey(Key, OldText, NewText);
        }
    }

    bApplyingHistory = bPrevApplying;

    RefreshPanel(EInspectorRefreshReason::ValuesChanged);

    if (!bAllOK)
    {
        OutError = CombinedErrors.TrimStartAndEnd();
    }

    UE_LOG(LogTemp, Log, TEXT("[RI] Snapshot imported: %s (ok=%d)"), *InFilePath, bAllOK ? 1 : 0);
    return bAllOK;
#else
    OutError = TEXT("Not available in Shipping");
    return false;
#endif
}
