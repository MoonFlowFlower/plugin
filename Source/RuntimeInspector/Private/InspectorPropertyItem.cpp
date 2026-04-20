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
    static bool RI_IsSyntheticActorWorldPropertyName(FName PropertyName)
    {
        return PropertyName == TEXT("ActorWorldLocation")
            || PropertyName == TEXT("ActorWorldRotation")
            || PropertyName == TEXT("ActorWorldScale");
    }

    static FString RI_GetSyntheticActorWorldDisplayName(FName PropertyName)
    {
        if (PropertyName == TEXT("ActorWorldLocation"))
        {
            return TEXT("Location");
        }
        if (PropertyName == TEXT("ActorWorldRotation"))
        {
            return TEXT("Rotation");
        }
        if (PropertyName == TEXT("ActorWorldScale"))
        {
            return TEXT("Scale");
        }
        return PropertyName.ToString();
    }

    static FString RI_ExportStructValueText(const UScriptStruct* ScriptStruct, const void* ValuePtr)
    {
        if (!ScriptStruct || !ValuePtr)
        {
            return FString();
        }

        FString ExportedText;
        ScriptStruct->ExportText(ExportedText, ValuePtr, nullptr, nullptr, PPF_None, nullptr);
        return ExportedText;
    }

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

    static FString RI_NormalizeCategorySegment(const FString& InSegment)
    {
        const FString Trimmed = InSegment.TrimStartAndEnd();
        return Trimmed.IsEmpty() ? TEXT("Default") : Trimmed;
    }

    static void RI_SplitCategoryPath(const FString& InPath, TArray<FString>& OutSegments)
    {
        OutSegments.Reset();

        FString EffectivePath = InPath.TrimStartAndEnd();
        if (EffectivePath.IsEmpty())
        {
            EffectivePath = TEXT("Default");
        }

        EffectivePath.ParseIntoArray(OutSegments, TEXT("|"), true);
        if (OutSegments.Num() == 0)
        {
            OutSegments.Add(TEXT("Default"));
        }

        for (FString& Segment : OutSegments)
        {
            Segment = RI_NormalizeCategorySegment(Segment);
        }
    }

    static bool RI_RecordSyntheticTrackingChange(
        UInspectorPropertyItem* Item,
        UObject* TrackingTargetObject,
        FName TrackingPropertyName,
        const FString& OldText,
        const FString& NewText)
    {
        if (!Item || !TrackingTargetObject || TrackingPropertyName.IsNone() || OldText == NewText)
        {
            return true;
        }

        if (UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(Item->GetOuter()))
        {
            FInspectorChange Change;
            Change.ChangeType = EInspectorChangeType::Property;
            Change.Target = TrackingTargetObject;
            Change.PropertyName = TrackingPropertyName;
            Change.OldValueText = OldText;
            Change.NewValueText = NewText;
            Change.DebugObjectName = GetNameSafe(TrackingTargetObject);
            Sub->RecordChange(Change);
        }

        return true;
    }
}

void UInspectorPropertyItem::Init(UObject* InTarget, FName InPropertyName)
{
    Target = InTarget;
    PropertyName = InPropertyName;
    TrackingTarget = InTarget;
    TrackingPropertyName = InPropertyName;
    SyntheticKind = ESyntheticKind::None;
}

void UInspectorPropertyItem::InitSyntheticActorWorld(
    AActor* InActor,
    FName InSyntheticPropertyName,
    USceneComponent* InTrackingRootComponent,
    FName InTrackingPropertyName,
    ESyntheticKind InSyntheticKind)
{
    Target = InActor;
    PropertyName = InSyntheticPropertyName;
    TrackingTarget = InTrackingRootComponent;
    TrackingPropertyName = InTrackingPropertyName;
    SyntheticKind = InSyntheticKind;
}

