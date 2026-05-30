#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "InspectorColorPickerWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRIColorPickerColorDelegate, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRIColorPickerSimpleDelegate);

UCLASS()
class RUNTIMEINSPECTOR_API UInspectorColorPickerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInspectorColorPickerWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintAssignable, Category = "RuntimeInspector|ColorPicker")
    FRIColorPickerColorDelegate OnPreviewColorChanged;

    UPROPERTY(BlueprintAssignable, Category = "RuntimeInspector|ColorPicker")
    FRIColorPickerColorDelegate OnColorAccepted;

    UPROPERTY(BlueprintAssignable, Category = "RuntimeInspector|ColorPicker")
    FRIColorPickerSimpleDelegate OnColorCanceled;

    void InitializePicker(const FLinearColor& InInitialColor, const TArray<FLinearColor>& InRecentColors);
    void SetPickerColor(const FLinearColor& InColor, bool bBroadcastPreview);
    FLinearColor GetPickerColor() const { return CurrentColor; }
    void SetRecentColors(const TArray<FLinearColor>& InRecentColors);
    void RefreshPickerForAutomation();

    UEditableTextBox* GetInputR() const { return InputR.Get(); }
    UEditableTextBox* GetInputG() const { return InputG.Get(); }
    UEditableTextBox* GetInputB() const { return InputB.Get(); }
    UEditableTextBox* GetInputA() const { return InputA.Get(); }
    UEditableTextBox* GetInputHex() const { return InputHex.Get(); }
    UButton* GetApplyButton() const { return ApplyButton.Get(); }
    UButton* GetCancelButton() const { return CancelButton.Get(); }

    bool IsNativeColorPickerReadyForAutomation() const;
    bool HasNativeColorPickerContractForAutomation() const;
    bool SetRgbChannelTextForAutomation(int32 ChannelIndex, const FString& InText);
    bool SetHexTextForAutomation(const FString& InText);
    bool ApplySaturationValueForAutomation(float InSaturation, float InValue);
    bool ApplyHueForAutomation(float InHue);
    bool ApplyOpacityForAutomation(float InAlpha);
    bool HasHitTestSafeDragLayersForAutomation() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    enum class ERIColorPickerDragRegion : uint8
    {
        None,
        SaturationValue,
        Hue,
        Opacity
    };

    enum class ERIColorPickerInputMode : uint8
    {
        RGB,
        HSV
    };

    void BuildWidgetTree();
    void RefreshAll(bool bRegenerateTextures);
    void RefreshTextFields();
    void RefreshTextureBrushes(bool bRegenerateTextures);
    void RefreshMarkers();
    void RefreshInputModeVisibility();
    void RefreshPreviewBlocks();
    void RefreshRecentSwatches();

    UTexture2D* BuildSaturationValueTexture();
    UTexture2D* BuildHueTexture();
    UTexture2D* BuildOpacityTexture();

    void SetColorFromHSV(float InHue, float InSaturation, float InValue, float InAlpha, bool bBroadcastPreview);
    void SetColorInternal(const FLinearColor& InColor, bool bBroadcastPreview, bool bRegenerateTextures);
    void BroadcastPreviewIfNeeded();
    bool BeginPointerDragAtScreenPosition(const FVector2D& ScreenPosition);
    bool ApplyDragAtScreenPosition(const FVector2D& ScreenPosition, ERIColorPickerDragRegion Region, bool bBroadcastPreview);
    bool IsWidgetUnderScreenPosition(const UWidget* Widget, const FVector2D& ScreenPosition) const;
    void ApplyRgbTextValues(bool bBroadcastPreview);
    void ApplyHsvTextValues(bool bBroadcastPreview);
    void ApplyHexTextValue(const FText& InText, bool bBroadcastPreview);

    static void LinearRgbToHSV(const FLinearColor& InColor, float& OutHue, float& OutSaturation, float& OutValue);
    static FLinearColor HSVToLinearRgb(float InHue, float InSaturation, float InValue, float InAlpha);
    static FString ToHexText(const FLinearColor& InColor);
    static bool ParseHexText(const FText& InText, FLinearColor& OutColor);
    static bool ParseRgbChannelText(const FText& InText, float CurrentValue, float& OutValue);
    static bool ParseHsvChannelText(const FText& InText, int32 ChannelIndex, float CurrentValue, float& OutValue);

    UTextBlock* MakeLabel(const FString& Label, int32 Size, bool bBold) const;
    UEditableTextBox* MakeInputBox(const FName& Name, const FString& InitialText) const;
    UButton* MakePickerButton(const FName& Name, const FString& Label, bool bPrimary) const;
    UWidget* MakeColorBlock(const FName& Name, float Width, float Height, UBorder*& OutBorder) const;
    UWidget* MakeChannelRow(const FString& Label, UEditableTextBox* Input, const FLinearColor& LabelColor) const;
    UButton* MakeModeButton(const FName& Name, const FString& Label, ERIColorPickerInputMode Mode) const;
    UButton* MakeRecentColorButton(int32 Index, const FLinearColor& Color) const;

    UFUNCTION()
    void HandleApplyClicked();

    UFUNCTION()
    void HandleCancelClicked();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleRgbTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleHsvTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleHexTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleRgbModeClicked();

    UFUNCTION()
    void HandleHsvModeClicked();

    UFUNCTION()
    void HandleRecentColor0Clicked();
    UFUNCTION()
    void HandleRecentColor1Clicked();
    UFUNCTION()
    void HandleRecentColor2Clicked();
    UFUNCTION()
    void HandleRecentColor3Clicked();
    UFUNCTION()
    void HandleRecentColor4Clicked();
    UFUNCTION()
    void HandleRecentColor5Clicked();
    UFUNCTION()
    void HandleRecentColor6Clicked();
    UFUNCTION()
    void HandleRecentColor7Clicked();

    void HandleRecentColorClicked(int32 Index);

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ModalBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> SaturationValueCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> SaturationValueImage = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> SaturationValueMarker = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> HueCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> HueImage = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> HueMarker = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> OpacityCanvas = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> OpacityImage = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> OpacityMarker = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> CurrentPreviewBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PreviousPreviewBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RgbModeButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> HsvModeButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RgbInputsBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> HsvInputsBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputR = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputG = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputB = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputA = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputH = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputS = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputV = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputHsvA = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> InputHex = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RecentSwatchBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ApplyButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CancelButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CloseButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> SaturationValueTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> HueTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> OpacityTexture = nullptr;

    UPROPERTY(Transient)
    TArray<FLinearColor> RecentColors;

    FLinearColor InitialColor = FLinearColor::Black;
    FLinearColor CurrentColor = FLinearColor::Black;
    FLinearColor LastBroadcastPreviewColor = FLinearColor::Black;

    float Hue = 0.0f;
    float Saturation = 0.0f;
    float Value = 0.0f;
    float Alpha = 1.0f;
    bool bUpdatingFields = false;
    bool bHasBroadcastPreview = false;
    ERIColorPickerDragRegion ActiveDragRegion = ERIColorPickerDragRegion::None;
    ERIColorPickerInputMode InputMode = ERIColorPickerInputMode::RGB;
};
