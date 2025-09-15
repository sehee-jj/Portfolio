// Copyright Epic Games, Inc. All Rights Reserved.

#include "GunManager.h"
#include "Pistol.h"
#include "Rifle.h"
#include "Shotgun.h"

namespace
{
	// 무기 블루프린트 경로
	const FString PISTOL_BP_PATH = TEXT("/Game/Items/Blueprints/BP_Pistol.BP_Pistol_C");
	const FString RIFLE_BP_PATH = TEXT("/Game/Items/Blueprints/BP_Rifle.BP_Rifle_C");
	const FString SHOTGUN_BP_PATH = TEXT("/Game/Items/Blueprints/BP_Shotgun.BP_Shotgun_C");
}

UGunManager::UGunManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본 무기 설정
	OwnedWeapons.Add(EGunType::PISTOL);

	// 테스트용 추가 무기
	OwnedWeapons.Add(EGunType::RIFLE);
	OwnedWeapons.Add(EGunType::SHOTGUN);
}

void UGunManager::BeginPlay()
{
	Super::BeginPlay();
	InitializeWeaponPool();
}

void UGunManager::InitializeWeaponPool()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GunManager: World is null"));
		return;
	}

	// 무기 클래스 매핑
	const TMap<EGunType, FString> WeaponBlueprintPaths = {
		{EGunType::PISTOL, PISTOL_BP_PATH},
		{EGunType::RIFLE, RIFLE_BP_PATH},
		{EGunType::SHOTGUN, SHOTGUN_BP_PATH}
	};

	// 무기 인스턴스 생성 및 풀에 추가
	for (const auto& WeaponPair : WeaponBlueprintPaths)
	{
		const EGunType GunType = WeaponPair.Key;
		UClass* GunClass = LoadClass<AGun>(nullptr, *WeaponPair.Value);

		if (GunClass)
		{
			AGun* WeaponInstance = World->SpawnActor<AGun>(GunClass);
			if (WeaponInstance)
			{
				// 초기 상태: 숨김 및 충돌 비활성화
				WeaponInstance->SetActorHiddenInGame(true);
				WeaponInstance->SetActorEnableCollision(false);

				WeaponPool.Add(GunType, WeaponInstance);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GunManager: Failed to load weapon class for type %d"),
				static_cast<int32>(GunType));
		}
	}
}

TArray<EGunType> UGunManager::GetOwnedGunList() const
{
	return OwnedWeapons.Array();
}

AGun* UGunManager::GetWeapon(EGunType GunType) const
{
	if (AGun* const* FoundWeapon = WeaponPool.Find(GunType))
	{
		return *FoundWeapon;
	}

	UE_LOG(LogTemp, Warning, TEXT("무기 풀에서 해당 무기를 찾을 수 없음: %d"), GunType);
	return nullptr;
}

void UGunManager::AcquireWeapon(EGunType GunType)
{
	if (!HasWeapon(GunType))
	{
		OwnedWeapons.Add(GunType);
	}
}

bool UGunManager::HasWeapon(EGunType GunType) const
{
	return OwnedWeapons.Contains(GunType);
}