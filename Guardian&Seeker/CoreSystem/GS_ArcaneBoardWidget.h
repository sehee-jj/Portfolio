// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RuneSystem/GS_ArcaneBoardTypes.h"
#include "GS_ArcaneBoardWidget.generated.h"

class UUniformGridPanel;
class UButton;
class UGS_RuneInventoryWidget;
class UGS_StatPanelWidget;
class UGS_RuneGridCellWidget;
class UGS_DragVisualWidget;
class UGS_RuneTooltipWidget;
class UGS_ArcaneBoardManager;
class UGS_ArcaneBoardLPS;
class UGS_CommonTwoBtnPopup;

/**
 * 아케인 보드 메인 위젯
 */
UCLASS()
class GAS_API UGS_ArcaneBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGS_ArcaneBoardWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void RefreshForCurrCharacter();

	UFUNCTION()
	void OnStatsChanged(const FArcaneBoardStats& NewStats);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void StartRuneSelection(uint8 RuneID);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void EndRuneSelection(bool bPlaceRune = false);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void RequestShowTooltip(uint8 RuneID, const FVector2D& MousePos);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void HideTooltip();

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	uint8 GetSelectedRuneID() const;

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	bool HasUnsavedChanges() const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UUniformGridPanel* GridPanel;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UGS_RuneInventoryWidget* RuneInven;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UGS_StatPanelWidget* StatPanel;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* ApplyButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* ResetButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* PresetButton1;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* PresetButton2;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* PresetButton3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcaneBoard")
	TSubclassOf<UGS_RuneGridCellWidget> GridCellWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcaneBoard")
	TSubclassOf<UGS_DragVisualWidget> DragVisualWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcaneBoard")
	TSubclassOf<UGS_RuneTooltipWidget> TooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcaneBoard")
	TSubclassOf<UGS_CommonTwoBtnPopup> PresetSaveConfirmPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RunePickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RunePlaceSuccessSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RunePlaceFailSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RuneCancelSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RuneConnectionBonusSound;

private:
	// 핵심 참조
	UPROPERTY()
	UGS_ArcaneBoardManager* BoardManager;

	UPROPERTY()
	UGS_ArcaneBoardLPS* ArcaneBoardLPS;

	// 그리드 상태
	UPROPERTY()
	TMap<FIntPoint, UGS_RuneGridCellWidget*> GridCells;

	TArray<FIntPoint> PreviewCells;

	// 선택 상태
	uint8 SelectedRuneID;
	bool bIsInSelectionMode;

	UPROPERTY()
	UGS_DragVisualWidget* SelectionVisualWidget;

	UPROPERTY()
	UGS_RuneGridCellWidget* LastClickedCell;

	// 툴팁 시스템
	UPROPERTY()
	UGS_RuneTooltipWidget* RuneTooltipWidget;

	uint8 CurrTooltipRuneID;
	FTimerHandle TooltipDelayTimer;

	// 프리셋 저장 확인 시스템
	int32 PendingPresetIndex;

	UPROPERTY()
	UGS_CommonTwoBtnPopup* PresetSaveConfirmPopup;

	// 버튼 이벤트 핸들러
	UFUNCTION()
	void OnApplyButtonClicked();

	UFUNCTION()
	void OnResetButtonClicked();

	UFUNCTION()
	void OnPresetButton1Clicked();

	UFUNCTION()
	void OnPresetButton2Clicked();

	UFUNCTION()
	void OnPresetButton3Clicked();

	// 시스템 초기화
	void BindToLPS();
	void UnbindFromLPS();

	// 그리드 관리
	void GenerateGridLayout();
	void UpdateGridVisuals();
	void UpdateGridPreview(uint8 RuneID, const FIntPoint& ReferenceCellPos);
	void ClearPreview();

	// 드래그 앤 드롭
	void PositionDragVisualAtMouse();
	bool StartRuneReposition(uint8 RuneID);
	UGS_RuneGridCellWidget* GetCellAtPos(const FVector2D& ScreenPos);

	// 툴팁
	void ShowTooltip(uint8 RuneID, const FVector2D& MousePos);
	void CancelTooltipRequest();
	bool ShouldShowTooltip() const;
	bool IsMouseOverTooltipWidget(const FVector2D& ScreenPos);

	// 프리셋 관리
	void ShowPresetSaveConfirmPopup(int32 TargetPresetIndex);
	void SwitchToPreset(int32 PresetIndex);
	void OnPresetSaveYes();
	void OnPresetSaveNo();
	void UpdatePresetButtonVisuals();

	// 유틸리티
	FVector2D GetArcaneBoardCellSize() const;
};