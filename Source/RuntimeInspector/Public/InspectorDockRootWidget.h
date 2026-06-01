#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorViewModelTypes.h"
#include "InspectorDockRootWidget.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UEditableTextBox;
class UHorizontalBox;
class UInspectorFilePageWidget;
class UInspectorFunctionsSectionWidget;
class UInspectorPropertiesSectionWidget;
class UInspectorSettingsPageWidget;
class UInspectorTestPageWidget;
class UInspectorWorldSubsystem;
class UOverlay;
class UScrollBox;
class USizeBox;
class UTextBlock;
class URuntimeInspectorController;
class UVerticalBox;
class UWidget;
class UWidgetSwitcher;
enum class EInspectorRefreshReason : uint8;

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorDockFunctionRunProxy : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TObjectPtr<class UInspectorDockRootWidget> Owner;

    UPROPERTY(Transient)
    FName FunctionName = NAME_None;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorDockPatchActionProxy : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TObjectPtr<class UInspectorDockRootWidget> Owner;

    UPROPERTY(Transient)
    FGuid PatchId;

    UFUNCTION()
    void HandleRevertClicked();
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorDockComponentActionProxy : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TObjectPtr<class UInspectorDockRootWidget> Owner;

    UPROPERTY(Transient)
    FString ComponentName;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorDockFavoriteActionProxy : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TObjectPtr<class UInspectorDockRootWidget> Owner;

    UPROPERTY(Transient)
    TObjectPtr<UObject> SourceItem;

    UPROPERTY(Transient)
    bool bToggleFavorite = false;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorDockRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetController(URuntimeInspectorController* InController);
    void RefreshFromController();
    void RefreshFromController(EInspectorRefreshReason Reason);
    void RefreshFromController(EInspectorRefreshReason Reason, ERIViewModelHydrationMode HydrationMode);
    void SetActiveTab(ERIInspectorTab InTab);
    ERIInspectorTab GetActiveTab() const { return CurrentViewModel.ActiveTab; }
    bool IsOnlyModifyEnabled() const { return CurrentViewModel.bOnlyModify; }
    FString GetDockLayoutDebugSummary() const;
    bool AreHostedActorSectionsDeferredRefreshPendingForAutomation() const;
    bool IsOpenHydrationPendingForAutomation() const { return bOpenHydrationPending; }
    void CancelOpenHydrationRefresh();
    bool FlushHostedActorSectionsDeferredRefreshForAutomation();
    double GetLastComponentFocusIntentMsForAutomation() const { return LastComponentFocusIntentMs; }

    UInspectorFilePageWidget* GetHostedFilePage() const { return FilePageWidget.Get(); }
    UInspectorSettingsPageWidget* GetHostedSettingsPage() const { return SettingsPageWidget.Get(); }
    UInspectorTestPageWidget* GetHostedTestPage() const { return ToolsPageWidget.Get(); }
    UInspectorPropertiesSectionWidget* GetHostedActorAttributes() const { return ActorAttributesWidget.Get(); }
    UInspectorFunctionsSectionWidget* GetHostedActorFunctions() const { return ActorFunctionsWidget.Get(); }

    void HandleFunctionRunProxyClicked(FName FunctionName);
    void HandlePatchRevertProxyClicked(FGuid PatchId);
    void HandleComponentProxyClicked(const FString& RouteToken);
    void HandleFavoriteProxyClicked(UObject* SourceItem);
    void HandleFavoriteToggleProxyClicked(UObject* SourceItem);

protected:
    virtual void NativeConstruct() override;

