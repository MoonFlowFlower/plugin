// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorMaterialParamItem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UMeshComponent;
enum class EInspectorMatParamType : uint8;
#ifdef RUNTIMEINSPECTOR_InspectorMaterialParamItem_generated_h
#error "InspectorMaterialParamItem.generated.h already included, missing '#pragma once' in InspectorMaterialParamItem.h"
#endif
#define RUNTIMEINSPECTOR_InspectorMaterialParamItem_generated_h

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetParamType); \
	DECLARE_FUNCTION(execGetParamName); \
	DECLARE_FUNCTION(execGetSlotIndex); \
	DECLARE_FUNCTION(execGetMeshComponent); \
	DECLARE_FUNCTION(execIsEnum); \
	DECLARE_FUNCTION(execApplyFromText); \
	DECLARE_FUNCTION(execIsEditable); \
	DECLARE_FUNCTION(execGetValueText); \
	DECLARE_FUNCTION(execGetPropertyName);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInspectorMaterialParamItem(); \
	friend struct Z_Construct_UClass_UInspectorMaterialParamItem_Statics; \
public: \
	DECLARE_CLASS(UInspectorMaterialParamItem, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/RuntimeInspector"), NO_API) \
	DECLARE_SERIALIZER(UInspectorMaterialParamItem)


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInspectorMaterialParamItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UInspectorMaterialParamItem(UInspectorMaterialParamItem&&); \
	UInspectorMaterialParamItem(const UInspectorMaterialParamItem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInspectorMaterialParamItem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInspectorMaterialParamItem); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInspectorMaterialParamItem) \
	NO_API virtual ~UInspectorMaterialParamItem();


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_16_PROLOG
#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_INCLASS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h_19_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> RUNTIMEINSPECTOR_API UClass* StaticClass<class UInspectorMaterialParamItem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorMaterialParamItem_h


#define FOREACH_ENUM_EINSPECTORMATPARAMTYPE(op) \
	op(EInspectorMatParamType::Scalar) \
	op(EInspectorMatParamType::Vector) 

enum class EInspectorMatParamType : uint8;
template<> struct TIsUEnumClass<EInspectorMatParamType> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorMatParamType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
