#include "InspectorPropertyItem.h"
#include "InspectorPropertyUtils.h"
#include "InspectorWorldSubsystem.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/LightComponent.h"

#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

// For TBaseStructure<>
#include "UObject/NoExportTypes.h"


void UInspectorPropertyItem::Init(UObject* InTarget, FName InPropertyName)
{
    Target = InTarget;
    PropertyName = InPropertyName;
}

FString UInspectorPropertyItem::GetPropertyName() const
{
    return PropertyName.ToString();
}

FString UInspectorPropertyItem::GetValueText()
{
    FString Text;
    if (Target.IsValid())
    {
        InspectorPropertyUtils::GetValueAsText(Target.Get(), PropertyName, Text);
    }
    return Text;
}

bool UInspectorPropertyItem::IsEditable() const
{
    if (!Target.IsValid()) return false;

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    if (!Prop) return false;

    // 这里沿用你之前的“可写”规则：必须是 Edit 且不是 EditConst 等
    if (!Prop->HasAnyPropertyFlags(CPF_Edit)) return false;
    //if (Prop->HasAnyPropertyFlags(CPF_EditConst)) return false;
    //if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) return false;

    return true;
}

static EInspectorValueType GetTypeFromProperty(const FProperty* Prop)
{
    if (!Prop) return EInspectorValueType::Unsupported;

    if (Prop->IsA<FBoolProperty>())   return EInspectorValueType::Bool;
    if (Prop->IsA<FIntProperty>())    return EInspectorValueType::Int;
    if (Prop->IsA<FFloatProperty>())  return EInspectorValueType::Float;
    if (Prop->IsA<FDoubleProperty>()) return EInspectorValueType::Double;
    if (Prop->IsA<FStrProperty>())    return EInspectorValueType::String;
    if (Prop->IsA<FNameProperty>())   return EInspectorValueType::Name;

    if (Prop->IsA<FEnumProperty>())   return EInspectorValueType::Enum;

    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        if (ByteProp->Enum) return EInspectorValueType::Enum;
    }


    if (const FStructProperty* SP = CastField<FStructProperty>(Prop))
    {
        const UScriptStruct* S = SP->Struct;
        if (S == TBaseStructure<FVector2D>::Get()) return EInspectorValueType::Vector2;
        if (S == TBaseStructure<FVector>::Get())   return EInspectorValueType::Vector3;
        if (S == TBaseStructure<FVector4>::Get())  return EInspectorValueType::Vector4;
        if (S == TBaseStructure<FRotator>::Get())  return EInspectorValueType::Rotator;
        if (S == TBaseStructure<FTransform>::Get())return EInspectorValueType::Transform;
        if (S == TBaseStructure<FLinearColor>::Get()) return EInspectorValueType::LinearColor;
        if (S == TBaseStructure<FColor>::Get()) return EInspectorValueType::Color;
    }
    return EInspectorValueType::Unsupported;
}

EInspectorValueType UInspectorPropertyItem::GetValueType() const
{
    if (!Target.IsValid()) return EInspectorValueType::Unsupported;
    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    return GetTypeFromProperty(Prop);
}

bool UInspectorPropertyItem::IsEnum() const
{
    return GetValueType() == EInspectorValueType::Enum;
}

static UEnum* GetEnumFromProperty(FProperty* Prop)
{
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        return EnumProp->GetEnum();
    }
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        return ByteProp->Enum;
    }
    return nullptr;
}

void UInspectorPropertyItem::GetEnumOptions(TArray<FString>& OutOptions) const
{
    OutOptions.Reset();
    if (!Target.IsValid()) return;

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    UEnum* Enum = GetEnumFromProperty(Prop);
    if (!Enum) return;

    // 过滤掉 _MAX 之类的隐藏项（可选）
    const int32 Num = Enum->NumEnums();
    for (int32 i = 0; i < Num; ++i)
    {
        if (Enum->HasMetaData(TEXT("Hidden"), i)) continue;

        const FString Name = Enum->GetNameStringByIndex(i);
        if (Name.EndsWith(TEXT("_MAX"))) continue;

        OutOptions.Add(Name);
    }
}

