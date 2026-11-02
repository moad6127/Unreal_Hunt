// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class HUNT_API AHuntProjectile : public AActor
{
	GENERATED_BODY()

public:
	AHuntProjectile();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 궤적 디버그 선을 이어 그릴 직전 위치
	FVector LastTrailLocation = FVector::ZeroVector;
};
