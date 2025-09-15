// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/RuneSystem/GS_ArcaneBoardWidget.h"
#include "RuneSystem/GS_ArcaneBoardManager.h"
#include "RuneSystem/GS_GridLayoutDataAsset.h"
#include "RuneSystem/GS_ArcaneBoardLPS.h"
#include "Components/UniformGridPanel.h"
#include "Components/Button.h"
#include "UI/RuneSystem/GS_RuneGridCellWidget.h"
#include "UI/RuneSystem/GS_RuneInventoryWidget.h"
#include "UI/RuneSystem/GS_StatPanelWidget.h"
#include "UI/RuneSystem/GS_DragVisualWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/RuneSystem/GS_RuneTooltipWidget.h"
#include "Kismet/GameplayStatics.h"

UGS_ArcaneBoardWidget::UGS_ArcaneBoardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SelectedRuneID = 0;
	bIsInSelectionMode = false;
	SelectionVisualWidget = nullptr;
	RuneTooltipWidget = nullptr;
	CurrTooltipRuneID = 0;
	LastClickedCell = nullptr;
	BoardManager = nullptr;
	ArcaneBoardLPS = nullptr;
}

void UGS_ArcaneBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 이벤트 바인딩
	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &UGS_ArcaneBoardWidget::OnApplyButtonClicked);
	}

	if (ResetButton)
	{
		ResetButton->OnClicked.AddDynamic(this, &UGS_ArcaneBoardWidget::OnResetButtonClicked);
	}

	if (PresetButton1)
	{
		PresetButton1->OnClicked.AddDynamic(this, &UGS_ArcaneBoardWidget::OnPresetButton1Clicked);
	}

	if (PresetButton2)
	{
		PresetButton2->OnClicked.AddDynamic(this, &UGS_ArcaneBoardWidget::OnPresetButton2Clicked);
	}

	if (PresetButton3)
	{
		PresetButton3->OnClicked.AddDynamic(this, &UGS_ArcaneBoardWidget::OnPresetButton3Clicked);
	}

	BindToLPS();
}

void UGS_ArcaneBoardWidget::NativeDestruct()
{
	UnbindFromLPS();

	if (IsValid(SelectionVisualWidget))
	{
		SelectionVisualWidget->RemoveFromParent();
		SelectionVisualWidget = nullptr;
	}

	if (IsValid(RuneTooltipWidget))
	{
		RuneTooltipWidget->RemoveFromParent();
		RuneTooltipWidget = nullptr;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TooltipDelayTimer);
	}

	Super::NativeDestruct();
}

FReply UGS_ArcaneBoardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();

	// 툴팁 위치 업데이트
	if (RuneTooltipWidget)
	{
		if (IsMouseOverTooltipWidget(MousePos))
		{
			FVector2D ViewportMousePos = FVector2D::ZeroVector;
			if (GetWorld())
			{
				ViewportMousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
			}
			RuneTooltipWidget->SetPositionInViewport(ViewportMousePos, false);
		}
		else
		{
			HideTooltip();
		}
	}

	// 드래그 중이 아니면 리턴
	if (!bIsInSelectionMode || !SelectionVisualWidget)
	{
		return Reply;
	}

	// 드래그 비주얼 위치 업데이트
	PositionDragVisualAtMouse();

	// 그리드 미리보기 업데이트
	UGS_RuneGridCellWidget* CellUnderMouse = GetCellAtPos(MousePos);
	if (CellUnderMouse)
	{
		UpdateGridPreview(SelectedRuneID, CellUnderMouse->GetCellPos());
	}
	else
	{
		ClearPreview();
	}

	return Reply;
}

FReply UGS_ArcaneBoardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	HideTooltip();

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
		UGS_RuneGridCellWidget* CellUnderMouse = GetCellAtPos(MousePos);
		LastClickedCell = CellUnderMouse;

		if (CellUnderMouse)
		{
			uint8 RuneID = CellUnderMouse->GetPlacedRuneID();

			if (bIsInSelectionMode)
			{
				EndRuneSelection(true);
				return FReply::Handled();
			}
			else if (RuneID > 0)
			{
				StartRuneReposition(RuneID);
				return FReply::Handled();
			}
		}
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		LastClickedCell = nullptr;
		EndRuneSelection(false);
		return FReply::Handled();
	}

	return Reply;
}