bool UInspectorPropertyItem::ApplyFromText(const FString& NewText, FString& OutError)
{
    OutError.Reset();
    if (!Target.IsValid())
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    // 1) 记录旧值
    FString OldText;
    InspectorPropertyUtils::GetValueAsText(Target.Get(), PropertyName, OldText);

    // 2) 写入新值
    FString Err;
    const bool bOK = InspectorPropertyUtils::SetValueFromText(Target.Get(), PropertyName, NewText, &Err);
    if (!bOK)
    {
        OutError = Err;
        return false;
    }


    // 3) 记录写入后的“真实值”（可能被规范化）
    FString NewTextActual;
    InspectorPropertyUtils::GetValueAsText(Target.Get(), PropertyName, NewTextActual);

    // ---- Fixup: make changes take effect immediately (runtime-friendly) ----
    if (UObject* Obj = Target.Get())
    {
        FProperty* Prop = InspectorPropertyUtils::FindProperty(Obj, PropertyName);

        // 1) 对 Scene/Primitive 组件做通用刷新
        if (UActorComponent* AC = Cast<UActorComponent>(Obj))
        {
            AC->MarkRenderStateDirty();
            AC->MarkRenderTransformDirty();
        }

        // 2) 对“常见需要走 setter”的字段做补偿（避免仅改属性但引擎没触发内部逻辑）
        if (USceneComponent* SC = Cast<USceneComponent>(Obj))
        {
            // Visibility
            if (Prop && PropertyName == TEXT("bVisible"))
            {
                if (FBoolProperty* BP = CastField<FBoolProperty>(Prop))
                {
                    const bool b = BP->GetPropertyValue_InContainer(Obj);
                    SC->SetVisibility(b, true);
                }
            }
            // HiddenInGame
            else if (Prop && PropertyName == TEXT("bHiddenInGame"))
            {
                if (FBoolProperty* BP = CastField<FBoolProperty>(Prop))
                {
                    const bool b = BP->GetPropertyValue_InContainer(Obj);
                    SC->SetHiddenInGame(b, true);
                }
            }
            // Mobility（很多情况下只改属性不会触发重注册）
            else if (Prop && PropertyName == TEXT("Mobility"))
            {
                // Mobility 通常是 enum/byte
                EComponentMobility::Type Mobility = SC->Mobility;

                if (FEnumProperty* EP = CastField<FEnumProperty>(Prop))
                {
                    const void* VPtr = EP->ContainerPtrToValuePtr<void>(Obj);
                    const int64 V = EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(VPtr);
                    Mobility = static_cast<EComponentMobility::Type>(V);
                }
                else if (FByteProperty* BP = CastField<FByteProperty>(Prop))
                {
                    const uint8 V = BP->GetPropertyValue_InContainer(Obj);
                    Mobility = static_cast<EComponentMobility::Type>(V);
                }

                SC->SetMobility(Mobility);
            }
        }

        // 3) Light 常见字段（Intensity 改完立刻更新）
        if (ULightComponent* LC = Cast<ULightComponent>(Obj))
        {
            if (Prop && PropertyName == TEXT("Intensity"))
            {
                float V = 0.f;

                if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
                {
                    V = FP->GetPropertyValue_InContainer(Obj);
                }
                else if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
                {
                    V = static_cast<float>(DP->GetPropertyValue_InContainer(Obj));
                }

                LC->SetIntensity(V);
            }
        }
    }
    // ---- Fixup end ----

    // 4) 把变更交给 Subsystem（你的 Item Outer 正好是 Subsystem）
    if (UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(GetOuter()))
    {
        FInspectorChange Change;
        Change.Target = Target.Get();
        Change.PropertyName = PropertyName;
        Change.OldValueText = OldText;
        Change.NewValueText = NewTextActual;
        Change.DebugObjectName = GetNameSafe(Target.Get());
        Sub->RecordChange(Change);
    }

    return true;
}

// ===== Struct helpers (Vector/Rotator/Transform/Color) =====

static bool RI_GetStructPtr(UObject* Target, FName PropertyName, const UScriptStruct* WantStruct, void*& OutValuePtr, FProperty*& OutProp)
{
    OutValuePtr = nullptr;
    OutProp = nullptr;
    if (!Target) return false;

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target, PropertyName);
    if (!Prop) return false;

    const FStructProperty* SP = CastField<FStructProperty>(Prop);
    if (!SP || SP->Struct != WantStruct) return false;

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Target);
    if (!ValuePtr) return false;

    OutValuePtr = ValuePtr;
    OutProp = Prop;
    return true;
}

