#include "InspectorMaterialParamItem.h"
#include "InspectorWorldSubsystem.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"


static bool TryParseFloat(const FString& S, float& Out)
{
    return LexTryParseString(Out, *S.TrimStartAndEnd());
}

static bool TryParseLinearColor(const FString& S, FLinearColor& Out)
{
    // Supports both "R,G,B,A" and "R G B A".
    TArray<FString> Parts;
    S.Replace(TEXT(" "), TEXT(",")).ParseIntoArray(Parts, TEXT(","), true);
    if (Parts.Num() < 3) return false;

    float R = 0, G = 0, B = 0, A = 1;
    if (!TryParseFloat(Parts[0], R)) return false;
    if (!TryParseFloat(Parts[1], G)) return false;
    if (!TryParseFloat(Parts[2], B)) return false;
    if (Parts.Num() >= 4) { if (!TryParseFloat(Parts[3], A)) return false; }

    Out = FLinearColor(R, G, B, A);
    return true;
}

void UInspectorMaterialParamItem::Init(UMeshComponent* InComp, int32 InSlotIndex, FName InParamName, EInspectorMatParamType InType)
{
    TargetComp = InComp;
    SlotIndex = InSlotIndex;
    ParamName = InParamName;
    ParamType = InType;
}

FString UInspectorMaterialParamItem::GetPropertyName() const
{
    const TCHAR* Prefix = (ParamType == EInspectorMatParamType::Scalar) ? TEXT("Scalar") : TEXT("Vector");
    return FString::Printf(TEXT("%s: %s"), Prefix, *ParamName.ToString());
}

FString UInspectorMaterialParamItem::GetValueText()
{
    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp) return TEXT("");

    // Read the current material value without creating or mutating anything.
    UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIndex);
    if (!Mat) return TEXT("");

    // Prefer MID reads because Undo/Redo/Apply all mutate the MID.
    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat))
    {
        if (ParamType == EInspectorMatParamType::Scalar)
        {
            const float V = MID->K2_GetScalarParameterValue(ParamName);
            return FString::SanitizeFloat(V);
        }
        else
        {
            const FLinearColor C = MID->K2_GetVectorParameterValue(ParamName);
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
    }

    // Non-MID fallback: read the default value from the material interface.
    const FMaterialParameterInfo Info(ParamName);

    if (ParamType == EInspectorMatParamType::Scalar)
    {
        float V = 0.f;
        if (Mat->GetScalarParameterValue(Info, V))
        {
            return FString::SanitizeFloat(V);
        }
        return TEXT("");
    }
    else
    {
        FLinearColor C;
        if (Mat->GetVectorParameterValue(Info, C))
        {
            return FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f"), C.R, C.G, C.B, C.A);
        }
        return TEXT("");
    }
}

bool UInspectorMaterialParamItem::ApplyFromText(const FString& NewText, FString& OutError)
{
    OutError.Reset();

    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(GetOuter());
    if (!Sub)
    {
        OutError = TEXT("Subsystem invalid (Outer is not UInspectorWorldSubsystem)");
        return false;
    }

    if (!Sub->IsRIEnabled())
    {
        OutError = Sub->GetRIDisabledReason();
        return false;
    }

    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(MeshComp);
    if (!PrimComp)
    {
        OutError = TEXT("Target is not PrimitiveComponent");
        return false;
    }

    // Capture the old value before mutating anything.
    const FString OldText = GetValueText();

    // Writes are the only place where creating or reusing a MID is allowed.
    UMaterialInstanceDynamic* MID = Sub->GetOrCreateMID(PrimComp, SlotIndex);
    if (!MID)
    {
        OutError = TEXT("Failed to get/create MID");
        return false;
    }

    if (ParamType == EInspectorMatParamType::Scalar)
    {
        float NewV = 0.f;
        if (!TryParseFloat(NewText, NewV))
        {
            OutError = TEXT("Invalid float");
            return false;
        }
        MID->SetScalarParameterValue(ParamName, NewV);
    }
    else
    {
        FLinearColor NewC;
        if (!TryParseLinearColor(NewText, NewC))
        {
            OutError = TEXT("Invalid color. Use R,G,B or R,G,B,A");
            return false;
        }
        MID->SetVectorParameterValue(ParamName, NewC);
    }

    PrimComp->MarkRenderStateDirty();

    // Record a dedicated material change for Undo/Redo instead of faking a property name.
    if (!Sub->IsApplyingHistory())
    {
        FInspectorChange Change;
        Change.DebugObjectName = GetNameSafe(PrimComp);

        Change.TargetComponent = PrimComp;
        Change.MaterialIndex = SlotIndex;
        Change.ParamName = ParamName;

        if (ParamType == EInspectorMatParamType::Scalar)
        {
            Change.ChangeType = EInspectorChangeType::MaterialScalar;
            // Old/new scalar values can be read directly from the serialized text.
            TryParseFloat(OldText, Change.OldScalar);
            Change.NewScalar = MID->K2_GetScalarParameterValue(ParamName);
        }
        else
        {
            Change.ChangeType = EInspectorChangeType::MaterialVector;
            TryParseLinearColor(OldText, Change.OldVector);
            Change.NewVector = MID->K2_GetVectorParameterValue(ParamName);
        }

        Sub->RecordChange(Change);
    }

    return true;
}


