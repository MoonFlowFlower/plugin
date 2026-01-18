#include "InspectorPropertyUtils.h"

#include "UObject/NoExportTypes.h"

namespace InspectorPropertyUtils
{
    FProperty* FindProperty(UObject* Target, FName PropertyName)
    {
        if (!Target) return nullptr;
        UClass* Cls = Target->GetClass();
        if (!Cls) return nullptr;

        return FindFProperty<FProperty>(Cls, PropertyName);
    }

    static bool IsEditableByFlags(const FProperty* Prop)
    {
        if (!Prop) return false;

        // 必须可编辑
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) return false;

        // 不能是只读
        if (Prop->HasAnyPropertyFlags(CPF_EditConst)) return false;

        // 不能禁止实例编辑
        if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) return false;

        return true;
    }

    bool CanSetFromText(UObject* Obj, const FProperty* Prop)
    {
        if (!Prop) return false;

        // 必须可编辑
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) return false;

        //// 不能是只读
        //if (Prop->HasAnyPropertyFlags(CPF_EditConst)) return false;

        //// 不能禁止实例编辑
        //if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) return false;

        return true;
    }


    static bool IsSupportedReadableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;

        // 过滤掉太危险/没意义的（你可以逐步放开）
        if (Prop->IsA<FObjectPropertyBase>()) return false; // UObject*、组件指针等先不碰
        if (Prop->IsA<FArrayProperty>() ||
            Prop->IsA<FMapProperty>() || 
            Prop->IsA<FSetProperty>()) return false;
        if (const FStructProperty* SP = CastField<FStructProperty>(Prop))
        {
            const UScriptStruct* S = SP->Struct;
            if (S == TBaseStructure<FVector2D>::Get()) return true;
            if (S == TBaseStructure<FVector>::Get())   return true;
            if (S == TBaseStructure<FVector4>::Get())  return true;
            if (S == TBaseStructure<FRotator>::Get())  return true;
            if (S == TBaseStructure<FTransform>::Get())return true;
            if (S == TBaseStructure<FLinearColor>::Get()) return true;
            if (S == TBaseStructure<FColor>::Get()) return true;
            return false;
        }
 // 先别做，后面专门支持Vector/Rotator再放开

        // 先支持基础类型 + enum（读起来稳定）
        if (Prop->IsA<FBoolProperty>()) return true;
        if (Prop->IsA<FIntProperty>()) return true;
        if (Prop->IsA<FFloatProperty>() ||
            Prop->IsA<FDoubleProperty>()) return true;
        if (Prop->IsA<FStrProperty>()) return true;
        if (Prop->IsA<FNameProperty>()) return true;
        if (Prop->IsA<FEnumProperty>()) return true;
        if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop)) return ByteProp->Enum != nullptr;

        return false;
    }

    bool IsEditableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;
       //  仍然推荐：只允许 Edit 的写入
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) return false;
        //if (Prop->HasAnyPropertyFlags(CPF_EditConst)) return false;
        //if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) return false;
        return true;
    }

    void GatherProperties(UObject* Target, TArray<FName>& OutPropertyNames)
    {
        OutPropertyNames.Reset();
        if (!Target) return;

        for (TFieldIterator<FProperty> It(Target->GetClass()); It; ++It)
        {
            const FProperty* Prop = *It;
            if (!IsSupportedReadableProperty(Prop)) continue;
            OutPropertyNames.Add(Prop->GetFName());
        }
    }
    
    bool IsSupportedEditableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;
        if (!IsEditableProperty(Prop)) return false;
        if (!IsSupportedReadableProperty(Prop)) return false;
        return true;
	}

    bool GetValueAsText(UObject* Target, FName PropertyName, FString& OutText)
    {
        OutText.Reset();
        if (!Target) return false;

        FProperty* Prop = FindProperty(Target, PropertyName);
        if (!Prop) return false;

        void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Target);
        if (!ValuePtr) return false;

        // Export 成字符串
        Prop->ExportTextItem_Direct(OutText, ValuePtr, nullptr, Target, PPF_None);
        return true;
    }

    bool SetValueFromText(UObject* Target, FName PropertyName, const FString& InText, FString* OutError)
    {
        if (!Target) return false;

        FProperty* Prop = FindProperty(Target, PropertyName);
        if (!Prop)
        {
            if (OutError) *OutError = TEXT("Property not found");
            return false;
        }

        if (!IsSupportedEditableProperty(Prop))
        {
            if (OutError) *OutError = TEXT("Property is not editable or not supported");
            return false;
        }

        void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Target);
        if (!ValuePtr)
        {
            if (OutError) *OutError = TEXT("Invalid value pointer");
            return false;
        }

        // Import 字符串写入
        const TCHAR* Buffer = *InText;
        const TCHAR* Result = Prop->ImportText_Direct(Buffer, ValuePtr, Target, PPF_None);

        if (!Result)
        {
            if (OutError) *OutError = TEXT("ImportText failed: check input format");
            return false;
        }

        // 让某些系统感知变化（轻量处理）
        Target->MarkPackageDirty(); // runtime里没啥副作用；后续你也可改成仅Editor
        return true;
    }

    
}