private:
    void BuildWidgetTreeIfNeeded();
    void BuildLeftPanel(UVerticalBox* OutPanel);
    void BuildCenterOverlay(UOverlay* OutOverlay);
    void BuildRightInspector(UVerticalBox* OutPanel);
    void BuildActorTab(UVerticalBox* OutPanel);
    void BuildChangesTab(UScrollBox* OutScrollBox);
    void BuildHostedPageTabs();
    void BuildHostedActorSections(UInspectorWorldSubsystem* InspectorSubsystem);
    void ScheduleOpenHydrationRefresh();
    void ProcessOpenHydrationRefresh(int32 Serial);

    void RefreshLayoutForViewport();
    void RefreshActorContext(const FRIInspectorViewModel& ViewModel);
    void RefreshHostedActorSections(const FRIInspectorViewModel& ViewModel);
    void RefreshHostedActorSectionsDeferred(const FRIInspectorViewModel& ViewModel);
    void RefreshAfterComponentFocus(const FString& ComponentName, bool bFocusSucceeded);
    void RefreshComponentSelectionPresentation(const FString& ComponentName);
    void RefreshChangesTab(const FRIInspectorViewModel& ViewModel);
    void RefreshTabPresentation(const FRIInspectorViewModel& ViewModel);
    void RefreshActionBar(const FRIInspectorViewModel& ViewModel);
    void RefreshViewportOverlay(const FRIInspectorViewModel& ViewModel);

    UTextBlock* MakeBoundText(const FName& Name) const;
    UButton* MakeDockButton(const FName& Name, const FString& Label, bool bPrimary = false, float WidthOverride = 0.0f, int32 FontSize = 0) const;
    UButton* MakeDockIconButton(const FName& Name, const FString& Label, const TCHAR* IconAssetName, bool bPrimary = false, float WidthOverride = 0.0f, int32 FontSize = 0, float IconSize = 11.0f) const;
    void SetTabButtonStyle(UButton* Button, bool bActive) const;

    UFUNCTION()
    void HandleActorTabClicked();
    UFUNCTION()
    void HandleChangesTabClicked();
    UFUNCTION()
    void HandleSettingsTabClicked();
    UFUNCTION()
    void HandleToolsTabClicked();
    UFUNCTION()
    void HandleOnlyModifyChanged(bool bIsChecked);
    UFUNCTION()
    void HandleRefreshClicked();
    UFUNCTION()
    void HandleResetClicked();
    UFUNCTION()
    void HandleUndoClicked();
    UFUNCTION()
    void HandleRedoClicked();
    UFUNCTION()
    void HandleApplyClicked();
    UFUNCTION()
    void HandleSearchTextChanged(const FText& InText);

    UPROPERTY(Transient)
    TObjectPtr<URuntimeInspectorController> Controller;

    UPROPERTY(Transient)
    FRIInspectorViewModel CurrentViewModel;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> RootOverlay;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> DockBox;
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> LeftPanelSizeBox;
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> RightPanelSizeBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> LeftPanelBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RightPanelBox;
    UPROPERTY(Transient)
    TObjectPtr<UWidgetSwitcher> TabSwitcher;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ActorTabPageBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ActorAttributesHostBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ActorFunctionsHostBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> SettingsHostBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ToolsHostBox;
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ActorAttributesFrameSizeBox;
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ActorFunctionsFrameSizeBox;
    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ChangesTabScrollBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ComponentListBox;
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> FavoritesFrameSizeBox;
    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> FavoritesScrollBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> FavoritesListBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> PatchListBox;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> TabButtonBox;
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ActionBarBox;
    UPROPERTY(Transient)
    TObjectPtr<UWidget> ComponentTitleWidget;
    UPROPERTY(Transient)
    TObjectPtr<UWidget> FavoritesTitleWidget;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActorNameText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActorClassText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActorPathText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StagedBannerText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeaderText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActionStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ActorTabButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> ChangesTabButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> SettingsTabButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> ToolsTabButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> ApplyButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> ResetButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> UndoButton;
    UPROPERTY(Transient)
    TObjectPtr<UButton> RedoButton;
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> OnlyModifyCheckBox;
    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> SearchTextBox;

    UPROPERTY(Transient)
    TObjectPtr<UInspectorPropertiesSectionWidget> ActorAttributesWidget;
    UPROPERTY(Transient)
    TObjectPtr<UInspectorFunctionsSectionWidget> ActorFunctionsWidget;
    UPROPERTY(Transient)
    TObjectPtr<UInspectorFilePageWidget> FilePageWidget;
    UPROPERTY(Transient)
    TObjectPtr<UInspectorSettingsPageWidget> SettingsPageWidget;
    UPROPERTY(Transient)
    TObjectPtr<UInspectorTestPageWidget> ToolsPageWidget;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> ActionProxies;

    TMap<FString, TWeakObjectPtr<UBorder>> ComponentRowSurfaces;
    TMap<FString, TWeakObjectPtr<UTextBlock>> ComponentRowTexts;

    double LastComponentFocusIntentMs = 0.0;
    double LastViewModelRefreshMs = 0.0;
    double LastHostedCreateMs = 0.0;
    double LastOpenHydrationViewModelMs = 0.0;

    bool bWidgetTreeBuilt = false;
    bool bLeftPanelCompact = false;
    bool bSuppressSearchTextChanged = false;
    bool bOpenHydrationPending = false;
    int32 OpenHydrationSerial = 0;
};
