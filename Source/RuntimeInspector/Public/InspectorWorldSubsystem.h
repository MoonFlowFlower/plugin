#pragma once


#include "CoreMinimal.h"
#include "InspectorDefines.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SharedPointer.h"
#include "InspectorGroupItem.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "InspectorMaterialParamItem.h"
#include "InspectorTypes.h"


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
class UUserWidget;
class UInspectorPropertyItem;
class FRuntimeInspectorInputProcessor;

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

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    bool IsRIEnabled() const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Security")
    FString GetRIDisabledReason() const;

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

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    bool IsOpen() const { return bOpen; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void PickActorInView(); // F2: �ӽ���������ѡ��


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

    bool IsApplyingHistory() const { return bApplyingHistory; }

    UMaterialInstanceDynamic* GetOrCreateMID(UPrimitiveComponent* Comp, int32 MaterialIndex);


    // ===== Favorites (Pin) =====
    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Favorites")
    bool IsFavoriteForItem(UInspectorPropertyItem* Item) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Favorites")
    void ToggleFavoriteForItem(UInspectorPropertyItem* Item);

    // ===== Snapshot (Modified / Export / Import) =====
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    int32 GetModifiedCount() const { return ModifiedValueByKey.Num(); }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    void ClearModified();

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool ExportSnapshot(bool bOnlyModified, FString& OutFilePath, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    bool ImportSnapshot(const FString& InFilePath, FString& OutError);

    // Last snapshot import report (cached for UI)
    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    FRIImportReport GetLastImportReport() const { return LastImportReport; }

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector|Snapshot")
    void ClearLastImportReport();

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    bool IsPropertyItemModified(const UInspectorPropertyItem* Item) const;

    UFUNCTION(BlueprintPure, Category = "RuntimeInspector|Snapshot")
    bool IsMaterialItemModified(const UInspectorMaterialParamItem* Item) const;

    UFUNCTION(BlueprintCallable, Category = "RuntimeInspector")
    void GetPropertyItemsForSelectedEx(const FString& SearchText, bool bOnlyModified, TArray<UObject*>& OutItems);


private:
    APlayerController* GetLocalPC() const;
    bool ApplyChange(const FInspectorChange& Change, bool bUseNewValue);
    void EnsurePanelWidget();
    //void RefreshPanel();

    // <<< ADD���۵�״̬����
    UPROPERTY()
    TMap<FString, bool> GroupExpandedMap;

private:
    bool bOpen = false;
    bool bInputsBound = false;
    bool bInspectorOpen = false;

    TSharedPtr<FRuntimeInspectorInputProcessor> InputProcessor;

    void RegisterInputProcessor();
    void UnregisterInputProcessor();

    TWeakObjectPtr<AActor> SelectedActor;

    TWeakObjectPtr<UUserWidget> PanelWidget;

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

        // --- ������������У�飨Tick���ã�---
        float ValidateAccum = 0.f;
        void ValidateSelection();

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

        UPROPERTY(Transient)
        TMap<FString, FString> BaselineValueByKey;

        UPROPERTY(Transient)
        TMap<FString, FString> ModifiedValueByKey;

        FString GetSnapshotsDir() const;
        FString MakePropertySnapshotKey(UObject* TargetObject, FName PropertyName) const;
        FString MakeMaterialSnapshotKey(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorMatParamType Type, FName ParamName) const;

        void TrackModifiedForKey(const FString& Key, const FString& OldText, const FString& NewText);
        void UpdateModifiedStateFromCurrentValue(UObject* TargetObject, FName PropertyName);
        void UpdateModifiedStateFromCurrentMaterial(UPrimitiveComponent* Comp, int32 SlotIndex, EInspectorChangeType ChangeType, FName ParamName);



	// ===== Material Slot Selection =====
    public:
        UFUNCTION(BlueprintCallable)
        void SetPropertyView_MaterialOnly(UMeshComponent* InComp, int32 InSlot)
        {
        #if !UE_BUILD_SHIPPING
                    PropertyViewMode = ERIPropertyViewMode::MaterialOnly;
                    ViewMeshComp = InComp;
                    ViewMaterialSlot = InSlot;
                    RefreshPanel(); // �������� OnInspectorRefreshEx�������Ǹ�
        #endif
                }

        UFUNCTION(BlueprintCallable)
        void SetPropertyView_Full()
        {
        #if !UE_BUILD_SHIPPING
                    PropertyViewMode = ERIPropertyViewMode::Full;
                    ViewMeshComp = nullptr;
                    ViewMaterialSlot = INDEX_NONE;
                    RefreshPanel();
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
    private:
        void TryBindInputs();
        void OnPickKeyPressed(); // NEW

		// ===== Actor Outline Support =====
    private:
        TWeakObjectPtr<AActor> OutlinedActor;

        bool bWarnedCustomDepthStencil = false;
        bool EnsureCustomDepthStencilEnabled();

        void SetActorOutline(AActor* Actor, bool bEnable, int32 StencilValue = 1);

        UCameraComponent* FindOutlineCamera(APlayerController* PC) const;

        void EnableOutlinePP(bool bEnable);

        // 运行时创建的 MID
        UPROPERTY(Transient)
        TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

        // 记录相机原本的 PostProcessBlendWeight，关闭时恢复
        float SavedCamPPBlendWeight = 0.0f;
        bool bSavedCamPPBlendWeightValid = false;

};


