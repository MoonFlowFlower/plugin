#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorFunctionsSectionWidget.generated.h"

class UBorder;
class UButton;
class UInspectorFunctionItem;
class UInspectorFunctionRowWidget;
class UInspectorWorldSubsystem;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorFunctionsSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorFunctionsSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void RefreshFromSubsystem();

    bool HasFunctionsSection() const { return FunctionsSectionBorder != nullptr; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    UWidget* CreateSectionTitle(const FString& InTitle);
    UWidget* CreateFunctionRow(UInspectorFunctionItem* Item);

private:
    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;

    UPROPERTY(Transient)
    UBorder* FunctionsSectionBorder = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* RootBox = nullptr;

    UPROPERTY(Transient)
    UTextBlock* FocusSummaryText = nullptr;

    UPROPERTY(Transient)
    UVerticalBox* FunctionsEntriesBox = nullptr;

    UPROPERTY(Transient)
    UScrollBox* FunctionsScrollBox = nullptr;
};
