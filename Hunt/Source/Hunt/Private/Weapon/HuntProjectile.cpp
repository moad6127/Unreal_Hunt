// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HuntProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"

AHuntProjectile::AHuntProjectile()
{
	// 궤적 디버그 선을 매 프레임 이어 그린다
	PrimaryActorTick.bCanEverTick = true;

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

	LastTrailLocation = GetActorLocation();
}

void AHuntProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// shortcut: 탄도 확인용 디버그 선, 예광탄 이펙트가 필요해지면 나이아가라로 교체
	const FVector Location = GetActorLocation();
	DrawDebugLine(GetWorld(), LastTrailLocation, Location, FColor::Yellow, false, 5.f);
	LastTrailLocation = Location;
}

void AHuntProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// shortcut: 명중 확인용 디버그 표시뿐, M4에서 부위별 데미지로 교체
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 1.f, 12, FColor::Red, false, 5.f);
	// 마지막 Tick 이후 명중 지점까지 남은 궤적
	DrawDebugLine(GetWorld(), LastTrailLocation, Hit.ImpactPoint, FColor::Yellow, false, 5.f);
	UE_LOG(LogTemp, Log, TEXT("Projectile hit: %s"), *GetNameSafe(OtherActor));

	Destroy();
}
