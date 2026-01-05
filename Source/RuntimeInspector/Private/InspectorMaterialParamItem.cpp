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
    // 支持 "R,G,B,A" 或 "R G B A"
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

    // 读值必须无副作用：只读 Slot 上“当前材质”（可能是 MID / MIC / Material）
    UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIndex);
    if (!Mat) return TEXT("");

    // 优先从 MID 读（Undo/Redo/Apply 都在改 MID）
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

    // 非 MID：尝试从 MaterialInterface 读默认值（有些材质/实例也能读到）
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

    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(MeshComp);
    if (!PrimComp)
    {
        OutError = TEXT("Target is not PrimitiveComponent");
        return false;
    }

    // 旧值（无副作用）
    const FString OldText = GetValueText();

    // 写值：这里才允许创建/复用 MID
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

    // 记录 Undo/Redo：用“材质专用 change”，不要再伪造 PropertyName
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
            // 旧/新数值直接从文本取（旧值 OldText 里是 float）
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
