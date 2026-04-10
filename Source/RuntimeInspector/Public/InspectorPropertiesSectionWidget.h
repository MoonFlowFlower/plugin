#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPropertiesSectionWidget.generated.h"

class UInspectorPropertyRowWidget;
class UInspectorWorldSubsystem;
class UBorder;
class UScrollBox;
class UVerticalBox;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorPropertiesSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorPropertiesSectionWidget(const FObjectInitializer& ObjectInitializer);

    void SetInspectorSubsystem(UInspectorWorldSubsystem* InSubsystem);
    void SetOnlyModified(bool bInOnlyModified);
    void RefreshFromSubsystem();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTree();
    UWidget* CreatePropertyRow(UObject* ItemObject);

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ScrollBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> EntriesBox = nullptr;

    TWeakObjectPtr<UInspectorWorldSubsystem> Subsystem;
    bool bOnlyModified = false;
};
