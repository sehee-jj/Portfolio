// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GS_DragVisualWidget.generated.h"

class UUniformGridPanel;
class UImage;
class USizeBox;

/**
 * 드래그 중 룬을 보여주는 위젯
 */
UCLASS()
class GAS_API UGS_DragVisualWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGS_DragVisualWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void Setup(uint8 InRuneID, UTexture2D* InTexture, const TMap<FIntPoint, UTexture2D*>& RuneShape, const FVector2D& InBaseCellSize, float ScaleFactor = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	FVector2D GetReferenceCellOffset() const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USizeBox* DragVisualSizeBox;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UUniformGridPanel* RuneGridPanel;

private:
	// Setup 데이터
	uint8 RuneID;
	FVector2D BaseCellSize;
	float CurrentScaleFactor;
	TMap<FIntPoint, UTexture2D*> CachedRuneShape;

	// 그리드 데이터
	FIntPoint ReferenceCellPos;
	FVector2D ReferenceCellOffset;
	TMap<FIntPoint, UImage*> GridCellWidgets;
	TMap<FIntPoint, FIntPoint> CellToGridPosMap;

	// 그리드 생성
	void CreateRuneShapeGrid(const TMap<FIntPoint, UTexture2D*>& RuneShape);
	void CalculateGridBounds(const TMap<FIntPoint, UTexture2D*>& RuneShape, FIntPoint& MinPos, FIntPoint& MaxPos);

	// 오프셋 계산
	void CalculateReferenceCellOffset();
	void SetDragVisualSize(const FIntPoint& GridSize);
};