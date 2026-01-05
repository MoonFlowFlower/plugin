// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorTypes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorTypes() {}

// Begin Cross Module References
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorValueType();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Enum EInspectorValueType
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EInspectorValueType;
static UEnum* EInspectorValueType_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EInspectorValueType.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EInspectorValueType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_EInspectorValueType, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("EInspectorValueType"));
	}
	return Z_Registration_Info_UEnum_EInspectorValueType.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorValueType>()
{
	return EInspectorValueType_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Bool.DisplayName", "Bool" },
		{ "Bool.Name", "EInspectorValueType::Bool" },
		{ "Double.DisplayName", "Double" },
		{ "Double.Name", "EInspectorValueType::Double" },
		{ "Enum.DisplayName", "Enum" },
		{ "Enum.Name", "EInspectorValueType::Enum" },
		{ "Float.DisplayName", "Float" },
		{ "Float.Name", "EInspectorValueType::Float" },
		{ "Int.DisplayName", "Int" },
		{ "Int.Name", "EInspectorValueType::Int" },
		{ "ModuleRelativePath", "Public/InspectorTypes.h" },
		{ "Name.DisplayName", "Name" },
		{ "Name.Name", "EInspectorValueType::Name" },
		{ "String.DisplayName", "String" },
		{ "String.Name", "EInspectorValueType::String" },
		{ "Unsupported.DisplayName", "Unsupported" },
		{ "Unsupported.Name", "EInspectorValueType::Unsupported" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EInspectorValueType::Unsupported", (int64)EInspectorValueType::Unsupported },
		{ "EInspectorValueType::Bool", (int64)EInspectorValueType::Bool },
		{ "EInspectorValueType::Int", (int64)EInspectorValueType::Int },
		{ "EInspectorValueType::Float", (int64)EInspectorValueType::Float },
		{ "EInspectorValueType::Double", (int64)EInspectorValueType::Double },
		{ "EInspectorValueType::String", (int64)EInspectorValueType::String },
		{ "EInspectorValueType::Name", (int64)EInspectorValueType::Name },
		{ "EInspectorValueType::Enum", (int64)EInspectorValueType::Enum },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"EInspectorValueType",
	"EInspectorValueType",
	Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorValueType()
{
	if (!Z_Registration_Info_UEnum_EInspectorValueType.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EInspectorValueType.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_EInspectorValueType_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EInspectorValueType.InnerSingleton;
}
// End Enum EInspectorValueType

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorTypes_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EInspectorValueType_StaticEnum, TEXT("EInspectorValueType"), &Z_Registration_Info_UEnum_EInspectorValueType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1864999122U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorTypes_h_4050597479(TEXT("/Script/RuntimeInspector"),
	nullptr, 0,
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorTypes_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorTypes_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
