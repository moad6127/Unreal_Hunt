// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntAnimInstance.generated.h"

class AHuntCharacter;
class UAnimSequence;
class UCurveVector;

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

	// 기울기(-1~1): 앞뒤(가속 +), 좌우(오른쪽 가속 +). ABP의 ALS_N_Lean 블렌드스페이스(X = LR, Y = FB) 입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LeanFB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LeanLR = 0.f;

	// 기울기 애디티브 가중치(걷기 0.5 → 조깅 1, ALS Weight_Gait 방식)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LeanAlpha = 0.f;

	// 기울기가 가속 변화를 따라가는 속도(ALS GroundedLeanInterpSpeed = 4)
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float LeanInterpSpeed = 4.f;

	// 블렌드스페이스에 넣는 방향(Direction)이 실제 이동 방향을 따라가는 속도(ALS VelocityBlendInterpSpeed = 12). 0이면 바로
	// 높이면 반응이 빨라지고, 낮추면 방향을 바꿀 때 다리가 부드럽지만 바꾼 직후 발이 잠깐 미끄러진다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float DirectionInterpSpeed = 12.f;

	// 옆 구역의 반폭(도): 90±이 값 안에서는 골반 방향(앞/뒤 블렌드스페이스)을 바꾸지 않는다(ALS 방식)
	// 넓히면 방향을 자주 바꿔도 골반이 덜 돌지만, 블렌드스페이스가 반대 반구를 접어 읽는 만큼(최대 2배) 발이 앞뒤로 미끄러진다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SideQuadrantHalfWidth = 10.f;

	// 골반 방향을 바꿀 때 왼발이 오른발보다 캐릭터 기준 옆으로 이만큼(cm) 이상 왼쪽에 있는 순간을 기다린다(다리가 교차해 있는 동안에는 안 바꾼다)
	// 서 있을 때 두 발 옆 간격은 약 36cm, 옆걸음에서 다리를 교차하면 음수가 된다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float HipsSwitchFeetApart = 15.f;

	// 위 순간을 기다리는 최대 시간(초). 걷기 한 주기(약 1.1초) 안에는 보통 그런 순간이 온다
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float MaxHipsSwitchDelay = 1.f;

	// 뒤에서 옆으로 왔거나 좌우를 바로 바꿔 뒷걸음 골반(LB/RB)으로 옆걸음할 때, 이만큼(초) 지나면 앞을 보는 옆걸음(LF/RF)으로 돌아간다
	// ABP 앞↔뒤 전환 블렌드(0.5초)가 끝난 뒤가 되도록 맞춘다(ALS는 상태 가중치가 1이 된 뒤)
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float HipsSettleTime = 0.5f;

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

	// 대기 자세 회전(ALS_N_TurnIP_L90/R90) 재생 시간 = TurnTime × RelaxedTurnPlayRate. ABP ALS 회전 Sequence Evaluator(Explicit Time)에 넣는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TurnInPlace")
	float RelaxedTurnTime = 0.f;

	// ALS 회전은 다 도는 데 1.17초로 Lyra(0.63초)보다 느려, 계속 돌아볼 때 회전이 끝나기 전에 몸이 시선에 끌려간다. 이 배율로 빨리 재생한다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.1"))
	float RelaxedTurnPlayRate = 1.6f;

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

	// 회전량 커브가 끝난 뒤(TurnYawWeight = 0) 회전 애니메이션을 이만큼(초) 더 재생해 발을 내려놓는 마무리까지 보여 준다(Lyra Turn Recovery)
	// 그사이 몸과 시선이 다시 TurnStartAngle 이상 벌어지면 바로 끝내고 새로 돈다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnRecoveryTime = 0.5f;

	// 움직이는 동안 몸이 목표 방향(시선 또는 아래 이동 방향 쪽)으로 돌아가는 속도
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace", meta = (ClampMin = "0.0"))
	float RootYawBlendOutSpeed = 10.f;

	// 대기 자세로 걸을 때 몸(루트)을 이동 방향 쪽으로 돌리는 양(ALS YawOffset_FB/LR, 입력 = 시선 기준 이동 방향)
	// FB: X = 앞 구역, Y = 뒤 구역. LR: X = 왼쪽 구역, Y = 오른쪽 구역. 상체는 시선 애디티브(AimYaw)가 시선으로 되돌린다
	// 비어 있으면 몸을 시선 정면으로 둔다
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	TObjectPtr<UCurveVector> MoveYawOffsetFB;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	TObjectPtr<UCurveVector> MoveYawOffsetLR;

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

	// 질주 상체 블렌드(0~1, GroundSpeed가 SprintBlendSpeed 구간을 지나는 정도). ABP Overlay의 질주 블렌드와
	// 대기·벽 앞 내린 자세 가중치(질주 중에는 뺌)가 같이 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float SprintAlpha = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector2D SprintBlendSpeed = FVector2D(400.f, 550.f);

	// ABP Layering(ALS LayerBlending) 부위별 걸음 흔들림 가중치: 상체 = Overlay ⊕ (Base ⊖ ALS_N_Pose) × 가중치
	// 척추·머리(spine_01 레이어가 목까지 덮음). 준비·조준 값과 대기 값을 RelaxedAlpha로 섞는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Layering")
	float SpineAddAlpha = 0.f;

	// 팔(clavicle 레이어, 로컬 공간)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Layering")
	float ArmLAddAlpha = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Layering")
	float ArmRAddAlpha = 0.f;

	// 준비·조준: 이 값에서 시작해 속도가 ReadySpineFadeSpeed 구간을 지나며 0이 된다(총을 겨눈 상체가 흔들리지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReadySpineAdd = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering")
	FVector2D ReadySpineFadeSpeed = FVector2D(300.f, 600.f);

	// 대기: 걷기(WalkAnimSpeed)에서 RelaxedSpineAdd(ALS 걷기 실측 1.0), Jog(JogAnimSpeed)에서 RelaxedJogSpineAdd
	// Base가 GASP Jog라 1.0이면 상체가 너무 숙고 흔들린다(11-08)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RelaxedSpineAdd = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RelaxedJogSpineAdd = 0.4f;

	// 대기 팔(ALS 걷기 실측 왼쪽 0, 오른쪽 0.2~0.3). 준비·조준은 0
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RelaxedArmLAdd = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Layering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RelaxedArmRAdd = 0.3f;

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

	// 조준 중 서 있는 자세(Lyra 사격 Idle, 왼발 앞). 발의 Idle 자리(ik_foot 위치)를 여기서 읽는다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequence> IdleAnim;

	// 대기·준비 중 서 있는 자세(ALS 기본자세, 두 발 나란히). ABP Idle 하체와 같은 애니메이션. 비우면 IdleAnim 자리를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequence> RelaxedIdleAnim;

	// 서 있는 하체 진행도(0 = ALS 기본자세 두 발 나란히, 1 = Lyra 사격 자세). 대기가 아니면(준비·조준) 1로 LegStanceBlendTime 동안 일정하게 변한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	float ShootingLegsAlpha = 0.f;

	// 하체 전환 애니메이션을 평가할 시간 = ShootingLegsAlpha × 길이. ABP Idle 하체의 Sequence Evaluator(Explicit Time)에 넣는다
	// 조준하면 앞으로, 대기로 돌아가면 거꾸로 재생되는 셈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FootLock")
	float ShootingLegsTime = 0.f;

	// 서 있는 하체 전환 애니메이션(ALS 기본자세 → Lyra 사격 자세, 골반·다리와 발 옮기기 포함). ABP가 평가하는 것과 같은 애니메이션
	UPROPERTY(EditDefaultsOnly, Category = "FootLock")
	TObjectPtr<UAnimSequence> LegTransitionAnim;

	// 서 있는 하체가 두 자세 사이를 바꾸는 시간(초) = 전환 애니메이션 재생 시간
	UPROPERTY(EditDefaultsOnly, Category = "FootLock", meta = (ClampMin = "0.01"))
	float LegStanceBlendTime = 0.3f;

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

	void UpdateVelocity(float DeltaSeconds);
	void UpdateFoldedDirections(float DeltaSeconds);
	// 현재 방향의 가장 빠른 이동 애니메이션 속도
	float GetRunAnimSpeed() const;
	void UpdateGaitBlend();
	void UpdateMovingBackward(float DeltaSeconds);
	// 캐릭터 기준 옆으로 왼발이 오른발보다 왼쪽에 있는 거리(cm). 음수면 다리가 교차했다
	float GetFeetSideGap() const;
	void UpdateWeapon();
	void UpdateAimPitch();

	void UpdateTurnInPlace(float DeltaSeconds);
	void UpdateRootYawOffset(float DeltaSeconds, bool bCanTurn);
	// 이동 중 몸 Yaw 목표(ALS YawOffset): 대기 자세에서만, RelaxedAlpha만큼
	float GetMoveYawOffset() const;
	void AdvanceTurnTimers(float DeltaSeconds);
	void ApplyTurnYawCurve();
	void TryStartTurn(bool bCanTurn);
	void EndTurn();

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
		// 지금 서 있는 자세의 발 자리(컴포넌트 공간, 루트 회전 전). 조준 여부에 따라 아래 둘 중 하나
		FVector IdleLocation = FVector::ZeroVector;
	};
	FFootLock Feet[2];
	FVector AimIdleFeet[2] = {FVector::ZeroVector, FVector::ZeroVector};
	FVector RelaxedIdleFeet[2] = {FVector::ZeroVector, FVector::ZeroVector};
	float StandingTime = 0.f;
	bool bWasMovingForFootLock = false;

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
	// 회전 애니메이션의 RemainingTurnYaw(가중치로 나눈 값) 직전 프레임 값
	float PrevTurnYawCurve = 0.f;
	bool bTurnCurveActive = false;
	// 회전 마무리 중(회전량 커브는 끝났고 애니메이션 끝부분을 재생 중) / 남은 마무리 시간
	bool bTurnRecovering = false;
	float TurnRecoveryLeft = 0.f;
	float TurnCooldown = 0.f;
};
