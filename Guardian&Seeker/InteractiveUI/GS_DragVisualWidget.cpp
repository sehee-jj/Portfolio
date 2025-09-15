// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/RuneSystem/GS_DragVisualWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/UniformGridSlot.h"
#include "Components/UniformGridPanel.h"

UGS_DragVisualWidget::UGS_DragVisualWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    RuneID = 0;
    ReferenceCellPos = FIntPoint::ZeroValue;
    ReferenceCellOffset = FVector2D::ZeroVector;
    BaseCellSize = FVector2D(64.0f, 64.0f);
    CurrentScaleFactor = 1.0f;
}

void UGS_DragVisualWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UGS_DragVisualWidget::Setup(uint8 InRuneID, UTexture2D* InTexture, const TMap<FIntPoint, UTexture2D*>& RuneShape, const FVector2D& InBaseCellSize, float ScaleFactor)
{
    RuneID = InRuneID;
    CachedRuneShape = RuneShape;
    BaseCellSize = InBaseCellSize;
    CurrentScaleFactor = ScaleFactor;

    if (!IsValid(RuneGridPanel) || RuneShape.Num() == 0)
    {
        return;
    }

    // 그리드 바운드 계산
    FIntPoint MinPos, MaxPos;
    CalculateGridBounds(RuneShape, MinPos, MaxPos);

    // 그리드 크기 설정
    FIntPoint GridSize = FIntPoint(MaxPos.X - MinPos.X + 1, MaxPos.Y - MinPos.Y + 1);
    SetDragVisualSize(GridSize);

    // 룬 모양 그리드 생성
    CreateRuneShapeGrid(RuneShape);

    // 기준점 오프셋 계산
    CalculateReferenceCellOffset();
}

FVector2D UGS_DragVisualWidget::GetReferenceCellOffset() const
{
    return ReferenceCellOffset;
}

void UGS_DragVisualWidget::CreateRuneShapeGrid(const TMap<FIntPoint, UTexture2D*>& RuneShape)
{
    if (!IsValid(RuneGridPanel))
    {
        return;
    }

    // 기존 그리드 정리
    RuneGridPanel->ClearChildren();
    GridCellWidgets.Empty();
    CellToGridPosMap.Empty();

    // 그리드 바운드 계산
    FIntPoint MinPos, MaxPos;
    CalculateGridBounds(RuneShape, MinPos, MaxPos);

    // 기준점 설정 (0,0이 있으면 그것을 기준으로, 없으면 최소값)
    ReferenceCellPos = RuneShape.Contains(FIntPoint::ZeroValue) ? FIntPoint::ZeroValue : MinPos;

    // 각 룬 조각을 그리드에 배치
    for (const auto& ShapePair : RuneShape)
    {
        FIntPoint CellPos = ShapePair.Key;
        UTexture2D* CellTexture = ShapePair.Value;

        if (!CellTexture)
        {
            continue;
        }

        // 이미지 위젯 생성
        UImage* CellImage = NewObject<UImage>(this);
        CellImage->SetBrushFromTexture(CellTexture);

        // 그리드 위치 계산 (MinPos를 원점으로 이동)
        FIntPoint GridPos = CellPos - MinPos;
        UUniformGridSlot* ImageSlot = RuneGridPanel->AddChildToUniformGrid(CellImage, GridPos.X, GridPos.Y);

        if (ImageSlot)
        {
            ImageSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
            ImageSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
        }

        // 캐시 저장
        CellToGridPosMap.Add(CellPos, GridPos);
        GridCellWidgets.Add(CellPos, CellImage);
    }
}

void UGS_DragVisualWidget::CalculateGridBounds(const TMap<FIntPoint, UTexture2D*>& RuneShape, FIntPoint& MinPos, FIntPoint& MaxPos)
{
    if (RuneShape.Num() == 0)
    {
        MinPos = MaxPos = FIntPoint::ZeroValue;
        return;
    }

    MinPos = FIntPoint(INT_MAX, INT_MAX);
    MaxPos = FIntPoint(INT_MIN, INT_MIN);

    for (const auto& ShapePair : RuneShape)
    {
        FIntPoint Pos = ShapePair.Key;
        MinPos.X = FMath::Min(MinPos.X, Pos.X);
        MinPos.Y = FMath::Min(MinPos.Y, Pos.Y);
        MaxPos.X = FMath::Max(MaxPos.X, Pos.X);
        MaxPos.Y = FMath::Max(MaxPos.Y, Pos.Y);
    }
}

void UGS_DragVisualWidget::SetDragVisualSize(const FIntPoint& GridSize)
{
    if (!IsValid(DragVisualSizeBox))
    {
        return;
    }

    // 전체 드래그 비주얼 크기 계산
    FVector2D TotalSize = FVector2D(
        GridSize.Y * BaseCellSize.X * CurrentScaleFactor,
        GridSize.X * BaseCellSize.Y * CurrentScaleFactor
    );

    DragVisualSizeBox->SetWidthOverride(TotalSize.X);
    DragVisualSizeBox->SetHeightOverride(TotalSize.Y);
}

void UGS_DragVisualWidget::CalculateReferenceCellOffset()
{
    // 기준 셀이 실제 그리드에 존재하는지 확인
    if (CachedRuneShape.Contains(ReferenceCellPos) && CellToGridPosMap.Contains(ReferenceCellPos))
    {
        FIntPoint ActualGridPos = CellToGridPosMap[ReferenceCellPos];
        FVector2D ActualCellSize = BaseCellSize * CurrentScaleFactor;

        // UI 좌표계에서의 오프셋 계산
        FVector2D BaseOffset = FVector2D(
            ActualGridPos.Y * ActualCellSize.X + ActualCellSize.X * 0.5f,
            ActualGridPos.X * ActualCellSize.Y + ActualCellSize.Y * 0.5f
        );

        ReferenceCellOffset = BaseOffset;
    }
    else
    {
        // 폴백: 전체 크기의 중앙점 사용
        FIntPoint MinPos, MaxPos;
        CalculateGridBounds(CachedRuneShape, MinPos, MaxPos);

        FVector2D TotalSize = FVector2D(
            (MaxPos.Y - MinPos.Y + 1) * BaseCellSize.X * CurrentScaleFactor,
            (MaxPos.X - MinPos.X + 1) * BaseCellSize.Y * CurrentScaleFactor
        );

        ReferenceCellOffset = TotalSize * 0.5f;
    }
}