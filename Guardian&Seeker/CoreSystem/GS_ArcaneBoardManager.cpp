// Copyright Epic Games, Inc. All Rights Reserved.

#include "RuneSystem/GS_ArcaneBoardManager.h"
#include "RuneSystem/GS_GridLayoutDataAsset.h"
#include "RuneSystem/GS_EnumUtils.h"
#include "Engine/DataTable.h"

UGS_ArcaneBoardManager::UGS_ArcaneBoardManager()
{
	// 기본 초기화
	CurrClass = ECharacterClass::Ares;
	PlacedRunes.Empty();
	AppliedBoardStats = FArcaneBoardStats();
	CurrBoardStats = FArcaneBoardStats();
	CurrGridLayout = nullptr;
	ConnectedRuneCnt = 0;
	bHasUnsavedChanges = false;

	// 데이터 테이블 로드
	static ConstructorHelpers::FObjectFinder<UDataTable> RuneTableFinder(TEXT("/Game/DataTable/RuneSystem/DT_RuneDataTable"));
	if (RuneTableFinder.Succeeded())
	{
		RuneTable = RuneTableFinder.Object;
	}
	else
	{
		RuneTable = nullptr;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> GridLayoutTableFinder(TEXT("/Game/DataTable/RuneSystem/DT_GridLayoutTable"));
	if (GridLayoutTableFinder.Succeeded())
	{
		GridLayoutTable = GridLayoutTableFinder.Object;
	}
	else
	{
		GridLayoutTable = nullptr;
	}

	// 데이터 테이블 로드 후 캐시 초기화
	InitDataCache();
}

bool UGS_ArcaneBoardManager::SetCurrClass(ECharacterClass NewClass)
{
	if (CurrClass == NewClass && IsValid(CurrGridLayout))
	{
		return true;
	}

	if (!LoadGridLayoutForClass(NewClass))
	{
		return false;
	}

	bool bNeedGridReset = (CurrClass != NewClass);
	CurrClass = NewClass;
	CurrGridLayout = GridLayoutCache[NewClass];

	if (bNeedGridReset)
	{
		InitGridState();
		CalculateStatEffects();
		AppliedBoardStats = CurrBoardStats;
	}

	bHasUnsavedChanges = false;
	return true;
}

bool UGS_ArcaneBoardManager::LoadGridLayoutForClass(ECharacterClass TargetClass)
{
	if (GridLayoutCache.Contains(TargetClass))
	{
		return true;
	}

	if (!IsValid(GridLayoutTable))
	{
		return false;
	}

	FString RowName = UGS_EnumUtils::GetEnumAsString(TargetClass);
	FGridLayoutTableRow* LayoutRow = GridLayoutTable->FindRow<FGridLayoutTableRow>(*RowName, TEXT("LoadGridLayout"));

	if (!LayoutRow || LayoutRow->GridLayoutAsset.IsNull())
	{
		return false;
	}

	UGS_GridLayoutDataAsset* LayoutAsset = LayoutRow->GridLayoutAsset.LoadSynchronous();
	if (!LayoutAsset)
	{
		return false;
	}

	GridLayoutCache.Add(TargetClass, LayoutAsset);
	return true;
}

EPlacementResult UGS_ArcaneBoardManager::CheckRunePlacement(uint8 RuneID, const FIntPoint& Pos, TArray<uint8>& OutAffectedRuneIDs)
{
	OutAffectedRuneIDs.Empty();

	TArray<FIntPoint> RuneShape;
	if (!GetRuneShape(RuneID, RuneShape))
	{
		return EPlacementResult::OutOfBounds;
	}

	bool bHasOverlapping = false;
	bool bOutOfBounds = false;
	TSet<uint8> OverlappingRuneIDSet;

	for (const FIntPoint& Offset : RuneShape)
	{
		FIntPoint CellPos = Pos + Offset;

		if (!CurrGridState.Contains(CellPos))
		{
			bOutOfBounds = true;
			continue;
		}

		const FGridCellData& CellData = CurrGridState[CellPos];
		if (CellData.State == EGridCellState::Occupied && CellData.PlacedRuneID > 0)
		{
			bHasOverlapping = true;
			OverlappingRuneIDSet.Add(CellData.PlacedRuneID);
		}
	}

	OutAffectedRuneIDs = OverlappingRuneIDSet.Array();

	if (bOutOfBounds)
	{
		return EPlacementResult::OutOfBounds;
	}
	else if (bHasOverlapping)
	{
		return EPlacementResult::ReplaceExisting;
	}
	else
	{
		return EPlacementResult::Valid;
	}
}

bool UGS_ArcaneBoardManager::PlaceRune(uint8 RuneID, const FIntPoint& Pos, TArray<uint8>& OutRemovedRunes)
{
	OutRemovedRunes.Empty();

	TArray<uint8> AffectedRuneIDs;
	EPlacementResult PlacementResult = CheckRunePlacement(RuneID, Pos, AffectedRuneIDs);

	if (PlacementResult == EPlacementResult::OutOfBounds)
	{
		return false;
	}

	// 겹치는 룬들 제거
	if (PlacementResult == EPlacementResult::ReplaceExisting)
	{
		for (uint8 OverlappingRuneID : AffectedRuneIDs)
		{
			if (RemoveRune(OverlappingRuneID))
			{
				OutRemovedRunes.Add(OverlappingRuneID);
			}
		}
	}

	TMap<FIntPoint, UTexture2D*> RuneShape;
	if (!GetFragmentedRuneTexture(RuneID, RuneShape))
	{
		return false;
	}

	PlacedRunes.Add(FPlacedRuneInfo(RuneID, Pos));
	ApplyRuneToGrid(RuneID, Pos, EGridCellState::Occupied, true);

	bHasUnsavedChanges = true;
	CalculateStatEffects();
	OnStatsChanged.Broadcast(CurrBoardStats);

	return true;
}

bool UGS_ArcaneBoardManager::RemoveRune(uint8 RuneID)
{
	int32 RuneIndex = INDEX_NONE;
	for (int32 i = 0; i < PlacedRunes.Num(); ++i)
	{
		if (PlacedRunes[i].RuneID == RuneID)
		{
			RuneIndex = i;
			break;
		}
	}

	if (RuneIndex == INDEX_NONE)
	{
		return false;
	}

	FIntPoint RunePos = PlacedRunes[RuneIndex].Pos;
	ApplyRuneToGrid(RuneID, RunePos, EGridCellState::Empty, false);
	PlacedRunes.RemoveAt(RuneIndex);

	bHasUnsavedChanges = true;
	CalculateStatEffects();
	OnStatsChanged.Broadcast(CurrBoardStats);

	return true;
}

void UGS_ArcaneBoardManager::CalculateStatEffects()
{
	UpdateConnections();

	FGS_StatRow BaseStats, BonusStats;
	float ConnectionBonus = static_cast<float>(ConnectedRuneCnt);

	for (const FPlacedRuneInfo& RuneInfo : PlacedRunes)
	{
		FRuneTableRow RuneData;
		if (!GetRuneData(RuneInfo.RuneID, RuneData))
		{
			continue;
		}

		bool bIsConnected = IsRuneConnected(RuneInfo.RuneID);
		ApplyRuneStatEffect(RuneData.StatEffect, BaseStats, BonusStats, bIsConnected, ConnectionBonus);
	}

	CurrBoardStats.RuneStats = BaseStats;
	CurrBoardStats.BonusStats = BonusStats;
}

void UGS_ArcaneBoardManager::ApplyRuneStatEffect(const FStatEffect& StatEffect, FGS_StatRow& BaseStats,
	FGS_StatRow& BonusStats, bool bIsConnected, float BonusValue)
{
	const FName& StatName = StatEffect.StatName;
	float StatValue = StatEffect.Value;

	if (StatName == FName("HP"))
	{
		BaseStats.HP += StatValue;
		if (bIsConnected && BonusStats.HP == 0)
		{
			BonusStats.HP = BonusValue;
		}
	}
	else if (StatName == FName("ATK"))
	{
		BaseStats.ATK += StatValue;
		if (bIsConnected && BonusStats.ATK == 0)
		{
			BonusStats.ATK = BonusValue;
		}
	}
	else if (StatName == FName("DEF"))
	{
		BaseStats.DEF += StatValue;
		if (bIsConnected && BonusStats.DEF == 0)
		{
			BonusStats.DEF = BonusValue;
		}
	}
	else if (StatName == FName("AGL"))
	{
		BaseStats.AGL += StatValue;
		if (bIsConnected && BonusStats.AGL == 0)
		{
			BonusStats.AGL = BonusValue;
		}
	}
	else if (StatName == FName("ATS"))
	{
		BaseStats.ATS += StatValue;
		if (bIsConnected && BonusStats.ATS == 0)
		{
			BonusStats.ATS = BonusValue;
		}
	}
}

// DFS로 연결된 셀 탐색
void UGS_ArcaneBoardManager::UpdateConnections()
{
	for (auto& CellPair : CurrGridState)
	{
		CellPair.Value.bIsConnected = false;
	}

	TSet<FIntPoint> VisitedCells;
	FindConnectedCells(SpecialCellPos, VisitedCells);

	TSet<uint8> ConnectedRuneIDs;
	for (const auto& CellPair : CurrGridState)
	{
		if (CellPair.Value.bIsConnected && CellPair.Value.PlacedRuneID > 0)
		{
			ConnectedRuneIDs.Add(CellPair.Value.PlacedRuneID);
		}
	}

	ConnectedRuneCnt = ConnectedRuneIDs.Num();
}

void UGS_ArcaneBoardManager::FindConnectedCells(const FIntPoint CellPos, TSet<FIntPoint>& VisitedCells)
{
	if (VisitedCells.Contains(CellPos))
	{
		return;
	}

	FGridCellData CellData;
	if (!GetCellData(CellPos, CellData) || CellData.State == EGridCellState::Empty)
	{
		return;
	}

	VisitedCells.Add(CellPos);
	if (CurrGridState.Contains(CellPos))
	{
		CurrGridState[CellPos].bIsConnected = true;
	}

	static const TArray<FIntPoint> Directions = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };
	for (const FIntPoint& Direction : Directions)
	{
		FIntPoint NextPos = CellPos + Direction;
		FindConnectedCells(NextPos, VisitedCells);
	}
}

