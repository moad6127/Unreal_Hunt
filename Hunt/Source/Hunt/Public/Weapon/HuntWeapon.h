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

	// 왼손이 잡을 위치(월드). 지향사격 소켓과 조준 소켓(없으면 지향사격 소켓) 사이를 AimAlpha로 섞는다
	FVector GetLeftHandGripLocation(float AimAlpha) const;

	// 1인칭 왼손목이 갈 위치(월드). 총에 1인칭 왼손 소켓이 없으면 false (그립 포즈 그대로 둔다)
	bool GetFirstPersonLeftHandGripLocation(FVector& OutLocation) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleBoneName = TEXT("b_gun_muzzleflash");

	// 지향사격 왼손 소켓 (총 메시마다 추가)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName LeftHandSocketName = TEXT("Socket_LeftHand");

	// 조준 자세 애니메이션의 자연 그립 위치. 총에 없으면 지향사격 소켓을 그대로 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName LeftHandADSSocketName = TEXT("Socket_LeftHand_ADS");

	// 1인칭 그립 포즈의 왼손목(hand_l) 위치. 지향사격·조준 그립 포즈가 총 기준 같은 곳을 잡으므로 하나만 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName FirstPersonLeftHandSocketName = TEXT("Socket_LeftHand_FP");

private:
	double LastFireTime = -1.0e9;
};