FString UInspectorPropertyItem::GetPropertyName() const
{
    if (IsSyntheticActorWorldTransform())
    {
        return RI_GetSyntheticActorWorldDisplayName(PropertyName);
    }

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

FString UInspectorPropertyItem::GetPropertyNameWithoutOwnerPrefix() const
{
    if (IsSyntheticActorWorldTransform())
    {
        return RI_GetSyntheticActorWorldDisplayName(PropertyName);
    }

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

    return DisplayName;
}

FString UInspectorPropertyItem::GetCategoryPath() const
{
    if (IsSyntheticActorWorldTransform())
    {
        return TEXT("Actor Transform");
    }

    if (!Target.IsValid())
    {
        return TEXT("Default");
    }

    const FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    if (!Prop)
    {
        return TEXT("Default");
    }

    FString CategoryPath = TEXT("Default");
#if WITH_METADATA
    CategoryPath = Prop->GetMetaData(TEXT("Category"));
#endif
    TArray<FString> Segments;
    RI_SplitCategoryPath(CategoryPath, Segments);
    return FString::Join(Segments, TEXT("|"));
}

FString UInspectorPropertyItem::GetPrimaryCategoryName() const
{
    TArray<FString> Segments;
    RI_SplitCategoryPath(GetCategoryPath(), Segments);
    return Segments.Num() > 0 ? Segments[0] : FString(TEXT("Default"));
}

FString UInspectorPropertyItem::GetSubcategoryPath() const
{
    TArray<FString> Segments;
    RI_SplitCategoryPath(GetCategoryPath(), Segments);
    if (Segments.Num() <= 1)
    {
        return FString();
    }

    TArray<FString> TailSegments;
    for (int32 Index = 1; Index < Segments.Num(); ++Index)
    {
        TailSegments.Add(Segments[Index]);
    }
    return FString::Join(TailSegments, TEXT(" / "));
}

FString UInspectorPropertyItem::GetValueText()
{
    if (IsSyntheticActorWorldTransform())
    {
        AActor* Actor = Cast<AActor>(Target.Get());
        if (!Actor)
        {
            return FString();
        }

        if (SyntheticKind == ESyntheticKind::ActorWorldLocation || SyntheticKind == ESyntheticKind::ActorWorldScale)
        {
            const FVector Value = (SyntheticKind == ESyntheticKind::ActorWorldLocation)
                ? Actor->GetActorLocation()
                : Actor->GetActorScale3D();
            return RI_ExportStructValueText(TBaseStructure<FVector>::Get(), &Value);
        }

        if (SyntheticKind == ESyntheticKind::ActorWorldRotation)
        {
            const FRotator Value = Actor->GetActorRotation();
            return RI_ExportStructValueText(TBaseStructure<FRotator>::Get(), &Value);
        }

        return FString();
    }

    FString Text;
    if (Target.IsValid())
    {
        InspectorPropertyUtils::GetValueAsText(Target.Get(), PropertyName, Text);
    }
    return Text;
}

bool UInspectorPropertyItem::IsEditable() const
{
    if (IsSyntheticActorWorldTransform())
    {
        return Target.IsValid() && Cast<AActor>(Target.Get()) && TrackingTarget.IsValid() && !TrackingPropertyName.IsNone();
    }

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
    if (SyntheticKind == ESyntheticKind::ActorWorldLocation || SyntheticKind == ESyntheticKind::ActorWorldScale)
    {
        return EInspectorValueType::Vector3;
    }

    if (SyntheticKind == ESyntheticKind::ActorWorldRotation)
    {
        return EInspectorValueType::Rotator;
    }

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
    if (SyntheticKind == ESyntheticKind::ActorWorldLocation || SyntheticKind == ESyntheticKind::ActorWorldScale)
    {
        return TEXT("Vector");
    }

    if (SyntheticKind == ESyntheticKind::ActorWorldRotation)
    {
        return TEXT("Rotator");
    }

    if (!Target.IsValid())
    {
        return TEXT("Unknown");
    }

    FProperty* Prop = InspectorPropertyUtils::FindProperty(Target.Get(), PropertyName);
    return RI_GetPropertyTypeLabel(Prop);
}

FString UInspectorPropertyItem::GetReadOnlyReason() const
{
    if (IsSyntheticActorWorldTransform())
    {
        if (!Target.IsValid() || !Cast<AActor>(Target.Get()))
        {
            return TEXT("Actor unavailable");
        }
        if (!TrackingTarget.IsValid() || TrackingPropertyName.IsNone())
        {
            return TEXT("Root scene component unavailable");
        }
        return FString();
    }

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

UObject* UInspectorPropertyItem::GetTrackingTargetObject() const
{
    if (TrackingTarget.IsValid())
    {
        return TrackingTarget.Get();
    }

    return Target.Get();
}

FName UInspectorPropertyItem::GetTrackingPropertyFName() const
{
    return TrackingPropertyName.IsNone() ? PropertyName : TrackingPropertyName;
}

bool UInspectorPropertyItem::IsSyntheticActorWorldTransform() const
{
    return SyntheticKind == ESyntheticKind::ActorWorldLocation
        || SyntheticKind == ESyntheticKind::ActorWorldRotation
        || SyntheticKind == ESyntheticKind::ActorWorldScale
        || RI_IsSyntheticActorWorldPropertyName(PropertyName);
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

    const EInspectorValueType ValueType = GetValueType();
    if (IsSyntheticActorWorldTransform())
    {
        const TCHAR* Buffer = *NewText;
        if (ValueType == EInspectorValueType::Vector3)
        {
            FVector ParsedValue = FVector::ZeroVector;
            if (!TBaseStructure<FVector>::Get()->ImportText(Buffer, &ParsedValue, Obj, PPF_None, nullptr, TEXT("RIActorWorldVector")))
            {
                OutError = TEXT("ImportText failed: invalid vector input");
                return false;
            }
            return SetVector(ParsedValue, OutError);
        }

        if (ValueType == EInspectorValueType::Rotator)
        {
            FRotator ParsedValue = FRotator::ZeroRotator;
            if (!TBaseStructure<FRotator>::Get()->ImportText(Buffer, &ParsedValue, Obj, PPF_None, nullptr, TEXT("RIActorWorldRotator")))
            {
                OutError = TEXT("ImportText failed: invalid rotator input");
                return false;
            }
            return SetRotator(ParsedValue, OutError);
        }
    }

    if (ValueType == EInspectorValueType::Vector3
        || ValueType == EInspectorValueType::Rotator
        || ValueType == EInspectorValueType::Transform)
    {
        FProperty* Prop = InspectorPropertyUtils::FindProperty(Obj, PropertyName);
        FStructProperty* StructProp = CastField<FStructProperty>(Prop);
        if (!StructProp)
        {
            OutError = TEXT("Struct property not found");
            return false;
        }

        const TCHAR* Buffer = *NewText;
        if (ValueType == EInspectorValueType::Vector3)
        {
            FVector ParsedValue = FVector::ZeroVector;
            if (!StructProp->ImportText_Direct(Buffer, &ParsedValue, Obj, PPF_None))
            {
                OutError = TEXT("ImportText failed: invalid vector input");
                return false;
            }
            return SetVector(ParsedValue, OutError);
        }

        if (ValueType == EInspectorValueType::Rotator)
        {
            FRotator ParsedValue = FRotator::ZeroRotator;
            if (!StructProp->ImportText_Direct(Buffer, &ParsedValue, Obj, PPF_None))
            {
                OutError = TEXT("ImportText failed: invalid rotator input");
                return false;
            }
            return SetRotator(ParsedValue, OutError);
        }

        FTransform ParsedValue = FTransform::Identity;
        if (!StructProp->ImportText_Direct(Buffer, &ParsedValue, Obj, PPF_None))
        {
            OutError = TEXT("ImportText failed: invalid transform input");
            return false;
        }
        return SetTransform(ParsedValue, OutError);
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

    bool bAppliedViaSceneComponentSetter = false;
    if (USceneComponent* SC = Cast<USceneComponent>(Obj))
    {
        const FName PName = Item->GetPropertyFName();
        if (WantStruct == TBaseStructure<FVector>::Get() && PName == TEXT("RelativeLocation"))
        {
            SC->SetRelativeLocation(*reinterpret_cast<const FVector*>(InStructValue));
            bAppliedViaSceneComponentSetter = true;
        }
        else if (WantStruct == TBaseStructure<FRotator>::Get() && PName == TEXT("RelativeRotation"))
        {
            SC->SetRelativeRotation(*reinterpret_cast<const FRotator*>(InStructValue));
            bAppliedViaSceneComponentSetter = true;
        }
        else if (WantStruct == TBaseStructure<FVector>::Get() && PName == TEXT("RelativeScale3D"))
        {
            SC->SetRelativeScale3D(*reinterpret_cast<const FVector*>(InStructValue));
            bAppliedViaSceneComponentSetter = true;
        }
        else if (WantStruct == TBaseStructure<FTransform>::Get() && PName == TEXT("RelativeTransform"))
        {
            SC->SetRelativeTransform(*reinterpret_cast<const FTransform*>(InStructValue));
            bAppliedViaSceneComponentSetter = true;
        }
    }

    if (!bAppliedViaSceneComponentSetter)
    {
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
    if (IsSyntheticActorWorldTransform())
    {
        if (AActor* Actor = Cast<AActor>(Target.Get()))
        {
            if (SyntheticKind == ESyntheticKind::ActorWorldLocation)
            {
                OutValue = Actor->GetActorLocation();
                return true;
            }

            if (SyntheticKind == ESyntheticKind::ActorWorldScale)
            {
                OutValue = Actor->GetActorScale3D();
                return true;
            }
        }
        return false;
    }

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
    if (SyntheticKind == ESyntheticKind::ActorWorldRotation)
    {
        if (AActor* Actor = Cast<AActor>(Target.Get()))
        {
            OutValue = Actor->GetActorRotation();
            return true;
        }
        return false;
    }

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
    if (IsSyntheticActorWorldTransform())
    {
        AActor* Actor = Cast<AActor>(Target.Get());
        UObject* TrackingObject = GetTrackingTargetObject();
        const FName EffectiveTrackingProperty = GetTrackingPropertyFName();
        if (!Actor || !TrackingObject || EffectiveTrackingProperty.IsNone())
        {
            OutError = TEXT("Actor/root transform target unavailable");
            return false;
        }

        FString OldText;
        InspectorPropertyUtils::GetValueAsText(TrackingObject, EffectiveTrackingProperty, OldText);

        bool bApplied = false;
        if (SyntheticKind == ESyntheticKind::ActorWorldLocation)
        {
            bApplied = Actor->SetActorLocation(InValue, false, nullptr, ETeleportType::None);
        }
        else if (SyntheticKind == ESyntheticKind::ActorWorldScale)
        {
            Actor->SetActorScale3D(InValue);
            bApplied = true;
        }
        else
        {
            OutError = TEXT("Synthetic property is not a vector transform field");
            return false;
        }

        FString NewText;
        InspectorPropertyUtils::GetValueAsText(TrackingObject, EffectiveTrackingProperty, NewText);
        if (!bApplied && OldText == NewText)
        {
            OutError = TEXT("Actor transform setter did not apply");
            return false;
        }

        RI_RecordSyntheticTrackingChange(this, TrackingObject, EffectiveTrackingProperty, OldText, NewText);
        return true;
    }

    return RI_SetStructValue(this, &InValue, TBaseStructure<FVector>::Get(), OutError);
}

bool UInspectorPropertyItem::SetVector4(const FVector4& InValue, FString& OutError)
{
    return RI_SetStructValue(this, &InValue, TBaseStructure<FVector4>::Get(), OutError);
}

bool UInspectorPropertyItem::SetRotator(const FRotator& InValue, FString& OutError)
{
    if (SyntheticKind == ESyntheticKind::ActorWorldRotation)
    {
        AActor* Actor = Cast<AActor>(Target.Get());
        UObject* TrackingObject = GetTrackingTargetObject();
        const FName EffectiveTrackingProperty = GetTrackingPropertyFName();
        if (!Actor || !TrackingObject || EffectiveTrackingProperty.IsNone())
        {
            OutError = TEXT("Actor/root transform target unavailable");
            return false;
        }

        FString OldText;
        InspectorPropertyUtils::GetValueAsText(TrackingObject, EffectiveTrackingProperty, OldText);
        const bool bApplied = Actor->SetActorRotation(InValue, ETeleportType::None);
        FString NewText;
        InspectorPropertyUtils::GetValueAsText(TrackingObject, EffectiveTrackingProperty, NewText);
        if (!bApplied && OldText == NewText)
        {
            OutError = TEXT("Actor rotation setter did not apply");
            return false;
        }

        RI_RecordSyntheticTrackingChange(this, TrackingObject, EffectiveTrackingProperty, OldText, NewText);
        return true;
    }

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
