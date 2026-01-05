// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorBPLibrary.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UObject;
#ifdef RUNTIMEINSPECTOR_InspectorBPLibrary_generated_h
#error "InspectorBPLibrary.generated.h already included, missing '#pragma once' in InspectorBPLibrary.h"
#endif
#define RUNTIMEINSPECTOR_InspectorBPLibrary_generated_h

#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execToggleInspector);


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInspectorBPLibrary(); \
	friend struct Z_Construct_UClass_UInspectorBPLibrary_Statics; \
public: \
	DECLARE_CLASS(UInspectorBPLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/RuntimeInspector"), NO_API) \
	DECLARE_SERIALIZER(UInspectorBPLibrary)


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInspectorBPLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UInspectorBPLibrary(UInspectorBPLibrary&&); \
	UInspectorBPLibrary(const UInspectorBPLibrary&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInspectorBPLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInspectorBPLibrary); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInspectorBPLibrary) \
	NO_API virtual ~UInspectorBPLibrary();


#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_7_PROLOG
#define FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_INCLASS_NO_PURE_DECLS \
	FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h_10_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> RUNTIMEINSPECTOR_API UClass* StaticClass<class UInspectorBPLibrary>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorBPLibrary_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