void UGS_ArcaneBoardWidget::RefreshForCurrCharacter()
{
	if (IsValid(BoardManager))
	{
		GenerateGridLayout();
		UpdateGridVisuals();

		if (IsValid(RuneInven))
		{
			RuneInven->InitInven(BoardManager, this);
		}

		if (IsValid(StatPanel))
		{
			StatPanel->InitStatList(BoardManager);
			OnStatsChanged(BoardManager->CurrBoardStats);
		}

		UpdatePresetButtonVisuals();
	}
}

void UGS_ArcaneBoardWidget::OnStatsChanged(const FArcaneBoardStats& NewStats)
{
	if (IsValid(StatPanel))
	{
		StatPanel->UpdateStats(NewStats);
	}
}

void UGS_ArcaneBoardWidget::StartRuneSelection(uint8 RuneID)
{
	UE_LOG(LogTemp, Display, TEXT("룬 선택 시작: ID=%d"), RuneID);

	if (RunePickupSound)
	{
		UGameplayStatics::PlaySound2D(this, RunePickupSound);
	}

	HideTooltip();

	if (bIsInSelectionMode)
	{
		EndRuneSelection(false);
	}

	SelectedRuneID = RuneID;
	bIsInSelectionMode = true;

	if (IsValid(DragVisualWidgetClass) && IsValid(BoardManager))
	{
		SelectionVisualWidget = CreateWidget<UGS_DragVisualWidget>(this, DragVisualWidgetClass);
		if (SelectionVisualWidget)
		{
			UTexture2D* RuneTexture = BoardManager->GetRuneTexture(RuneID);
			TMap<FIntPoint, UTexture2D*> RuneShape;
			BoardManager->GetFragmentedRuneTexture(RuneID, RuneShape);

			FVector2D BoardCellSize = GetArcaneBoardCellSize();
			float ScaleFactor = 0.6f;

			SelectionVisualWidget->Setup(RuneID, RuneTexture, RuneShape, BoardCellSize, ScaleFactor);
			SelectionVisualWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			SelectionVisualWidget->AddToViewport(3);

			PositionDragVisualAtMouse();
		}
	}
}

void UGS_ArcaneBoardWidget::EndRuneSelection(bool bPlaceRune)
{
	HideTooltip();

	if (!bPlaceRune)
	{
		if (RuneCancelSound)
		{
			UGameplayStatics::PlaySound2D(this, RuneCancelSound);
		}

		if (IsValid(SelectionVisualWidget))
		{
			SelectionVisualWidget->RemoveFromParent();
			SelectionVisualWidget = nullptr;
		}
		ClearPreview();
	}
	else
	{
		if (IsValid(BoardManager) && SelectedRuneID > 0)
		{
			UGS_RuneGridCellWidget* TargetCell = LastClickedCell;

			if (IsValid(TargetCell))
			{
				FIntPoint PlacementPos = TargetCell->GetCellPos();
				int32 PreviousConnectedRuneCnt = BoardManager->ConnectedRuneCnt;
				TArray<uint8> RemovedRunes;

				bool bPlaceSuccess = BoardManager->PlaceRune(SelectedRuneID, PlacementPos, RemovedRunes);

				if (bPlaceSuccess)
				{
					if (RunePlaceSuccessSound)
					{
						UGameplayStatics::PlaySound2D(this, RunePlaceSuccessSound);
					}

					// 연결 보너스 체크
					if (BoardManager->ConnectedRuneCnt > PreviousConnectedRuneCnt)
					{
						if (RuneConnectionBonusSound)
						{
							if (GetWorld())
							{
								FTimerHandle ConnectionSoundTimer;
								GetWorld()->GetTimerManager().SetTimer(ConnectionSoundTimer, [this]()
									{
										UGameplayStatics::PlaySound2D(this, RuneConnectionBonusSound);
									}, 0.3f, false);
							}
						}
					}

					// 제거된 룬들 UI 업데이트
					for (uint8 RemovedRuneID : RemovedRunes)
					{
						if (IsValid(RuneInven))
						{
							RuneInven->UpdatePlacedStateOfRune(RemovedRuneID, false);
						}
					}

					UpdateGridVisuals();

					if (IsValid(RuneInven))
					{
						RuneInven->UpdatePlacedStateOfRune(SelectedRuneID, true);
					}
				}
				else
				{
					if (RunePlaceFailSound)
					{
						UGameplayStatics::PlaySound2D(this, RunePlaceFailSound);
					}
				}
			}
			else
			{
				if (RunePlaceFailSound)
				{
					UGameplayStatics::PlaySound2D(this, RunePlaceFailSound);
				}
				UE_LOG(LogTemp, Warning, TEXT("클릭한 셀을 찾을 수 없음"));
			}

			if (IsValid(SelectionVisualWidget))
			{
				SelectionVisualWidget->RemoveFromParent();
				SelectionVisualWidget = nullptr;
			}
		}
		ClearPreview();
	}

	LastClickedCell = nullptr;
	bIsInSelectionMode = false;
	SelectedRuneID = 0;
}

