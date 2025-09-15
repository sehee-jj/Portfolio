// Copyright Epic Games, Inc. All Rights Reserved.t Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlobalEnum.h"
#include "Entity.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class AGun;
class AItem;
class UGunManager;
class AHealingItem;
class AShield;
class USoundBase;

/**
 * 플레이어 캐릭터 클래스
 * 이동, 전투, 아이템 관리 등 핵심 게임플레이 로직 담당
 */
UCLASS()
class LOSTINEDEN_API APlayerCharacter : public AEntity
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// 상태 정보 접근자
	int32 GetShieldGauge() const;
	int32 GetMaxShieldGauge() const;
	int32 GetHealPotionCnt() const;
	AGun* GetCurrentWeapon();
	TMap<EItemType, int32>& GetAmmoInventory();

	// 상태 설정자
	void SetHealth(int32 HealthAmount);
	void SetShieldGauge(int32 ShieldAmount);

	// 게임플레이 액션
	void StartAttack();
	void StopAttack();
	void ReloadAmmo();
	void UseItem();
	void EquipWeapon(EGunType GunType);
	void Die();

	// 아이템 시스템
	void AddItem(AItem* Item);

	// 사운드
	void PlayZoomInSound();
	void PlayZoomOutSound();

	// 오버랩 이벤트
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

private:
	// === 컴포넌트 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component|Camera", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component|Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component|Collision", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	UGunManager* GunManager;

	// === 무기 시스템 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	AGun* CurrWeapon;

	// === 방어 시스템 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (AllowPrivateAccess = "true"))
	int32 ShieldGauge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (AllowPrivateAccess = "true"))
	int32 MaxShieldGauge;

	AShield* Shield;

	// === 이동 시스템 ===
	float NormalSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;

	// === 인벤토리 시스템 ===
	TMap<EItemType, int32> AmmoInventory;
	AHealingItem* HealPotion;
	TArray<AItem*> OverlappingItemList;

	// === 상태 관리 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;
	bool bCanChangeGun;

	// === 타이머 ===
	FTimerHandle InputDelayTimerHandle;
	FTimerHandle DelayDieState;

	// === 사운드 ===
	USoundBase* ZoomInSound;
	USoundBase* ZoomOutSound;

	// === 입력 처리 함수 ===
	UFUNCTION()
	void Move(const FInputActionValue& Value);
	UFUNCTION()
	void StartJump(const FInputActionValue& Value);
	UFUNCTION()
	void StopJump(const FInputActionValue& Value);
	UFUNCTION()
	void Look(const FInputActionValue& Value);
	UFUNCTION()
	void StartSprint(const FInputActionValue& Value);
	UFUNCTION()
	void StopSprint(const FInputActionValue& Value);
	UFUNCTION()
	void DoCrouch(const FInputActionValue& Value);
	UFUNCTION()
	void SelectWeapon(const FInputActionValue& Value);
	UFUNCTION()
	void PickupItem(const FInputActionValue& Value);

	// === 내부 함수 ===
	void ResetInput();
	void UpdateUI();
	FName GetWeaponSocketName(EGunType GunType) const;
};