// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Entity.generated.h"

/**
 * 게임 내 모든 생명체의 기본 클래스
 * 체력 시스템과 데미지 처리를 담당
 */
UCLASS()
class LOSTINEDEN_API AEntity : public ACharacter
{
	GENERATED_BODY()

public:
	AEntity();

	// 체력 정보 접근자
	int32 GetHealth() const;
	int32 GetMaxHealth() const;

	// 데미지 처리
	virtual float TakeDamage(float AmountDamage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// 체력 시스템
	int32 Health;
	int32 MaxHealth;
};