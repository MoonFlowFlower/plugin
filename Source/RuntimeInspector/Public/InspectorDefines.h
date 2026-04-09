#pragma once

#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRuntimeInspector, Log, All);

#ifndef RUNTIME_INSPECTOR_ENABLED
#if UE_BUILD_SHIPPING
#define RUNTIME_INSPECTOR_ENABLED 0
#else
#define RUNTIME_INSPECTOR_ENABLED 1
#endif
#endif
