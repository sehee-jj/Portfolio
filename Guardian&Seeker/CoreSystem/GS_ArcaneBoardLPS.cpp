// Copyright Epic Games, Inc. All Rights Reserved.

#include "RuneSystem/GS_ArcaneBoardLPS.h"
#include "RuneSystem/GS_ArcaneBoardManager.h"
#include "RuneSystem/GS_EnumUtils.h"
#include "UI/RuneSystem/GS_ArcaneBoardWidget.h"
#include "RuneSystem/GS_ArcaneBoardSaveGame.h"
#include "Character/GS_Character.h"
#include "Kismet/GameplayStatics.h"

void UGS_ArcaneBoardLPS::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentPresetIndex = 1;
}

ECharacterClass UGS_ArcaneBoardLPS::GetPlayerCharacterClass() const
{
    if (APlayerController* PC = GetLocalPlayer()->GetPlayerController(GetWorld()))
    {
        if (AGS_PlayerState* GSPlayerState = PC->GetPlayerState<AGS_PlayerState>())
        {
            if (GSPlayerState->CurrentPlayerRole == EPlayerRole::PR_Seeker)
            {
                ESeekerJob SeekerJob = GSPlayerState->CurrentSeekerJob;
                return MapSeekerJobToCharacterClass(SeekerJob);
            }
        }
    }
    return ECharacterClass::Ares;
}

void UGS_ArcaneBoardLPS::OnPlayerJobChanged(ESeekerJob SeekerJob)
{
    if (!IsValid(BoardManager))
    {
        GetOrCreateBoardManager();
    }

    ECharacterClass NewCharacterClass = MapSeekerJobToCharacterClass(SeekerJob);

    if (IsValid(BoardManager) && BoardManager->GetCurrClass() != NewCharacterClass)
    {
        BoardManager->SetCurrClass(NewCharacterClass);
        LoadBoardConfig();
    }
}

void UGS_ArcaneBoardLPS::InitializeRunes()
{
    if (!IsValid(BoardManager))
    {
        GetOrCreateBoardManager();
    }
    LoadBoardConfig();
}

void UGS_ArcaneBoardLPS::RefreshBoardForCurrCharacter()
{
    ECharacterClass CurrentClass = GetPlayerCharacterClass();

    if (!IsValid(BoardManager))
    {
        GetOrCreateBoardManager();
    }

    if (IsValid(BoardManager) && BoardManager->GetCurrClass() != CurrentClass)
    {
        BoardManager->SetCurrClass(CurrentClass);
        LoadBoardConfig();
    }
}

ECharacterClass UGS_ArcaneBoardLPS::MapSeekerJobToCharacterClass(ESeekerJob SeekerJob) const
{
    switch (SeekerJob)
    {
    case ESeekerJob::Ares: return ECharacterClass::Ares;
    case ESeekerJob::Chan: return ECharacterClass::Chan;
    case ESeekerJob::Merci: return ECharacterClass::Merci;
    default:
        return ECharacterClass::Ares;
    }
}

void UGS_ArcaneBoardLPS::ApplyBoardChanges()
{
    if (BoardManager)
    {
        BoardManager->ApplyChanges();
        SaveBoardConfig();
    }
}

void UGS_ArcaneBoardLPS::OnBoardStatsChanged(const FArcaneBoardStats& NewStats)
{
    RuneSystemStats = NewStats;

    if (CurrentUIWidget.IsValid())
    {
        CurrentUIWidget->OnStatsChanged(RuneSystemStats);
    }
}

bool UGS_ArcaneBoardLPS::HasUnsavedChanges() const
{
    if (IsValid(BoardManager))
    {
        return BoardManager->bHasUnsavedChanges;
    }
    return false;
}

