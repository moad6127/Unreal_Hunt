// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntWeapon.h"
#include "Character/HuntCharacter.h"
#include "Weapon/Attachment/GripAttachmentDefinition.h"
#include "Weapon/HuntProjectile.h"
#include "Weapon/Attachment/WeaponAttachmentComponent.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// shortcut: AR4·M4 공통 총구 뼈 이름, 뼈 이름이 다른 총이 생기면 WeaponDefinition으로 옮긴다
	const FName MuzzleBoneName(TEXT("b_gun_muzzleflash"));
	// 총구 부착물 메시의 끝(탄이 나가는 곳). 총 메시의 같은 이름 소켓은 부착물이 붙는 자리라 다르다
	const FName MuzzleSocketName(TEXT("Socket_Muzzle"));
	// 조준점을 찾는 카메라 트레이스 길이(cm). 아무것도 안 맞으면 이 거리의 점을 조준한다
	constexpr float AimTraceDistance = 100000.f;
	// 지향사격 왼손 소켓 (총 메시마다 추가)
	const FName LeftHandSocketName(TEXT("Socket_LeftHand"));
	// 조준 자세 애니메이션의 자연 그립 위치. 총에 없으면 지향사격 소켓을 그대로 쓴다
	const FName LeftHandADSSocketName(TEXT("Socket_LeftHand_ADS"));
	// 대기 자세(Relaxed)에서 왼손이 갈 곳. 없으면 지향사격 소켓을 쓴다(손잡이처럼 대기 손이 따로 필요한 부착물만 둔다)
	const FName LeftHandRelaxedSocketName(TEXT("Socket_LeftHand_Relaxed"));
	// 1인칭 그립 포즈의 왼손목(hand_l) 위치. 지향사격·조준 그립 포즈가 총 기준 같은 곳을 잡으므로 하나만 둔다
	const FName FirstPersonLeftHandSocketName(TEXT("Socket_LeftHand_FP"));
	// 오른손이 권총 손잡이를 쥐는 지점. 모든 총에 같은 규칙(손잡이가 몸통과 만나는 바로 아래, 앞뒤 가운데)으로 둔다
	const FName RightHandGripSocketName(TEXT("Socket_Grip"));
}

AHuntWeapon::AHuntWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Mesh);

	Attachments = CreateDefaultSubobject<UWeaponAttachmentComponent>(TEXT("Attachments"));
}

void AHuntWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 레벨 배치·BP·스폰 모두 Definition의 메시를 따르도록 여기서 설정
	Mesh->SetSkeletalMesh(Definition ? Definition->Mesh : nullptr);
}

const UGripAttachmentDefinition* AHuntWeapon::GetGrip() const
{
	return Cast<UGripAttachmentDefinition>(Attachments->GetEquippedAttachment(EHuntAttachmentSlot::Underbarrel));
}

FVector AHuntWeapon::GetLeftHandGripLocation(float AimAlpha, float RelaxedAlpha) const
{
	const FVector HipGrip = GetLeftHandSocketComponent(LeftHandSocketName)->GetSocketLocation(LeftHandSocketName);
	const USceneComponent* RelaxedComponent = GetLeftHandSocketComponent(LeftHandRelaxedSocketName);
	const FVector RelaxedGrip = RelaxedComponent->DoesSocketExist(LeftHandRelaxedSocketName) ? RelaxedComponent->GetSocketLocation(LeftHandRelaxedSocketName) : HipGrip;
	const USceneComponent* ADSComponent = GetLeftHandSocketComponent(LeftHandADSSocketName);
	const FVector ADSGrip = ADSComponent->DoesSocketExist(LeftHandADSSocketName) ? ADSComponent->GetSocketLocation(LeftHandADSSocketName) : HipGrip;
	return FMath::Lerp(FMath::Lerp(HipGrip, RelaxedGrip, RelaxedAlpha), ADSGrip, AimAlpha);
}

bool AHuntWeapon::GetLeftHandGripRotation(float AimAlpha, float RelaxedAlpha, FQuat& OutRotation) const
{
	const UStaticMeshComponent* Grip = Attachments->FindAttachmentSocketMesh(EHuntAttachmentSlot::Underbarrel, LeftHandSocketName);
	if (!Grip)
	{
		return false;
	}
	const FQuat HipRotation = Grip->GetSocketQuaternion(LeftHandSocketName);
	const FQuat RelaxedRotation = Grip->DoesSocketExist(LeftHandRelaxedSocketName) ? Grip->GetSocketQuaternion(LeftHandRelaxedSocketName) : HipRotation;
	const FQuat ADSRotation = Grip->DoesSocketExist(LeftHandADSSocketName) ? Grip->GetSocketQuaternion(LeftHandADSSocketName) : HipRotation;
	OutRotation = FQuat::Slerp(FQuat::Slerp(HipRotation, RelaxedRotation, RelaxedAlpha), ADSRotation, AimAlpha);
	return true;
}

