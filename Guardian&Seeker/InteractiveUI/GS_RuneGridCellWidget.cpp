// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/RuneSystem/GS_RuneGridCellWidget.h"
#include "UI/RuneSystem/GS_ArcaneBoardWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Blueprint/WidgetLayoutLibrary.h"

UGS_RuneGridCellWidget::UGS_RuneGridCellWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisualState = EGridCellVisualState::Normal;
	ParentBoardWidget = nullptr;
}

void UGS_RuneGridCellWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UGS_RuneGridCellWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (ParentBoardWidget)
	{
		if (CellData.PlacedRuneID > 0)
		{
			FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
			if (APlayerController* PC = GetOwningPlayer())
			{
				FGeometry ScreenGeometry = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC);
				MousePos = ScreenGeometry.AbsoluteToLocal(MousePos);
			}
			ParentBoardWidget->RequestShowTooltip(CellData.PlacedRuneID, MousePos);
		}
		else
		{
			ParentBoardWidget->HideTooltip();
		}
	}
}

void UGS_RuneGridCellWidget::InitCell(const FGridCellData& InCellData, UGS_ArcaneBoardWidget* InParentBoard)
{
	ParentBoardWidget = InParentBoard;
	SetCellData(InCellData);

	if (IsValid(PreviewImage))
	{
		PreviewImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UGS_RuneGridCellWidget::SetCellData(const FGridCellData& InCellData)
{
	CellData = InCellData;

	// 연결 상태에 따른 텍스처 선택
	UTexture2D* TextureToUse = CellData.bIsConnected && CellData.ConnectedRuneTextureFrag ?
		CellData.ConnectedRuneTextureFrag : CellData.RuneTextureFrag;

	SetRuneTexture(TextureToUse);

	// 특수 셀 배경색 설정
	if (CellData.bIsSpecialCell && IsValid(CellBG))
	{
		CellBG->SetColorAndOpacity(FLinearColor(0.f, 0.f, 1.f, 0.5f));
	}
}

FIntPoint UGS_RuneGridCellWidget::GetCellPos() const
{
	return CellData.Pos;
}

uint8 UGS_RuneGridCellWidget::GetPlacedRuneID() const
{
	return CellData.PlacedRuneID;
}

void UGS_RuneGridCellWidget::SetPreviewVisualState(EGridCellVisualState NewState)
{
	if (VisualState != NewState)
	{
		VisualState = NewState;

		switch (NewState)
		{
		case EGridCellVisualState::Normal:
			if (IsValid(CellBG))
			{
				CellBG->SetVisibility(ESlateVisibility::Visible);
			}
			if (IsValid(PreviewImage))
			{
				PreviewImage->SetVisibility(ESlateVisibility::Hidden);
			}
			break;
		case EGridCellVisualState::Valid:
			if (IsValid(CellBG))
			{
				CellBG->SetVisibility(ESlateVisibility::Hidden);
			}
			if (IsValid(PreviewImage))
			{
				PreviewImage->SetVisibility(ESlateVisibility::Visible);
				PreviewImage->SetColorAndOpacity(FLinearColor(0.f, 1.f, 0.f, 0.2f));
			}
			break;
		case EGridCellVisualState::ReplaceExisting:
			if (IsValid(CellBG))
			{
				CellBG->SetVisibility(ESlateVisibility::Hidden);
			}
			if (IsValid(PreviewImage))
			{
				PreviewImage->SetVisibility(ESlateVisibility::Visible);
				PreviewImage->SetColorAndOpacity(FLinearColor(1.f, 0.65f, 0.f, 0.2f));
			}
			break;
		case EGridCellVisualState::Invalid:
			if (IsValid(CellBG))
			{
				CellBG->SetVisibility(ESlateVisibility::Hidden);
			}
			if (IsValid(PreviewImage))
			{
				PreviewImage->SetVisibility(ESlateVisibility::Visible);
				PreviewImage->SetColorAndOpacity(FLinearColor(1.f, 0.f, 0.f, 0.2f));
			}
			break;
		default:
			break;
		}
	}
}

void UGS_RuneGridCellWidget::SetRuneTexture(UTexture2D* Texture)
{
	if (IsValid(RuneImage))
	{
		if (Texture)
		{
			RuneImage->SetBrushFromTexture(Texture);
			RuneImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			RuneImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}