// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorPropertyItem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UObject;
enum class EInspectorValueType : uint8;
#ifdef RUNTIMEINSPECTOR_InspectorPropertyItem_generated_h
#error "InspectorPropertyItem.generated.h already included, missing '#pragma once' in InspectorPropertyItem.h"
#endif
#define RUNTIMEINSPECTOR_InspectorPropertyItem_generated_h

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetPropertyFName); \
	DECLARE_FUNCTION(execGetTargetObject); \
	DECLARE_FUNCTION(execGetEnumOptions); \
	DECLARE_FUNCTION(execIsEnum); \
	DECLARE_FUNCTION(execGetValueType); \
	DECLARE_FUNCTION(execIsEditable); \
	DECLARE_FUNCTION(execIsValidItem); \
	DECLARE_FUNCTION(execApplyFromText); \
	DECLARE_FUNCTION(execGetValueText); \
	DECLARE_FUNCTION(execGetPropertyName);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInspectorPropertyItem(); \
	friend struct Z_Construct_UClass_UInspectorPropertyItem_Statics; \
public: \
	DECLARE_CLASS(UInspectorPropertyItem, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/RuntimeInspector"), NO_API) \
	DECLARE_SERIALIZER(UInspectorPropertyItem)


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInspectorPropertyItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UInspectorPropertyItem(UInspectorPropertyItem&&); \
	UInspectorPropertyItem(const UInspectorPropertyItem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInspectorPropertyItem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInspectorPropertyItem); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInspectorPropertyItem) \
	NO_API virtual ~UInspectorPropertyItem();


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_10_PROLOG
#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_INCLASS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h_13_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> RUNTIMEINSPECTOR_API UClass* StaticClass<class UInspectorPropertyItem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorPropertyItem_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