bool AHuntWeapon::GetFirstPersonLeftHandGripLocation(FVector& OutLocation) const
{
	const USceneComponent* SocketComponent = GetLeftHandSocketComponent(FirstPersonLeftHandSocketName);
	if (!SocketComponent->DoesSocketExist(FirstPersonLeftHandSocketName))
	{
		return false;
	}
	OutLocation = SocketComponent->GetSocketLocation(FirstPersonLeftHandSocketName);
	return true;
}

const USceneComponent* AHuntWeapon::GetLeftHandSocketComponent(FName SocketName) const
{
	// 손잡이 → 핸드가드 → 총 순서로 이 소켓이 있는 메시를 쓴다
	const UStaticMeshComponent* Part = Attachments->FindAttachmentSocketMesh(EHuntAttachmentSlot::Underbarrel, SocketName);
	if (!Part)
	{
		Part = Attachments->FindAttachmentSocketMesh(EHuntAttachmentSlot::Handguard, SocketName);
	}
	return Part ? static_cast<const USceneComponent*>(Part) : Mesh.Get();
}

FVector AHuntWeapon::GetRightHandGripOffset() const
{
	return Mesh->DoesSocketExist(RightHandGripSocketName)
		? Mesh->GetSocketTransform(RightHandGripSocketName, RTS_Component).GetLocation()
		: FVector::ZeroVector;
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
	const FVector AimPoint = GetAimPoint(OwnerPawn, ViewLocation, ViewDirection);

	// 벽에 붙어 총구가 조준점보다 앞에 나가 있으면 뒤로 쏘지 않도록 시선 방향을 쓴다
	const FVector Direction = (AimPoint - MuzzleLocation).GetSafeNormal();
	const FVector AimDirection = (Direction | ViewDirection) > 0.f ? Direction : ViewDirection;

	// 벽에 막혀 총을 내린 만큼 크로스헤어 대신 총구가 향한 방향으로 쏜다
	// shortcut: 총열이 총 메시의 +Y라고 가정(AR4), 축이 다른 총이 생기면 총구 소켓 방향으로 바꾼다
	const AHuntCharacter* Character = Cast<AHuntCharacter>(OwnerPawn);
	const float WallBlockAlpha = Character ? Character->GetWallBlockAlpha() : 0.f;
	return FMath::Lerp(AimDirection, Mesh->GetRightVector(), WallBlockAlpha).GetSafeNormal();
}

FVector AHuntWeapon::GetAimPoint(const APawn* OwnerPawn, const FVector& ViewLocation, const FVector& ViewDirection) const
{
	// 카메라 중앙 트레이스가 맞은 곳. 아무것도 안 맞으면 AimTraceDistance 앞의 점
	const FVector TraceEnd = ViewLocation + ViewDirection * AimTraceDistance;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponAim), false, OwnerPawn);
	Params.AddIgnoredActor(this);
	return GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params) ? Hit.ImpactPoint : TraceEnd;
}

FVector AHuntWeapon::GetSpawnLocation(const APawn* OwnerPawn) const
{
	// 1인칭 총은 줄여 그려 벽 앞에 보여도, 실제 총구(소켓)는 원래 크기 위치라 벽 너머에 있을 수 있다
	// 총구 부착물(소음기 등)에 Socket_Muzzle(부착물 끝)이 있으면 거기서 나간다
	// 없으면 핸드가드 총열 끝(Socket_Muzzle), 그것도 없으면 총 메시 총구 뼈
	const UStaticMeshComponent* MuzzleAttachment = Attachments->FindAttachmentSocketMesh(EHuntAttachmentSlot::Muzzle, MuzzleSocketName);
	if (!MuzzleAttachment)
	{
		MuzzleAttachment = Attachments->FindAttachmentSocketMesh(EHuntAttachmentSlot::Handguard, MuzzleSocketName);
	}
	const FVector MuzzleLocation = MuzzleAttachment ? MuzzleAttachment->GetSocketLocation(MuzzleSocketName) : Mesh->GetSocketLocation(MuzzleBoneName);
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
