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

        // ±ØÐë¿É±à¼­
        if (!Prop->HasAnyPropertyFlags(CPF_Edit)) return false;

        // ²»ÄÜÊÇÖ»¶Á
        if (Prop->HasAnyPropertyFlags(CPF_EditConst)) return false;

        // ²»ÄÜ½ûÖ¹ÊµÀý±à¼­
        if (Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) return false;

        return true;
    }

    bool CanSetFromText(UObject* Obj, const FProperty* Prop)
    {
        return IsSupportedEditableProperty(Prop);
    }


    static bool IsSupportedReadableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;

        // ¹ýÂËµôÌ«Î£ÏÕ/Ã»ÒâÒåµÄ£¨Äã¿ÉÒÔÖð²½·Å¿ª£©
        if (Prop->IsA<FObjectPropertyBase>()) return false; // UObject*¡¢×é¼þÖ¸ÕëµÈÏÈ²»Åö
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
 // ÏÈ±ð×ö£¬ºóÃæ×¨ÃÅÖ§³ÖVector/RotatorÔÙ·Å¿ª

        // ÏÈÖ§³Ö»ù´¡ÀàÐÍ + enum£¨¶ÁÆðÀ´ÎÈ¶¨£©
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

    bool IsDisplayableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;
        if (Prop->HasAnyPropertyFlags(CPF_Deprecated)) return false;
        if (Prop->IsA<FDelegateProperty>() || Prop->IsA<FMulticastDelegateProperty>()) return false;
        return true;
    }

    bool IsEditableProperty(const FProperty* Prop)
    {
        if (!Prop) return false;
       //  ÈÔÈ»ÍÆ¼ö£ºÖ»ÔÊÐí Edit µÄÐ´Èë
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
            if (!Prop) continue;
            if (!IsDisplayableProperty(Prop))
            {
                continue;
            }
            if (!Prop->HasAnyPropertyFlags(CPF_Edit) && !Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
            {
                continue;
            }
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

        // Export ³É×Ö·û´®
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

        // Import ×Ö·û´®Ð´Èë
        const TCHAR* Buffer = *InText;
        const TCHAR* Result = Prop->ImportText_Direct(Buffer, ValuePtr, Target, PPF_None);

        if (!Result)
        {
            if (OutError) *OutError = TEXT("ImportText failed: check input format");
            return false;
        }

        // ÈÃÄ³Ð©ÏµÍ³¸ÐÖª±ä»¯£¨ÇáÁ¿´¦Àí£©
        Target->MarkPackageDirty(); // runtimeÀïÃ»É¶¸±×÷ÓÃ£»ºóÐøÄãÒ²¿É¸Ä³É½öEditor
        return true;
    }

    
}
