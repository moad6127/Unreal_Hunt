// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntAnimInstance.generated.h"

class AHuntCharacter;

/**
 * 캐릭터 상태만 계산해 둔다. 스테이트 머신·블렌드는 ABP에서 구성
 */
UCLASS()
class HUNT_API UHuntAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// EditAnywhere: 애님 프리뷰 에디터에서 값을 바꿔 상태 전환을 확인할 수 있게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	// 속도와 입력 가속이 모두 있을 때만 true (멈출 때 바로 Idle로)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	// 이동 방향 (-180~180, 캐릭터 정면 기준). 8방향 블렌드스페이스용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	// 제자리 회전(Lyra 방식): 서 있을 때 액터는 시선을 따라 돌지만 몸은 이만큼 반대로 돌려 제자리에 둔다
	// ABP의 Rotate Root Bone(Yaw)에 넣는다. +면 몸이 시선보다 오른쪽을 보고 있다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float RootYawOffset = 0.f;

	// 루트를 돌린 만큼 상체를 되돌려 총이 시선을 향하게 한다. 에임 오프셋 Yaw(X)에 넣는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float AimYaw = 0.f;

	// 0 = 회전 안 함, 1 = 왼쪽, 2 = 오른쪽. ABP의 Blend Poses by int에 넣는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	int32 TurnDirection = 0;

	// 회전 애니메이션 재생 시간. ABP의 Sequence Evaluator(Explicit Time)에 넣는다
	// Sequence Player는 같은 방향으로 연달아 돌 때 처음부터 다시 재생되지 않아서 시간을 직접 준다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float TurnTime = 0.f;

	// 몸과 시선 차이가 이 각도를 넘으면 회전 애니메이션을 시작한다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnStartAngle = 50.f;

	// 몸과 시선이 이 이상 벌어지면 몸이 바로 따라온다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float MaxRootYawOffset = 120.f;

	// 몸과 시선 차이가 MaxRootYawOffset에 가까울수록 회전 애니메이션을 이 배율까지 빨리 재생한다 (빠르게 돌 때 몸이 끌려가지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "1.0"))
	float MaxTurnPlayRate = 1.75f;

	// 회전이 끝난 뒤 다음 회전까지 쉬는 시간. ABP 회전 블렌드 시간(0.2초) 이상이어야 이전 회전이 다 빠진 뒤 새로 시작해 튀지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnRetriggerDelay = 0.2f;

	// 움직이기 시작하면 몸이 시선 방향으로 돌아오는 속도
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float RootYawBlendOutSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon = false;

	// 시선 상하 각도(-90~90). 에임 오프셋으로 총이 시선을 따라오게 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimPitch = 0.f;

	// 조준 블렌드(0~1). ABP에서 조준 자세(MM_Rifle_Idle_ADS) 상체 블렌드 가중치로 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimAlpha = 0.f;

	// 조준 시 가슴(spine_05)에 더할 월드 회전: 총의 가늠자 방향이 시선과 같아지게 상체 전체를 돌리는 값
	// ABP의 Transform(Modify) Bone(spine_05, Rotation=Add/World, Alpha=AimAlpha)에 넣는다. 양팔이 같이 돌아 왼손목이 꺾이지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FRotator AimSpineCorrection = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName SightSocketName = TEXT("Socket_Sight");

	// 조준 중 손 회전이 시선을 따라가는 속도. 낮을수록 마우스를 돌릴 때 총이 늦게 따라온다 (0 = 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Sway", meta = (ClampMin = "0.0"))
	float AimSwayRotationSpeed = 20.f;

	// 조준 중 총이 시선에서 벗어날 수 있는 최대 각도(도). 빠르게 돌려도 이 이상 뒤처지지 않아 가늠자가 화면 중앙 근처에 남는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Sway", meta = (ClampMin = "0.0"))
	float AimSwayMaxAngle = 2.f;

	// 왼손 IK 목표 위치: 총의 왼손 소켓을 오른손(hand_r) 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, hand_r)에 넣는다
	// 총이 오른손에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	// 허리 사격 왼손 소켓. 조준 중에는 AimAlpha만큼 조준용 소켓 쪽으로 옮겨 간다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName LeftHandSocketName = TEXT("Socket_LeftHand");

	// 조준 자세 애니메이션의 자연 그립 위치. 총에 없으면 허리 사격 소켓을 그대로 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName LeftHandADSSocketName = TEXT("Socket_LeftHand_ADS");

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;

	void UpdateTurnInPlace(float DeltaSeconds);

	float PrevActorYaw = 0.f;
	// 회전 애니메이션의 RemainingTurnYaw(가중치로 나눈 값) 직전 프레임 값
	float PrevTurnYawCurve = 0.f;
	bool bTurnCurveActive = false;
	float TurnCooldown = 0.f;

	// 흔들림 보간된 오른손 목표 회전(월드)
	FQuat SmoothedHandQuat = FQuat::Identity;

	// 이전 프레임에 ABP가 가슴에 실제로 적용한 보정(Alpha 반영)
	FQuat AppliedSpineCorrection = FQuat::Identity;
};
