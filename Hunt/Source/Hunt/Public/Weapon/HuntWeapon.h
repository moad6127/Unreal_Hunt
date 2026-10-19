// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntWeapon.generated.h"

class USkeletalMeshComponent;
class UWeaponDefinition;

UCLASS()
class HUNT_API AHuntWeapon : public AActor
{
	GENERATED_BODY()

public:
	AHuntWeapon();
	virtual void OnConstruction(const FTransform& Transform) override;

	// 총구에서 AimRotation 방향으로 발사체를 쏜다. 발사 간격이 안 지났으면 무시
	void Fire(const FRotator& AimRotation);

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleBoneName = TEXT("b_gun_muzzleflash");

private:
	double LastFireTime = -1.0e9;
};
