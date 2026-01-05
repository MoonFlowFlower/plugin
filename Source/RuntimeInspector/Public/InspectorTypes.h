#pragma once

#include "CoreMinimal.h"


#include "InspectorTypes.generated.h"

UENUM(BlueprintType)
enum class EInspectorValueType : uint8
{
    Unsupported UMETA(DisplayName = "Unsupported"),
    Bool        UMETA(DisplayName = "Bool"),
    Int         UMETA(DisplayName = "Int"),
    Float       UMETA(DisplayName = "Float"),
    Double      UMETA(DisplayName = "Double"),
    String      UMETA(DisplayName = "String"),
    Name        UMETA(DisplayName = "Name"),
    Enum        UMETA(DisplayName = "Enum"),
};


