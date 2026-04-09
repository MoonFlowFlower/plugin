#include "InspectorNetworkCompareTypes.h"

FString RI_RuntimeCompareRoleLabel(ERIRuntimeCompareRole InRole)
{
    switch (InRole)
    {
    case ERIRuntimeCompareRole::Authority:
        return TEXT("Authority");
    case ERIRuntimeCompareRole::AutonomousProxy:
        return TEXT("AutonomousProxy");
    case ERIRuntimeCompareRole::SimulatedProxy:
        return TEXT("SimulatedProxy");
    default:
        return TEXT("Unknown");
    }
}

const TArray<ERIRuntimeCompareRole>& RI_GetRuntimeCompareRoles()
{
    static const TArray<ERIRuntimeCompareRole> Roles = {
        ERIRuntimeCompareRole::Authority,
        ERIRuntimeCompareRole::AutonomousProxy,
        ERIRuntimeCompareRole::SimulatedProxy
    };

    return Roles;
}