bool UGS_ArcaneBoardManager::IsRuneConnected(uint8 RuneID) const
{
	for (const auto& CellPair : CurrGridState)
	{
		if (CellPair.Value.PlacedRuneID == RuneID && CellPair.Value.bIsConnected)
		{
			return true;
		}
	}
	return false;
}

void UGS_ArcaneBoardManager::ApplyChanges()
{
	AppliedBoardStats = CurrBoardStats;
	bHasUnsavedChanges = false;
	OnStatsChanged.Broadcast(AppliedBoardStats);
}

void UGS_ArcaneBoardManager::ResetAllRune()
{
	if (PlacedRunes.Num() == 0)
	{
		return;
	}

	PlacedRunes.Empty();
	InitGridState();
	CurrBoardStats = FArcaneBoardStats();
	bHasUnsavedChanges = true;
	OnStatsChanged.Broadcast(CurrBoardStats);
}

void UGS_ArcaneBoardManager::LoadSavedData(ECharacterClass Class, const TArray<FPlacedRuneInfo>& Runes)
{
	PlacedRunes.Empty();
	InitGridState();
	CurrBoardStats = FArcaneBoardStats();

	for (const FPlacedRuneInfo& RuneInfo : Runes)
	{
		PlacedRunes.Add(RuneInfo);
		ApplyRuneToGrid(RuneInfo.RuneID, RuneInfo.Pos, EGridCellState::Occupied, true);
	}

	CalculateStatEffects();
	AppliedBoardStats = CurrBoardStats;
	OnStatsChanged.Broadcast(CurrBoardStats);
	bHasUnsavedChanges = false;
}

