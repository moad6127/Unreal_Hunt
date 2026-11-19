// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntAnimInstance.generated.h"

class AHuntCharacter;
class UAnimSequence;
class UAnimSequenceBase;
class UCurveVector;
class UAnimMontage;

// 발이 꼬인 채 멈출 때 재생하는 마지막 한 걸음 클립과 재생 구간(GASP 멈춤 애니메이션, 루트 모션)
USTRUCT()
struct FHuntStopClip
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimSequenceBase> Anim;

	UPROPERTY(EditDefaultsOnly)
	bool bMovingLeft = true;

	// 사격 자세(조준·준비)로 착지하는 클립. 아니면 대기 자세로 착지한다
	UPROPERTY(EditDefaultsOnly)
	bool bAimStance = false;

	UPROPERTY(EditDefaultsOnly)
	float StartTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float EndTime = 0.f;

	// 시작 프레임의 발 위치(컴포넌트 공간). 지금 자세와 가장 비슷한 클립을 고르는 데 쓴다
	UPROPERTY(EditDefaultsOnly)
	FVector2D FootL = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly)
	FVector2D FootR = FVector2D::ZeroVector;
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

	// ABP Locomotion > Stop 안쪽 상태에 들어갈 때 오는 노티파이(ALS (N) Stop States). Lock = 디딘 발 고정, Plant = 공중 발을 디딘 포즈로 옮긴 뒤 고정
	UFUNCTION()
	void AnimNotify_HuntStopLockL();

	UFUNCTION()
	void AnimNotify_HuntStopLockR();

	UFUNCTION()
	void AnimNotify_HuntStopPlantL();

	UFUNCTION()
	void AnimNotify_HuntStopPlantR();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	// 움직이고 있으면 true. 키를 뗀 뒤 감속하는 동안에도 true라 발걸음이 속도에 맞춰 줄어들며 멈춘다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	// 착지 후 지난 시간(초). ABP가 착지 회복 애디티브(MM_Rifle_Jump_RecoveryAdditive)를 이 시간으로 재생한다. 처음엔 클립 끝 너머라 효과 없음
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LandRecoveryTime = 10.f;

	// 뒤쪽 반구로 이동 중이면 true. 앞/뒤 이동 블렌드스페이스를 고른다(옆걸음 애니메이션이 앞뒤로 엉덩이 방향이 달라서)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bMovingBackward = false;

	// 앞/뒤 블렌드스페이스에 넣는 방향. Direction을 각자 담당 반구로 접은 값이라(앞쪽 180→0, 뒤쪽 0→180)
	// 전환 블렌드 중에도 샘플 범위 밖(옆걸음 좌우 50:50 섞임 등)을 읽지 않는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float ForwardDirection = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float BackwardDirection = 180.f;

	// ABP Stop의 Plant 포즈를 평가할 위치(사이클 정규화 시간, 0~1). 이동 방향에 따라 그 발을 디딘 시점이 다르다(ALS Plant 시간)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float PlantTimeL = 0.117f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float PlantTimeR = 0.618f;

	// 기울기(-1~1): 앞뒤(가속 +), 좌우(오른쪽 가속 +). ABP의 ALS_N_Lean 블렌드스페이스(X = LR, Y = FB) 입력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LeanFB = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LeanLR = 0.f;

	// 기울기 애디티브 가중치는 ABP가 Weight_Gait 커브로 정한다(걷기 1 → 0.5, 달리기 2 이상 → 1, ALS 방식)

	// 걸음 상태(false = 걷기, true = 조깅·질주). ABP는 걷기용·조깅용 블렌드스페이스를 섞지 않고 이 값으로 바꿔 끼운다(관성 블렌딩)
	// 걷기와 조깅을 한 블렌드스페이스 안에서 섞으면 중간 자세에서 다리가 모여 보이기 때문
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bRunGait = false;

	// 걷기용 블렌드스페이스에 넣는 속도. 걷기 샘플 속도까지만 올려 조깅 샘플이 섞이지 않게 한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkBlendSpeed = 0.f;

	// 속도 / 걷기 샘플 속도. MinPlayRate 아래로는 안 내리고 대신 WalkBlendSpeed를 낮춰 Idle과 섞는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkPlayRate = 1.f;

	// 조깅용 블렌드스페이스에 넣는 속도. 조깅 샘플 속도 아래로 내리지 않아 걷기 샘플이 섞이지 않게 한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RunBlendSpeed = 0.f;

	// 조깅용 블렌드스페이스의 재생 속도. 조깅 샘플보다 느리면(옆·뒤 조깅, 감속 중) 느리게, 가장 빠른 샘플보다 빠르면 빠르게 재생해
	// 다리가 몸 속도와 맞게 한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LocomotionPlayRate = 1.f;

	// 제자리 회전(Lyra 방식): 서 있을 때 액터는 시선을 따라 돌지만 몸은 이만큼 반대로 돌려 제자리에 둔다
	// ABP의 Rotate Root Bone(Yaw)에 넣는다. +면 몸이 시선보다 오른쪽을 보고 있다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float RootYawOffset = 0.f;

	// 루트를 돌린 만큼 상체를 되돌려 총이 시선을 향하게 한다. 에임 오프셋 Yaw(X)에 넣는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float AimYaw = 0.f;

	// 0 = 회전 안 함, 1 = 왼쪽, 2 = 오른쪽. ABP의 Blend Poses by int에 넣는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	int32 TurnDirection = 0;

	// 회전 애니메이션 재생 시간. ABP의 Sequence Evaluator(Explicit Time)에 넣는다
	// Sequence Player는 같은 방향으로 연달아 돌 때 처음부터 다시 재생되지 않아서 시간을 직접 준다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float TurnTime = 0.f;

	// 대기 자세 회전(ALS_N_TurnIP_L90/R90) 재생 시간 = TurnTime × RelaxedTurnPlayRate. ABP ALS 회전 Sequence Evaluator(Explicit Time)에 넣는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float RelaxedTurnTime = 0.f;

	// 대기 자세로 걸을 때 몸(루트)을 이동 방향 쪽으로 돌리는 양(ALS YawOffset_FB/LR, 입력 = 시선 기준 이동 방향)
	// FB: X = 앞 구역, Y = 뒤 구역. LR: X = 왼쪽 구역, Y = 오른쪽 구역. 상체는 시선 애디티브(AimYaw)가 시선으로 되돌린다
	// 비어 있으면 몸을 시선 정면으로 둔다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	TObjectPtr<UCurveVector> MoveYawOffsetFB;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	TObjectPtr<UCurveVector> MoveYawOffsetLR;

	// 시선 상하 각도(-90~90). 에임 오프셋으로 총이 시선을 따라오게 한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimPitch = 0.f;

	// 조준 블렌드(0~1). 조준 자세 블렌드 가중치로 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimAlpha = 0.f;

	// 벽에 막혀 총을 내린 정도(0~1). 내린 자세 애디티브(A_Rifle_LoweredArms_Additive)의 Alpha로 쓴다
	// 질주 상체 자세가 섞인 만큼은 뺀 값이다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float WallBlockAlpha = 0.f;

	// 대기 자세 블렌드(0~1, 질주 상체가 섞인 만큼 뺀 값). ALS 방식: 상체를 ALS 소총 대기 자세(ALS_Props_M4A1_Poses 0프레임)로 바꾸고
	// 총을 시선에 겨누는 Lyra 에임 오프셋 대신 목·머리 위주의 ALS 시선 애디티브(ALS_N_Look)를 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float RelaxedAlpha = 0.f;

	// 질주 상체 블렌드(0~1). GroundSpeed가 SprintBlendSpeed 구간의 중간을 넘으면 1, 아니면 0으로 SprintBlendTime 동안 부드럽게(시작·끝 감속) 변한다
	// 속도에 직접 묶으면 질주가 풀릴 때 상체가 감속(약 0.25초)을 따라 뚝 바뀐다. ABP Overlay의 질주 블렌드와
	// 대기·벽 앞 내린 자세 가중치(질주 중에는 뺌)가 같이 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SprintAlpha = 0.f;

	// Lyra 에임 오프셋(총을 시선에 겨눔) 가중치 = 1 - max(RelaxedAlpha, WallBlockAlpha)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimOffsetAlpha = 1.f;

	// 왼손 IK 가중치(총이 있으면 1). 대기 포즈(A_Hunt_Rifle_Relaxed_Pose)도 왼손 모양을 Lyra 그립과 같게 만들어 두어 IK를 그대로 건다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LeftHandIKAlpha = 0.f;

	// ALS 시선 스윕의 정규화 시간(0~1) = 몸 기준 시선 Yaw(AimYaw) -180~180. 0.5가 정면 (ALS ForwardYawTime)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LookYawTime = 0.5f;

	// 왼손 IK 목표 위치: 총의 왼손 소켓을 오른손(hand_r) 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, hand_r)에 넣는다
	// 총이 오른손에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	// 손잡이 손 가중치(손잡이에 손 포즈와 왼손 소켓이 있으면 1). ABP의 손가락 레이어와 손 방향(Modify Bone hand_l)이 쓴다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float LeftHandGripAlpha = 0.f;

	// 손잡이를 쥔 손 모양(손잡이 DA의 HandPose). ABP가 손가락 뼈만 이 포즈로 덮는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> LeftHandGripPose;

	// 기본 대기 자세 포즈·앞 달리기 팔(ABP 기본값에서 지정). 손잡이 DA에 3인칭 대기 포즈가 있으면 그것으로 바꾼다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> DefaultRelaxedPose;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> DefaultRelaxedRunArms;

	// 지금 쓸 대기 자세 포즈·앞 달리기 팔. ABP 대기 포즈 노드(Sequence 핀)가 읽는다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> RelaxedPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> RelaxedRunArms;

	// 손잡이를 쥔 왼손 방향(컴포넌트 공간) = 손잡이 Socket_LeftHand 회전
	// shortcut: 지난 프레임의 총 회전이라 몸이 빨리 돌면 한 프레임 늦는다, 손이 흔들려 보이면 hand_r 기준으로 바꾼다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FRotator LeftHandIKRotation = FRotator::ZeroRotator;

	// 발 고정(ALS Foot Lock): 멈추는 순간 디딘 발을 월드에 고정해 Idle로 섞이는 동안 미끄러지지 않게 한다
	// ABP가 발 IK(Control Rig) 앞에서 ik_foot_l/r을 이 위치(컴포넌트 공간)로 옮긴다. Alpha가 0이면 애니메이션 그대로
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	float FootLockAlphaL = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	float FootLockAlphaR = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	FVector FootLockLocationL = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	FVector FootLockLocationR = FVector::ZeroVector;

	// 멈출 때 상체 무게 이동 애디티브(ALS_N_Stop_L/R_Down). 왼발·오른발 고정에 맞춰 슬롯으로 재생한다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequenceBase> StopDownAnimL;

	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequenceBase> StopDownAnimR;

	// 서 있는 하체 진행도(0 = ALS 기본자세 두 발 나란히, 1 = Lyra 사격 자세). 대기가 아니면(준비·조준) 1로 LegStanceBlendTime 동안 일정하게 변한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	float ShootingLegsAlpha = 0.f;

	// 하체 전환 애니메이션을 평가할 시간 = ShootingLegsAlpha × 길이. ABP Idle 하체의 Sequence Evaluator(Explicit Time)에 넣는다
	// 조준하면 앞으로, 대기로 돌아가면 거꾸로 재생되는 셈
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FootLock")
	float ShootingLegsTime = 0.f;

	// 서 있는 하체 전환 애니메이션(ALS 기본자세 → Lyra 사격 자세, 골반·다리와 발 옮기기 포함). ABP가 평가하는 것과 같은 애니메이션
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequence> LegTransitionAnim;

	// 옆으로 가다 발이 꼬인 채 멈출 때 재생할 클립. 캡슐은 클립의 루트 모션이 움직인다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TArray<FHuntStopClip> CrossedStopClips;

	// Jog로 옆으로 가다 입력을 놓을 때 재생할 멈춤 클립(GASP Run 멈춤). 캡슐은 클립의 루트 모션이 움직인다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TArray<FHuntStopClip> JogStopClips;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;

	// 속도·이동 방향
	void UpdateVelocity();
	// 착지한 프레임에 LandRecoveryTime을 0으로 되돌린다
	void UpdateLandRecovery(float DeltaSeconds);
	// 이동 중인지(멈춤 상태로 갈지)·공중인지. 멈출 때 발이 꼬였으면 마지막 한 걸음 클립을 재생한다
	void UpdateShouldMove();
	// 가속 방향으로 몸 기울이기
	void UpdateLean(float DeltaSeconds);
	void UpdateFoldedDirections(float DeltaSeconds);
	// 현재 방향의 가장 빠른 이동 애니메이션 속도
	float GetRunAnimSpeed() const;
	void UpdateGaitBlend();
	void UpdateMovingBackward(float DeltaSeconds);
	// 발 위치(컴포넌트 공간). 0 = 왼발, 1 = 오른발
	FVector GetFootLocal(int32 Side) const;
	// 캐릭터 기준 옆으로 왼발이 오른발보다 왼쪽에 있는 거리(cm). 음수면 다리가 교차했다
	float GetFeetSideGap() const;
	void UpdateSprintBlend(float DeltaSeconds);
	// 조준·벽 앞·대기 자세 가중치
	void UpdateStanceAlphas();
	void UpdateLeftHandIK();
	// 손잡이 DA에 3인칭 대기 포즈가 있으면 그것, 없으면 기본 대기 포즈
	void UpdateRelaxedPoses();
	// SprintAlpha의 감속 전 진행도(0~1, 일정 속도로 변함)
	float SprintBlend = 0.f;
	void UpdateAimPitch();

	void UpdateTurnInPlace(float DeltaSeconds);
	void UpdateRootYawOffset(float DeltaSeconds, bool bCanTurn);
	// 이동 중 몸 Yaw 목표(ALS YawOffset): 대기 자세에서만, RelaxedAlpha만큼
	float GetMoveYawOffset() const;
	void AdvanceTurnTimers(float DeltaSeconds);
	void ApplyTurnYawCurve();
	void TryStartTurn(bool bCanTurn);
	void EndTurn();

	// 서 있는 하체 자세(ALS 기본 ↔ Lyra 사격) 전환 진행도
	void UpdateLegStance(float DeltaSeconds);
	// ABP Stop의 Plant 포즈 평가 시간(이동 방향별 ALS Plant 시간)
	void UpdatePlantTimes();
	void UpdateFootLock(float DeltaSeconds);
	void UpdateFootLockSide(int32 Side, bool bStanding, float DeltaSeconds);

	// 발 하나의 고정 상태. 0 = 왼발, 1 = 오른발
	struct FFootLock
	{
		bool bLocked = false;
		// Stop 노티파이가 고정을 요청함 / 남은 대기 시간(Plant는 다리가 디딘 포즈로 옮겨질 때까지 기다린다)
		bool bLockRequested = false;
		float LockDelay = 0.f;
		float Alpha = 0.f;
		FVector LockedWorld = FVector::ZeroVector;
	};
	FFootLock Feet[2];
	void RequestStopFoot(int32 Side, bool bPlant);
	bool TryPlayCrossedStop();
	bool TryPlayJogStop();
	// 표에서 지금 이동 방향·자세·발 위치에 맞는 클립을 골라 재생한다. ClipFootSpeed = 클립 시작 시 디딘 발의 몸 기준 속도(cm/s)
	bool TryPlayStopClip(const TArray<FHuntStopClip>& Clips, float PlayRate, float ClipFootSpeed);
	const FHuntStopClip* FindStopClip(const TArray<FHuntStopClip>& Clips, const FVector2D& FootL, const FVector2D& FootR) const;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CrossedStopMontage;
	// 클립의 끝 위치(블렌드 아웃 시작) / 클립 재생 중 / 클립 뒤 Stop 진입에서 발 처리를 건너뛴다
	float CrossedStopEndTime = 0.f;
	bool bCrossedStopActive = false;
	bool bSkipStopFeet = false;

	float PrevActorYaw = 0.f;
	// 가속(기울기) 계산용 지난 프레임 속도
	FVector PrevVelocity = FVector::ZeroVector;
	// 골반 방향 전환을 기다린 시간
	float HipsSwitchWaitTime = 0.f;
	// 뒤 블렌드스페이스 방향을 정뒤 = 0으로 둔 보간 값(-90~90) / 지난 프레임 움직이고 있었는지(출발할 때 보간 없이 맞추려고)
	float BackwardDirectionOffset = 0.f;
	bool bWasMovingForDirection = false;
	// 원하는 골반 방향(true = 뒷걸음 블렌드스페이스) / 마지막으로 골반 방향을 바꾼 뒤 지난 시간
	bool bHipsBackwardTarget = false;
	float TimeSinceHipsSwitch = 0.f;
	// 캐릭터 정면 기준 실제 이동 방향(-180~180). Direction은 여기서 몸이 돌아간 만큼(RootYawOffset)을 뺀 값
	float VelocityDirection = 0.f;
	// 몸 기준 이동 방향(-180~180). ForwardDirection·BackwardDirection과 멈춤 클립 선택에 쓴다
	float Direction = 0.f;
	// 회전 애니메이션의 RemainingTurnYaw(가중치로 나눈 값) 직전 프레임 값
	float PrevTurnYawCurve = 0.f;
	bool bTurnCurveActive = false;
	// 회전 마무리 중(회전량 커브는 끝났고 애니메이션 끝부분을 재생 중) / 남은 마무리 시간
	bool bTurnRecovering = false;
	float TurnRecoveryLeft = 0.f;
	float TurnCooldown = 0.f;
};
