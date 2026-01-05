// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorGroupItem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef RUNTIMEINSPECTOR_InspectorGroupItem_generated_h
#error "InspectorGroupItem.generated.h already included, missing '#pragma once' in InspectorGroupItem.h"
#endif
#define RUNTIMEINSPECTOR_InspectorGroupItem_generated_h

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_7_DELEGATE \
RUNTIMEINSPECTOR_API void FOnInspectorGroupExpandedChanged_DelegateWrapper(const FMulticastScriptDelegate& OnInspectorGroupExpandedChanged, bool bIsExpanded);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execIsMaterialSlotNode); \
	DECLARE_FUNCTION(execIsMaterialsNode); \
	DECLARE_FUNCTION(execIsComponentGroup); \
	DECLARE_FUNCTION(execIsMaterialSlot); \
	DECLARE_FUNCTION(execIsMaterialsRoot); \
	DECLARE_FUNCTION(execToggleExpanded); \
	DECLARE_FUNCTION(execSetIsExpanded); \
	DECLARE_FUNCTION(execGetIsExpanded);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInspectorGroupItem(); \
	friend struct Z_Construct_UClass_UInspectorGroupItem_Statics; \
public: \
	DECLARE_CLASS(UInspectorGroupItem, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/RuntimeInspector"), NO_API) \
	DECLARE_SERIALIZER(UInspectorGroupItem)


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInspectorGroupItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UInspectorGroupItem(UInspectorGroupItem&&); \
	UInspectorGroupItem(const UInspectorGroupItem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInspectorGroupItem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInspectorGroupItem); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInspectorGroupItem) \
	NO_API virtual ~UInspectorGroupItem();


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_24_PROLOG
#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_INCLASS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h_27_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> RUNTIMEINSPECTOR_API UClass* StaticClass<class UInspectorGroupItem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorGroupItem_h


#define FOREACH_ENUM_EINSPECTORGROUPKIND(op) \
	op(EInspectorGroupKind::RootActor) \
	op(EInspectorGroupKind::RootComponents) \
	op(EInspectorGroupKind::Component) \
	op(EInspectorGroupKind::MaterialsRoot) \
	op(EInspectorGroupKind::MaterialSlot) 

enum class EInspectorGroupKind : uint8;
template<> struct TIsUEnumClass<EInspectorGroupKind> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorGroupKind>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