void UGS_ArcaneBoardManager::InitGridState()
{
	CurrGridState.Empty();
	PlacedRunes.Empty();

	if (!IsValid(CurrGridLayout))
	{
		return;
	}

	for (const FGridCellData& Cell : CurrGridLayout->GridCells)
	{
		FGridCellData NewCell = Cell;

		if (NewCell.State == EGridCellState::Empty)
		{
			NewCell.PlacedRuneID = 0;
		}
		else if (NewCell.State == EGridCellState::Occupied)
		{
			PlacedRunes.Add(FPlacedRuneInfo(Cell.PlacedRuneID, Cell.Pos));
		}

		CurrGridState.Add(Cell.Pos, NewCell);

		if (Cell.bIsSpecialCell)
		{
			SpecialCellPos = Cell.Pos;
		}
	}
}

void UGS_ArcaneBoardManager::ApplyRuneToGrid(uint8 RuneID, const FIntPoint& Position, EGridCellState NewState, bool bApplyTexture)
{
	TMap<FIntPoint, UTexture2D*> RuneShape, ConnectedRuneShape;
	if (!GetFragmentedRuneTexture(RuneID, RuneShape) ||
		!GetConnectedFragmentedRuneTexture(RuneID, ConnectedRuneShape))
	{
		return;
	}

	for (const auto& ShapePair : RuneShape)
	{
		FIntPoint CellPos = Position + ShapePair.Key;

		UTexture2D* NormalTexture = nullptr;
		UTexture2D* ConnectedTexture = nullptr;
		uint8 RuneIDToApply = 0;

		if (NewState == EGridCellState::Occupied && bApplyTexture)
		{
			NormalTexture = ShapePair.Value;
			RuneIDToApply = RuneID;

			if (ConnectedRuneShape.Contains(ShapePair.Key))
			{
				ConnectedTexture = ConnectedRuneShape[ShapePair.Key];
			}
		}

		UpdateCellState(CellPos, NewState, RuneIDToApply, NormalTexture, ConnectedTexture);
	}
}

