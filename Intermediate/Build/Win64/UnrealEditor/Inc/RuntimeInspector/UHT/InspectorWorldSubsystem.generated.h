// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorWorldSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class AActor;
class UInspectorPropertyItem;
class UMeshComponent;
class UObject;
#ifdef RUNTIMEINSPECTOR_InspectorWorldSubsystem_generated_h
#error "InspectorWorldSubsystem.generated.h already included, missing '#pragma once' in InspectorWorldSubsystem.h"
#endif
#define RUNTIMEINSPECTOR_InspectorWorldSubsystem_generated_h

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_65_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FInspectorChange_Statics; \
	RUNTIMEINSPECTOR_API static class UScriptStruct* StaticStruct();


template<> RUNTIMEINSPECTOR_API UScriptStruct* StaticStruct<struct FInspectorChange>();

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execToggleFavoriteForAnyItem); \
	DECLARE_FUNCTION(execSetPropertyView_Full); \
	DECLARE_FUNCTION(execSetPropertyView_MaterialOnly); \
	DECLARE_FUNCTION(execHandleSelectedActorDestroyed); \
	DECLARE_FUNCTION(execToggleFavoriteForItem); \
	DECLARE_FUNCTION(execIsFavoriteForItem); \
	DECLARE_FUNCTION(execGetGroupExpanded); \
	DECLARE_FUNCTION(execToggleGroupExpanded); \
	DECLARE_FUNCTION(execRedo); \
	DECLARE_FUNCTION(execUndo); \
	DECLARE_FUNCTION(execCanRedo); \
	DECLARE_FUNCTION(execCanUndo); \
	DECLARE_FUNCTION(execGetPinnedItemsForSelected); \
	DECLARE_FUNCTION(execGetGroupItemsForSelected); \
	DECLARE_FUNCTION(execGetPropertyItemsForSelected); \
	DECLARE_FUNCTION(execSetSelectedActor); \
	DECLARE_FUNCTION(execGetSelectedActor); \
	DECLARE_FUNCTION(execPickActorInView); \
	DECLARE_FUNCTION(execIsOpen); \
	DECLARE_FUNCTION(execClose); \
	DECLARE_FUNCTION(execOpen); \
	DECLARE_FUNCTION(execToggle);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInspectorWorldSubsystem(); \
	friend struct Z_Construct_UClass_UInspectorWorldSubsystem_Statics; \
public: \
	DECLARE_CLASS(UInspectorWorldSubsystem, UTickableWorldSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/RuntimeInspector"), NO_API) \
	DECLARE_SERIALIZER(UInspectorWorldSubsystem)


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInspectorWorldSubsystem(); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UInspectorWorldSubsystem(UInspectorWorldSubsystem&&); \
	UInspectorWorldSubsystem(const UInspectorWorldSubsystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInspectorWorldSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInspectorWorldSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UInspectorWorldSubsystem) \
	NO_API virtual ~UInspectorWorldSubsystem();


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_91_PROLOG
#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_INCLASS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h_94_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> RUNTIMEINSPECTOR_API UClass* StaticClass<class UInspectorWorldSubsystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorWorldSubsystem_h


#define FOREACH_ENUM_EINSPECTORCHANGETYPE(op) \
	op(EInspectorChangeType::Property) \
	op(EInspectorChangeType::MaterialScalar) \
	op(EInspectorChangeType::MaterialVector) 

enum class EInspectorChangeType : uint8;
template<> struct TIsUEnumClass<EInspectorChangeType> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorChangeType>();

#define FOREACH_ENUM_ERIPROPERTYVIEWMODE(op) \
	op(ERIPropertyViewMode::Full) \
	op(ERIPropertyViewMode::MaterialOnly) 

enum class ERIPropertyViewMode : uint8;
template<> struct TIsUEnumClass<ERIPropertyViewMode> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<ERIPropertyViewMode>();

#define FOREACH_ENUM_EINSPECTORREFRESHREASON(op) \
	op(EInspectorRefreshReason::ValuesChanged) \
	op(EInspectorRefreshReason::UIStateChanged) \
	op(EInspectorRefreshReason::UndoRedo) \
	op(EInspectorRefreshReason::StructureChanged) \
	op(EInspectorRefreshReason::TargetInvalid) 

enum class EInspectorRefreshReason : uint8;
template<> struct TIsUEnumClass<EInspectorRefreshReason> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorRefreshReason>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
