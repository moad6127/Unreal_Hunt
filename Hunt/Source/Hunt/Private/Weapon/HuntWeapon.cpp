// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntWeapon.h"
#include "Character/HuntCharacter.h"
#include "Weapon/HuntProjectile.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AHuntWeapon::AHuntWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Mesh);
}

void AHuntWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 레벨 배치·BP·스폰 모두 Definition의 메시를 따르도록 여기서 설정
	Mesh->SetSkeletalMesh(Definition ? Definition->Mesh : nullptr);
	if (Definition)
	{
		FireMode = Definition->FireMode;
	}
}

FVector AHuntWeapon::GetLeftHandGripLocation(float AimAlpha) const
{
	const FVector HipGrip = Mesh->GetSocketLocation(LeftHandSocketName);
	const FVector ADSGrip = Mesh->DoesSocketExist(LeftHandADSSocketName) ? Mesh->GetSocketLocation(LeftHandADSSocketName) : HipGrip;
	return FMath::Lerp(HipGrip, ADSGrip, AimAlpha);
}

bool AHuntWeapon::GetFirstPersonLeftHandGripLocation(FVector& OutLocation) const
{
	if (!Mesh->DoesSocketExist(FirstPersonLeftHandSocketName))
	{
		return false;
	}
	OutLocation = Mesh->GetSocketLocation(FirstPersonLeftHandSocketName);
	return true;
}

void AHuntWeapon::StartFire()
{
	if (!Definition)
	{
		return;
	}
	bFirstShot = true;

	const double Remaining = LastFireTime + Definition->FireInterval - GetWorld()->GetTimeSeconds();
	if (FireMode == EHuntFireMode::Single)
	{
		if (Remaining <= 0.0)
		{
			Shoot();
		}
		return;
	}

	// 연발: 발사 간격이 남았으면 그만큼 기다렸다가 첫 발
	if (Remaining <= 0.0)
	{
		Shoot();
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AHuntWeapon::Shoot, Definition->FireInterval, true);
	}
	else
	{
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AHuntWeapon::Shoot, Definition->FireInterval, true, Remaining);
	}
}

void AHuntWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

bool AHuntWeapon::IsAutoFiring() const
{
	return GetWorldTimerManager().IsTimerActive(AutoFireTimer);
}

void AHuntWeapon::SetFireMode(EHuntFireMode NewFireMode)
{
	StopFire();
	FireMode = NewFireMode;
}

void AHuntWeapon::Shoot()
{
	const APawn* OwnerPawn = GetInstigator();
	if (!Definition || !Definition->ProjectileClass || !OwnerPawn)
	{
		return;
	}
	LastFireTime = GetWorld()->GetTimeSeconds();

	const FVector MuzzleLocation = Mesh->GetSocketLocation(MuzzleBoneName);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AHuntProjectile>(Definition->ProjectileClass, MuzzleLocation, OwnerPawn->GetControlRotation(), Params);

	if (Definition->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Definition->FireSound, MuzzleLocation);
	}

	// 탄은 반동 전 조준점으로 나가고, 다음 발부터 올라간 조준선을 따른다
	if (AHuntCharacter* Character = Cast<AHuntCharacter>(GetInstigator()))
	{
		const float HipMultiplier = FMath::Lerp(Definition->HipFireRecoilMultiplier, 1.f, Character->GetAimBlend());
		const float Multiplier = (bFirstShot ? Definition->FirstShotRecoilMultiplier : 1.f) * HipMultiplier;
		const float Yaw = FMath::FRandRange(-Definition->HorizontalRecoil, Definition->HorizontalRecoil);
		Character->AddRecoil(FRotator(Definition->VerticalRecoil, Yaw, 0.f) * Multiplier);
		Character->AddWeaponKick();
		Character->PlayFireAnimation();
	}
	bFirstShot = false;
}