void UGS_ArcaneBoardWidget::RequestShowTooltip(uint8 RuneID, const FVector2D& MousePos)
{
	if (!ShouldShowTooltip())
	{
		return;
	}

	if (IsValid(RuneTooltipWidget))
	{
		if (CurrTooltipRuneID == RuneID)
		{
			FVector2D ViewportMousePos = FVector2D::ZeroVector;
			if (GetWorld())
			{
				ViewportMousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
			}

			RuneTooltipWidget->SetPositionInViewport(ViewportMousePos, false);
			return;
		}
		else
		{
			HideTooltip();
		}
	}

	CancelTooltipRequest();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TooltipDelayTimer,
			[this, RuneID, MousePos]()
			{
				FVector2D ViewportMousePos = FVector2D::ZeroVector;
				if (GetWorld())
				{
					ViewportMousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
				}
				ShowTooltip(RuneID, ViewportMousePos);
			},
			0.5f,
			false
		);
	}
}

void UGS_ArcaneBoardWidget::HideTooltip()
{
	CancelTooltipRequest();
	CurrTooltipRuneID = 0;

	if (IsValid(RuneTooltipWidget))
	{
		RuneTooltipWidget->RemoveFromParent();
		RuneTooltipWidget = nullptr;
	}
}

uint8 UGS_ArcaneBoardWidget::GetSelectedRuneID() const
{
	return SelectedRuneID;
}

bool UGS_ArcaneBoardWidget::HasUnsavedChanges() const
{
	if (IsValid(BoardManager))
	{
		return BoardManager->bHasUnsavedChanges;
	}
	return false;
}

// 버튼 이벤트 핸들러
void UGS_ArcaneBoardWidget::OnApplyButtonClicked()
{
	if (UGS_ArcaneBoardLPS* LPS = GetOwningLocalPlayer()->GetSubsystem<UGS_ArcaneBoardLPS>())
	{
		if (HasUnsavedChanges())
		{
			LPS->ApplyBoardChanges();
		}
	}
}

void UGS_ArcaneBoardWidget::OnResetButtonClicked()
{
	if (!IsValid(BoardManager))
	{
		return;
	}

	BoardManager->ResetAllRune();
	UpdateGridVisuals();

	if (IsValid(RuneInven))
	{
		RuneInven->InitInven(BoardManager, this);
	}
}

void UGS_ArcaneBoardWidget::OnPresetButton1Clicked()
{
	LoadPreset(1);
}

void UGS_ArcaneBoardWidget::OnPresetButton2Clicked()
{
	LoadPreset(2);
}

void UGS_ArcaneBoardWidget::OnPresetButton3Clicked()
{
	LoadPreset(3);
}

// 시스템 초기화
void UGS_ArcaneBoardWidget::BindToLPS()
{
	ArcaneBoardLPS = GetOwningLocalPlayer()->GetSubsystem<UGS_ArcaneBoardLPS>();
	if (IsValid(ArcaneBoardLPS))
	{
		ArcaneBoardLPS->SetCurrUIWidget(this);

		BoardManager = ArcaneBoardLPS->GetOrCreateBoardManager();
		if (IsValid(BoardManager))
		{
			ArcaneBoardLPS->LoadBoardConfig();
			RefreshForCurrCharacter();
		}
	}
}

void UGS_ArcaneBoardWidget::UnbindFromLPS()
{
	if (IsValid(ArcaneBoardLPS))
	{
		ArcaneBoardLPS->ClearCurrUIWidget();
	}
}

// 그리드 관리
void UGS_ArcaneBoardWidget::GenerateGridLayout()
{
	if (!IsValid(BoardManager) || !IsValid(GridPanel) || !IsValid(GridCellWidgetClass))
	{
		return;
	}

	UGS_GridLayoutDataAsset* GridLayout = BoardManager->GetCurrGridLayout();
	if (!GridLayout)
	{
		return;
	}

	GridPanel->ClearChildren();
	GridCells.Empty();

	for (const FGridCellData& CellData : GridLayout->GridCells)
	{
		UGS_RuneGridCellWidget* CellWidget = CreateWidget<UGS_RuneGridCellWidget>(this, GridCellWidgetClass);
		if (CellWidget)
		{
			CellWidget->InitCell(CellData, this);
			GridPanel->AddChildToUniformGrid(CellWidget, CellData.Pos.X, CellData.Pos.Y);
			GridCells.Add(CellData.Pos, CellWidget);
		}
	}
}

