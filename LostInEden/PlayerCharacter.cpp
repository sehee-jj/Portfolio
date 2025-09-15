// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerCharacter.h"
#include "MainPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GunManager.h"
#include "Gun.h"
#include "Pistol.h"
#include "Rifle.h"
#include "Shotgun.h"
#include "Shield.h"
#include "HealingItem.h"
#include "Magazine.h"
#include "EdenGameState.h"

namespace
{
	// 이동 관련 상수
	constexpr float NORMAL_SPEED = 600.0f;
	constexpr float SPRINT_MULTIPLIER = 1.5f;
	constexpr float CAMERA_ARM_LENGTH = 300.0f;

	// 체력/방어 관련 상수
	constexpr int32 PLAYER_MAX_HEALTH = 200;
	constexpr int32 MAX_SHIELD_GAUGE = 50;

	// 타이머 관련 상수
	constexpr float WEAPON_CHANGE_DELAY = 2.0f;
	constexpr float DEATH_DELAY = 2.5f;

	// 사운드 경로
	const FString ZOOM_IN_SOUND_PATH = TEXT("/Game/Sounds/Lyra_ZoomIn_01.Lyra_ZoomIn_01");
	const FString ZOOM_OUT_SOUND_PATH = TEXT("/Game/Sounds/Lyra_ZoomOut_01.Lyra_ZoomOut_01");
}

APlayerCharacter::APlayerCharacter()
{
	// 카메라 시스템
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = CAMERA_ARM_LENGTH;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	// 무기 시스템
	GunManager = CreateDefaultSubobject<UGunManager>(TEXT("GunManager"));

	// 충돌 시스템
	BoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollisionComp->SetupAttachment(RootComponent);
	BoxCollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::OnOverlapBegin);
	BoxCollisionComp->OnComponentEndOverlap.AddDynamic(this, &APlayerCharacter::OnOverlapEnd);

	// 이동 시스템
	NormalSpeed = NORMAL_SPEED;
	SprintSpeedMultiplier = SPRINT_MULTIPLIER;
	SprintSpeed = NormalSpeed * SprintSpeedMultiplier;
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// 플레이어 스탯
	MaxHealth = PLAYER_MAX_HEALTH;
	Health = MaxHealth;
	MaxShieldGauge = MAX_SHIELD_GAUGE;
	ShieldGauge = 0;

	// 사운드
	ZoomInSound = LoadObject<USoundBase>(GetTransientPackage(), *ZOOM_IN_SOUND_PATH);
	ZoomOutSound = LoadObject<USoundBase>(GetTransientPackage(), *ZOOM_OUT_SOUND_PATH);

	// 초기 상태
	bIsDead = false;
	bCanChangeGun = true;
	CurrWeapon = nullptr;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기본 권총 장착
	EquipWeapon(EGunType::PISTOL);

	// 아이템 인스턴스 생성
	HealPotion = GetWorld()->SpawnActor<AHealingItem>(AHealingItem::StaticClass());
	Shield = GetWorld()->SpawnActor<AShield>(AShield::StaticClass());
}

// === 상태 정보 접근자 ===
int32 APlayerCharacter::GetShieldGauge() const
{
	return ShieldGauge;
}

int32 APlayerCharacter::GetMaxShieldGauge() const
{
	return MaxShieldGauge;
}

int32 APlayerCharacter::GetHealPotionCnt() const
{
	return HealPotion ? HealPotion->GetCount() : 0;
}

AGun* APlayerCharacter::GetCurrentWeapon()
{
	return CurrWeapon;
}

TMap<EItemType, int32>& APlayerCharacter::GetAmmoInventory()
{
	return AmmoInventory;
}

// === 상태 설정자 ===
void APlayerCharacter::SetHealth(int32 HealthAmount)
{
	Health = HealthAmount;
}

void APlayerCharacter::SetShieldGauge(int32 ShieldAmount)
{
	ShieldGauge = ShieldAmount;
}

// === 전투 시스템 ===
float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = 0.0f;

	// 쉴드가 있으면 쉴드부터 차감
	if (ShieldGauge > 0)
	{
		ShieldGauge -= DamageAmount;
		if (ShieldGauge < 0)
		{
			ActualDamage = -ShieldGauge;
			ShieldGauge = 0;
		}
	}
	else
	{
		ActualDamage = DamageAmount;
	}

	Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	UpdateUI();

	// 사망 처리
	if (Health == 0 && !bIsDead)
	{
		bIsDead = true;
		DisableInput(Cast<APlayerController>(GetController()));
		GetWorldTimerManager().SetTimer(DelayDieState, this, &APlayerCharacter::Die, DEATH_DELAY, false);
	}

	return ActualDamage;
}

