// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"

AHuntProjectile::AHuntProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(1.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Collision);

	// 기본값은 소총탄 대략치(약 800m/s). 탄종별 수치는 BP에서 조정
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 80000.f;
	ProjectileMovement->MaxSpeed = 80000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 3.f;
}

void AHuntProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 쏜 사람과 총에 바로 부딪히지 않게
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->IgnoreActorWhenMoving(GetInstigator(), true);
	Collision->OnComponentHit.AddDynamic(this, &AHuntProjectile::OnHit);
}

void AHuntProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// shortcut: 명중 확인용 디버그 표시뿐, M4에서 부위별 데미지로 교체
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 8.f, 12, FColor::Red, false, 5.f);
	UE_LOG(LogTemp, Log, TEXT("Projectile hit: %s"), *GetNameSafe(OtherActor));

	Destroy();
}
