#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

// Shipping禁用宏
#if UE_BUILD_SHIPPING
#define RUNTIME_INSPECTOR_ENABLED 0
#else
#define RUNTIME_INSPECTOR_ENABLED 1
#endif

namespace InspectorPropertyUtils
{
    // 只做一级属性名（后续再做 a.b.c）
    bool IsSupportedEditableProperty(const FProperty* Prop);
    bool IsEditableProperty(const FProperty* Prop);
    bool CanSetFromText(UObject* obj,const FProperty* Prop);
    void GatherProperties(UObject* Target, TArray<FName>& OutPropertyNames);

    bool GetValueAsText(UObject* Target, FName PropertyName, FString& OutText);
    bool SetValueFromText(UObject* Target, FName PropertyName, const FString& InText, FString* OutError = nullptr);

    FProperty* FindProperty(UObject* Target, FName PropertyName);
}