void APlayerCharacter::StartAttack()
{
	if (!CurrWeapon) return;

	bCanChangeGun = false;

	switch (CurrWeapon->GetGunType())
	{
	case EGunType::RIFLE:
		if (ARifle* Rifle = Cast<ARifle>(CurrWeapon))
		{
			Rifle->StartAutoFire();
		}
		break;
	default:
		CurrWeapon->Fire();
		break;
	}

	UpdateUI();
}

void APlayerCharacter::StopAttack()
{
	if (CurrWeapon)
	{
		if (ARifle* Rifle = Cast<ARifle>(CurrWeapon))
		{
			Rifle->StopAutoFire();
		}
	}

	UpdateUI();
	bCanChangeGun = true;
}

void APlayerCharacter::ReloadAmmo()
{
	if (CurrWeapon)
	{
		CurrWeapon->Reload();
	}
}

// === 무기 시스템 ===
void APlayerCharacter::EquipWeapon(EGunType GunType)
{
	if (!GunManager || !GunManager->HasWeapon(GunType))
	{
		return;
	}

	// 현재 무기 숨기기
	if (CurrWeapon)
	{
		CurrWeapon->SetActorHiddenInGame(true);
		CurrWeapon->SetActorEnableCollision(false);
		CurrWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	// 새 무기 장착
	CurrWeapon = GunManager->GetWeapon(GunType);
	if (CurrWeapon)
	{
		CurrWeapon->SetActorHiddenInGame(false);
		CurrWeapon->SetActorEnableCollision(false);

		const FName SocketName = GetWeaponSocketName(GunType);
		if (GetMesh()->DoesSocketExist(SocketName))
		{
			CurrWeapon->AttachToComponent(GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}

		CurrWeapon->SetOwner(this);
		UpdateUI();
	}
}

FName APlayerCharacter::GetWeaponSocketName(EGunType GunType) const
{
	switch (GunType)
	{
	case EGunType::PISTOL:
		return FName("Pistol_Socket");
	case EGunType::RIFLE:
		return FName("Rifle_Socket");
	case EGunType::SHOTGUN:
		return FName("Shotgun_Socket");
	default:
		return NAME_None;
	}
}

// === 아이템 시스템 ===
void APlayerCharacter::AddItem(AItem* Item)
{
	if (!Item) return;

	// 무기 아이템 처리
	if (AGun* Gun = Cast<AGun>(Item))
	{
		if (GunManager && !GunManager->HasWeapon(Gun->GetGunType()))
		{
			GunManager->AcquireWeapon(Gun->GetGunType());
		}
		return;
	}

	// 탄약 아이템 처리
	if (AMagazine* Magazine = Cast<AMagazine>(Item))
	{
		const EItemType AmmoType = Magazine->GetAmmoType();
		const int32 CurrentAmount = AmmoInventory.FindRef(AmmoType);
		AmmoInventory.Add(AmmoType, CurrentAmount + Magazine->GetAmmoAmount());
		return;
	}

	// 기타 아이템 처리
	switch (Item->GetItemType())
	{
	case EItemType::SHIELD:
		if (Shield)
		{
			Shield->Use(this);
		}
		break;
	case EItemType::HEALINGITEM:
		if (HealPotion)
		{
			HealPotion->IncrementCount(1);
		}
		break;
	}
}

void APlayerCharacter::UseItem()
{
	if (HealPotion)
	{
		HealPotion->Use(this);
		UpdateUI();
	}
}

// === 사운드 시스템 ===
void APlayerCharacter::PlayZoomInSound()
{
	if (ZoomInSound)
	{
		UGameplayStatics::PlaySound2D(this, ZoomInSound);
	}
}

void APlayerCharacter::PlayZoomOutSound()
{
	if (ZoomOutSound)
	{
		UGameplayStatics::PlaySound2D(this, ZoomOutSound);
	}
}

// === 게임 상태 관리 ===
void APlayerCharacter::Die()
{
	if (AEdenGameState* GameState = Cast<AEdenGameState>(UGameplayStatics::GetGameState(this)))
	{
		GameState->OnGameOver();
	}
}

void APlayerCharacter::UpdateUI()
{
	if (AMainPlayerController* PC = Cast<AMainPlayerController>(GetController()))
	{
		PC->UpdateHUD();
	}
}

// === 오버랩 이벤트 ===
void APlayerCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		if (AItem* Item = Cast<AItem>(OtherActor))
		{
			OverlappingItemList.Add(Item);
		}
	}
}

void APlayerCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OverlappingItemList.Remove(Cast<AItem>(OtherActor));
}

