// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntAnimInstance.generated.h"

class AHuntCharacter;
class UAnimSequence;

// 이동 방향별 애니메이션 한 벌. 비워 둔 방향은 가장 가까운 방향(없으면 F)을 쓴다
USTRUCT(BlueprintType)
struct FHuntDirectionalAnims
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> F;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> FL;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> FR;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> L;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> R;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> BL;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> BR;
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> B;

	// Direction: 캐릭터 정면 기준 -180~180(+ = 오른쪽)
	UAnimSequence* Pick(float Direction) const;
};

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

	// 이동 키를 누르고 있다(가속 중). 출발·정지 상태 전환에 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StartStop")
	bool bHasAcceleration = false;

	// 정지 애니메이션을 끝까지(멈추는 지점 + StopSettleTime) 재생했다. Stop → Idle 조건
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StartStop")
	bool bStopFinished = false;

	// 정지 애니메이션의 멈추는 지점 뒤로 더 재생할 시간(초). 무게를 싣고 서는 마무리 자세
	UPROPERTY(EditDefaultsOnly, Category = "StartStop", meta = (ClampMin = "0.0"))
	float StopSettleTime = 0.3f;

	// 지금 재생할 정지 애니메이션. Stop 상태의 Sequence Evaluator에 넣는다
	UPROPERTY(Transient, BlueprintReadOnly, Category = "StartStop")
	TObjectPtr<UAnimSequence> StopAnim;

	// 정지 애니메이션 재생 위치(초). Sequence Evaluator의 Explicit Time에 넣는다
	// 지금 속도와 같은 지점에서 시작해, 캐릭터가 실제로 멈추는 시각에 애니메이션도 멈추는 지점에 닿도록 빠르게 재생한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StartStop")
	float StopTime = 0.f;

	// 질주 정지(GASP 달리기 정지)
	UPROPERTY(EditDefaultsOnly, Category = "StartStop")
	FHuntDirectionalAnims SprintStops;

	// 걷기·조깅 정지(GASP 걷기 정지)
	UPROPERTY(EditDefaultsOnly, Category = "StartStop")
	FHuntDirectionalAnims WalkStops;

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

	// 걷기 샘플보다 빠르게 움직이면 그만큼 빨리 재생한다 (1 미만으로는 안 내림)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkPlayRate = 1.f;

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

	// 움직이기 시작하면 몸이 시선 방향으로 돌아오는 속도
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

	// 왼손 IK 목표 위치: 총의 왼손 소켓을 오른손(hand_r) 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, hand_r)에 넣는다
	// 총이 오른손에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;

	void UpdateVelocity();
	void UpdateStartStop(float DeltaSeconds);
	// Distance 커브가 Distance가 되는 재생 시간. 커브는 시간에 따라 늘어나므로 이분 탐색
	static float FindTimeAtDistance(const UAnimSequence* Anim, float Distance);
	// 정지 애니메이션에서 멈추기 전 마지막으로 루트 속도가 Speed 이상이던 재생 시간
	static float FindStopTimeAtSpeed(const UAnimSequence* Anim, float Speed);

	// 이동 설정의 제동(마찰 + 감속)으로 Speed에서 멈출 때까지 걸리는 시간(초)
	float EstimateBrakingTime(float Speed) const;

	// 정지 애니메이션에서 멈추는 지점(Distance 커브 0)과, 끝낼 시간(멈추는 지점 + StopSettleTime)
	float StopPointTime = 0.f;
	float StopEndTime = 0.f;
	// 멈추는 지점까지의 재생 속도(캐릭터가 실제로 멈추는 시각에 맞춘 값). 그 뒤 마무리는 1배
	float StopRate = 1.f;
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

	float PrevActorYaw = 0.f;
	bool bWasAccelerating = false;
	// 회전 애니메이션의 RemainingTurnYaw(가중치로 나눈 값) 직전 프레임 값
	float PrevTurnYawCurve = 0.f;
	bool bTurnCurveActive = false;
	float TurnCooldown = 0.f;
};