EInspectorValueType UInspectorMaterialParamItem::GetValueType() const
{
    // Material params are currently only Scalar(float) and Vector(FLinearColor)
    return (ParamType == EInspectorMatParamType::Scalar) ? EInspectorValueType::Float : EInspectorValueType::LinearColor;
}

bool UInspectorMaterialParamItem::GetScalar(float& OutValue, FString& OutError)
{
    OutError.Reset();
    OutValue = 0.f;

    if (ParamType != EInspectorMatParamType::Scalar)
    {
        OutError = TEXT("ParamType is not Scalar");
        return false;
    }

    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIndex);
    if (!Mat)
    {
        OutError = TEXT("Material missing");
        return false;
    }

    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat))
    {
        OutValue = MID->K2_GetScalarParameterValue(ParamName);
        return true;
    }

    const FMaterialParameterInfo Info(ParamName);
    float V = 0.f;
    if (Mat->GetScalarParameterValue(Info, V))
    {
        OutValue = V;
        return true;
    }

    OutError = TEXT("Scalar parameter not found");
    return false;
}

bool UInspectorMaterialParamItem::GetVector(FLinearColor& OutValue, FString& OutError)
{
    OutError.Reset();
    OutValue = FLinearColor::Black;

    if (ParamType != EInspectorMatParamType::Vector)
    {
        OutError = TEXT("ParamType is not Vector");
        return false;
    }

    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIndex);
    if (!Mat)
    {
        OutError = TEXT("Material missing");
        return false;
    }

    if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat))
    {
        OutValue = MID->K2_GetVectorParameterValue(ParamName);
        return true;
    }

    const FMaterialParameterInfo Info(ParamName);
    FLinearColor C;
    if (Mat->GetVectorParameterValue(Info, C))
    {
        OutValue = C;
        return true;
    }

    OutError = TEXT("Vector parameter not found");
    return false;
}

bool UInspectorMaterialParamItem::SetScalar(float NewValue, FString& OutError)
{
    OutError.Reset();

    if (ParamType != EInspectorMatParamType::Scalar)
    {
        OutError = TEXT("ParamType is not Scalar");
        return false;
    }

    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(GetOuter());
    if (!Sub)
    {
        OutError = TEXT("Subsystem invalid (Outer is not UInspectorWorldSubsystem)");
        return false;
    }

    if (!Sub->IsRIEnabled())
    {
        OutError = Sub->GetRIDisabledReason();
        return false;
    }

    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(MeshComp);
    if (!PrimComp)
    {
        OutError = TEXT("Target is not PrimitiveComponent");
        return false;
    }

    // Old value (for history)
    float OldV = 0.f;
    {
        FString Tmp;
        GetScalar(OldV, Tmp); // best-effort
    }

    UMaterialInstanceDynamic* MID = Sub->GetOrCreateMID(PrimComp, SlotIndex);
    if (!MID)
    {
        OutError = TEXT("Failed to get/create MID");
        return false;
    }

    MID->SetScalarParameterValue(ParamName, NewValue);
    PrimComp->MarkRenderStateDirty();

    if (!Sub->IsApplyingHistory())
    {
        FInspectorChange Change;
        Change.DebugObjectName = GetNameSafe(PrimComp);
        Change.TargetComponent = PrimComp;
        Change.MaterialIndex = SlotIndex;
        Change.ParamName = ParamName;
        Change.ChangeType = EInspectorChangeType::MaterialScalar;
        Change.OldScalar = OldV;
        Change.NewScalar = MID->K2_GetScalarParameterValue(ParamName);
        Sub->RecordChange(Change);
    }

    return true;
}

bool UInspectorMaterialParamItem::SetVector(const FLinearColor& NewValue, FString& OutError)
{
    OutError.Reset();

    if (ParamType != EInspectorMatParamType::Vector)
    {
        OutError = TEXT("ParamType is not Vector");
        return false;
    }

    UMeshComponent* MeshComp = TargetComp.Get();
    if (!MeshComp)
    {
        OutError = TEXT("Target invalid");
        return false;
    }

    UInspectorWorldSubsystem* Sub = Cast<UInspectorWorldSubsystem>(GetOuter());
    if (!Sub)
    {
        OutError = TEXT("Subsystem invalid (Outer is not UInspectorWorldSubsystem)");
        return false;
    }

    if (!Sub->IsRIEnabled())
    {
        OutError = Sub->GetRIDisabledReason();
        return false;
    }

    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(MeshComp);
    if (!PrimComp)
    {
        OutError = TEXT("Target is not PrimitiveComponent");
        return false;
    }

    // Old value (for history)
    FLinearColor OldC = FLinearColor::Black;
    {
        FString Tmp;
        GetVector(OldC, Tmp); // best-effort
    }

    UMaterialInstanceDynamic* MID = Sub->GetOrCreateMID(PrimComp, SlotIndex);
    if (!MID)
    {
        OutError = TEXT("Failed to get/create MID");
        return false;
    }

    MID->SetVectorParameterValue(ParamName, NewValue);
    PrimComp->MarkRenderStateDirty();

    if (!Sub->IsApplyingHistory())
    {
        FInspectorChange Change;
        Change.DebugObjectName = GetNameSafe(PrimComp);
        Change.TargetComponent = PrimComp;
        Change.MaterialIndex = SlotIndex;
        Change.ParamName = ParamName;
        Change.ChangeType = EInspectorChangeType::MaterialVector;
        Change.OldVector = OldC;
        Change.NewVector = MID->K2_GetVectorParameterValue(ParamName);
        Sub->RecordChange(Change);
    }

    return true;
}
