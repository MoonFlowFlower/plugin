// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RuntimeInspector/Public/InspectorWorldSubsystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeInspectorWorldSubsystem() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FLinearColor();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UMeshComponent_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UPrimitiveComponent_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UTickableWorldSubsystem();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorPropertyItem_NoRegister();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorWorldSubsystem();
RUNTIMEINSPECTOR_API UClass* Z_Construct_UClass_UInspectorWorldSubsystem_NoRegister();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason();
RUNTIMEINSPECTOR_API UEnum* Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode();
RUNTIMEINSPECTOR_API UScriptStruct* Z_Construct_UScriptStruct_FInspectorChange();
UMG_API UClass* Z_Construct_UClass_UUserWidget_NoRegister();
UPackage* Z_Construct_UPackage__Script_RuntimeInspector();
// End Cross Module References

// Begin Enum EInspectorChangeType
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EInspectorChangeType;
static UEnum* EInspectorChangeType_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EInspectorChangeType.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EInspectorChangeType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("EInspectorChangeType"));
	}
	return Z_Registration_Info_UEnum_EInspectorChangeType.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorChangeType>()
{
	return EInspectorChangeType_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "MaterialScalar.Name", "EInspectorChangeType::MaterialScalar" },
		{ "MaterialVector.Name", "EInspectorChangeType::MaterialVector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
		{ "Property.Name", "EInspectorChangeType::Property" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EInspectorChangeType::Property", (int64)EInspectorChangeType::Property },
		{ "EInspectorChangeType::MaterialScalar", (int64)EInspectorChangeType::MaterialScalar },
		{ "EInspectorChangeType::MaterialVector", (int64)EInspectorChangeType::MaterialVector },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"EInspectorChangeType",
	"EInspectorChangeType",
	Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType()
{
	if (!Z_Registration_Info_UEnum_EInspectorChangeType.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EInspectorChangeType.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EInspectorChangeType.InnerSingleton;
}
// End Enum EInspectorChangeType

// Begin Enum ERIPropertyViewMode
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ERIPropertyViewMode;
static UEnum* ERIPropertyViewMode_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ERIPropertyViewMode.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ERIPropertyViewMode.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("ERIPropertyViewMode"));
	}
	return Z_Registration_Info_UEnum_ERIPropertyViewMode.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<ERIPropertyViewMode>()
{
	return ERIPropertyViewMode_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "Full.Name", "ERIPropertyViewMode::Full" },
		{ "MaterialOnly.Comment", "// \xc4\xac\xef\xbf\xbd\xcf\xa3\xef\xbf\xbd""Actor + Components + Materials\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "MaterialOnly.Name", "ERIPropertyViewMode::MaterialOnly" },
		{ "MaterialOnly.ToolTip", "\xc4\xac\xef\xbf\xbd\xcf\xa3\xef\xbf\xbd""Actor + Components + Materials\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ERIPropertyViewMode::Full", (int64)ERIPropertyViewMode::Full },
		{ "ERIPropertyViewMode::MaterialOnly", (int64)ERIPropertyViewMode::MaterialOnly },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"ERIPropertyViewMode",
	"ERIPropertyViewMode",
	Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode()
{
	if (!Z_Registration_Info_UEnum_ERIPropertyViewMode.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ERIPropertyViewMode.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_ERIPropertyViewMode_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ERIPropertyViewMode.InnerSingleton;
}
// End Enum ERIPropertyViewMode

// Begin Enum EInspectorRefreshReason
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EInspectorRefreshReason;
static UEnum* EInspectorRefreshReason_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EInspectorRefreshReason.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EInspectorRefreshReason.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("EInspectorRefreshReason"));
	}
	return Z_Registration_Info_UEnum_EInspectorRefreshReason.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorRefreshReason>()
{
	return EInspectorRefreshReason_StaticEnum();
}
struct Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
		{ "StructureChanged.Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xd2\xaa\xd3\xb2\xcb\xa2\xef\xbf\xbd\xef\xbf\xbd Entry\n" },
		{ "StructureChanged.DisplayName", "Structure Changed" },
		{ "StructureChanged.Name", "EInspectorRefreshReason::StructureChanged" },
		{ "StructureChanged.ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xd2\xaa\xd3\xb2\xcb\xa2\xef\xbf\xbd\xef\xbf\xbd Entry" },
		{ "TargetInvalid.Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc9\xbe/\xd1\xa1\xef\xbf\xbd\xd0\xb1\xe4\xbb\xaf\n" },
		{ "TargetInvalid.DisplayName", "Target Invalid" },
		{ "TargetInvalid.Name", "EInspectorRefreshReason::TargetInvalid" },
		{ "TargetInvalid.ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc9\xbe/\xd1\xa1\xef\xbf\xbd\xd0\xb1\xe4\xbb\xaf" },
		{ "UIStateChanged.Comment", "// \xd6\xbb\xcb\xa2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xca\xbe\xd6\xb5\n" },
		{ "UIStateChanged.DisplayName", "UI State Changed" },
		{ "UIStateChanged.Name", "EInspectorRefreshReason::UIStateChanged" },
		{ "UIStateChanged.ToolTip", "\xd6\xbb\xcb\xa2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xca\xbe\xd6\xb5" },
		{ "UndoRedo.Comment", "// \xef\xbf\xbd\xdb\xb5\xef\xbf\xbd/\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "UndoRedo.DisplayName", "Undo/Redo" },
		{ "UndoRedo.Name", "EInspectorRefreshReason::UndoRedo" },
		{ "UndoRedo.ToolTip", "\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd/\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
		{ "ValuesChanged.DisplayName", "Values Changed" },
		{ "ValuesChanged.Name", "EInspectorRefreshReason::ValuesChanged" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EInspectorRefreshReason::ValuesChanged", (int64)EInspectorRefreshReason::ValuesChanged },
		{ "EInspectorRefreshReason::UIStateChanged", (int64)EInspectorRefreshReason::UIStateChanged },
		{ "EInspectorRefreshReason::UndoRedo", (int64)EInspectorRefreshReason::UndoRedo },
		{ "EInspectorRefreshReason::StructureChanged", (int64)EInspectorRefreshReason::StructureChanged },
		{ "EInspectorRefreshReason::TargetInvalid", (int64)EInspectorRefreshReason::TargetInvalid },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	"EInspectorRefreshReason",
	"EInspectorRefreshReason",
	Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::Enum_MetaDataParams), Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason()
{
	if (!Z_Registration_Info_UEnum_EInspectorRefreshReason.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EInspectorRefreshReason.InnerSingleton, Z_Construct_UEnum_RuntimeInspector_EInspectorRefreshReason_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EInspectorRefreshReason.InnerSingleton;
}
// End Enum EInspectorRefreshReason

// Begin ScriptStruct FInspectorChange
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_InspectorChange;
class UScriptStruct* FInspectorChange::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_InspectorChange.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_InspectorChange.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FInspectorChange, (UObject*)Z_Construct_UPackage__Script_RuntimeInspector(), TEXT("InspectorChange"));
	}
	return Z_Registration_Info_UScriptStruct_InspectorChange.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UScriptStruct* StaticStruct<FInspectorChange>()
{
	return FInspectorChange::StaticStruct();
}
struct Z_Construct_UScriptStruct_FInspectorChange_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Target_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// ====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd6\xb6\xce\xa3\xef\xbf\xbd\xca\xbe\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd======\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd6\xb6\xce\xa3\xef\xbf\xbd\xca\xbe\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd======" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PropertyName_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OldValueText_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewValueText_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DebugObjectName_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ChangeType_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// ====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd ======\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd ======" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetComponent_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// ====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xca\xb2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd8\xb7\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xcf\xa2 ======\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "====== \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xca\xb2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd8\xb7\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xcf\xa2 ======" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaterialIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ParamName_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OldScalar_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewScalar_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OldVector_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewVector_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_Target;
	static const UECodeGen_Private::FNamePropertyParams NewProp_PropertyName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_OldValueText;
	static const UECodeGen_Private::FStrPropertyParams NewProp_NewValueText;
	static const UECodeGen_Private::FStrPropertyParams NewProp_DebugObjectName;
	static const UECodeGen_Private::FBytePropertyParams NewProp_ChangeType_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_ChangeType;
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_TargetComponent;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MaterialIndex;
	static const UECodeGen_Private::FNamePropertyParams NewProp_ParamName;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_OldScalar;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_NewScalar;
	static const UECodeGen_Private::FStructPropertyParams NewProp_OldVector;
	static const UECodeGen_Private::FStructPropertyParams NewProp_NewVector;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FInspectorChange>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
};
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_Target = { "Target", nullptr, (EPropertyFlags)0x0014000000000000, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, Target), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Target_MetaData), NewProp_Target_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_PropertyName = { "PropertyName", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, PropertyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PropertyName_MetaData), NewProp_PropertyName_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldValueText = { "OldValueText", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, OldValueText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OldValueText_MetaData), NewProp_OldValueText_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewValueText = { "NewValueText", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, NewValueText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewValueText_MetaData), NewProp_NewValueText_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_DebugObjectName = { "DebugObjectName", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, DebugObjectName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DebugObjectName_MetaData), NewProp_DebugObjectName_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ChangeType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ChangeType = { "ChangeType", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, ChangeType), Z_Construct_UEnum_RuntimeInspector_EInspectorChangeType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ChangeType_MetaData), NewProp_ChangeType_MetaData) }; // 4024484970
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_TargetComponent = { "TargetComponent", nullptr, (EPropertyFlags)0x0014000000080008, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, TargetComponent), Z_Construct_UClass_UPrimitiveComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetComponent_MetaData), NewProp_TargetComponent_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_MaterialIndex = { "MaterialIndex", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, MaterialIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaterialIndex_MetaData), NewProp_MaterialIndex_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ParamName = { "ParamName", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, ParamName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ParamName_MetaData), NewProp_ParamName_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldScalar = { "OldScalar", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, OldScalar), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OldScalar_MetaData), NewProp_OldScalar_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewScalar = { "NewScalar", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, NewScalar), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewScalar_MetaData), NewProp_NewScalar_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldVector = { "OldVector", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, OldVector), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OldVector_MetaData), NewProp_OldVector_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewVector = { "NewVector", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FInspectorChange, NewVector), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewVector_MetaData), NewProp_NewVector_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FInspectorChange_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_Target,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_PropertyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldValueText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewValueText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_DebugObjectName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ChangeType_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ChangeType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_TargetComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_MaterialIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_ParamName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldScalar,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewScalar,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_OldVector,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FInspectorChange_Statics::NewProp_NewVector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FInspectorChange_Statics::PropPointers) < 2048);
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FInspectorChange_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
	nullptr,
	&NewStructOps,
	"InspectorChange",
	Z_Construct_UScriptStruct_FInspectorChange_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FInspectorChange_Statics::PropPointers),
	sizeof(FInspectorChange),
	alignof(FInspectorChange),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000005),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FInspectorChange_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FInspectorChange_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FInspectorChange()
{
	if (!Z_Registration_Info_UScriptStruct_InspectorChange.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_InspectorChange.InnerSingleton, Z_Construct_UScriptStruct_FInspectorChange_Statics::StructParams);
	}
	return Z_Registration_Info_UScriptStruct_InspectorChange.InnerSingleton;
}
// End ScriptStruct FInspectorChange