void UGS_ArcaneBoardWidget::UpdateGridVisuals()
{
	if (!IsValid(BoardManager))
	{
		return;
	}

	for (auto& CellPair : GridCells)
	{
		UGS_RuneGridCellWidget* CellWidget = CellPair.Value;
		FGridCellData UpdatedCellData;

		if (BoardManager->GetCellData(CellPair.Key, UpdatedCellData))
		{
			CellWidget->SetCellData(UpdatedCellData);
		}
	}
}

void UGS_ArcaneBoardWidget::UpdateGridPreview(uint8 RuneID, const FIntPoint& ReferenceCellPos)
{
	if (!IsValid(BoardManager))
	{
		return;
	}

	ClearPreview();

	TArray<uint8> AffectedRuneIDs;
	EPlacementResult PlacementResult = BoardManager->CheckRunePlacement(RuneID, ReferenceCellPos, AffectedRuneIDs);

	TArray<FIntPoint> RuneShape;
	if (!BoardManager->GetRuneShape(RuneID, RuneShape))
	{
		return;
	}

	EGridCellVisualState PreviewState;
	switch (PlacementResult)
	{
	case EPlacementResult::Valid:
		PreviewState = EGridCellVisualState::Valid;
		break;
	case EPlacementResult::ReplaceExisting:
		PreviewState = EGridCellVisualState::ReplaceExisting;
		break;
	case EPlacementResult::OutOfBounds:
	default:
		PreviewState = EGridCellVisualState::Invalid;
		break;
	}

	for (const FIntPoint& Offset : RuneShape)
	{
		FIntPoint CellPos = ReferenceCellPos + Offset;

		if (GridCells.Contains(CellPos))
		{
			UGS_RuneGridCellWidget* CellWidget = GridCells[CellPos];
			if (CellWidget)
			{
				CellWidget->SetPreviewVisualState(PreviewState);
				PreviewCells.Add(CellPos);
			}
		}
	}
}

void UGS_ArcaneBoardWidget::ClearPreview()
{
	for (const FIntPoint& CellPos : PreviewCells)
	{
		if (GridCells.Contains(CellPos))
		{
			UGS_RuneGridCellWidget* CellWidget = GridCells[CellPos];
			if (IsValid(CellWidget))
			{
				CellWidget->SetPreviewVisualState(EGridCellVisualState::Normal);
			}
		}
	}

	PreviewCells.Empty();
}

// 드래그 앤 드롭
void UGS_ArcaneBoardWidget::PositionDragVisualAtMouse()
{
	if (!IsValid(SelectionVisualWidget) || !GetWorld())
	{
		return;
	}

	FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	FVector2D ReferenceCellOffset = SelectionVisualWidget->GetReferenceCellOffset();
	FVector2D AdjustedPos = MousePos - ReferenceCellOffset;

	SelectionVisualWidget->SetPositionInViewport(AdjustedPos, false);
}

bool UGS_ArcaneBoardWidget::StartRuneReposition(uint8 RuneID)
{
	if (!BoardManager->RemoveRune(RuneID))
	{
		return false;
	}

	UpdateGridVisuals();

	if (IsValid(RuneInven))
	{
		RuneInven->UpdatePlacedStateOfRune(RuneID, false);
	}

	StartRuneSelection(RuneID);
	return true;
}

UGS_RuneGridCellWidget* UGS_ArcaneBoardWidget::GetCellAtPos(const FVector2D& ScreenPos)
{
	for (auto& CellPair : GridCells)
	{
		UGS_RuneGridCellWidget* CellWidget = CellPair.Value;
		if (!IsValid(CellWidget))
		{
			continue;
		}

		FGeometry CellGeometry = CellWidget->GetCachedGeometry();
		FVector2D LocalMousePos = CellGeometry.AbsoluteToLocal(ScreenPos);
		FVector2D LocalSize = CellGeometry.GetLocalSize();

		if (LocalMousePos.X >= 0 && LocalMousePos.Y >= 0 &&
			LocalMousePos.X <= LocalSize.X && LocalMousePos.Y <= LocalSize.Y)
		{
			return CellWidget;
		}
	}
	return nullptr;
}