// === 입력 처리 함수들 ===
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AMainPlayerController* PlayerController = Cast<AMainPlayerController>(GetController()))
		{
			if (PlayerController->MoveAction)
			{
				EnhancedInput->BindAction(
					PlayerController->MoveAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::Move);
			}

			if (PlayerController->JumpAction)
			{
				EnhancedInput->BindAction(
					PlayerController->JumpAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::StartJump);

				EnhancedInput->BindAction(
					PlayerController->JumpAction, 
					ETriggerEvent::Completed, 
					this, 
					&APlayerCharacter::StopJump);
			}

			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(
					PlayerController->LookAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::Look);
			}

			if (PlayerController->SprintAction)
			{
				EnhancedInput->BindAction(
					PlayerController->SprintAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::StartSprint);

				EnhancedInput->BindAction(
					PlayerController->SprintAction, 
					ETriggerEvent::Completed, 
					this, 
					&APlayerCharacter::StopSprint);
			}

			if (PlayerController->CrouchAction)
			{
				EnhancedInput->BindAction(
					PlayerController->CrouchAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::DoCrouch);
			}

			if (PlayerController->AttackAction)
			{
				EnhancedInput->BindAction(
					PlayerController->AttackAction, 
					ETriggerEvent::Started, 
					this, 
					&APlayerCharacter::StartAttack);

				EnhancedInput->BindAction(
					PlayerController->AttackAction, 
					ETriggerEvent::Completed, 
					this, 
					&APlayerCharacter::StopAttack);

				EnhancedInput->BindAction(
					PlayerController->AttackAction, 
					ETriggerEvent::Canceled, 
					this, 
					&APlayerCharacter::StopAttack);
			}

			if (PlayerController->ReloadAction)
			{
				EnhancedInput->BindAction(
					PlayerController->ReloadAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::ReloadAmmo);
			}

			if (PlayerController->ChangeGunAction)
			{
				EnhancedInput->BindAction(
					PlayerController->ChangeGunAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::SelectWeapon);
			}

			if (PlayerController->PickupAction)
			{
				EnhancedInput->BindAction(
					PlayerController->PickupAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::PickupItem);
			}

			if (PlayerController->UseItemAction)
			{
				EnhancedInput->BindAction(
					PlayerController->UseItemAction, 
					ETriggerEvent::Triggered, 
					this, 
					&APlayerCharacter::UseItem);
			}

			if (PlayerController->ZoomAction)
			{
				EnhancedInput->BindAction(
					PlayerController->ZoomAction, 
					ETriggerEvent::Started, 
					this, 
					&APlayerCharacter::PlayZoomInSound);

				EnhancedInput->BindAction(
					PlayerController->ZoomAction, 
					ETriggerEvent::Completed, 
					this, 
					&APlayerCharacter::PlayZoomOutSound);
			}
		}
	}
}

// === 이동 및 액션 입력 함수들 ===
void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.X);
	}

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(GetActorRightVector(), MoveInput.Y);
	}
}

void APlayerCharacter::StartJump(const FInputActionValue& Value)
{
	if (Controller && Value.Get<bool>())
	{
		Jump();
	}
}

void APlayerCharacter::StopJump(const FInputActionValue& Value)
{
	if (Controller && !Value.Get<bool>())
	{
		StopJumping();
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void APlayerCharacter::StartSprint(const FInputActionValue& Value)
{
	if (Controller && GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void APlayerCharacter::StopSprint(const FInputActionValue& Value)
{
	if (Controller && GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	}
}

void APlayerCharacter::DoCrouch(const FInputActionValue& Value)
{
	CanCrouch() ? Crouch() : UnCrouch();
}

void APlayerCharacter::SelectWeapon(const FInputActionValue& Value)
{
	if (!bCanChangeGun || !GunManager)
	{
		return;
	}

	const TArray<EGunType>& GunList = GunManager->GetOwnedGunList();
	if (GunList.Num() <= 1)
	{
		return;
	}

	const EGunType CurrentGunType = CurrWeapon ? CurrWeapon->GetGunType() : EGunType::PISTOL;
	int32 CurrentIndex = GunList.Find(CurrentGunType);
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = 0;
	}

	const int32 Direction = FMath::RoundToInt(Value.Get<float>());
	const int32 NextIndex = (CurrentIndex + Direction + GunList.Num()) % GunList.Num();

	EquipWeapon(GunList[NextIndex]);

	bCanChangeGun = false;
	GetWorldTimerManager().SetTimer(InputDelayTimerHandle, this, &APlayerCharacter::ResetInput, WEAPON_CHANGE_DELAY, false);
}

void APlayerCharacter::PickupItem(const FInputActionValue& Value)
{
	if (!OverlappingItemList.IsEmpty())
	{
		AItem* OverlappingItem = OverlappingItemList[0];
		OverlappingItemList.RemoveAt(0);
		AddItem(OverlappingItem);
		OverlappingItem->Destroy();
	}

	UpdateUI();
}

// === 내부 유틸리티 함수 ===
void APlayerCharacter::ResetInput()
{
	bCanChangeGun = true;
}