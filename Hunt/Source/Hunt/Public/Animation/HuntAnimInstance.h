// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntAnimInstance.generated.h"

class AHuntCharacter;
class UAnimSequence;

/**
 * 3인칭 전신용. 다른 사람이 보는 내 몸, 내 그림자, 내 하체(Leader Pose)가 이 포즈를 쓴다
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
	float GroundSpeed = 0.f;

	// 움직이고 있으면 true. 키를 뗀 뒤 감속하는 동안에도 true라 발걸음이 속도에 맞춰 줄어들며 멈춘다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	// 이동 방향 (-180~180, 캐릭터 정면 기준). 8방향 블렌드스페이스용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	// 뒤쪽 반구로 이동 중이면 true. 앞/뒤 이동 블렌드스페이스를 고른다(옆걸음 애니메이션이 앞뒤로 엉덩이 방향이 달라서)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bMovingBackward = false;

	// 앞/뒤 블렌드스페이스에 넣는 방향. Direction을 각자 담당 반구로 접은 값이라(앞쪽 180→0, 뒤쪽 0→180)
	// 전환 블렌드 중에도 샘플 범위 밖(옆걸음 좌우 50:50 섞임 등)을 읽지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float ForwardDirection = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float BackwardDirection = 180.f;

	// ±90도 근처에서 앞/뒤가 오락가락하지 않도록 이만큼 넘어가야 바뀐다(도)
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float BackwardHysteresis = 10.f;

	// 걸음 상태(false = 걷기, true = 조깅·질주). ABP는 걷기용·조깅용 블렌드스페이스를 섞지 않고 이 값으로 바꿔 끼운다(관성 블렌딩)
	// 걷기와 조깅을 한 블렌드스페이스 안에서 섞으면 중간 자세에서 다리가 모여 보이기 때문
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bRunGait = false;

	// 걷기용 블렌드스페이스에 넣는 속도. 걷기 샘플 속도까지만 올려 조깅 샘플이 섞이지 않게 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkBlendSpeed = 0.f;

	// 속도 / 걷기 샘플 속도. MinPlayRate 아래로는 안 내리고 대신 WalkBlendSpeed를 낮춰 Idle과 섞는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkPlayRate = 1.f;

	// 걷기·조깅 재생 속도 하한. 너무 느리게 재생하면 슬로모션처럼 보인다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinPlayRate = 0.85f;

	// 조깅용 블렌드스페이스에 넣는 속도. 조깅 샘플 속도 아래로 내리지 않아 걷기 샘플이 섞이지 않게 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RunBlendSpeed = 0.f;

	// 조깅용 블렌드스페이스의 재생 속도. 조깅 샘플보다 느리면(옆·뒤 조깅, 감속 중) 느리게, 가장 빠른 샘플보다 빠르면 빠르게 재생해
	// 다리가 몸 속도와 맞게 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LocomotionPlayRate = 1.f;

	// 가장 빠른 이동 애니메이션 자체의 속도 = BS_Hunt_Loco_Fwd/Bwd의 최고 Speed 샘플 값 (정면 ALS 질주 / 옆·뒤 ALS 달리기)
	// shortcut: 블렌드스페이스 샘플 값을 옮겨 적은 것, 샘플 속도를 바꾸면 여기도 맞춰야 한다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1.0"))
	float ForwardRunAnimSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1.0"))
	float SideRunAnimSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1.0"))
	float BackwardRunAnimSpeed = 350.f;

	// 조깅 샘플(ALS 달리기) 속도. 모든 방향이 같다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1.0"))
	float JogAnimSpeed = 350.f;

	// 걷기 샘플(ALS 걷기) 속도. 모든 방향이 같다(ALS 원본 ABP의 걷기 기준 속도)
	// shortcut: 블렌드스페이스 샘플 값을 옮겨 적은 것, 샘플 속도를 바꾸면 여기도 맞춰야 한다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1.0"))
	float WalkAnimSpeed = 150.f;

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
	// 에임 오프셋(AO_MM_Rifle_Idle_Hipfire) Yaw 범위가 ±90이라 그 이상이면 상체가 시선을 못 따라가 비틀려 보인다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float MaxRootYawOffset = 90.f;

	// 몸과 시선 차이가 MaxRootYawOffset에 가까울수록 회전 애니메이션을 이 배율까지 빨리 재생한다 (빠르게 돌 때 몸이 끌려가지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "1.0"))
	float MaxTurnPlayRate = 1.75f;

	// 회전이 끝난 뒤 다음 회전까지 쉬는 시간. ABP 회전 블렌드 시간(0.2초) 이상이어야 이전 회전이 다 빠진 뒤 새로 시작해 튀지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnRetriggerDelay = 0.2f;

	// 움직이는 동안 몸이 목표 방향(시선 또는 아래 이동 방향 쪽)으로 돌아가는 속도
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float RootYawBlendOutSpeed = 10.f;

	// 전신 손의 총(BodyWeapon)이 이 메시에 붙어 있으면 true
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon = false;

	// 시선 상하 각도(-90~90). 에임 오프셋으로 총이 시선을 따라오게 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimPitch = 0.f;

	// 조준 블렌드(0~1). 조준 자세 블렌드 가중치로 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimAlpha = 0.f;

	// 벽에 막혀 총을 내린 정도(0~1). 내린 자세 애디티브(A_Rifle_LoweredArms_Additive)의 Alpha로 쓴다
	// 질주 상체 자세가 섞인 만큼은 뺀 값이다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float WallBlockAlpha = 0.f;

	// 대기 자세 블렌드(0~1, 질주 상체가 섞인 만큼 뺀 값). ALS 방식: 상체를 ALS 소총 대기 자세(ALS_Props_M4A1_Poses 0프레임)로 바꾸고
	// 총을 시선에 겨누는 Lyra 에임 오프셋 대신 목·머리 위주의 ALS 시선 애디티브(ALS_N_Look)를 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float RelaxedAlpha = 0.f;

	// Lyra 에임 오프셋(총을 시선에 겨눔) 가중치 = 1 - RelaxedAlpha
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimOffsetAlpha = 1.f;

	// 왼손 IK 가중치(총이 있으면 1). 대기 포즈(A_Hunt_Rifle_Relaxed_Pose)도 왼손 모양을 Lyra 그립과 같게 만들어 두어 IK를 그대로 건다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LeftHandIKAlpha = 0.f;

	// ALS 시선 스윕의 정규화 시간(0~1) = 몸 기준 시선 Yaw(AimYaw) -180~180. 0.5가 정면 (ALS ForwardYawTime)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LookYawTime = 0.5f;

	// 왼손 IK 목표 위치: 총의 왼손 소켓을 오른손(hand_r) 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, hand_r)에 넣는다
	// 총이 오른손에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	// 발 고정(ALS Foot Lock): 멈추는 순간 디딘 발을 월드에 고정해 Idle로 섞이는 동안 미끄러지지 않게 한다
	// ABP가 발 IK(Control Rig) 앞에서 ik_foot_l/r을 이 위치(컴포넌트 공간)로 옮긴다. Alpha가 0이면 애니메이션 그대로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	float FootLockAlphaL = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	float FootLockAlphaR = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	FVector FootLockLocationL = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	FVector FootLockLocationR = FVector::ZeroVector;

	// 서 있는 자세. 발의 Idle 자리(ik_foot 위치)를 여기서 읽는다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequence> IdleAnim;

	// 고정된 발이 Idle 자리에서 이만큼(cm, 수평) 벗어나 있으면 한 걸음 옮긴다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.0"))
	float FootStepThreshold = 15.f;

	// 옮기는 발이 반대쪽 발과 옆으로 이만큼(cm)은 떨어져 있게 한다(다리가 X자로 꼬이지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.0"))
	float FootStepMinGap = 10.f;

	// 옮기는 발을 드는 높이(cm)
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.0"))
	float FootStepLift = 6.f;

	// 멈추고 나서 이 시간(초)이 지나야 발을 옮긴다(Idle로 섞이는 중에는 옮기지 않는다)
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.0"))
	float FootStepDelay = 0.3f;

	// 발 하나를 옮기는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.05"))
	float FootStepDuration = 0.3f;

	// 움직이기 시작할 때 고정을 푸는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.01"))
	float FootUnlockTime = 0.15f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;

	void UpdateVelocity();
	void UpdateFoldedDirections();
	// 현재 방향의 가장 빠른 이동 애니메이션 속도
	float GetRunAnimSpeed() const;
	void UpdateGaitBlend();
	void UpdateMovingBackward();
	void UpdateWeapon();
	void UpdateAimPitch();

	void UpdateTurnInPlace(float DeltaSeconds);
	void UpdateRootYawOffset(float DeltaSeconds, bool bCanTurn);
	void AdvanceTurnTimers(float DeltaSeconds);
	void ApplyTurnYawCurve();
	void TryStartTurn(bool bCanTurn);

	void UpdateFootLock(float DeltaSeconds);

	// 발 하나의 고정 상태. 0 = 왼발, 1 = 오른발
	struct FFootLock
	{
		bool bLocked = false;
		bool bStepping = false;
		// 이번에 멈춘 뒤 이미 한 번 옮겼다
		bool bStepped = false;
		float Alpha = 0.f;
		float StepTime = 0.f;
		FVector LockedWorld = FVector::ZeroVector;
		FVector StepStartWorld = FVector::ZeroVector;
		// Idle 자세의 발 자리(컴포넌트 공간, 루트 회전 전)
		FVector IdleLocation = FVector::ZeroVector;
	};
	FFootLock Feet[2];
	float StandingTime = 0.f;
	bool bWasMovingForFootLock = false;

	float PrevActorYaw = 0.f;
	// 캐릭터 정면 기준 실제 이동 방향(-180~180). Direction은 여기서 몸이 돌아간 만큼(RootYawOffset)을 뺀 값
	float VelocityDirection = 0.f;
	// 회전 애니메이션의 RemainingTurnYaw(가중치로 나눈 값) 직전 프레임 값
	float PrevTurnYawCurve = 0.f;
	bool bTurnCurveActive = false;
	float TurnCooldown = 0.f;
};