static bool RI_SetStructValue(UInspectorPropertyItem* Item, const void* InStructValue, const UScriptStruct* WantStruct, FString& OutError)
{
    OutError.Reset();
    if (!Item) { OutError = TEXT("Invalid item"); return false; }

    UObject* Obj = Item->GetTargetObject();
    if (!Obj) { OutError = TEXT("Target invalid"); return false; }

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Obj, Item->GetPropertyFName());
    if (!Prop) { OutError = TEXT("Property not found"); return false; }

    if (!InspectorPropertyUtils::IsEditableProperty(Prop))
    {
        OutError = TEXT("Property is not editable");
        return false;
    }

    const FStructProperty* SP = CastField<FStructProperty>(Prop);
    if (!SP || SP->Struct != WantStruct)
    {
        OutError = TEXT("Property is not the expected struct type");
        return false;
    }

    // 1) old text
    FString OldText;
    InspectorPropertyUtils::GetValueAsText(Obj, Item->GetPropertyFName(), OldText);

    // 2) write memory
    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
    if (!ValuePtr)
    {
        OutError = TEXT("Invalid value pointer");
        return false;
    }

    SP->Struct->CopyScriptStruct(ValuePtr, InStructValue);

    // 3) runtime fixups (minimal)
    if (UActorComponent* AC = Cast<UActorComponent>(Obj))
    {
        AC->MarkRenderStateDirty();
        AC->MarkRenderTransformDirty();
    }

    if (USceneComponent* SC = Cast<USceneComponent>(Obj))
    {
        // If user edits these common fields, go through setters to ensure engine side-effects.
        const FName PName = Item->GetPropertyFName();
        if (PName == TEXT("RelativeLocation"))
        {
            SC->SetRelativeLocation(SC->GetRelativeLocation());
        }
        else if (PName == TEXT("RelativeRotation"))
        {
            SC->SetRelativeRotation(SC->GetRelativeRotation());
        }
        else if (PName == TEXT("RelativeScale3D"))
        {
            SC->SetRelativeScale3D(SC->GetRelativeScale3D());
        }
        else if (PName == TEXT("RelativeTransform"))
        {
            SC->SetRelativeTransform(SC->GetRelativeTransform());
        }
    }

    // 4) new text + record change
    FString NewText;
    InspectorPropertyUtils::GetValueAsText(Obj, Item->GetPropertyFName(), NewText);

    if (UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(Item->GetOuter()))
    {
        FInspectorChange Change;
        Change.Target = Obj;
        Change.PropertyName = Item->GetPropertyFName();
        Change.OldValueText = OldText;
        Change.NewValueText = NewText;
        Change.DebugObjectName = GetNameSafe(Obj);
        Sub->RecordChange(Change);
    }

    return true;
}

bool UInspectorPropertyItem::GetVector2D(FVector2D& OutValue) const
{
    OutValue = FVector2D::ZeroVector;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FVector2D>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FVector2D*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetVector(FVector& OutValue) const
{
    OutValue = FVector::ZeroVector;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FVector>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FVector*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetVector4(FVector4& OutValue) const
{
    OutValue = FVector4(0,0,0,0);
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FVector4>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FVector4*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetRotator(FRotator& OutValue) const
{
    OutValue = FRotator::ZeroRotator;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FRotator>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FRotator*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetTransform(FTransform& OutValue) const
{
    OutValue = FTransform::Identity;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FTransform>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FTransform*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetLinearColor(FLinearColor& OutValue) const
{
    OutValue = FLinearColor::Black;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FLinearColor>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FLinearColor*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::GetColor(FColor& OutValue) const
{
    OutValue = FColor::Black;
    void* Ptr = nullptr;
    FProperty* Prop = nullptr;
    if (!Target.IsValid()) return false;
    if (!RI_GetStructPtr(Target.Get(), PropertyName, TBaseStructure<FColor>::Get(), Ptr, Prop)) return false;
    OutValue = *reinterpret_cast<FColor*>(Ptr);
    return true;
}

bool UInspectorPropertyItem::SetVector2D(const FVector2D& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FVector2D>::Get(), OutError);
}

bool UInspectorPropertyItem::SetVector(const FVector& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FVector>::Get(), OutError);
}

bool UInspectorPropertyItem::SetVector4(const FVector4& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FVector4>::Get(), OutError);
}

bool UInspectorPropertyItem::SetRotator(const FRotator& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FRotator>::Get(), OutError);
}

bool UInspectorPropertyItem::SetTransform(const FTransform& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FTransform>::Get(), OutError);
}

bool UInspectorPropertyItem::SetLinearColor(const FLinearColor& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FLinearColor>::Get(), OutError);
}

bool UInspectorPropertyItem::SetColor(const FColor& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FColor>::Get(), OutError);
}
