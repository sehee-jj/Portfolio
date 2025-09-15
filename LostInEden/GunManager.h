// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlobalEnum.h"
#include "GunManager.generated.h"

class AGun;

/**
 * 무기 시스템 관리 컴포넌트
 * 오브젝트 풀링 패턴을 사용하여 무기 인스턴스 관리
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOSTINEDEN_API UGunManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UGunManager();

	// 무기 소유 및 접근
	TArray<EGunType> GetOwnedGunList() const;
	AGun* GetWeapon(EGunType GunType) const;
	bool HasWeapon(EGunType GunType) const;
	void AcquireWeapon(EGunType GunType);

protected:
	virtual void BeginPlay() override;

private:
	// 무기 오브젝트 풀
	UPROPERTY()
	TMap<TEnumAsByte<EGunType>, AGun*> WeaponPool;

	// 플레이어가 소유한 무기 목록
	TSet<EGunType> OwnedWeapons;

	// 무기 풀 초기화
	void InitializeWeaponPool();
};