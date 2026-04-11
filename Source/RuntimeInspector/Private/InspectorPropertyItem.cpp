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

namespace
{
    static FString RI_GetPropertyDisplayNameRuntimeSafe(const FProperty* Prop)
    {
        if (!Prop)
        {
            return FString();
        }

#if WITH_EDITOR
        const FString FriendlyName = Prop->GetDisplayNameText().ToString();
        if (!FriendlyName.IsEmpty())
        {
            return FriendlyName;
        }
#endif

        const FString AuthoredName = Prop->GetAuthoredName();
        return AuthoredName.IsEmpty() ? Prop->GetName() : AuthoredName;
    }
}

void UInspectorPropertyItem::Init(UObject* InTarget, FName InPropertyName)
{
    Target = InTarget;
    PropertyName = InPropertyName;
}

FString UInspectorPropertyItem::GetPropertyName() const
{
    if (!Target.IsValid())
    {
        return PropertyName.ToString();
    }

    const FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);

    FString DisplayName = PropertyName.ToString();
    if (Prop)
    {
        const FString Friendly = RI_GetPropertyDisplayNameRuntimeSafe(Prop);
        if (!Friendly.IsEmpty())
        {
            DisplayName = Friendly;
        }
    }

    if (!OwnerPrefix.IsEmpty())
    {
        return FString::Printf(TEXT("%s / %s"), *OwnerPrefix, *DisplayName);
    }

    return DisplayName;
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

    return InspectorPropertyUtils::CanSetFromText(Target.Get(), Prop);
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

bool UInspectorPropertyItem::IsReadOnly() const
{
    return IsValidItem() && !IsEditable();
}

static FString RI_GetPropertyTypeLabel(const FProperty* Prop)
{
    if (!Prop)
    {
        return TEXT("Unknown");
    }

    if (const FStructProperty* StructProperty = CastField<FStructProperty>(Prop))
    {
        return StructProperty->Struct ? StructProperty->Struct->GetName() : TEXT("Struct");
    }
    if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Prop))
    {
        return EnumProperty->GetEnum() ? EnumProperty->GetEnum()->GetName() : TEXT("Enum");
    }
    if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Prop))
    {
        if (ByteProperty->Enum)
        {
            return ByteProperty->Enum->GetName();
        }
    }

    return Prop->GetCPPType();
}

FString UInspectorPropertyItem::GetTypeLabel() const
{
    if (!Target.IsValid())
    {
        return TEXT("Unknown");
    }

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    return RI_GetPropertyTypeLabel(Prop);
}

FString UInspectorPropertyItem::GetReadOnlyReason() const
{
    if (!Target.IsValid())
    {
        return TEXT("Target invalid");
    }

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    if (!Prop)
    {
        return TEXT("Property not found");
    }
    if (!Prop->HasAnyPropertyFlags(CPF_Edit))
    {
        return TEXT("Property is visible but not editable");
    }
    if (!InspectorPropertyUtils::IsSupportedEditableProperty(Prop))
    {
        return FString::Printf(TEXT("Read-only %s"), *RI_GetPropertyTypeLabel(Prop));
    }

    return FString();
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
#if WITH_EDITOR
        if (Enum->HasMetaData(TEXT("Hidden"), i)) continue;
#endif

        const FString Name = Enum->GetNameStringByIndex(i);
        if (Name.EndsWith(TEXT("_MAX"))) continue;

        OutOptions.Add(Name);
    }
}

bool UInspectorPropertyItem::ApplyFromText(const FString& NewText, FString& OutError)
{
    OutError.Reset();
    UObject* Obj = Target.Get();
    if (!Obj)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    // Delegate apply/undo/modified tracking to the subsystem so it can debounce writes.
    if (UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(GetOuter()))
    {
        return Sub->RequestApplyPropertyText(Obj, PropertyName, NewText, OutError);
    }

    FString Err;
    const bool bOK = InspectorPropertyUtils::SetValueFromText(Obj, PropertyName, NewText, &Err);
    if (!bOK)
    {
        OutError = Err;
        return false;
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
