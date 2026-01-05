// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorMaterialParamItem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorMaterialParamItem() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_UMeshComponent_NoRegister();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorMaterialParamItem();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorMaterialParamItem_NoRegister();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Enum EInspectorMatParamType
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EInspectorMatParamType;
static UEnum* EInspectorMatParamType_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EInspectorMatParamType.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EInspectorMatParamType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("EInspectorMatParamType"));
	}
	return Z_Registration_Info_UEnum_EInspectorMatParamType.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorMatParamType>()
{
	return EInspectorMatParamType_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
		{ "Scalar.Name", "EInspectorMatParamType::Scalar" },
		{ "Vector.Name", "EInspectorMatParamType::Vector" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EInspectorMatParamType::Scalar", (int64)EInspectorMatParamType::Scalar },
		{ "EInspectorMatParamType::Vector", (int64)EInspectorMatParamType::Vector },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"EInspectorMatParamType",
	"EInspectorMatParamType",
	Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType()
{
	if (!Z_Registration_Info_UEnum_EInspectorMatParamType.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EInspectorMatParamType.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EInspectorMatParamType.InnerSingleton;
}
// End Enum EInspectorMatParamType

// Begin Class UInspectorMaterialParamItem Function ApplyFromText
struct Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics
{
	struct InspectorMaterialParamItem_eventApplyFromText_Parms
	{
		FString NewText;
		FString OutError;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
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
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_NewText = { "NewText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventApplyFromText_Parms, NewText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewText_MetaData), NewProp_NewText_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_OutError = { "OutError", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventApplyFromText_Parms, OutError), METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorMaterialParamItem_eventApplyFromText_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorMaterialParamItem_eventApplyFromText_Parms), &Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_NewText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_OutError,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "ApplyFromText", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::InspectorMaterialParamItem_eventApplyFromText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::InspectorMaterialParamItem_eventApplyFromText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execApplyFromText)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_NewText);
	P_GET_PROPERTY_REF(FStrProperty,Z_Param_Out_OutError);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->ApplyFromText(Z_Param_NewText,Z_Param_Out_OutError);
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function ApplyFromText

// Begin Class UInspectorMaterialParamItem Function GetMeshComponent
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics
{
	struct InspectorMaterialParamItem_eventGetMeshComponent_Parms
	{
		UMeshComponent* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000080588, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetMeshComponent_Parms, ReturnValue), Z_Construct_UClass_UMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ReturnValue_MetaData), NewProp_ReturnValue_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetMeshComponent", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::InspectorMaterialParamItem_eventGetMeshComponent_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::InspectorMaterialParamItem_eventGetMeshComponent_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetMeshComponent)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UMeshComponent**)Z_Param__Result=P_THIS->GetMeshComponent();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetMeshComponent

// Begin Class UInspectorMaterialParamItem Function GetParamName
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics
{
	struct InspectorMaterialParamItem_eventGetParamName_Parms
	{
		FName ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FNamePropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetParamName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetParamName", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::InspectorMaterialParamItem_eventGetParamName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::InspectorMaterialParamItem_eventGetParamName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetParamName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FName*)Z_Param__Result=P_THIS->GetParamName();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetParamName

// Begin Class UInspectorMaterialParamItem Function GetParamType
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics
{
	struct InspectorMaterialParamItem_eventGetParamType_Parms
	{
		EInspectorMatParamType ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetParamType_Parms, ReturnValue), Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType, METADATA_PARAMS(0, nullptr) }; // 611925172
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::NewProp_ReturnValue_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetParamType", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::InspectorMaterialParamItem_eventGetParamType_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::InspectorMaterialParamItem_eventGetParamType_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetParamType)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(EInspectorMatParamType*)Z_Param__Result=P_THIS->GetParamType();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetParamType

// Begin Class UInspectorMaterialParamItem Function GetPropertyName
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics
{
	struct InspectorMaterialParamItem_eventGetPropertyName_Parms
	{
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetPropertyName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetPropertyName", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::InspectorMaterialParamItem_eventGetPropertyName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::InspectorMaterialParamItem_eventGetPropertyName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetPropertyName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=P_THIS->GetPropertyName();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetPropertyName

// Begin Class UInspectorMaterialParamItem Function GetSlotIndex
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics
{
	struct InspectorMaterialParamItem_eventGetSlotIndex_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd4\xbc\xef\xbf\xbd\xef\xbf\xbd\xc4\xb8\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd4\xbc\xef\xbf\xbd\xef\xbf\xbd\xc4\xb8\xef\xbf\xbd" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetSlotIndex_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetSlotIndex", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::InspectorMaterialParamItem_eventGetSlotIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::InspectorMaterialParamItem_eventGetSlotIndex_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetSlotIndex)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetSlotIndex();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetSlotIndex

// Begin Class UInspectorMaterialParamItem Function GetValueText
struct Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics
{
	struct InspectorMaterialParamItem_eventGetValueText_Parms
	{
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorMaterialParamItem_eventGetValueText_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "GetValueText", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::InspectorMaterialParamItem_eventGetValueText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::InspectorMaterialParamItem_eventGetValueText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execGetValueText)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=P_THIS->GetValueText();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function GetValueText

// Begin Class UInspectorMaterialParamItem Function IsEditable
struct Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics
{
	struct InspectorMaterialParamItem_eventIsEditable_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorMaterialParamItem_eventIsEditable_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorMaterialParamItem_eventIsEditable_Parms), &Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "IsEditable", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::InspectorMaterialParamItem_eventIsEditable_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::InspectorMaterialParamItem_eventIsEditable_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execIsEditable)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsEditable();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function IsEditable

// Begin Class UInspectorMaterialParamItem Function IsEnum
struct Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics
{
	struct InspectorMaterialParamItem_eventIsEnum_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorMaterialParamItem_eventIsEnum_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorMaterialParamItem_eventIsEnum_Parms), &Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorMaterialParamItem, nullptr, "IsEnum", nullptr, nullptr, Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::InspectorMaterialParamItem_eventIsEnum_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::InspectorMaterialParamItem_eventIsEnum_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorMaterialParamItem::execIsEnum)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsEnum();
	P_NATIVE_END;
}
// End Class UInspectorMaterialParamItem Function IsEnum

// Begin Class UInspectorMaterialParamItem
void UInspectorMaterialParamItem::StaticRegisterNativesUInspectorMaterialParamItem()
{
	UClass* Class = UInspectorMaterialParamItem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ApplyFromText", &UInspectorMaterialParamItem::execApplyFromText },
		{ "GetMeshComponent", &UInspectorMaterialParamItem::execGetMeshComponent },
		{ "GetParamName", &UInspectorMaterialParamItem::execGetParamName },
		{ "GetParamType", &UInspectorMaterialParamItem::execGetParamType },
		{ "GetPropertyName", &UInspectorMaterialParamItem::execGetPropertyName },
		{ "GetSlotIndex", &UInspectorMaterialParamItem::execGetSlotIndex },
		{ "GetValueText", &UInspectorMaterialParamItem::execGetValueText },
		{ "IsEditable", &UInspectorMaterialParamItem::execIsEditable },
		{ "IsEnum", &UInspectorMaterialParamItem::execIsEnum },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInspectorMaterialParamItem);
UClass* Z_Construct_UClass_UInspectorMaterialParamItem_NoRegister()
{
	return UInspectorMaterialParamItem::StaticClass();
}
struct Z_Construct_UClass_UInspectorMaterialParamItem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "InspectorMaterialParamItem.h" },
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetComp_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SlotIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ParamName_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ParamType_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorMaterialParamItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_TargetComp;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SlotIndex;
	static const UECodeGen_Private::FNamePropertyParams NewProp_ParamName;
	static const UECodeGen_Private::FBytePropertyParams NewProp_ParamType_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_ParamType;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_ApplyFromText, "ApplyFromText" }, // 2325017005
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetMeshComponent, "GetMeshComponent" }, // 3071243897
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamName, "GetParamName" }, // 1595521042
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetParamType, "GetParamType" }, // 1479346441
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetPropertyName, "GetPropertyName" }, // 2861072202
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetSlotIndex, "GetSlotIndex" }, // 3711995090
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_GetValueText, "GetValueText" }, // 3895086458
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_IsEditable, "IsEditable" }, // 640659787
		{ &Z_Construct_UFunction_UInspectorMaterialParamItem_IsEnum, "IsEnum" }, // 2381971772
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UInspectorMaterialParamItem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_TargetComp = { "TargetComp", nullptr, (EPropertyFlags)0x0044000000080008, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorMaterialParamItem, TargetComp), Z_Construct_UClass_UMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetComp_MetaData), NewProp_TargetComp_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_SlotIndex = { "SlotIndex", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorMaterialParamItem, SlotIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SlotIndex_MetaData), NewProp_SlotIndex_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamName = { "ParamName", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorMaterialParamItem, ParamName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ParamName_MetaData), NewProp_ParamName_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamType = { "ParamType", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorMaterialParamItem, ParamType), Z_Construct_UEnum_RuntimeInspector_EInspectorMatParamType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ParamType_MetaData), NewProp_ParamType_MetaData) }; // 611925172
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UInspectorMaterialParamItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_TargetComp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_SlotIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamType_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorMaterialParamItem_Statics::NewProp_ParamType,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorMaterialParamItem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UInspectorMaterialParamItem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorMaterialParamItem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UInspectorMaterialParamItem_Statics::ClassParams = {
	&UInspectorMaterialParamItem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UInspectorMaterialParamItem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorMaterialParamItem_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorMaterialParamItem_Statics::Class_MetaDataParams), Z_Construct_UClass_UInspectorMaterialParamItem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UInspectorMaterialParamItem()
{
	if (!Z_Registration_Info_UClass_UInspectorMaterialParamItem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UInspectorMaterialParamItem.OuterSingleton, Z_Construct_UClass_UInspectorMaterialParamItem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UInspectorMaterialParamItem.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UClass* StaticClass<UInspectorMaterialParamItem>()
{
	return UInspectorMaterialParamItem::StaticClass();
}
UInspectorMaterialParamItem::UInspectorMaterialParamItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UInspectorMaterialParamItem);
UInspectorMaterialParamItem::~UInspectorMaterialParamItem() {}
// End Class UInspectorMaterialParamItem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EInspectorMatParamType_StaticEnum, TEXT("EInspectorMatParamType"), &Z_Registration_Info_UEnum_EInspectorMatParamType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 611925172U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UInspectorMaterialParamItem, UInspectorMaterialParamItem::StaticClass, TEXT("UInspectorMaterialParamItem"), &Z_Registration_Info_UClass_UInspectorMaterialParamItem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UInspectorMaterialParamItem), 346760890U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_890158460(TEXT("/Script/RuntimeInspector"),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_Statics::ClassInfo),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