// Begin Class UInspectorWorldSubsystem Function CanRedo
struct Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics
{
	struct InspectorWorldSubsystem_eventCanRedo_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventCanRedo_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventCanRedo_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "CanRedo", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::InspectorWorldSubsystem_eventCanRedo_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::InspectorWorldSubsystem_eventCanRedo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execCanRedo)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->CanRedo();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function CanRedo

// Begin Class UInspectorWorldSubsystem Function CanUndo
struct Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics
{
	struct InspectorWorldSubsystem_eventCanUndo_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventCanUndo_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventCanUndo_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "CanUndo", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::InspectorWorldSubsystem_eventCanUndo_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::InspectorWorldSubsystem_eventCanUndo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execCanUndo)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->CanUndo();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function CanUndo

// Begin Class UInspectorWorldSubsystem Function Close
struct Z_Construct_UFunction_UInspectorWorldSubsystem_Close_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_Close_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "Close", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Close_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_Close_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_Close()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_Close_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execClose)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Close();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function Close

// Begin Class UInspectorWorldSubsystem Function GetGroupExpanded
struct Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics
{
	struct InspectorWorldSubsystem_eventGetGroupExpanded_Parms
	{
		FString GroupKey;
		bool bDefault;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Groups" },
		{ "CPP_Default_bDefault", "true" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GroupKey_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_GroupKey;
	static void NewProp_bDefault_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bDefault;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_GroupKey = { "GroupKey", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetGroupExpanded_Parms, GroupKey), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GroupKey_MetaData), NewProp_GroupKey_MetaData) };
void Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_bDefault_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventGetGroupExpanded_Parms*)Obj)->bDefault = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_bDefault = { "bDefault", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventGetGroupExpanded_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_bDefault_SetBit, METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventGetGroupExpanded_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventGetGroupExpanded_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_GroupKey,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_bDefault,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "GetGroupExpanded", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::InspectorWorldSubsystem_eventGetGroupExpanded_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::InspectorWorldSubsystem_eventGetGroupExpanded_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execGetGroupExpanded)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_GroupKey);
	P_GET_UBOOL(Z_Param_bDefault);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->GetGroupExpanded(Z_Param_GroupKey,Z_Param_bDefault);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function GetGroupExpanded

// Begin Class UInspectorWorldSubsystem Function GetGroupItemsForSelected
struct Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics
{
	struct InspectorWorldSubsystem_eventGetGroupItemsForSelected_Parms
	{
		FString SearchText;
		TArray<UObject*> OutGroups;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SearchText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_SearchText;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_OutGroups_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_OutGroups;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_SearchText = { "SearchText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetGroupItemsForSelected_Parms, SearchText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SearchText_MetaData), NewProp_SearchText_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_OutGroups_Inner = { "OutGroups", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_OutGroups = { "OutGroups", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetGroupItemsForSelected_Parms, OutGroups), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_SearchText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_OutGroups_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::NewProp_OutGroups,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "GetGroupItemsForSelected", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::InspectorWorldSubsystem_eventGetGroupItemsForSelected_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::InspectorWorldSubsystem_eventGetGroupItemsForSelected_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execGetGroupItemsForSelected)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_SearchText);
	P_GET_TARRAY_REF(UObject*,Z_Param_Out_OutGroups);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetGroupItemsForSelected(Z_Param_SearchText,Z_Param_Out_OutGroups);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function GetGroupItemsForSelected

// Begin Class UInspectorWorldSubsystem Function GetPinnedItemsForSelected
struct Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics
{
	struct InspectorWorldSubsystem_eventGetPinnedItemsForSelected_Parms
	{
		FString SearchText;
		TArray<UObject*> OutPinnedItems;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SearchText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_SearchText;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_OutPinnedItems_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_OutPinnedItems;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_SearchText = { "SearchText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetPinnedItemsForSelected_Parms, SearchText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SearchText_MetaData), NewProp_SearchText_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_OutPinnedItems_Inner = { "OutPinnedItems", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_OutPinnedItems = { "OutPinnedItems", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetPinnedItemsForSelected_Parms, OutPinnedItems), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_SearchText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_OutPinnedItems_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::NewProp_OutPinnedItems,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "GetPinnedItemsForSelected", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::InspectorWorldSubsystem_eventGetPinnedItemsForSelected_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::InspectorWorldSubsystem_eventGetPinnedItemsForSelected_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execGetPinnedItemsForSelected)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_SearchText);
	P_GET_TARRAY_REF(UObject*,Z_Param_Out_OutPinnedItems);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetPinnedItemsForSelected(Z_Param_SearchText,Z_Param_Out_OutPinnedItems);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function GetPinnedItemsForSelected

// Begin Class UInspectorWorldSubsystem Function GetPropertyItemsForSelected
struct Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics
{
	struct InspectorWorldSubsystem_eventGetPropertyItemsForSelected_Parms
	{
		FString SearchText;
		TArray<UObject*> OutItems;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SearchText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_SearchText;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_OutItems_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_OutItems;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_SearchText = { "SearchText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetPropertyItemsForSelected_Parms, SearchText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SearchText_MetaData), NewProp_SearchText_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_OutItems_Inner = { "OutItems", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_OutItems = { "OutItems", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetPropertyItemsForSelected_Parms, OutItems), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_SearchText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_OutItems_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::NewProp_OutItems,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "GetPropertyItemsForSelected", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::InspectorWorldSubsystem_eventGetPropertyItemsForSelected_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::InspectorWorldSubsystem_eventGetPropertyItemsForSelected_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execGetPropertyItemsForSelected)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_SearchText);
	P_GET_TARRAY_REF(UObject*,Z_Param_Out_OutItems);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetPropertyItemsForSelected(Z_Param_SearchText,Z_Param_Out_OutItems);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function GetPropertyItemsForSelected

// Begin Class UInspectorWorldSubsystem Function GetSelectedActor
struct Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics
{
	struct InspectorWorldSubsystem_eventGetSelectedActor_Parms
	{
		AActor* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// F2: \xef\xbf\xbd\xd3\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd1\xa1\xef\xbf\xbd\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "F2: \xef\xbf\xbd\xd3\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd1\xa1\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventGetSelectedActor_Parms, ReturnValue), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "GetSelectedActor", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::InspectorWorldSubsystem_eventGetSelectedActor_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::InspectorWorldSubsystem_eventGetSelectedActor_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execGetSelectedActor)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(AActor**)Z_Param__Result=P_THIS->GetSelectedActor();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function GetSelectedActor

// Begin Class UInspectorWorldSubsystem Function HandleSelectedActorDestroyed
struct Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics
{
	struct InspectorWorldSubsystem_eventHandleSelectedActorDestroyed_Parms
	{
		AActor* DestroyedActor;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_DestroyedActor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::NewProp_DestroyedActor = { "DestroyedActor", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventHandleSelectedActorDestroyed_Parms, DestroyedActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::NewProp_DestroyedActor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "HandleSelectedActorDestroyed", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::InspectorWorldSubsystem_eventHandleSelectedActorDestroyed_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00040401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::InspectorWorldSubsystem_eventHandleSelectedActorDestroyed_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execHandleSelectedActorDestroyed)
{
	P_GET_OBJECT(AActor,Z_Param_DestroyedActor);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->HandleSelectedActorDestroyed(Z_Param_DestroyedActor);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function HandleSelectedActorDestroyed

// Begin Class UInspectorWorldSubsystem Function IsFavoriteForItem
struct Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics
{
	struct InspectorWorldSubsystem_eventIsFavoriteForItem_Parms
	{
		UInspectorPropertyItem* Item;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Favorites" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// ===== Favorites (Pin) =====\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "===== Favorites (Pin) =====" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Item;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_Item = { "Item", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventIsFavoriteForItem_Parms, Item), Z_Construct_UClass_UInspectorPropertyItem_NoRegister, METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventIsFavoriteForItem_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventIsFavoriteForItem_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_Item,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "IsFavoriteForItem", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::InspectorWorldSubsystem_eventIsFavoriteForItem_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::InspectorWorldSubsystem_eventIsFavoriteForItem_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execIsFavoriteForItem)
{
	P_GET_OBJECT(UInspectorPropertyItem,Z_Param_Item);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsFavoriteForItem(Z_Param_Item);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function IsFavoriteForItem

// Begin Class UInspectorWorldSubsystem Function IsOpen
struct Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics
{
	struct InspectorWorldSubsystem_eventIsOpen_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventIsOpen_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventIsOpen_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "IsOpen", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::InspectorWorldSubsystem_eventIsOpen_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::InspectorWorldSubsystem_eventIsOpen_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execIsOpen)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->IsOpen();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function IsOpen

// Begin Class UInspectorWorldSubsystem Function Open
struct Z_Construct_UFunction_UInspectorWorldSubsystem_Open_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_Open_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "Open", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Open_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_Open_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_Open()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_Open_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execOpen)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Open();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function Open

// Begin Class UInspectorWorldSubsystem Function PickActorInView
struct Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "PickActorInView", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execPickActorInView)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->PickActorInView();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function PickActorInView

// Begin Class UInspectorWorldSubsystem Function Redo
struct Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics
{
	struct InspectorWorldSubsystem_eventRedo_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventRedo_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventRedo_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "Redo", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::InspectorWorldSubsystem_eventRedo_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::InspectorWorldSubsystem_eventRedo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_Redo()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_Redo_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execRedo)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->Redo();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function Redo

// Begin Class UInspectorWorldSubsystem Function SetPropertyView_Full
struct Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "SetPropertyView_Full", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execSetPropertyView_Full)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetPropertyView_Full();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function SetPropertyView_Full

// Begin Class UInspectorWorldSubsystem Function SetPropertyView_MaterialOnly
struct Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics
{
	struct InspectorWorldSubsystem_eventSetPropertyView_MaterialOnly_Parms
	{
		UMeshComponent* InComp;
		int32 InSlot;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_InComp_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InComp;
	static const UECodeGen_Private::FIntPropertyParams NewProp_InSlot;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::NewProp_InComp = { "InComp", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventSetPropertyView_MaterialOnly_Parms, InComp), Z_Construct_UClass_UMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_InComp_MetaData), NewProp_InComp_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::NewProp_InSlot = { "InSlot", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventSetPropertyView_MaterialOnly_Parms, InSlot), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::NewProp_InComp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::NewProp_InSlot,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "SetPropertyView_MaterialOnly", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::InspectorWorldSubsystem_eventSetPropertyView_MaterialOnly_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::InspectorWorldSubsystem_eventSetPropertyView_MaterialOnly_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execSetPropertyView_MaterialOnly)
{
	P_GET_OBJECT(UMeshComponent,Z_Param_InComp);
	P_GET_PROPERTY(FIntProperty,Z_Param_InSlot);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetPropertyView_MaterialOnly(Z_Param_InComp,Z_Param_InSlot);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function SetPropertyView_MaterialOnly

// Begin Class UInspectorWorldSubsystem Function SetSelectedActor
struct Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics
{
	struct InspectorWorldSubsystem_eventSetSelectedActor_Parms
	{
		AActor* NewActor;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_NewActor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::NewProp_NewActor = { "NewActor", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventSetSelectedActor_Parms, NewActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::NewProp_NewActor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "SetSelectedActor", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::InspectorWorldSubsystem_eventSetSelectedActor_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::InspectorWorldSubsystem_eventSetSelectedActor_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execSetSelectedActor)
{
	P_GET_OBJECT(AActor,Z_Param_NewActor);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetSelectedActor(Z_Param_NewActor);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function SetSelectedActor

// Begin Class UInspectorWorldSubsystem Function Toggle
struct Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "Toggle", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execToggle)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Toggle();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function Toggle

// Begin Class UInspectorWorldSubsystem Function ToggleFavoriteForAnyItem
struct Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics
{
	struct InspectorWorldSubsystem_eventToggleFavoriteForAnyItem_Parms
	{
		UObject* Item;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Item;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::NewProp_Item = { "Item", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventToggleFavoriteForAnyItem_Parms, Item), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::NewProp_Item,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "ToggleFavoriteForAnyItem", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::InspectorWorldSubsystem_eventToggleFavoriteForAnyItem_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::InspectorWorldSubsystem_eventToggleFavoriteForAnyItem_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execToggleFavoriteForAnyItem)
{
	P_GET_OBJECT(UObject,Z_Param_Item);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ToggleFavoriteForAnyItem(Z_Param_Item);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function ToggleFavoriteForAnyItem

// Begin Class UInspectorWorldSubsystem Function ToggleFavoriteForItem
struct Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics
{
	struct InspectorWorldSubsystem_eventToggleFavoriteForItem_Parms
	{
		UInspectorPropertyItem* Item;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Favorites" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Item;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::NewProp_Item = { "Item", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventToggleFavoriteForItem_Parms, Item), Z_Construct_UClass_UInspectorPropertyItem_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::NewProp_Item,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "ToggleFavoriteForItem", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::InspectorWorldSubsystem_eventToggleFavoriteForItem_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::InspectorWorldSubsystem_eventToggleFavoriteForItem_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execToggleFavoriteForItem)
{
	P_GET_OBJECT(UInspectorPropertyItem,Z_Param_Item);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ToggleFavoriteForItem(Z_Param_Item);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function ToggleFavoriteForItem

// Begin Class UInspectorWorldSubsystem Function ToggleGroupExpanded
struct Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics
{
	struct InspectorWorldSubsystem_eventToggleGroupExpanded_Parms
	{
		FString GroupKey;
		bool bDefault;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector|Groups" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// >>> ADD\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc6\xa3\xef\xbf\xbd""BP \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xa3\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", ">>> ADD\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc6\xa3\xef\xbf\xbd""BP \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xa3\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GroupKey_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_GroupKey;
	static void NewProp_bDefault_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bDefault;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_GroupKey = { "GroupKey", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(InspectorWorldSubsystem_eventToggleGroupExpanded_Parms, GroupKey), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GroupKey_MetaData), NewProp_GroupKey_MetaData) };
void Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_bDefault_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventToggleGroupExpanded_Parms*)Obj)->bDefault = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_bDefault = { "bDefault", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventToggleGroupExpanded_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_bDefault_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_GroupKey,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::NewProp_bDefault,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "ToggleGroupExpanded", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::InspectorWorldSubsystem_eventToggleGroupExpanded_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::InspectorWorldSubsystem_eventToggleGroupExpanded_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execToggleGroupExpanded)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_GroupKey);
	P_GET_UBOOL(Z_Param_bDefault);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ToggleGroupExpanded(Z_Param_GroupKey,Z_Param_bDefault);
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function ToggleGroupExpanded

// Begin Class UInspectorWorldSubsystem Function Undo
struct Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics
{
	struct InspectorWorldSubsystem_eventUndo_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "RuntimeInspector" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((InspectorWorldSubsystem_eventUndo_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(InspectorWorldSubsystem_eventUndo_Parms), &Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UInspectorWorldSubsystem, nullptr, "Undo", nullptr, nullptr, Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::PropPointers), sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::InspectorWorldSubsystem_eventUndo_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::Function_MetaDataParams), Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::InspectorWorldSubsystem_eventUndo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UInspectorWorldSubsystem_Undo()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UInspectorWorldSubsystem_Undo_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UInspectorWorldSubsystem::execUndo)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->Undo();
	P_NATIVE_END;
}
// End Class UInspectorWorldSubsystem Function Undo

// Begin Class UInspectorWorldSubsystem
void UInspectorWorldSubsystem::StaticRegisterNativesUInspectorWorldSubsystem()
{
	UClass* Class = UInspectorWorldSubsystem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "CanRedo", &UInspectorWorldSubsystem::execCanRedo },
		{ "CanUndo", &UInspectorWorldSubsystem::execCanUndo },
		{ "Close", &UInspectorWorldSubsystem::execClose },
		{ "GetGroupExpanded", &UInspectorWorldSubsystem::execGetGroupExpanded },
		{ "GetGroupItemsForSelected", &UInspectorWorldSubsystem::execGetGroupItemsForSelected },
		{ "GetPinnedItemsForSelected", &UInspectorWorldSubsystem::execGetPinnedItemsForSelected },
		{ "GetPropertyItemsForSelected", &UInspectorWorldSubsystem::execGetPropertyItemsForSelected },
		{ "GetSelectedActor", &UInspectorWorldSubsystem::execGetSelectedActor },
		{ "HandleSelectedActorDestroyed", &UInspectorWorldSubsystem::execHandleSelectedActorDestroyed },
		{ "IsFavoriteForItem", &UInspectorWorldSubsystem::execIsFavoriteForItem },
		{ "IsOpen", &UInspectorWorldSubsystem::execIsOpen },
		{ "Open", &UInspectorWorldSubsystem::execOpen },
		{ "PickActorInView", &UInspectorWorldSubsystem::execPickActorInView },
		{ "Redo", &UInspectorWorldSubsystem::execRedo },
		{ "SetPropertyView_Full", &UInspectorWorldSubsystem::execSetPropertyView_Full },
		{ "SetPropertyView_MaterialOnly", &UInspectorWorldSubsystem::execSetPropertyView_MaterialOnly },
		{ "SetSelectedActor", &UInspectorWorldSubsystem::execSetSelectedActor },
		{ "Toggle", &UInspectorWorldSubsystem::execToggle },
		{ "ToggleFavoriteForAnyItem", &UInspectorWorldSubsystem::execToggleFavoriteForAnyItem },
		{ "ToggleFavoriteForItem", &UInspectorWorldSubsystem::execToggleFavoriteForItem },
		{ "ToggleGroupExpanded", &UInspectorWorldSubsystem::execToggleGroupExpanded },
		{ "Undo", &UInspectorWorldSubsystem::execUndo },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInspectorWorldSubsystem);
UClass* Z_Construct_UClass_UInspectorWorldSubsystem_NoRegister()
{
	return UInspectorWorldSubsystem::StaticClass();
}
struct Z_Construct_UClass_UInspectorWorldSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "InspectorWorldSubsystem.h" },
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GroupExpandedMap_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// <<< ADD\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "<<< ADD\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xdb\xb5\xef\xbf\xbd\xd7\xb4\xcc\xac\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PanelWidgetClass_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd4\xba\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xee\xa3\xbb\xef\xbf\xbd\xef\xbf\xbd\xd3\xb2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd2\xbb\xef\xbf\xbd\xef\xbf\xbd\xc4\xac\xef\xbf\xbd\xef\xbf\xbd\xc2\xb7\xef\xbf\xbd\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd4\xba\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xee\xa3\xbb\xef\xbf\xbd\xef\xbf\xbd\xd3\xb2\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xd2\xbb\xef\xbf\xbd\xef\xbf\xbd\xc4\xac\xef\xbf\xbd\xef\xbf\xbd\xc2\xb7\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UndoStack_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RedoStack_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ItemPool_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// --- Item \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xb3\xd8\xa3\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xbf\xef\xbf\xbd\xef\xbf\xbd GetPropertyItemsForSelected \xef\xbf\xbd\xef\xbf\xbd NewObject ---\n" },
#endif
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "--- Item \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xb3\xd8\xa3\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xc3\xbf\xef\xbf\xbd\xef\xbf\xbd GetPropertyItemsForSelected \xef\xbf\xbd\xef\xbf\xbd NewObject ---" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ViewMeshComp_MetaData[] = {
		{ "ModuleRelativePath", "Public/InspectorWorldSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBoolPropertyParams NewProp_GroupExpandedMap_ValueProp;
	static const UECodeGen_Private::FStrPropertyParams NewProp_GroupExpandedMap_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_GroupExpandedMap;
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_PanelWidgetClass;
	static const UECodeGen_Private::FStructPropertyParams NewProp_UndoStack_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_UndoStack;
	static const UECodeGen_Private::FStructPropertyParams NewProp_RedoStack_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_RedoStack;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ItemPool_ValueProp;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ItemPool_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_ItemPool;
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_ViewMeshComp;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_CanRedo, "CanRedo" }, // 312551322
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_CanUndo, "CanUndo" }, // 4250364832
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_Close, "Close" }, // 2814037334
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupExpanded, "GetGroupExpanded" }, // 4223509563
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_GetGroupItemsForSelected, "GetGroupItemsForSelected" }, // 3341510318
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_GetPinnedItemsForSelected, "GetPinnedItemsForSelected" }, // 535546982
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_GetPropertyItemsForSelected, "GetPropertyItemsForSelected" }, // 1131777483
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_GetSelectedActor, "GetSelectedActor" }, // 3123083317
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_HandleSelectedActorDestroyed, "HandleSelectedActorDestroyed" }, // 1379559361
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_IsFavoriteForItem, "IsFavoriteForItem" }, // 4065757052
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_IsOpen, "IsOpen" }, // 617645214
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_Open, "Open" }, // 423556438
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_PickActorInView, "PickActorInView" }, // 947952844
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_Redo, "Redo" }, // 1805302416
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_Full, "SetPropertyView_Full" }, // 1927615690
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_SetPropertyView_MaterialOnly, "SetPropertyView_MaterialOnly" }, // 954541864
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_SetSelectedActor, "SetSelectedActor" }, // 4157152789
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_Toggle, "Toggle" }, // 4065563745
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForAnyItem, "ToggleFavoriteForAnyItem" }, // 3115740661
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleFavoriteForItem, "ToggleFavoriteForItem" }, // 3869314381
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_ToggleGroupExpanded, "ToggleGroupExpanded" }, // 2749041999
		{ &Z_Construct_UFunction_UInspectorWorldSubsystem_Undo, "Undo" }, // 1769976381
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UInspectorWorldSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap_ValueProp = { "GroupExpandedMap", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap_Key_KeyProp = { "GroupExpandedMap_Key", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap = { "GroupExpandedMap", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, GroupExpandedMap), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GroupExpandedMap_MetaData), NewProp_GroupExpandedMap_MetaData) };
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_PanelWidgetClass = { "PanelWidgetClass", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, PanelWidgetClass), Z_Construct_UClass_UUserWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PanelWidgetClass_MetaData), NewProp_PanelWidgetClass_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_UndoStack_Inner = { "UndoStack", nullptr, (EPropertyFlags)0x0000008000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FInspectorChange, METADATA_PARAMS(0, nullptr) }; // 3591531654
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_UndoStack = { "UndoStack", nullptr, (EPropertyFlags)0x0040008000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, UndoStack), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UndoStack_MetaData), NewProp_UndoStack_MetaData) }; // 3591531654
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_RedoStack_Inner = { "RedoStack", nullptr, (EPropertyFlags)0x0000008000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FInspectorChange, METADATA_PARAMS(0, nullptr) }; // 3591531654
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_RedoStack = { "RedoStack", nullptr, (EPropertyFlags)0x0040008000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, RedoStack), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RedoStack_MetaData), NewProp_RedoStack_MetaData) }; // 3591531654
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool_ValueProp = { "ItemPool", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool_Key_KeyProp = { "ItemPool_Key", nullptr, (EPropertyFlags)0x0100000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool = { "ItemPool", nullptr, (EPropertyFlags)0x0144000000002000, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, ItemPool), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ItemPool_MetaData), NewProp_ItemPool_MetaData) };
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ViewMeshComp = { "ViewMeshComp", nullptr, (EPropertyFlags)0x0044000000082008, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UInspectorWorldSubsystem, ViewMeshComp), Z_Construct_UClass_UMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ViewMeshComp_MetaData), NewProp_ViewMeshComp_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UInspectorWorldSubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_GroupExpandedMap,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_PanelWidgetClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_UndoStack_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_UndoStack,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_RedoStack_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_RedoStack,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ItemPool,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UInspectorWorldSubsystem_Statics::NewProp_ViewMeshComp,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorWorldSubsystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UInspectorWorldSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UTickableWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_RuntimeInspector,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorWorldSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UInspectorWorldSubsystem_Statics::ClassParams = {
	&UInspectorWorldSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UInspectorWorldSubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorWorldSubsystem_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UInspectorWorldSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UInspectorWorldSubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UInspectorWorldSubsystem()
{
	if (!Z_Registration_Info_UClass_UInspectorWorldSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UInspectorWorldSubsystem.OuterSingleton, Z_Construct_UClass_UInspectorWorldSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UInspectorWorldSubsystem.OuterSingleton;
}
template<> RUNTIMEINSPECTOR_API UClass* StaticClass<UInspectorWorldSubsystem>()
{
	return UInspectorWorldSubsystem::StaticClass();
}
UInspectorWorldSubsystem::UInspectorWorldSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UInspectorWorldSubsystem);
UInspectorWorldSubsystem::~UInspectorWorldSubsystem() {}
// End Class UInspectorWorldSubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EInspectorChangeType_StaticEnum, TEXT("EInspectorChangeType"), &Z_Registration_Info_UEnum_EInspectorChangeType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 4024484970U) },
		{ ERIPropertyViewMode_StaticEnum, TEXT("ERIPropertyViewMode"), &Z_Registration_Info_UEnum_ERIPropertyViewMode, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 862743030U) },
		{ EInspectorRefreshReason_StaticEnum, TEXT("EInspectorRefreshReason"), &Z_Registration_Info_UEnum_EInspectorRefreshReason, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2612633809U) },
	};
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FInspectorChange::StaticStruct, Z_Construct_UScriptStruct_FInspectorChange_Statics::NewStructOps, TEXT("InspectorChange"), &Z_Registration_Info_UScriptStruct_InspectorChange, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FInspectorChange), 3591531654U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UInspectorWorldSubsystem, UInspectorWorldSubsystem::StaticClass, TEXT("UInspectorWorldSubsystem"), &Z_Registration_Info_UClass_UInspectorWorldSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UInspectorWorldSubsystem), 561711971U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_2493614408(TEXT("/Script/RuntimeInspector"),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::ScriptStructInfo),
	Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