void UGS_ArcaneBoardLPS::SaveBoardConfig(int32 PresetIndex)
{
    if (!IsValid(BoardManager))
    {
        return;
    }

    int32 TargetPresetIndex = (PresetIndex == -1) ? CurrentPresetIndex : PresetIndex;
    if (TargetPresetIndex < 1 || TargetPresetIndex > 3)
    {
        return;
    }

    UGS_ArcaneBoardSaveGame* SaveGameInstance = GetOrCreateSaveGame();
    if (!SaveGameInstance)
    {
        return;
    }

    ECharacterClass CurrClass = BoardManager->CurrClass;

    if (!SaveGameInstance->SavedRunesByClass.Contains(CurrClass))
    {
        SaveGameInstance->SavedRunesByClass.Add(CurrClass, FArcaneBoardPresets());
    }

    FArcaneBoardPresets& ClassPresets = SaveGameInstance->SavedRunesByClass[CurrClass];
    TArray<FPlacedRuneInfo>* TargetPreset = GetPresetArray(ClassPresets, TargetPresetIndex);

    if (TargetPreset)
    {
        *TargetPreset = BoardManager->PlacedRunes;
        ClassPresets.LastUsedPresetIndex = TargetPresetIndex;
        CurrentPresetIndex = TargetPresetIndex;
    }

    SaveGameInstance->OwnedRuneIDs = OwnedRuneIDs;

    const FString SaveSlotName = TEXT("ArcaneBoardSave");
    const int32 UserIdx = 0;
    bool bSaveSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveSlotName, UserIdx);

    if (bSaveSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("SaveBoardConfig: 저장 성공 - 클래스: %s, 프리셋: %d, 룬 개수: %d"),
            *UGS_EnumUtils::GetEnumAsString(CurrClass),
            TargetPresetIndex,
            BoardManager->PlacedRunes.Num());

        BoardManager->bHasUnsavedChanges = false;
    }
}

void UGS_ArcaneBoardLPS::LoadBoardConfig(int32 PresetIndex)
{
    if (!IsValid(BoardManager))
    {
        return;
    }

    const FString SaveSlotName = TEXT("ArcaneBoardSave");
    const int32 UserIndex = 0;

    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("LoadBoardConfig: 세이브 파일이 존재하지 않습니다."));
        CurrentPresetIndex = 1;
        return;
    }

    UGS_ArcaneBoardSaveGame* LoadedSaveGame = Cast<UGS_ArcaneBoardSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

    if (!LoadedSaveGame)
    {
        UE_LOG(LogTemp, Error, TEXT("LoadBoardConfig: 세이브 파일 로드 실패"));
        return;
    }

    ECharacterClass CurrClass = BoardManager->CurrClass;
    LoadRuneInventory(LoadedSaveGame);

    if (!LoadedSaveGame->SavedRunesByClass.Contains(CurrClass))
    {
        UE_LOG(LogTemp, Log, TEXT("LoadBoardConfig: 현재 직업(%s)에 대한 프리셋 데이터가 없습니다."),
            *UGS_EnumUtils::GetEnumAsString(CurrClass));
        CurrentPresetIndex = 1;
        BoardManager->LoadSavedData(CurrClass, TArray<FPlacedRuneInfo>());
        return;
    }

    const FArcaneBoardPresets& ClassPresets = LoadedSaveGame->SavedRunesByClass[CurrClass];
    int32 TargetPresetIndex = DetermineTargetPresetIndex(PresetIndex, ClassPresets);

    if (TargetPresetIndex < 1 || TargetPresetIndex > 3)
    {
        return;
    }

    CurrentPresetIndex = TargetPresetIndex;

    const TArray<FPlacedRuneInfo>* TargetPreset = GetPresetArray(ClassPresets, TargetPresetIndex);
    if (TargetPreset)
    {
        BoardManager->LoadSavedData(CurrClass, *TargetPreset);

        UE_LOG(LogTemp, Log, TEXT("LoadBoardConfig: 로드 성공 - 클래스: %s, 프리셋: %d, 룬 개수: %d"),
            *UGS_EnumUtils::GetEnumAsString(CurrClass),
            TargetPresetIndex,
            TargetPreset->Num());
    }

    BoardManager->bHasUnsavedChanges = false;
}

bool UGS_ArcaneBoardLPS::IsPresetEmpty(int32 PresetIndex) const
{
    if (PresetIndex < 1 || PresetIndex > 3)
    {
        return true;
    }

    const FString SaveSlotName = TEXT("ArcaneBoardSave");
    const int32 UserIdx = 0;

    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIdx))
    {
        return true;
    }

    UGS_ArcaneBoardSaveGame* LoadedSaveGame = Cast<UGS_ArcaneBoardSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIdx));

    if (!LoadedSaveGame || !IsValid(BoardManager))
    {
        return true;
    }

    ECharacterClass CurrClass = BoardManager->CurrClass;

    if (!LoadedSaveGame->SavedRunesByClass.Contains(CurrClass))
    {
        return true;
    }

    const FArcaneBoardPresets& ClassPresets = LoadedSaveGame->SavedRunesByClass[CurrClass];
    const TArray<FPlacedRuneInfo>* TargetPreset = GetPresetArray(ClassPresets, PresetIndex);

    return !TargetPreset || TargetPreset->Num() == 0;
}

