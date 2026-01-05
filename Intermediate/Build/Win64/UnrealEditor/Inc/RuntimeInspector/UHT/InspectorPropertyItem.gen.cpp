// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorPropertyItem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorPropertyItem() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorPropertyItem();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorPropertyItem_NoRegister();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorValueType();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Class UInspectorPropertyItem Function ApplyFromText
struct Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics
{
	struct InspectorPropertyItem_eventApplyFromText_Parms
	{
		FString NewText;
		FString OutError;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_NewText;
	static const UECodeGen_Private::FStrPropertyParams NewProp_OutError;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_NewText = { "NewText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventApplyFromText_Parms, NewText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewText_MetaData), NewProp_NewText_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_OutError = { "OutError", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventApplyFromText_Parms, OutError), METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorPropertyItem_eventApplyFromText_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorPropertyItem_eventApplyFromText_Parms), &Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_NewText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_OutError,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "ApplyFromText", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::InspectorPropertyItem_eventApplyFromText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::InspectorPropertyItem_eventApplyFromText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execApplyFromText)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_NewText);
	P_GET_PROPERTY_REF(FStrProperty,Z_Param_Out_OutError);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->ApplyFromText(Z_Param_NewText,Z_Param_Out_OutError);
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function ApplyFromText

// Begin Class UInspectorPropertyItem Function GetEnumOptions
struct Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics
{
	struct InspectorPropertyItem_eventGetEnumOptions_Parms
	{
		TArray<FString> OutOptions;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_OutOptions_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_OutOptions;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::NewProp_OutOptions_Inner = { "OutOptions", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::NewProp_OutOptions = { "OutOptions", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetEnumOptions_Parms, OutOptions), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::NewProp_OutOptions_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::NewProp_OutOptions,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetEnumOptions", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::InspectorPropertyItem_eventGetEnumOptions_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::InspectorPropertyItem_eventGetEnumOptions_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetEnumOptions)
{
	P_GET_TARRAY_REF(FString,Z_Param_Out_OutOptions);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetEnumOptions(Z_Param_Out_OutOptions);
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetEnumOptions

// Begin Class UInspectorPropertyItem Function GetPropertyFName
struct Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics
{
	struct InspectorPropertyItem_eventGetPropertyFName_Parms
	{
		FName ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FNamePropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetPropertyFName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetPropertyFName", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::InspectorPropertyItem_eventGetPropertyFName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::InspectorPropertyItem_eventGetPropertyFName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetPropertyFName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FName*)Z_Param__Result=P_THIS->GetPropertyFName();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetPropertyFName

// Begin Class UInspectorPropertyItem Function GetPropertyName
struct Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics
{
	struct InspectorPropertyItem_eventGetPropertyName_Parms
	{
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetPropertyName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetPropertyName", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::InspectorPropertyItem_eventGetPropertyName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::InspectorPropertyItem_eventGetPropertyName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetPropertyName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=P_THIS->GetPropertyName();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetPropertyName

// Begin Class UInspectorPropertyItem Function GetTargetObject
struct Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics
{
	struct InspectorPropertyItem_eventGetTargetObject_Parms
	{
		UObject* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetTargetObject_Parms, ReturnValue), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetTargetObject", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::InspectorPropertyItem_eventGetTargetObject_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::InspectorPropertyItem_eventGetTargetObject_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetTargetObject)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UObject**)Z_Param__Result=P_THIS->GetTargetObject();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetTargetObject

// Begin Class UInspectorPropertyItem Function GetValueText
struct Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics
{
	struct InspectorPropertyItem_eventGetValueText_Parms
	{
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetValueText_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetValueText", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::InspectorPropertyItem_eventGetValueText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::InspectorPropertyItem_eventGetValueText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetValueText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetValueText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetValueText)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=P_THIS->GetValueText();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetValueText

// Begin Class UInspectorPropertyItem Function GetValueType
struct Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics
{
	struct InspectorPropertyItem_eventGetValueType_Parms
	{
		EInspectorValueType ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorPropertyItem_eventGetValueType_Parms, ReturnValue), Z_Construct_UEnum_RuntimeInspector_EInspectorValueType, METADATA_PARAMS(0, nullptr) }; // 1864999122
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::NewProp_ReturnValue_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "GetValueType", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::InspectorPropertyItem_eventGetValueType_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::InspectorPropertyItem_eventGetValueType_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_GetValueType()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_GetValueType_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execGetValueType)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(EInspectorValueType*)Z_Param__Result=P_THIS->GetValueType();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function GetValueType

// Begin Class UInspectorPropertyItem Function IsEditable
struct Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics
{
	struct InspectorPropertyItem_eventIsEditable_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorPropertyItem_eventIsEditable_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorPropertyItem_eventIsEditable_Parms), &Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "IsEditable", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::InspectorPropertyItem_eventIsEditable_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::InspectorPropertyItem_eventIsEditable_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_IsEditable()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_IsEditable_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execIsEditable)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsEditable();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function IsEditable

// Begin Class UInspectorPropertyItem Function IsEnum
struct Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics
{
	struct InspectorPropertyItem_eventIsEnum_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorPropertyItem_eventIsEnum_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorPropertyItem_eventIsEnum_Parms), &Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "IsEnum", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::InspectorPropertyItem_eventIsEnum_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::InspectorPropertyItem_eventIsEnum_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_IsEnum()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_IsEnum_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execIsEnum)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsEnum();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function IsEnum

// Begin Class UInspectorPropertyItem Function IsValidItem
struct Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics
{
	struct InspectorPropertyItem_eventIsValidItem_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorPropertyItem_eventIsValidItem_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorPropertyItem_eventIsValidItem_Parms), &Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorPropertyItem, nullptr, "IsValidItem", nullptr, nullptr, Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::InspectorPropertyItem_eventIsValidItem_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::InspectorPropertyItem_eventIsValidItem_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorPropertyItem::execIsValidItem)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsValidItem();
	P_NATIVE_END;
}
// End Class UInspectorPropertyItem Function IsValidItem

// Begin Class UInspectorPropertyItem
void UInspectorPropertyItem::StaticRegisterNativesUInspectorPropertyItem()
{
	UClass* Class = UInspectorPropertyItem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ApplyFromText", &UInspectorPropertyItem::execApplyFromText },
		{ "GetEnumOptions", &UInspectorPropertyItem::execGetEnumOptions },
		{ "GetPropertyFName", &UInspectorPropertyItem::execGetPropertyFName },
		{ "GetPropertyName", &UInspectorPropertyItem::execGetPropertyName },
		{ "GetTargetObject", &UInspectorPropertyItem::execGetTargetObject },
		{ "GetValueText", &UInspectorPropertyItem::execGetValueText },
		{ "GetValueType", &UInspectorPropertyItem::execGetValueType },
		{ "IsEditable", &UInspectorPropertyItem::execIsEditable },
		{ "IsEnum", &UInspectorPropertyItem::execIsEnum },
		{ "IsValidItem", &UInspectorPropertyItem::execIsValidItem },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInspectorPropertyItem);
UClass* Z_Construct_UClass_UInspectorPropertyItem_NoRegister()
{
	return UInspectorPropertyItem::StaticClass();
}
struct Z_Construct_UClass_UInspectorPropertyItem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "InspectorPropertyItem.h" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OwnerPrefix_MetaData[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorPropertyItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_OwnerPrefix;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UInspectorPropertyItem_ApplyFromText, "ApplyFromText" }, // 852978135
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetEnumOptions, "GetEnumOptions" }, // 1756343502
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyFName, "GetPropertyFName" }, // 270516044
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetPropertyName, "GetPropertyName" }, // 1129236665
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetTargetObject, "GetTargetObject" }, // 3481610024
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetValueText, "GetValueText" }, // 2236054701
		{ &Z_Construct_UFunction_UInspectorPropertyItem_GetValueType, "GetValueType" }, // 3871615007
		{ &Z_Construct_UFunction_UInspectorPropertyItem_IsEditable, "IsEditable" }, // 3268911625
		{ &Z_Construct_UFunction_UInspectorPropertyItem_IsEnum, "IsEnum" }, // 3421480235
		{ &Z_Construct_UFunction_UInspectorPropertyItem_IsValidItem, "IsValidItem" }, // 3692137658
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UInspectorPropertyItem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UInspectorPropertyItem_Statics::NewProp_OwnerPrefix = { "OwnerPrefix", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorPropertyItem, OwnerPrefix), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OwnerPrefix_MetaData), NewProp_OwnerPrefix_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UInspectorPropertyItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorPropertyItem_Statics::NewProp_OwnerPrefix,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorPropertyItem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UInspectorPropertyItem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorPropertyItem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UInspectorPropertyItem_Statics::ClassParams = {
	&UInspectorPropertyItem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UInspectorPropertyItem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorPropertyItem_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorPropertyItem_Statics::Class_MetaDataParams), Z_Construct_UClass_UInspectorPropertyItem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UInspectorPropertyItem()
{
	if (!Z_Registration_Info_UClass_UInspectorPropertyItem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UInspectorPropertyItem.OuterSingleton, Z_Construct_UClass_UInspectorPropertyItem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UInspectorPropertyItem.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UClass* StaticClass<UInspectorPropertyItem>()
{
	return UInspectorPropertyItem::StaticClass();
}
UInspectorPropertyItem::UInspectorPropertyItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UInspectorPropertyItem);
UInspectorPropertyItem::~UInspectorPropertyItem() {}
// End Class UInspectorPropertyItem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UInspectorPropertyItem, UInspectorPropertyItem::StaticClass, TEXT("UInspectorPropertyItem"), &Z_Registration_Info_UClass_UInspectorPropertyItem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UInspectorPropertyItem), 1469509910U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_2447417162(TEXT("/Script/RuntimeInspector"),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
