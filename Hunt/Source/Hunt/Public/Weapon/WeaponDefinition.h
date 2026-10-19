// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDefinition.generated.h"

class AHuntProjectile;
class USkeletalMesh;
class USoundBase;

/**
 * 총 본체 데이터. 총 한 종류당 에셋 하나 (DA_AR4 등)
 */
UCLASS()
class HUNT_API UWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> Mesh;

	// 탄속·낙차는 발사체 BP에서 조정한다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AHuntProjectile> ProjectileClass;

	// 발사 간격(초)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float FireInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<USoundBase> FireSound;
};