int32 UGS_ArcaneBoardLPS::GetCurrentPresetIndex() const
{
    return CurrentPresetIndex;
}

UGS_ArcaneBoardManager* UGS_ArcaneBoardLPS::GetOrCreateBoardManager()
{
    if (!IsValid(BoardManager))
    {
        BoardManager = NewObject<UGS_ArcaneBoardManager>(this);
        BoardManager->OnStatsChanged.AddDynamic(this, &UGS_ArcaneBoardLPS::OnBoardStatsChanged);

        ECharacterClass CurrPlayerClass = GetPlayerCharacterClass();
        BoardManager->SetCurrClass(CurrPlayerClass);
    }

    return BoardManager;
}

TArray<uint8> UGS_ArcaneBoardLPS::GetOwnedRunes() const
{
    return OwnedRuneIDs.Array();
}

void UGS_ArcaneBoardLPS::AddRuneToInventory(uint8 RuneID)
{
    if (RuneID > 0)
    {
        OwnedRuneIDs.Add(RuneID);
        UE_LOG(LogTemp, Log, TEXT("룬 획득: ID=%d, 총 소유 룬 개수: %d"), RuneID, OwnedRuneIDs.Num());

        // 변경사항 저장
        SaveBoardConfig();
    }
}

void UGS_ArcaneBoardLPS::InitializeTestRunes()
{
    OwnedRuneIDs.Empty();
    for (uint8 i = 1; i <= 8; ++i)
    {
        OwnedRuneIDs.Add(i);
    }
}

void UGS_ArcaneBoardLPS::SetCurrUIWidget(UGS_ArcaneBoardWidget* Widget)
{
    CurrentUIWidget = Widget;

    if (Widget)
    {
        Widget->OnStatsChanged(RuneSystemStats);
    }
}

void UGS_ArcaneBoardLPS::ClearCurrUIWidget()
{
    CurrentUIWidget = nullptr;
}

void UGS_ArcaneBoardLPS::LoadRuneInventory(UGS_ArcaneBoardSaveGame* SaveGame)
{
    if (SaveGame->OwnedRuneIDs.Num() > 0)
    {
        OwnedRuneIDs = SaveGame->OwnedRuneIDs;
        UE_LOG(LogTemp, Log, TEXT("룬 인벤토리 로드 성공: %d개 룬"), OwnedRuneIDs.Num());
    }
    else
    {
        InitializeTestRunes();
    }
}

int32 UGS_ArcaneBoardLPS::DetermineTargetPresetIndex(int32 RequestedIndex, const FArcaneBoardPresets& ClassPresets) const
{
    if (RequestedIndex == -1)
    {
        return (ClassPresets.LastUsedPresetIndex >= 1 && ClassPresets.LastUsedPresetIndex <= 3)
            ? ClassPresets.LastUsedPresetIndex : 1;
    }
    else
    {
        return RequestedIndex;
    }
}

UGS_ArcaneBoardSaveGame* UGS_ArcaneBoardLPS::GetOrCreateSaveGame()
{
    const FString SaveSlotName = TEXT("ArcaneBoardSave");
    const int32 UserIdx = 0;

    UGS_ArcaneBoardSaveGame* SaveGameInstance = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIdx))
    {
        SaveGameInstance = Cast<UGS_ArcaneBoardSaveGame>(
            UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIdx));
    }

    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<UGS_ArcaneBoardSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UGS_ArcaneBoardSaveGame::StaticClass()));
    }

    return SaveGameInstance;
}

const TArray<FPlacedRuneInfo>* UGS_ArcaneBoardLPS::GetPresetArray(const FArcaneBoardPresets& Presets, int32 PresetIndex) const
{
    switch (PresetIndex)
    {
    case 1: return &Presets.Preset1;
    case 2: return &Presets.Preset2;
    case 3: return &Presets.Preset3;
    default: return nullptr;
    }
}

TArray<FPlacedRuneInfo>* UGS_ArcaneBoardLPS::GetPresetArray(FArcaneBoardPresets& Presets, int32 PresetIndex)
{
    return const_cast<TArray<FPlacedRuneInfo>*>(
        static_cast<const UGS_ArcaneBoardLPS*>(this)->GetPresetArray(
            static_cast<const FArcaneBoardPresets&>(Presets), PresetIndex));
}