// 툴팁
void UGS_ArcaneBoardWidget::ShowTooltip(uint8 RuneID, const FVector2D& MousePos)
{
	if (!ShouldShowTooltip() || !IsValid(BoardManager) || !IsValid(TooltipWidgetClass))
	{
		return;
	}

	if (MousePos.X <= 0 && MousePos.Y <= 0)
	{
		return;
	}

	FRuneTableRow RuneData;
	if (!BoardManager->GetRuneData(RuneID, RuneData))
	{
		return;
	}

	HideTooltip();

	RuneTooltipWidget = CreateWidget<UGS_RuneTooltipWidget>(this, TooltipWidgetClass);
	if (RuneTooltipWidget)
	{
		CurrTooltipRuneID = RuneID;
		RuneTooltipWidget->SetRuneData(RuneData);

		RuneTooltipWidget->SetPositionInViewport(MousePos, false);
		RuneTooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		RuneTooltipWidget->AddToViewport(5);
	}
}

void UGS_ArcaneBoardWidget::CancelTooltipRequest()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TooltipDelayTimer);
	}
}

bool UGS_ArcaneBoardWidget::ShouldShowTooltip() const
{
	if (bIsInSelectionMode && SelectedRuneID > 0)
	{
		return false;
	}
	return true;
}

bool UGS_ArcaneBoardWidget::IsMouseOverTooltipWidget(const FVector2D& ScreenPos)
{
	UGS_RuneGridCellWidget* CellUnderMouse = GetCellAtPos(ScreenPos);
	if (CellUnderMouse && CellUnderMouse->GetPlacedRuneID() > 0)
	{
		return true;
	}

	if (IsValid(RuneInven))
	{
		FGeometry InvenGeometry = RuneInven->GetCachedGeometry();
		FVector2D LocalMousePos = InvenGeometry.AbsoluteToLocal(ScreenPos);
		FVector2D InvenSize = InvenGeometry.GetLocalSize();

		if (LocalMousePos.X >= 0 && LocalMousePos.Y >= 0 &&
			LocalMousePos.X <= InvenSize.X && LocalMousePos.Y <= InvenSize.Y)
		{
			return true;
		}
	}

	return false;
}

// 프리셋 관리
void UGS_ArcaneBoardWidget::LoadPreset(int32 PresetIndex)
{
	if (IsValid(ArcaneBoardLPS))
	{
		if (HasUnsavedChanges())
		{
			UE_LOG(LogTemp, Warning, TEXT("저장하지 않은 변경사항이 있습니다. 프리셋 %d로 전환합니다."), PresetIndex);
		}

		ArcaneBoardLPS->LoadBoardConfig(PresetIndex);
		RefreshForCurrCharacter();
	}

	UpdatePresetButtonVisuals();
}

void UGS_ArcaneBoardWidget::UpdatePresetButtonVisuals()
{
	if (!IsValid(ArcaneBoardLPS))
	{
		return;
	}

	int32 CurrentPresetIndex = ArcaneBoardLPS->GetCurrentPresetIndex();

	if (IsValid(PresetButton1))
	{
		FLinearColor ButtonColor = (CurrentPresetIndex == 1) ? FLinearColor::Green : FLinearColor::Gray;
		PresetButton1->SetBackgroundColor(ButtonColor);
	}

	if (IsValid(PresetButton2))
	{
		FLinearColor ButtonColor = (CurrentPresetIndex == 2) ? FLinearColor::Green : FLinearColor::Gray;
		PresetButton2->SetBackgroundColor(ButtonColor);
	}

	if (IsValid(PresetButton3))
	{
		FLinearColor ButtonColor = (CurrentPresetIndex == 3) ? FLinearColor::Green : FLinearColor::Gray;
		PresetButton3->SetBackgroundColor(ButtonColor);
	}
}

// 유틸리티
FVector2D UGS_ArcaneBoardWidget::GetArcaneBoardCellSize() const
{
	if (GridCells.Num() == 0)
	{
		return FVector2D(64.0f, 64.0f);
	}

	for (const auto& CellPair : GridCells)
	{
		if (IsValid(CellPair.Value))
		{
			FGeometry CellGeometry = CellPair.Value->GetCachedGeometry();
			return CellGeometry.GetLocalSize();
		}
	}

	return FVector2D(64.0f, 64.0f);
}