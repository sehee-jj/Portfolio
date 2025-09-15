// Copyright Epic Games, Inc. All Rights Reserved.

#include "Entity.h"
#include "EdenGameState.h"

namespace
{
	constexpr int32 DEFAULT_MAX_HEALTH = 100;
}

AEntity::AEntity()
{
	MaxHealth = DEFAULT_MAX_HEALTH;
	Health = MaxHealth;
}

int32 AEntity::GetHealth() const
{
	return Health;
}

int32 AEntity::GetMaxHealth() const
{
	return MaxHealth;
}

float AEntity::TakeDamage(float AmountDamage, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float Damage = Super::TakeDamage(AmountDamage, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Clamp<int32>(Health - Damage, 0, MaxHealth);

	return Damage;
}