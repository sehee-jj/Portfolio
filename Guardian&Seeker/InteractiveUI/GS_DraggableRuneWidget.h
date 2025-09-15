// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GS_DraggableRuneWidget.generated.h"

class UImage;
class UGS_ArcaneBoardWidget;

/**
 * 드래그 가능한 룬 위젯
 */
UCLASS()
class GAS_API UGS_DraggableRuneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGS_DraggableRuneWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void InitRuneWidget(uint8 InRuneID, UTexture2D* InRuneTexture, UGS_ArcaneBoardWidget* BoardWidget = nullptr);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	uint8 GetRuneID() const;

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard")
	void SetPlaced(bool bPlaced);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* RuneImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* RuneState;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* SelectionIndicator;

private:
	uint8 RuneID;
	bool bIsPlaced;

	UPROPERTY()
	UGS_ArcaneBoardWidget* ParentBoardWidget;

	void SetRuneTexture(UTexture2D* Texture);
	void SetRuneVisualState(bool bHovered, bool bDisabled = false);
};