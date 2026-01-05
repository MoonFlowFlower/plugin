// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "InspectorTypes.h"
#include "Templates/IsUEnumClass.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef RUNTIMEINSPECTOR_InspectorTypes_generated_h
#error "InspectorTypes.generated.h already included, missing '#pragma once' in InspectorTypes.h"
#endif
#define RUNTIMEINSPECTOR_InspectorTypes_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Project_Game_UE_PluginMaker_Plugins_RuntimeInspector_Source_RuntimeInspector_Public_InspectorTypes_h


#define FOREACH_ENUM_EINSPECTORVALUETYPE(op) \
	op(EInspectorValueType::Unsupported) \
	op(EInspectorValueType::Bool) \
	op(EInspectorValueType::Int) \
	op(EInspectorValueType::Float) \
	op(EInspectorValueType::Double) \
	op(EInspectorValueType::String) \
	op(EInspectorValueType::Name) \
	op(EInspectorValueType::Enum) 

enum class EInspectorValueType : uint8;
template<> struct TIsUEnumClass<EInspectorValueType> { enum { Value = true }; };
template<> RUNTIMEINSPECTOR_API UEnum* StaticEnum<EInspectorValueType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
