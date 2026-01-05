// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorGroupItem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorGroupItem() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorGroupItem();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorGroupItem_NoRegister();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind();
RUNTIMEINSPECTOR_API UFunction* Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Delegate FOnInspectorGroupExpandedChanged
struct Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics
{
	struct _Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms
	{
		bool bIsExpanded;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_bIsExpanded_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsExpanded;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::NewProp_bIsExpanded_SetBit(void* Obj)
{
	((_Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms*)Obj)->bIsExpanded = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::NewProp_bIsExpanded = { "bIsExpanded", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(_Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms), &Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::NewProp_bIsExpanded_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::NewProp_bIsExpanded,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector, nullptr, "OnInspectorGroupExpandedChanged__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::_Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::_Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FOnInspectorGroupExpandedChanged_DelegateWrapper(const FMulticastScriptDelegate& OnInspectorGroupExpandedChanged, bool bIsExpanded)
{
	struct _Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms
	{
		bool bIsExpanded;
	};
	_Script_RuntimeInspector_eventOnInspectorGroupExpandedChanged_Parms Parms;
	Parms.bIsExpanded=bIsExpanded ? true : false;
	OnInspectorGroupExpandedChanged.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FOnInspectorGroupExpandedChanged

// Begin Enum EInspectorGroupKind
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EInspectorGroupKind;
static UEnum* EInspectorGroupKind_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EInspectorGroupKind.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EInspectorGroupKind.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("EInspectorGroupKind"));
	}
	return Z_Registration_Info_UEnum_EInspectorGroupKind.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorGroupKind>()
{
	return EInspectorGroupKind_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Component.DisplayName", "Component" },
		{ "Component.Name", "EInspectorGroupKind::Component" },
		{ "MaterialSlot.DisplayName", "MaterialSlot" },
		{ "MaterialSlot.Name", "EInspectorGroupKind::MaterialSlot" },
		{ "MaterialsRoot.DisplayName", "MaterialsRoot" },
		{ "MaterialsRoot.Name", "EInspectorGroupKind::MaterialsRoot" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
		{ "RootActor.DisplayName", "ActorRoot" },
		{ "RootActor.Name", "EInspectorGroupKind::RootActor" },
		{ "RootComponents.DisplayName", "ComponentsRoot" },
		{ "RootComponents.Name", "EInspectorGroupKind::RootComponents" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EInspectorGroupKind::RootActor", (int64)EInspectorGroupKind::RootActor },
		{ "EInspectorGroupKind::RootComponents", (int64)EInspectorGroupKind::RootComponents },
		{ "EInspectorGroupKind::Component", (int64)EInspectorGroupKind::Component },
		{ "EInspectorGroupKind::MaterialsRoot", (int64)EInspectorGroupKind::MaterialsRoot },
		{ "EInspectorGroupKind::MaterialSlot", (int64)EInspectorGroupKind::MaterialSlot },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"EInspectorGroupKind",
	"EInspectorGroupKind",
	Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind()
{
	if (!Z_Registration_Info_UEnum_EInspectorGroupKind.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EInspectorGroupKind.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EInspectorGroupKind.InnerSingleton;
}
// End Enum EInspectorGroupKind

// Begin Class UInspectorGroupItem Function GetIsExpanded
struct Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics
{
	struct InspectorGroupItem_eventGetIsExpanded_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Group" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventGetIsExpanded_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventGetIsExpanded_Parms), &Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "GetIsExpanded", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::InspectorGroupItem_eventGetIsExpanded_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::InspectorGroupItem_eventGetIsExpanded_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execGetIsExpanded)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->GetIsExpanded();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function GetIsExpanded

// Begin Class UInspectorGroupItem Function IsComponentGroup
struct Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics
{
	struct InspectorGroupItem_eventIsComponentGroup_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventIsComponentGroup_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventIsComponentGroup_Parms), &Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "IsComponentGroup", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::InspectorGroupItem_eventIsComponentGroup_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::InspectorGroupItem_eventIsComponentGroup_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execIsComponentGroup)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsComponentGroup();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function IsComponentGroup

// Begin Class UInspectorGroupItem Function IsMaterialSlot
struct Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics
{
	struct InspectorGroupItem_eventIsMaterialSlot_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventIsMaterialSlot_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventIsMaterialSlot_Parms), &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "IsMaterialSlot", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::InspectorGroupItem_eventIsMaterialSlot_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::InspectorGroupItem_eventIsMaterialSlot_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execIsMaterialSlot)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsMaterialSlot();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function IsMaterialSlot

// Begin Class UInspectorGroupItem Function IsMaterialSlotNode
struct Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics
{
	struct InspectorGroupItem_eventIsMaterialSlotNode_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventIsMaterialSlotNode_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventIsMaterialSlotNode_Parms), &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "IsMaterialSlotNode", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::InspectorGroupItem_eventIsMaterialSlotNode_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::InspectorGroupItem_eventIsMaterialSlotNode_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execIsMaterialSlotNode)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsMaterialSlotNode();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function IsMaterialSlotNode

// Begin Class UInspectorGroupItem Function IsMaterialsNode
struct Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics
{
	struct InspectorGroupItem_eventIsMaterialsNode_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventIsMaterialsNode_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventIsMaterialsNode_Parms), &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "IsMaterialsNode", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::InspectorGroupItem_eventIsMaterialsNode_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::InspectorGroupItem_eventIsMaterialsNode_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execIsMaterialsNode)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsMaterialsNode();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function IsMaterialsNode

// Begin Class UInspectorGroupItem Function IsMaterialsRoot
struct Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics
{
	struct InspectorGroupItem_eventIsMaterialsRoot_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorGroupItem_eventIsMaterialsRoot_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventIsMaterialsRoot_Parms), &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "IsMaterialsRoot", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::InspectorGroupItem_eventIsMaterialsRoot_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::InspectorGroupItem_eventIsMaterialsRoot_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execIsMaterialsRoot)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsMaterialsRoot();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function IsMaterialsRoot

// Begin Class UInspectorGroupItem Function SetIsExpanded
struct Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics
{
	struct InspectorGroupItem_eventSetIsExpanded_Parms
	{
		bool bInExpanded;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Group" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_bInExpanded_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bInExpanded;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::NewProp_bInExpanded_SetBit(void* Obj)
{
	((InspectorGroupItem_eventSetIsExpanded_Parms*)Obj)->bInExpanded = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::NewProp_bInExpanded = { "bInExpanded", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorGroupItem_eventSetIsExpanded_Parms), &Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::NewProp_bInExpanded_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::NewProp_bInExpanded,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "SetIsExpanded", nullptr, nullptr, Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::InspectorGroupItem_eventSetIsExpanded_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::InspectorGroupItem_eventSetIsExpanded_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execSetIsExpanded)
{
	P_GET_UBOOL(Z_Param_bInExpanded);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetIsExpanded(Z_Param_bInExpanded);
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function SetIsExpanded

// Begin Class UInspectorGroupItem Function ToggleExpanded
struct Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Group" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorGroupItem, nullptr, "ToggleExpanded", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorGroupItem::execToggleExpanded)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ToggleExpanded();
	P_NATIVE_END;
}
// End Class UInspectorGroupItem Function ToggleExpanded

// Begin Class UInspectorGroupItem
void UInspectorGroupItem::StaticRegisterNativesUInspectorGroupItem()
{
	UClass* Class = UInspectorGroupItem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetIsExpanded", &UInspectorGroupItem::execGetIsExpanded },
		{ "IsComponentGroup", &UInspectorGroupItem::execIsComponentGroup },
		{ "IsMaterialSlot", &UInspectorGroupItem::execIsMaterialSlot },
		{ "IsMaterialSlotNode", &UInspectorGroupItem::execIsMaterialSlotNode },
		{ "IsMaterialsNode", &UInspectorGroupItem::execIsMaterialsNode },
		{ "IsMaterialsRoot", &UInspectorGroupItem::execIsMaterialsRoot },
		{ "SetIsExpanded", &UInspectorGroupItem::execSetIsExpanded },
		{ "ToggleExpanded", &UInspectorGroupItem::execToggleExpanded },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInspectorGroupItem);
UClass* Z_Construct_UClass_UInspectorGroupItem_NoRegister()
{
	return UInspectorGroupItem::StaticClass();
}
struct Z_Construct_UClass_UInspectorGroupItem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * ListView \xef\xbf\xbd\xc4\xa1\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd0\xa1\xef\xbf\xbdItem\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n * - RootActor / RootComponents\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n * - Component\xef\xbf\xbd\xef\xbf\xbd\xc4\xb3\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n */" },
#endif
		{ "IncludePath", "InspectorGroupItem.h" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "ListView \xef\xbf\xbd\xc4\xa1\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd0\xa1\xef\xbf\xbdItem\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n- RootActor / RootComponents\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n- Component\xef\xbf\xbd\xef\xbf\xbd\xc4\xb3\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Kind_MetaData[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DisplayName_MetaData[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_StableKey_MetaData[] = {
		{ "Category", "RuntimeInspector" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xda\xb1\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbdStable\xef\xbf\xbd\xef\xbf\xbd */" },
#endif
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xda\xb1\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbdStable\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bExpanded_MetaData[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetObject_MetaData[] = {
		{ "Category", "RuntimeInspector" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Component \xef\xbf\xbd\xef\xbf\xbd\xc5\xbb\xef\xbf\xbd\xef\xbf\xbd\xd0\xa3\xef\xbf\xbd\xd6\xb8\xef\xbf\xbd\xef\xbf\xbd UActorComponent\xef\xbf\xbd\xef\xbf\xbd */" },
#endif
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Component \xef\xbf\xbd\xef\xbf\xbd\xc5\xbb\xef\xbf\xbd\xef\xbf\xbd\xd0\xa3\xef\xbf\xbd\xd6\xb8\xef\xbf\xbd\xef\xbf\xbd UActorComponent\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnExpandedChanged_MetaData[] = {
		{ "Category", "RuntimeInspector|Group" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaterialSlotIndex_MetaData[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorGroupItem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_Kind_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_Kind;
	static const UECodeGen_Private::FStrPropertyParams NewProp_DisplayName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_StableKey;
	static void NewProp_bExpanded_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bExpanded;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TargetObject;
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnExpandedChanged;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MaterialSlotIndex;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UInspectorGroupItem_GetIsExpanded, "GetIsExpanded" }, // 3864826889
		{ &Z_Construct_UFunction_UInspectorGroupItem_IsComponentGroup, "IsComponentGroup" }, // 3992132258
		{ &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlot, "IsMaterialSlot" }, // 2543843648
		{ &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialSlotNode, "IsMaterialSlotNode" }, // 2887919220
		{ &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsNode, "IsMaterialsNode" }, // 979134555
		{ &Z_Construct_UFunction_UInspectorGroupItem_IsMaterialsRoot, "IsMaterialsRoot" }, // 81098117
		{ &Z_Construct_UFunction_UInspectorGroupItem_SetIsExpanded, "SetIsExpanded" }, // 2155240824
		{ &Z_Construct_UFunction_UInspectorGroupItem_ToggleExpanded, "ToggleExpanded" }, // 1466975870
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UInspectorGroupItem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_Kind_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_Kind = { "Kind", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, Kind), Z_Construct_UEnum_RuntimeInspector_EInspectorGroupKind, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Kind_MetaData), NewProp_Kind_MetaData) }; // 4214915923
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_DisplayName = { "DisplayName", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, DisplayName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DisplayName_MetaData), NewProp_DisplayName_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_StableKey = { "StableKey", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, StableKey), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_StableKey_MetaData), NewProp_StableKey_MetaData) };
void Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_bExpanded_SetBit(void* Obj)
{
	((UInspectorGroupItem*)Obj)->bExpanded = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_bExpanded = { "bExpanded", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UInspectorGroupItem), &Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_bExpanded_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bExpanded_MetaData), NewProp_bExpanded_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_TargetObject = { "TargetObject", nullptr, (EPropertyFlags)0x0114000000000014, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, TargetObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetObject_MetaData), NewProp_TargetObject_MetaData) };
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_OnExpandedChanged = { "OnExpandedChanged", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, OnExpandedChanged), Z_Construct_UDelegateFunction_RuntimeInspector_OnInspectorGroupExpandedChanged__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnExpandedChanged_MetaData), NewProp_OnExpandedChanged_MetaData) }; // 1816558960
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_MaterialSlotIndex = { "MaterialSlotIndex", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorGroupItem, MaterialSlotIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaterialSlotIndex_MetaData), NewProp_MaterialSlotIndex_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UInspectorGroupItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_Kind_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_Kind,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_DisplayName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_StableKey,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_bExpanded,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_TargetObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_OnExpandedChanged,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorGroupItem_Statics::NewProp_MaterialSlotIndex,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorGroupItem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UInspectorGroupItem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorGroupItem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UInspectorGroupItem_Statics::ClassParams = {
	&UInspectorGroupItem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UInspectorGroupItem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorGroupItem_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorGroupItem_Statics::Class_MetaDataParams), Z_Construct_UClass_UInspectorGroupItem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UInspectorGroupItem()
{
	if (!Z_Registration_Info_UClass_UInspectorGroupItem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UInspectorGroupItem.OuterSingleton, Z_Construct_UClass_UInspectorGroupItem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UInspectorGroupItem.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UClass* StaticClass<UInspectorGroupItem>()
{
	return UInspectorGroupItem::StaticClass();
}
UInspectorGroupItem::UInspectorGroupItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UInspectorGroupItem);
UInspectorGroupItem::~UInspectorGroupItem() {}
// End Class UInspectorGroupItem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EInspectorGroupKind_StaticEnum, TEXT("EInspectorGroupKind"), &Z_Registration_Info_UEnum_EInspectorGroupKind, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 4214915923U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UInspectorGroupItem, UInspectorGroupItem::StaticClass, TEXT("UInspectorGroupItem"), &Z_Registration_Info_UClass_UInspectorGroupItem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UInspectorGroupItem), 3571007620U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_1898417889(TEXT("/Script/RuntimeInspector"),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_Statics::ClassInfo),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
