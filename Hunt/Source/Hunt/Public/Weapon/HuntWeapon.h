// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponDefinition.h"
#include "HuntWeapon.generated.h"

class USkeletalMeshComponent;

UCLASS()
class HUNT_API AHuntWeapon : public AActor
{
	GENERATED_BODY()

public:
	AHuntWeapon();
	virtual void OnConstruction(const FTransform& Transform) override;

	// 방아쇠를 당김. 단발은 한 발(발사 간격이 안 지났으면 무시), 연발은 StopFire까지 FireInterval마다 쏜다
	void StartFire();
	void StopFire();

	// 연발로 방아쇠를 누르고 있는 중
	bool IsAutoFiring() const;

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	// 왼손이 잡을 위치(월드). 지향사격 소켓과 조준 소켓(없으면 지향사격 소켓) 사이를 AimAlpha로 섞는다
	FVector GetLeftHandGripLocation(float AimAlpha) const;

	// 1인칭 왼손목이 갈 위치(월드). 총에 1인칭 왼손 소켓이 없으면 false (그립 포즈 그대로 둔다)
	bool GetFirstPersonLeftHandGripLocation(FVector& OutLocation) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

	// 시작 값은 Definition->FireMode
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	EHuntFireMode FireMode = EHuntFireMode::Single;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleBoneName = TEXT("b_gun_muzzleflash");

	// 조준점을 찾는 카메라 트레이스 길이(cm). 아무것도 안 맞으면 이 거리의 점을 조준한다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float AimTraceDistance = 100000.f;

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
	// 총구에서 크로스헤어가 가리키는 점을 향해, 현재 탄퍼짐 안에서 발사체를 쏜다
	void Shoot();
	// 카메라 중앙 트레이스로 조준점을 구해 총구에서 그 점을 향하는 방향
	FVector GetAimDirection(const APawn* OwnerPawn, const FVector& MuzzleLocation) const;
	// 탄이 생겨날 위치. 벽에 붙어 총구가 벽 너머에 있으면 카메라와 총구 사이의 벽 앞으로 당긴다
	FVector GetSpawnLocation(const APawn* OwnerPawn) const;
	// 플레이어는 실제 카메라 위치·방향, 컨트롤러가 없으면 폰의 눈 위치
	static void GetOwnerViewPoint(const APawn* OwnerPawn, FVector& OutLocation, FRotator& OutRotation);
	void SpawnProjectile(const FVector& Location, const FRotator& Rotation);
	void PlayFireSound(const FVector& Location);
	// 소유 캐릭터의 조준선 반동과 총 킥 (첫 발·지향사격 배율 적용)
	void ApplyRecoilToOwner();

	double LastFireTime = -1.0e9;
	FTimerHandle AutoFireTimer;
	bool bFirstShot = true;
};
