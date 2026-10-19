// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntWeapon.h"
#include "Weapon/HuntProjectile.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
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
}

void AHuntWeapon::Fire(const FRotator& AimRotation)
{
	if (!Definition || !Definition->ProjectileClass)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < Definition->FireInterval)
	{
		return;
	}
	LastFireTime = Now;

	const FVector MuzzleLocation = Mesh->GetSocketLocation(MuzzleBoneName);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AHuntProjectile>(Definition->ProjectileClass, MuzzleLocation, AimRotation, Params);

	if (Definition->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Definition->FireSound, MuzzleLocation);
	}
}