void UGS_ArcaneBoardManager::UpdateCellState(const FIntPoint& Pos, EGridCellState NewState, uint8 RuneID,
	UTexture2D* RuneTextureFrag, UTexture2D* ConnectedRuneTextureFrag)
{
	if (CurrGridState.Contains(Pos))
	{
		FGridCellData& CellData = CurrGridState[Pos];
		CellData.State = NewState;
		CellData.PlacedRuneID = RuneID;
		CellData.RuneTextureFrag = RuneTextureFrag;
		CellData.ConnectedRuneTextureFrag = ConnectedRuneTextureFrag;
	}
}

void UGS_ArcaneBoardManager::InitDataCache()
{
	RuneDataCache.Empty();
	GridLayoutCache.Empty();
	CacheRuneData();
	CacheGridLayouts();
	SetCurrClass(CurrClass);
}

void UGS_ArcaneBoardManager::CacheRuneData()
{
	if (!IsValid(RuneTable))
	{
		return;
	}

	TArray<FRuneTableRow*> RuneRows;
	RuneTable->GetAllRows<FRuneTableRow>(TEXT("CacheRuneData"), RuneRows);

	for (FRuneTableRow* Row : RuneRows)
	{
		if (Row)
		{
			RuneDataCache.Add(Row->RuneID, *Row);
		}
	}
}

void UGS_ArcaneBoardManager::CacheGridLayouts()
{
	if (!IsValid(GridLayoutTable))
	{
		return;
	}

	TArray<FGridLayoutTableRow*> GridRows;
	GridLayoutTable->GetAllRows<FGridLayoutTableRow>(TEXT("CacheGridLayouts"), GridRows);

	for (FGridLayoutTableRow* Row : GridRows)
	{
		if (Row && !Row->GridLayoutAsset.IsNull())
		{
			UGS_GridLayoutDataAsset* LoadedAsset = Row->GridLayoutAsset.LoadSynchronous();
			if (LoadedAsset)
			{
				GridLayoutCache.Add(LoadedAsset->CharacterClass, LoadedAsset);
			}
		}
	}
}

bool UGS_ArcaneBoardManager::GetRuneData(uint8 RuneID, FRuneTableRow& OutData)
{
	// 캐시에서 먼저 찾기
	if (RuneDataCache.Contains(RuneID))
	{
		OutData = RuneDataCache[RuneID];
		return true;
	}

	// 캐시에 없으면 테이블에서 찾아서 캐싱
	if (IsValid(RuneTable))
	{
		FString RowName = FString::FromInt(RuneID);
		FRuneTableRow* FoundRow = RuneTable->FindRow<FRuneTableRow>(*RowName, TEXT("GetRuneData"));

		if (FoundRow)
		{
			RuneDataCache.Add(RuneID, *FoundRow);
			OutData = *FoundRow;
			return true;
		}
	}

	return false;
}

bool UGS_ArcaneBoardManager::GetCellData(const FIntPoint& Pos, FGridCellData& OutCellData)
{
	if (CurrGridState.Contains(Pos))
	{
		OutCellData = CurrGridState[Pos];
		return true;
	}
	return false;
}

bool UGS_ArcaneBoardManager::GetRuneShape(uint8 RuneID, TArray<FIntPoint>& OutShape)
{
	FRuneTableRow RuneData;
	if (GetRuneData(RuneID, RuneData))
	{
		RuneData.RuneShape.GenerateKeyArray(OutShape);
		return true;
	}
	return false;
}

UTexture2D* UGS_ArcaneBoardManager::GetRuneTexture(uint8 RuneID)
{
	FRuneTableRow RuneData;
	if (GetRuneData(RuneID, RuneData))
	{
		return RuneData.RuneTexture.LoadSynchronous();
	}
	return nullptr;
}

bool UGS_ArcaneBoardManager::GetFragmentedRuneTexture(uint8 RuneID, TMap<FIntPoint, UTexture2D*>& OutShape)
{
	FRuneTableRow RuneData;
	if (GetRuneData(RuneID, RuneData))
	{
		OutShape = RuneData.RuneShape;
		return true;
	}
	return false;
}

bool UGS_ArcaneBoardManager::GetConnectedFragmentedRuneTexture(uint8 RuneID, TMap<FIntPoint, UTexture2D*>& OutShape)
{
	FRuneTableRow RuneData;
	if (GetRuneData(RuneID, RuneData))
	{
		OutShape = RuneData.ConnectedRuneShape;
		return true;
	}
	return false;
}