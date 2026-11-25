// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntFirstPersonAnimInstance.generated.h"

class AHuntCharacter;

/**
 * 1인칭 팔용. 팔은 카메라에 붙어 시선 회전을 그대로 따라가므로 에임 오프셋·제자리 회전·이동 방향은 필요 없다
 * 조준, 이동 여부, 왼손 IK 목표를 계산한다. 왼손 모양은 그립 포즈 애니메이션에 들어 있다. 블렌드는 ABP에서 구성
 */
UCLASS()
class HUNT_API UHuntFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	// 1인칭 총 애님 레이어가 메인 값을 같은 이름으로 복사해 읽는다
	friend class UHuntFirstPersonWeaponLayersAnimInstance;

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 걷기/달리기 additive를 고르는 데 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	// 움직이고 있으면 true. 키를 뗀 뒤 감속하는 동안에도 true (전신과 같은 기준)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	// 걷기·질주 애디티브의 재생 시간(Sequence Evaluator Explicit Time). 전신 Locomotion 동기화 그룹의 발 위치(Foot_L → Foot_R → Foot_L)를
	// 매 프레임 1인칭 팔 흔들림 한 주기로 바꿔서, 1인칭 팔이 그림자(전신)와 같은 빠르기·같은 쪽으로 흔들린다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkSwayTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float SprintSwayTime = 0.f;

	// 조준 블렌드(0~1). 지향사격 ↔ 조준 자세 블렌드 가중치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimAlpha = 0.f;

	// 벽에 막혀 총을 내린 정도(0~1). 내린 자세 애디티브(A_Rifle_LoweredArms_Additive)의 Alpha로 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float WallBlockAlpha = 0.f;

	// 대기 자세 블렌드(0~1). 기본 그립 포즈 ↔ 대기 포즈(A_FP_Rifle_Relaxed_Pose_Grip) 블렌드 가중치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float RelaxedAlpha = 0.f;

	// 총이 이 팔에 붙어 있고 1인칭 왼손 소켓이 있으면 true. 왼손 IK의 Alpha로 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bLeftHandIK = false;

	// 왼손 IK 목표: 총의 1인칭 왼손 소켓을 ik_hand_gun 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, ik_hand_gun)에 넣는다
	// 손 모양·팔꿈치는 그립 포즈가 정하고, IK는 총마다 다른 잡는 위치로 손목만 옮긴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;

	// 전신 걸음 위치(0~1)가 한 바퀴 돌 때마다 1 늘린다. 긴 1인칭 애니메이션(여러 주기)을 앞에서부터 차례로 쓰기 위함
	float LastStepPhase = 0.f;
	int32 StepCycle = 0;

	// 그림자(전신) 걸음 위치에 맞춘 걷기·질주 애디티브 재생 시간
	void UpdateSwayTimes();
	void UpdateLeftHandIK();
};
