// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorBPLibrary.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorBPLibrary() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UBlueprintFunctionLibrary();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorBPLibrary();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorBPLibrary_NoRegister();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Class UInspectorBPLibrary Function ToggleInspector
struct Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics
{
	struct InspectorBPLibrary_eventToggleInspector_Parms
	{
		UObject* WorldContextObject;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorBPLibrary.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WorldContextObject;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::NewProp_WorldContextObject = { "WorldContextObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorBPLibrary_eventToggleInspector_Parms, WorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::NewProp_WorldContextObject,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorBPLibrary, nullptr, "ToggleInspector", nullptr, nullptr, Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::InspectorBPLibrary_eventToggleInspector_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::InspectorBPLibrary_eventToggleInspector_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorBPLibrary::execToggleInspector)
{
	P_GET_OBJECT(UObject,Z_Param_WorldContextObject);
	P_FINISH;
	P_NATIVE_BEGIN;
	UInspectorBPLibrary::ToggleInspector(Z_Param_WorldContextObject);
	P_NATIVE_END;
}
// End Class UInspectorBPLibrary Function ToggleInspector

// Begin Class UInspectorBPLibrary
void UInspectorBPLibrary::StaticRegisterNativesUInspectorBPLibrary()
{
	UClass* Class = UInspectorBPLibrary::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ToggleInspector", &UInspectorBPLibrary::execToggleInspector },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInspectorBPLibrary);
UClass* Z_Construct_UClass_UInspectorBPLibrary_NoRegister()
{
	return UInspectorBPLibrary::StaticClass();
}
struct Z_Construct_UClass_UInspectorBPLibrary_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "InspectorBPLibrary.h" },
		{ "ModuleRelativePath", "Public/InspectorBPLibrary.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UInspectorBPLibrary_ToggleInspector, "ToggleInspector" }, // 658979506
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UInspectorBPLibrary>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UInspectorBPLibrary_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UBlueprintFunctionLibrary,
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorBPLibrary_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UInspectorBPLibrary_Statics::ClassParams = {
	&UInspectorBPLibrary::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorBPLibrary_Statics::Class_MetaDataParams), Z_Construct_UClass_UInspectorBPLibrary_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UInspectorBPLibrary()
{
	if (!Z_Registration_Info_UClass_UInspectorBPLibrary.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UInspectorBPLibrary.OuterSingleton, Z_Construct_UClass_UInspectorBPLibrary_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UInspectorBPLibrary.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UClass* StaticClass<UInspectorBPLibrary>()
{
	return UInspectorBPLibrary::StaticClass();
}
UInspectorBPLibrary::UInspectorBPLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UInspectorBPLibrary);
UInspectorBPLibrary::~UInspectorBPLibrary() {}
// End Class UInspectorBPLibrary

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UInspectorBPLibrary, UInspectorBPLibrary::StaticClass, TEXT("UInspectorBPLibrary"), &Z_Registration_Info_UClass_UInspectorBPLibrary, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UInspectorBPLibrary), 1759917794U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_4204772811(TEXT("/Script/RuntimeInspector"),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
