// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntWeapon.generated.h"

class UGripAttachmentDefinition;
class USkeletalMeshComponent;
class UWeaponAttachmentComponent;
class UWeaponDefinition;

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
	UWeaponAttachmentComponent* GetAttachments() const { return Attachments; }

	// 손잡이 슬롯에 달린 손잡이 부착물 (없거나 손잡이가 아니면 nullptr)
	const UGripAttachmentDefinition* GetGrip() const;

	// 왼손이 잡을 위치(월드). 지향사격 소켓 → 대기 소켓(RelaxedAlpha) → 조준 소켓(AimAlpha) 순서로 섞는다. 대기·조준 소켓이 없으면 지향사격 소켓
	FVector GetLeftHandGripLocation(float AimAlpha, float RelaxedAlpha) const;

	// 손잡이 부착물에 왼손 소켓이 있으면 그 회전(월드, 왼손 hand_l 방향). 위치와 같은 순서로 섞는다. 없으면 false (애니메이션의 손 방향 그대로)
	bool GetLeftHandGripRotation(float AimAlpha, float RelaxedAlpha, FQuat& OutRotation) const;

	// 1인칭 왼손목이 갈 위치(월드). 총에 1인칭 왼손 소켓이 없으면 false (그립 포즈 그대로 둔다)
	bool GetFirstPersonLeftHandGripLocation(FVector& OutLocation) const;

	// 오른손이 쥐는 지점(Socket_Grip)의 총 메시 공간 위치. 손 기준점에 이 지점이 오도록 총을 붙인다. 소켓이 없으면 원점
	FVector GetRightHandGripOffset() const;

	// 3인칭 오른손목(hand_r)이 갈 곳(월드). 몸통 등 필수 부품 → 총 순서로 Socket_RightHand를 찾는다. 없으면 false (애니메이션의 손 그대로)
	bool GetRightHandGripTransform(FTransform& OutTransform) const;

	// 대기 자세의 3인칭 오른손목(hand_r) 자리(총 메시 공간). 몸통 등 필수 부품 → 총 순서로 Socket_RightHand_Relaxed를 찾는다. 없으면 false
	// 있으면 대기 동안 대기 애니메이션 손이 총을 쥐고(오른손 IK·방아쇠 손가락 끔) 총이 이 자리 기준으로 손에 붙는다
	bool GetRightHandRelaxedTransform(FTransform& OutTransform) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponAttachmentComponent> Attachments;

private:
	// 이 왼손 소켓이 있는 메시: 손잡이 → 필수 부품(핸드가드) → 총 순서
	const USceneComponent* GetLeftHandSocketComponent(FName SocketName) const;
	// 총구에서 크로스헤어가 가리키는 점을 향해, 현재 탄퍼짐 안에서 발사체를 쏜다
	void Shoot();
	// 카메라 중앙 트레이스로 조준점을 구해 총구에서 그 점을 향하는 방향
	FVector GetAimDirection(const APawn* OwnerPawn, const FVector& MuzzleLocation) const;
	// 카메라 중앙 트레이스로 찾은 조준점
	FVector GetAimPoint(const APawn* OwnerPawn, const FVector& ViewLocation, const FVector& ViewDirection) const;
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
