// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntWeapon.h"
#include "Character/HuntCharacter.h"
#include "Weapon/HuntProjectile.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// shortcut: AR4의 총구 뼈 이름, 뼈 이름이 다른 총이 생기면 WeaponDefinition으로 옮긴다
	const FName MuzzleBoneName(TEXT("b_gun_muzzleflash"));
	// 조준점을 찾는 카메라 트레이스 길이(cm). 아무것도 안 맞으면 이 거리의 점을 조준한다
	constexpr float AimTraceDistance = 100000.f;
	// 지향사격 왼손 소켓 (총 메시마다 추가)
	const FName LeftHandSocketName(TEXT("Socket_LeftHand"));
	// 조준 자세 애니메이션의 자연 그립 위치. 총에 없으면 지향사격 소켓을 그대로 쓴다
	const FName LeftHandADSSocketName(TEXT("Socket_LeftHand_ADS"));
	// 1인칭 그립 포즈의 왼손목(hand_l) 위치. 지향사격·조준 그립 포즈가 총 기준 같은 곳을 잡으므로 하나만 둔다
	const FName FirstPersonLeftHandSocketName(TEXT("Socket_LeftHand_FP"));
}

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
	if (Definition->FireMode == EHuntFireMode::Single)
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

void AHuntWeapon::Shoot()
{
	const APawn* OwnerPawn = GetInstigator();
	if (!Definition || !Definition->ProjectileClass || !OwnerPawn)
	{
		return;
	}
	LastFireTime = GetWorld()->GetTimeSeconds();

	const FVector SpawnLocation = GetSpawnLocation(OwnerPawn);
	const AHuntCharacter* Character = Cast<AHuntCharacter>(OwnerPawn);
	const float SpreadRad = FMath::DegreesToRadians(Character ? Character->GetCurrentSpread() : 0.f);
	const FVector Direction = FMath::VRandCone(GetAimDirection(OwnerPawn, SpawnLocation), SpreadRad);
	SpawnProjectile(SpawnLocation, Direction.Rotation());
	PlayFireSound(SpawnLocation);

	// 탄은 반동 전 조준점으로 나가고, 다음 발부터 올라간 조준선을 따른다
	ApplyRecoilToOwner();
	bFirstShot = false;
}

FVector AHuntWeapon::GetAimDirection(const APawn* OwnerPawn, const FVector& MuzzleLocation) const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetOwnerViewPoint(OwnerPawn, ViewLocation, ViewRotation);

	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * AimTraceDistance;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponAim), false, OwnerPawn);
	Params.AddIgnoredActor(this);
	const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params) ? Hit.ImpactPoint : TraceEnd;

	// 벽에 붙어 총구가 조준점보다 앞에 나가 있으면 뒤로 쏘지 않도록 시선 방향을 쓴다
	const FVector Direction = (AimPoint - MuzzleLocation).GetSafeNormal();
	const FVector AimDirection = (Direction | ViewDirection) > 0.f ? Direction : ViewDirection;

	// 벽에 막혀 총을 내린 만큼 크로스헤어 대신 총구가 향한 방향으로 쏜다
	// shortcut: 총열이 총 메시의 +Y라고 가정(AR4), 축이 다른 총이 생기면 총구 소켓 방향으로 바꾼다
	const AHuntCharacter* Character = Cast<AHuntCharacter>(OwnerPawn);
	const float WallBlockAlpha = Character ? Character->GetWeaponLowerAlpha() : 0.f;
	return FMath::Lerp(AimDirection, Mesh->GetRightVector(), WallBlockAlpha).GetSafeNormal();
}

FVector AHuntWeapon::GetSpawnLocation(const APawn* OwnerPawn) const
{
	// 1인칭 총은 줄여 그려 벽 앞에 보여도, 실제 총구(소켓)는 원래 크기 위치라 벽 너머에 있을 수 있다
	const FVector MuzzleLocation = Mesh->GetSocketLocation(MuzzleBoneName);
	FVector ViewLocation;
	FRotator ViewRotation;
	GetOwnerViewPoint(OwnerPawn, ViewLocation, ViewRotation);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponMuzzle), false, OwnerPawn);
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, MuzzleLocation, ECC_Visibility, Params))
	{
		// 벽 표면에서 살짝 띄워 생기자마자 그 벽에 맞게 한다
		return Hit.ImpactPoint + Hit.ImpactNormal * 2.f;
	}
	return MuzzleLocation;
}

void AHuntWeapon::GetOwnerViewPoint(const APawn* OwnerPawn, FVector& OutLocation, FRotator& OutRotation)
{
	if (const AController* Controller = OwnerPawn->GetController())
	{
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else
	{
		OwnerPawn->GetActorEyesViewPoint(OutLocation, OutRotation);
	}
}

void AHuntWeapon::SpawnProjectile(const FVector& Location, const FRotator& Rotation)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AHuntProjectile>(Definition->ProjectileClass, Location, Rotation, Params);
}

void AHuntWeapon::PlayFireSound(const FVector& Location)
{
	if (Definition->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Definition->FireSound, Location);
	}
}

void AHuntWeapon::ApplyRecoilToOwner()
{
	AHuntCharacter* Character = Cast<AHuntCharacter>(GetInstigator());
	if (!Character)
	{
		return;
	}

	const float HipMultiplier = FMath::Lerp(Definition->HipFireRecoilMultiplier, 1.f, Character->GetAimBlend());
	const float Multiplier = (bFirstShot ? Definition->FirstShotRecoilMultiplier : 1.f) * HipMultiplier;
	const float Yaw = FMath::FRandRange(-Definition->HorizontalRecoil, Definition->HorizontalRecoil);
	Character->AddRecoil(FRotator(Definition->VerticalRecoil, Yaw, 0.f) * Multiplier);
	Character->AddWeaponKick();
}
