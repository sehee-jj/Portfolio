// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GS_ArcaneBoardTableRows.h"
#include "GS_ArcaneBoardTypes.h"
#include "GS_ArcaneBoardManager.generated.h"

class UGS_GridLayoutDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsChangedDelegate, const FArcaneBoardStats&, BoardStats);

/**
 * 룬 시스템 핵심 매니저
 * - 룬 배치/제거, 실시간 스탯 계산, 연결성 탐지
 */
UCLASS()
class GAS_API UGS_ArcaneBoardManager : public UObject
{
	GENERATED_BODY()

public:
	UGS_ArcaneBoardManager();

	UPROPERTY(BlueprintReadWrite, Category = "ArcaneBoard")
	ECharacterClass CurrClass;

	UPROPERTY(BlueprintReadOnly, Category = "ArcaneBoard")
	TArray<FPlacedRuneInfo> PlacedRunes;

	UPROPERTY(BlueprintReadOnly, Category = "ArcaneBoard")
	FArcaneBoardStats AppliedBoardStats;

	UPROPERTY(BlueprintReadOnly, Category = "ArcaneBoard")
	FArcaneBoardStats CurrBoardStats;

	UPROPERTY(BlueprintReadOnly, Category = "ArcaneBoard")
	bool bHasUnsavedChanges;

	UPROPERTY(BlueprintReadOnly, Category = "ArcaneBoard")
	int32 ConnectedRuneCnt;

	UPROPERTY(BlueprintAssignable, Category = "ArcaneBoard|Events")
	FOnStatsChangedDelegate OnStatsChanged;

	// 클래스/그리드 관리
	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Class")
	bool SetCurrClass(ECharacterClass NewClass);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Class")
	ECharacterClass GetCurrClass() { return CurrClass; }

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Grid")
	UGS_GridLayoutDataAsset* GetCurrGridLayout() const { return CurrGridLayout; }

	// 룬 배치 시스템
	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Placement")
	EPlacementResult CheckRunePlacement(uint8 RuneID, const FIntPoint& Pos, TArray<uint8>& OutAffectedRuneIDs);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Placement")
	bool PlaceRune(uint8 RuneID, const FIntPoint& Pos, TArray<uint8>& OutRemovedRunes);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Placement")
	bool RemoveRune(uint8 RuneID);

	// 스탯 계산
	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Stats")
	void CalculateStatEffects();

	// 상태 관리
	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|State")
	void ApplyChanges();

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|State")
	void ResetAllRune();

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|State")
	void LoadSavedData(ECharacterClass Class, const TArray<FPlacedRuneInfo>& Runes);

	// 데이터 접근
	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	bool GetRuneData(uint8 RuneID, FRuneTableRow& OutData);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	bool GetCellData(const FIntPoint& Pos, FGridCellData& OutCellData);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	bool GetRuneShape(uint8 RuneID, TArray<FIntPoint>& OutShape);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	UTexture2D* GetRuneTexture(uint8 RuneID);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	bool GetFragmentedRuneTexture(uint8 RuneID, TMap<FIntPoint, UTexture2D*>& OutShape);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	bool GetConnectedFragmentedRuneTexture(uint8 RuneID, TMap<FIntPoint, UTexture2D*>& OutShape);

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Data")
	void InitDataCache();

	UFUNCTION(BlueprintCallable, Category = "ArcaneBoard|Grid")
	void InitGridState();

private:
	UPROPERTY()
	UDataTable* RuneTable;

	UPROPERTY()
	UDataTable* GridLayoutTable;

	TMap<uint8, FRuneTableRow> RuneDataCache;
	TMap<ECharacterClass, UGS_GridLayoutDataAsset*> GridLayoutCache;

	UPROPERTY()
	UGS_GridLayoutDataAsset* CurrGridLayout;

	TMap<FIntPoint, FGridCellData> CurrGridState;
	FIntPoint SpecialCellPos;

	bool LoadGridLayoutForClass(ECharacterClass TargetClass);
	void ApplyRuneStatEffect(const FStatEffect& StatEffect, FGS_StatRow& BaseStats,
		FGS_StatRow& BonusStats, bool bIsConnected, float BonusValue);

	// DFS로 연결된 셀 탐색
	void UpdateConnections();
	void FindConnectedCells(const FIntPoint CellPos, TSet<FIntPoint>& VisitedCells);
	bool IsRuneConnected(uint8 RuneID) const;

	void ApplyRuneToGrid(uint8 RuneID, const FIntPoint& Position, EGridCellState NewState, bool bApplyTexture = true);
	void UpdateCellState(const FIntPoint& Pos, EGridCellState NewState, uint8 RuneID = 0,
		UTexture2D* RuneTextureFrag = nullptr, UTexture2D* ConnectedRuneTextureFrag = nullptr);

	void CacheRuneData();
	void CacheGridLayouts();
};