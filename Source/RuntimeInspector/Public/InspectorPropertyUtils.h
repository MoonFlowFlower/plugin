#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

#include "InspectorDefines.h"

namespace InspectorPropertyUtils
{
    // ֻһ a.b.c
    bool IsSupportedEditableProperty(const FProperty* Prop);
    bool IsEditableProperty(const FProperty* Prop);
    bool CanSetFromText(UObject* obj,const FProperty* Prop);
    void GatherProperties(UObject* Target, TArray<FName>& OutPropertyNames);

    bool GetValueAsText(UObject* Target, FName PropertyName, FString& OutText);
    bool SetValueFromText(UObject* Target, FName PropertyName, const FString& InText, FString* OutError = nullptr);

    FProperty* FindProperty(UObject* Target, FName PropertyName);
}
