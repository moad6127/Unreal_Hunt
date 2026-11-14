// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveVector.h"

namespace
{
	const FName FootBones[2] = {TEXT("foot_l"), TEXT("foot_r")};
	// 서 있는 발의 높이(cm, 대기 자세 ik_foot Z). 발을 고정할 때 이 높이에 둔다
	constexpr float StandingFootZ = 8.9f;

	// 아래 값들은 에셋·ABP 설정과 맞물려 있어 따로 조절할 값이 아니다. 그쪽을 바꾸면 여기도 맞춘다
	// 가장 빠른 이동 애니메이션 자체의 속도 = BS_Hunt_Loco_Fwd/Bwd의 최고 Speed 샘플 값 (정면 ALS 질주 / 옆·뒤 ALS 달리기)
	constexpr float ForwardRunAnimSpeed = 600.f;
	// 조깅 샘플(ALS 달리기) 속도. 모든 방향이 같고, 옆·뒤의 최고 속도이기도 하다
	constexpr float JogAnimSpeed = 350.f;
	// 걷기 샘플(ALS 걷기) 속도. 모든 방향이 같다(ALS 원본 ABP의 걷기 기준 속도)
	constexpr float WalkAnimSpeed = 150.f;
	// 뒤에서 옆으로 왔거나 좌우를 바로 바꿔 뒷걸음 골반(LB/RB)으로 옆걸음할 때, 이만큼(초) 지나면 앞을 보는 옆걸음(LF/RF)으로 돌아간다
	// ABP Gait 앞↔뒤 전환 블렌드(0.5초)가 끝난 뒤가 되도록 맞춘다(ALS는 상태 가중치가 1이 된 뒤)
	constexpr float HipsSettleTime = 0.5f;
	// 몸과 시선이 이 이상 벌어지면 몸이 바로 따라온다
	// 에임 오프셋(AO_MM_Rifle_Idle_Hipfire) Yaw 범위가 ±90이라 그 이상이면 상체가 시선을 못 따라가 비틀려 보인다
	constexpr float MaxRootYawOffset = 90.f;
	// 회전이 끝난 뒤 다음 회전까지 쉬는 시간. ABP 회전 블렌드 시간(0.2초) 이상이어야 이전 회전이 다 빠진 뒤 새로 시작해 튀지 않는다
	constexpr float TurnRetriggerDelay = 0.2f;
}

void UHuntAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AHuntCharacter>(TryGetPawnOwner());
	if (Character)
	{
		PrevActorYaw = Character->GetActorRotation().Yaw;
	}
}

void UHuntAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 프리뷰에는 캐릭터가 없으므로 값을 덮어쓰지 않는다
	if (!Character)
	{
		return;
	}

	UpdateVelocity(DeltaSeconds);
	UpdateFoldedDirections(DeltaSeconds);
	UpdateGaitBlend();
	UpdateMovingBackward(DeltaSeconds);
	UpdateWeapon(DeltaSeconds);
	UpdateAimPitch();
	UpdateTurnInPlace(DeltaSeconds);
	UpdateFootLock(DeltaSeconds);
}

bool UHuntAnimInstance::TryPlayCrossedStop()
{
	// 옆걸음으로 멈출 때 두 발이 꼬여 있을 때만. 클립의 시작 속도(186)를 멈춤 진입 속도(약 150)에 맞춘다
	return GetFeetSideGap() < 0.f && TryPlayStopClip(CrossedStopClips, 0.8f, 186.f);
}

bool UHuntAnimInstance::TryPlayJogStop()
{
	// 걷기 멈춤과 같은 조건: 입력을 놓은 뒤 두 발이 꼬였을 때만. 걷기 최고 속도(165)보다 확실히 빠를 때만 Jog로 본다
	// 몸이 뒤를 향한 옆걸음은 클립(LR·RL)과 골반 방향이 달라 제외한다
	constexpr float MinJogStopSpeed = 200.f;
	if (GroundSpeed <= MinJogStopSpeed || bMovingBackward || GetFeetSideGap() >= 0.f)
	{
		return false;
	}
	// 클립의 시작 속도(350)를 지금 속도에 맞춘다
	return TryPlayStopClip(JogStopClips, FMath::Clamp(GroundSpeed / 350.f, 0.6f, 1.f), 350.f);
}

bool UHuntAnimInstance::TryPlayStopClip(const TArray<FHuntStopClip>& Clips, float PlayRate, float ClipFootSpeed)
{
	if (FMath::Abs(FMath::Abs(Direction) - 90.f) > 45.f || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}
	const USkeletalMeshComponent* Mesh = GetOwningComponent();
	const FTransform& MeshTransform = Mesh->GetComponentTransform();
	const FVector FootLocalL = MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[0]));
	const FVector FootLocalR = MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[1]));
	const FVector2D FootL(FootLocalL);
	const FVector2D FootR(FootLocalR);

	const FHuntStopClip* Best = nullptr;
	double BestScore = TNumericLimits<double>::Max();
	const bool bAimStance = ShootingLegsAlpha >= 0.5f;
	for (const FHuntStopClip& Clip : Clips)
	{
		const double Score = FVector2D::DistSquared(Clip.FootL, FootL) + FVector2D::DistSquared(Clip.FootR, FootR);
		if (Clip.Anim && Clip.bMovingLeft == (Direction < 0.f) && Clip.bAimStance == bAimStance && Score < BestScore)
		{
			Best = &Clip;
			BestScore = Score;
		}
	}
	if (!Best)
	{
		return false;
	}
	// 디딘 발이 클립과 같은 자리에서 시작하도록 시작 시간을 맞춘다. 디딘 발은 몸 기준 이동 반대쪽으로 ClipFootSpeed로 움직인다. 블렌드 아웃 시작 시간은 실제 시간 기준이라 재생 속도로 나눈다
	const bool bLeftDown = FootLocalL.Z <= FootLocalR.Z;
	const double ClipX = bLeftDown ? Best->FootL.X : Best->FootR.X;
	const double GameX = bLeftDown ? FootL.X : FootR.X;
	const float StartOffset = FMath::Clamp(static_cast<float>((Best->bMovingLeft ? ClipX - GameX : GameX - ClipX) / ClipFootSpeed), -0.12f, 0.12f);
	CrossedStopMontage = PlaySlotAnimationAsDynamicMontage(Best->Anim, TEXT("DefaultSlot"), 0.15f, 0.25f, PlayRate, 1, (Best->Anim->GetPlayLength() - Best->EndTime) / PlayRate, Best->StartTime + StartOffset);
	CrossedStopEndTime = Best->EndTime;
	return CrossedStopMontage != nullptr;
}

void UHuntAnimInstance::RequestStopFoot(int32 Side, bool bPlant)
{
	if (bSkipStopFeet)
	{
		bSkipStopFeet = false;
		return;
	}
	Feet[Side].bLockRequested = true;
	// ALS Plant 전환 크로스페이드가 0.1초라 다리가 디딘 포즈로 옮겨진 뒤에 고정한다
	Feet[Side].LockDelay = bPlant ? 0.1f : 0.f;
	if (UAnimSequenceBase* StopDown = Side == 0 ? StopDownAnimL : StopDownAnimR)
	{
		// ALS PlayTransition 값: 블렌드 0.2초, 재생 1.5배
		PlaySlotAnimationAsDynamicMontage(StopDown, TEXT("DefaultSlot"), 0.2f, 0.2f, 1.5f);
	}
}

void UHuntAnimInstance::AnimNotify_HuntStopLockL() { RequestStopFoot(0, false); }
void UHuntAnimInstance::AnimNotify_HuntStopLockR() { RequestStopFoot(1, false); }
void UHuntAnimInstance::AnimNotify_HuntStopPlantL() { RequestStopFoot(0, true); }
void UHuntAnimInstance::AnimNotify_HuntStopPlantR() { RequestStopFoot(1, true); }

void UHuntAnimInstance::UpdateFootLock(float DeltaSeconds)
{
	// 서 있는 자세: 대기가 아니면(준비·조준) Lyra 사격 자세(왼발 앞), 대기면 ALS 기본자세(두 발 나란히)
	// ABP Idle 하체는 전환 애니메이션(LegTransitionAnim)을 ShootingLegsTime으로 평가한다. 발 옮기기도 애니메이션에 들어 있다
	// 마지막 한 걸음 클립이 블렌드 아웃까지 끝날 때는 클립을 고를 때의 자세로 착지한다. 그동안 자세가 바뀌면 전환 애니메이션이 클립에 가려져 발이 끌려간다
	if (!CrossedStopMontage || !GetInstanceForMontage(CrossedStopMontage))
	{
		ShootingLegsAlpha = FMath::FInterpConstantTo(ShootingLegsAlpha, Character->GetRelaxedBlend() < 0.5f ? 1.f : 0.f, DeltaSeconds, 1.f / LegStanceBlendTime);
	}
	ShootingLegsTime = LegTransitionAnim ? ShootingLegsAlpha * LegTransitionAnim->GetPlayLength() : 0.f;
	const bool bLegTransition = ShootingLegsAlpha > 0.f && ShootingLegsAlpha < 1.f;

	// 제자리 회전 중이나 하체 전환 중에는 애니메이션이 발을 옮기므로 고정하지 않는다
	// 마지막 한 걸음 클립 중에는 애니메이션이 발과 몸을 옮긴다. 이동 입력이 들어오면 바로 끊는다
	const bool bCrossedStop = CrossedStopMontage && Montage_IsActive(CrossedStopMontage);
	if (bCrossedStop && !Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		Montage_Stop(0.2f, CrossedStopMontage);
	}
	const bool bStanding = !bShouldMove && !bIsFalling && TurnDirection == 0 && !bLegTransition && !bCrossedStop;

	// ALS Plant 시간(초, 걷기 사이클 1.133초): {정면, 왼쪽, 오른쪽}. 뒷걸음과 앞걸음이 따로 있고, 사이 방향은 보간한다
	static const float PlantSecondsL[2][3] = {{0.133f, 0.2f, 0.233f}, {0.133f, 0.133f, 0.233f}};
	static const float PlantSecondsR[2][3] = {{0.7f, 0.8f, 0.766f}, {0.7f, 0.8f, 0.7f}};
	const float PlantDirection = bMovingBackward ? BackwardDirection : ForwardDirection;
	const float SideAlpha = FMath::Clamp((bMovingBackward ? 180.f - FMath::Abs(PlantDirection) : FMath::Abs(PlantDirection)) / 90.f, 0.f, 1.f);
	const int32 PlantRow = bMovingBackward ? 1 : 0;
	const int32 PlantSide = PlantDirection < 0.f ? 1 : 2;
	PlantTimeL = FMath::Lerp(PlantSecondsL[PlantRow][0], PlantSecondsL[PlantRow][PlantSide], SideAlpha) / 1.133f;
	PlantTimeR = FMath::Lerp(PlantSecondsR[PlantRow][0], PlantSecondsR[PlantRow][PlantSide], SideAlpha) / 1.133f;

	const USkeletalMeshComponent* Mesh = GetOwningComponent();
	const FTransform& MeshTransform = Mesh->GetComponentTransform();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FFootLock& Foot = Feet[Side];
		FVector Target = Foot.LockedWorld;
		const float FootLockCurve = GetCurveValue(Side == 0 ? TEXT("FootLock_L") : TEXT("FootLock_R"));
		// Stop_Down의 FootLock 커브 +1: 아직 안 디딘 반대쪽 발을 지금 자리에 고정한다
		if (bStanding && !Foot.bLocked && !Foot.bLockRequested && FootLockCurve >= 0.99f)
		{
			Foot.bLockRequested = true;
			Foot.LockDelay = 0.f;
		}
		// 고정은 ABP Stop 상태의 노티파이가 요청한다(ALS (N) Stop States). Plant는 다리가 디딘 포즈로 옮겨진 뒤에 고정한다
		if (Foot.bLockRequested)
		{
			Foot.LockDelay -= DeltaSeconds;
		}
		const bool bLockNow = Foot.bLockRequested && Foot.LockDelay <= 0.f;
		if (!bStanding || (!Foot.bLocked && !bLockNow))
		{
			// 움직이기 시작하면 고정을 짧게 풀어 발이 순간 이동하지 않게 한다
			Foot.bLocked = false;
			Foot.bLockRequested = Foot.bLockRequested && bStanding;
			Foot.Alpha = FMath::FInterpConstantTo(Foot.Alpha, 0.f, DeltaSeconds, 1.f / FootUnlockTime);
		}
		else if (!Foot.bLocked)
		{
			// 지금 자리(지난 프레임 최종 포즈)의 바닥 높이에 고정한다
			FVector FootLocal = MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[Side]));
			FootLocal.Z = StandingFootZ;
			Foot.LockedWorld = MeshTransform.TransformPosition(FootLocal);
			Foot.Alpha = 1.f;
			Foot.bLocked = true;
			Foot.bLockRequested = false;
			Target = Foot.LockedWorld;
		}
		else
		{
			// ALS SetFootLocking: Stop_Down의 FootLock 커브(-1 = 풀림)만큼 고정을 푼다. 커브가 돌아오면 그 자리에 다시 고정한다
			const float LockCurve = 1.f + FMath::Clamp(FootLockCurve, -1.f, 0.f);
			if (LockCurve < Foot.Alpha)
			{
				Foot.Alpha = LockCurve;
			}
			else if (Foot.Alpha < 1.f && LockCurve >= 0.99f)
			{
				Foot.bLocked = false;
				Foot.bLockRequested = true;
				Foot.LockDelay = 0.f;
			}
		}
		(Side == 0 ? FootLockAlphaL : FootLockAlphaR) = Foot.Alpha;
		(Side == 0 ? FootLockLocationL : FootLockLocationR) = MeshTransform.InverseTransformPosition(Target);
	}

}

void UHuntAnimInstance::UpdateVelocity(float DeltaSeconds)
{
	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VelocityDirection = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw) : 0.f;
	// 이동 블렌드스페이스는 돌아간 몸 기준 방향으로 고른다(몸이 이동 방향 쪽으로 돌아가 있으면 옆걸음이 대각선 걸음이 된다)
	Direction = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(VelocityDirection - RootYawOffset) : 0.f;
	// ALS ShouldMove 방식: 입력이 있거나 멈춤 속도(AHuntCharacter::StopBrakeSpeed, ALS 150)보다 빠르면 이동 중. 그 아래면 ABP Stop 상태로 간다
	// 다른 플레이어 캐릭터(시뮬레이티드 프록시)는 입력(가속)을 모르므로 입력이 있는 것으로 보고 속도로만 판정한다
	const bool bHasMovementInput = Character->GetLocalRole() == ROLE_SimulatedProxy
		|| !Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
	const bool bWantsMove = GroundSpeed > 3.f && (bHasMovementInput || GroundSpeed > Character->GetStopBrakeSpeed());
	const bool bWasClipActive = bCrossedStopActive;
	bCrossedStopActive = CrossedStopMontage && Montage_IsActive(CrossedStopMontage) && Montage_GetPosition(CrossedStopMontage) < CrossedStopEndTime;
	if (bShouldMove && !bCrossedStopActive && ((!bHasMovementInput && TryPlayJogStop()) || (!bWantsMove && TryPlayCrossedStop())))
	{
		bCrossedStopActive = true;
	}
	// 클립이 끝나며 들어가는 Stop 상태에서는 발이 이미 제자리라 발 고정·Stop_Down을 하지 않는다
	bSkipStopFeet = !bWantsMove && (bSkipStopFeet || (bWasClipActive && !bCrossedStopActive));
	// 클립 중에는 ABP를 Move에 둔다. 클립이 섞여 들어오는 동안 아래 포즈가 Plant·Idle로 바뀌면 발이 밀리고 흔들린다
	bShouldMove = bWantsMove || bCrossedStopActive;
	bIsFalling = Character->GetCharacterMovement()->IsFalling();

	// 기울기(ALS CalculateRelativeAccelerationAmount + InterpLeanAmount): 속도 변화(가속)를 캐릭터 기준으로 바꿔
	// 가속 중이면 최대 가속, 감속 중이면 최대 제동으로 나눈 비율(-1~1). ABP가 ALS_N_Lean 애디티브로 몸을 그쪽으로 기울인다
	if (DeltaSeconds > KINDA_SMALL_NUMBER)
	{
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const FVector Accel = (Velocity - PrevVelocity) / DeltaSeconds;
		const float MaxAccel = FVector::DotProduct(Accel, Velocity) > 0.f ? Movement->GetMaxAcceleration() : Movement->GetMaxBrakingDeceleration();
		// 몸(루트)이 RootYawOffset만큼 돌아가 있으면 그 몸 기준으로 기울인다
		const FVector Relative = (Character->GetActorRotation() + FRotator(0.f, RootYawOffset, 0.f)).UnrotateVector(Accel.GetClampedToMaxSize(MaxAccel)) / FMath::Max(MaxAccel, 1.f);
		LeanFB = FMath::FInterpTo(LeanFB, Relative.X, DeltaSeconds, LeanInterpSpeed);
		LeanLR = FMath::FInterpTo(LeanLR, Relative.Y, DeltaSeconds, LeanInterpSpeed);
	}
	PrevVelocity = Velocity;
}

void UHuntAnimInstance::UpdateFoldedDirections(float DeltaSeconds)
{
	// 반대 반구의 방향은 앞뒤로 뒤집어 접는다(예: -120 → 앞쪽 -60). 두 값 모두 끊김 없이 이어진다
	const float AbsDirection = FMath::Abs(Direction);
	const float MirroredDirection = (Direction < 0.f ? -1.f : 1.f) * (180.f - AbsDirection);
	const float TargetForward = AbsDirection <= 90.f ? Direction : MirroredDirection;
	// 뒤 블렌드스페이스 방향은 정뒤(180)를 0으로 둔 값(-90~90)으로 바꿔 보간한다
	const float Backward = AbsDirection >= 90.f ? Direction : MirroredDirection;
	const float TargetBackwardOffset = (Backward < 0.f ? -1.f : 1.f) * (180.f - FMath::Abs(Backward));

	// ALS InterpVelocityBlend처럼 블렌드스페이스 방향은 바로 넣지 않고 DirectionInterpSpeed로 따라가게 한다
	// 각도가 아니라 반구마다 접은 값(-90~90)을 선형으로 보간한다: 좌우를 뒤집으면(180° 차이) 앞 블렌드스페이스는 정면(LF→F→RF),
	// 뒤 블렌드스페이스는 정뒤(LB→B→RB)를 지나 이어진다. 각도로 보간하면 앞/뒤 어느 쪽으로 돌지 매번 달라져 골반 상태와 어긋났다
	// 서 있다가 출발할 때는 바로 맞춘다(지난번 멈춘 방향에서 보간하지 않게)
	const float Step = DirectionInterpSpeed > 0.f && bWasMovingForDirection ? FMath::Min(1.f, DeltaSeconds * DirectionInterpSpeed) : 1.f;
	bWasMovingForDirection = bShouldMove;
	ForwardDirection += (TargetForward - ForwardDirection) * Step;
	BackwardDirectionOffset += (TargetBackwardOffset - BackwardDirectionOffset) * Step;
	BackwardDirection = (BackwardDirectionOffset < 0.f ? -1.f : 1.f) * (180.f - FMath::Abs(BackwardDirectionOffset));
}

float UHuntAnimInstance::GetRunAnimSpeed() const
{
	// 블렌드스페이스의 최고 속도 샘플이 정면·옆·뒤(0/90/180도)에만 있어서 그 사이는 선형으로 섞는다
	return FMath::Lerp(ForwardRunAnimSpeed, JogAnimSpeed, FMath::Min(FMath::Abs(Direction) / 90.f, 1.f));
}

void UHuntAnimInstance::UpdateGaitBlend()
{
	// 조깅 중이어도 걷기 샘플보다 느리면(벽에 막혀 미끄러질 때, 멈추는 중) 걷기로 바꾼다
	// 조깅 블렌드스페이스는 조깅 샘플 속도로 고정되어 느리면 큰 보폭을 슬로모션으로 밟는다
	bRunGait = Character->IsRunGait() && GroundSpeed > WalkAnimSpeed;
	// 느릴 때는 재생 속도를 MinPlayRate까지만 내리고, 나머지는 Idle과 섞어(블렌드 속도) 보폭을 줄여 맞춘다
	WalkPlayRate = FMath::Max(GroundSpeed / WalkAnimSpeed, MinPlayRate);
	WalkBlendSpeed = FMath::Min(GroundSpeed / WalkPlayRate, WalkAnimSpeed);
	RunBlendSpeed = FMath::Max(GroundSpeed, JogAnimSpeed);
	LocomotionPlayRate = FMath::Max(GroundSpeed / FMath::Clamp(GroundSpeed, JogAnimSpeed, FMath::Max(JogAnimSpeed, GetRunAnimSpeed())), MinPlayRate);
}

float UHuntAnimInstance::GetFeetSideGap() const
{
	// 캐릭터 기준 옆 방향으로 왼발이 오른발보다 얼마나 왼쪽에 있는지(cm). 음수면 다리가 교차했다
	// 메시 컴포넌트 공간 +X = 캐릭터 왼쪽. 지난 프레임 최종 포즈 기준
	const USkeletalMeshComponent* Mesh = GetOwningComponent();
	const FTransform& MeshTransform = Mesh->GetComponentTransform();
	return MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[0])).X
		- MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[1])).X;
}

void UHuntAnimInstance::UpdateMovingBackward(float DeltaSeconds)
{
	if (!bShouldMove)
	{
		HipsSwitchWaitTime = 0.f;
		return;
	}

	// ALS 방식(Directional States): 캐릭터 정면 기준 이동 방향을 앞·옆·뒤 구역으로 나눈다
	// 앞 구역이면 앞걸음, 뒤 구역이면 뒷걸음. 옆 구역에 들어오면 일단 지금 골반 방향을 유지하고(뒤에서 왔으면 LB/RB),
	// 좌우를 바로 뒤집을 때는 골반 상태를 그대로 두고 블렌드스페이스 방향 보간(UpdateFoldedDirections)이 정면/정뒤를 지나 이어지게 한다
	// 뒷걸음 골반으로 옆걸음하는 건 잠깐뿐: 전환 블렌드가 끝나면(HipsSettleTime) 앞을 보는 옆걸음(LF/RF)으로 돌아간다(ALS LB→LF, RB→RF)
	TimeSinceHipsSwitch += DeltaSeconds;
	const float AbsDirection = FMath::Abs(VelocityDirection);
	if (AbsDirection < 90.f - SideQuadrantHalfWidth)
	{
		bHipsBackwardTarget = false;
	}
	else if (AbsDirection > 90.f + SideQuadrantHalfWidth)
	{
		bHipsBackwardTarget = true;
	}
	else if (bMovingBackward && bHipsBackwardTarget && TimeSinceHipsSwitch >= HipsSettleTime)
	{
		bHipsBackwardTarget = false;
	}

	// 골반 방향은 다리가 교차하지 않을 때만 바꾼다(ALS와 같은 조건: 옆걸음 애니메이션의 Feet_Crossing 커브 = 0, 지난 프레임 최종 포즈 커브)
	// 앞뒤 달리기와 Jog 앞쪽 옆걸음에는 이 커브가 없어(0) 바로 바뀐다
	if (bHipsBackwardTarget == bMovingBackward)
	{
		HipsSwitchWaitTime = 0.f;
		return;
	}
	HipsSwitchWaitTime += DeltaSeconds;
	// 이동 방향이 반대 반구 깊숙이(정면·정뒤 ±45도 안) 들어갔으면 기다리지 않는다: 블렌드스페이스가 방향을 접어 읽어서,
	// 기다리는 동안 반대 방향 걸음(문워크)이 나온다(11-10 로그: 뒤로 가는데 앞걸음 0.75초). 걷기 옆걸음에서 넘어올 때 커브가 아직 남아 있을 수 있다
	const bool bDeepInTargetHemisphere = bHipsBackwardTarget ? AbsDirection > 135.f : AbsDirection < 45.f;
	if (bDeepInTargetHemisphere || GetCurveValue(TEXT("Feet_Crossing")) < 0.01f || HipsSwitchWaitTime >= MaxHipsSwitchDelay)
	{
		bMovingBackward = bHipsBackwardTarget;
		HipsSwitchWaitTime = 0.f;
		TimeSinceHipsSwitch = 0.f;
	}
}

void UHuntAnimInstance::UpdateWeapon(float DeltaSeconds)
{
	const AHuntWeapon* Weapon = Character->GetBodyWeapon();
	const bool bHasWeapon = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent();
	AimAlpha = Character->GetAimBlend();
	// 내린 자세 애디티브는 지향사격 상체 기준이라 질주 상체 자세 위에서는 뺀다
	const bool bSprinting = GroundSpeed > (SprintBlendSpeed.X + SprintBlendSpeed.Y) * 0.5f;
	SprintBlend = FMath::FInterpConstantTo(SprintBlend, bSprinting ? 1.f : 0.f, DeltaSeconds, 1.f / SprintBlendTime);
	SprintAlpha = FMath::SmoothStep(0.f, 1.f, SprintBlend);
	WallBlockAlpha = Character->GetWallBlockAlpha() * (1.f - SprintAlpha);
	RelaxedAlpha = Character->GetRelaxedBlend() * (1.f - SprintAlpha);
	if (Character->IsLocallyControlled() && GetSkelMeshComponent()->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::WorldSpaceRepresentation)
	{
		RelaxedAlpha *= LocalShadowRelaxedScale;
	}
	AimOffsetAlpha = 1.f - RelaxedAlpha;
	LeftHandIKAlpha = bHasWeapon ? 1.f : 0.f;
	if (bHasWeapon)
	{
		const FTransform HandTransform = GetSkelMeshComponent()->GetSocketTransform(TEXT("hand_r"));
		LeftHandIKLocation = HandTransform.InverseTransformPosition(Weapon->GetLeftHandGripLocation(AimAlpha));
	}
}

void UHuntAnimInstance::UpdateAimPitch()
{
	// 컨트롤러가 있으면 컨트롤 회전(GetBaseAimRotation은 플레이어면 카메라 회전이라 고정 카메라에서 안 변함), 다른 플레이어 캐릭터는 복제된 피치
	const FRotator AimRotation = Character->GetController() ? Character->GetControlRotation() : Character->GetBaseAimRotation();
	// 벽에 막혀 총을 내린 만큼 위아래 조준을 뺀다. Yaw는 제자리 회전 보정이라 그대로 둔다
	// 대기 자세에서는 빼지 않는다: 피치가 상체를 숙여 머리(카메라)를 앞으로 내보내야 아래를 볼 때 카메라가 하체 메시 안쪽 위에 오지 않는다
	AimPitch = FRotator::NormalizeAxis(AimRotation.Pitch) * (1.f - Character->GetWallBlockAlpha());
}

void UHuntAnimInstance::UpdateTurnInPlace(float DeltaSeconds)
{
	const bool bCanTurn = !bShouldMove && !bIsFalling;
	UpdateRootYawOffset(DeltaSeconds, bCanTurn);
	AdvanceTurnTimers(DeltaSeconds);
	RelaxedTurnTime = TurnTime * RelaxedTurnPlayRate;
	ApplyTurnYawCurve();

	// 회전 마무리: 끝나거나, 그사이 몸과 시선이 다시 벌어지면(새 회전 필요) Idle로 돌아간다
	// shortcut: 커브가 끝내 안 들어오는 등 회전이 끝나지 않는 경우를 대비해 MaxTurnDuration이 지나면 강제로 끝낸다, 원인을 찾으면 지운다
	constexpr float MaxTurnDuration = 3.f;
	if (bTurnRecovering)
	{
		TurnRecoveryLeft -= DeltaSeconds;
		if (TurnRecoveryLeft <= 0.f || FMath::Abs(RootYawOffset) > TurnStartAngle)
		{
			EndTurn();
		}
	}
	else if (TurnDirection != 0 && TurnTime > MaxTurnDuration)
	{
		EndTurn();
	}

	RootYawOffset = FMath::Clamp(RootYawOffset, -MaxRootYawOffset, MaxRootYawOffset);

	TryStartTurn(bCanTurn);
	AimYaw = -RootYawOffset;
	LookYawTime = FMath::GetMappedRangeValueClamped(FVector2D(-180.f, 180.f), FVector2D(0.f, 1.f), AimYaw);
}

void UHuntAnimInstance::UpdateRootYawOffset(float DeltaSeconds, bool bCanTurn)
{
	const float ActorYaw = Character->GetActorRotation().Yaw;
	const float YawDelta = FRotator::NormalizeAxis(ActorYaw - PrevActorYaw);
	PrevActorYaw = ActorYaw;

	if (bCanTurn)
	{
		// 액터가 시선을 따라 돈 만큼 반대로 돌려 몸을 월드에 고정. NormalizeAxis를 쓰면 빠르게 돌 때 180을 넘어 부호가 뒤집힌다
		RootYawOffset = FMath::Clamp(RootYawOffset - YawDelta, -MaxRootYawOffset, MaxRootYawOffset);
	}
	else
	{
		// 이동 중: 몸을 시선 방향으로 되돌린다. 대기 자세면 ALS처럼 이동 방향 쪽으로 YawOffset만큼 돌린 곳으로
		// ALS SmoothCharacterRotation처럼 도는 속도를 초당 MaxMoveYawRate로 제한한다. 구역 경계에서 목표가 수십 도 뛰어도 몸이 휙 돌지 않게
		constexpr float MaxMoveYawRate = 500.f;
		const float Desired = FMath::FInterpTo(RootYawOffset, GetMoveYawOffset(), DeltaSeconds, RootYawBlendOutSpeed);
		RootYawOffset += FMath::Clamp(Desired - RootYawOffset, -MaxMoveYawRate * DeltaSeconds, MaxMoveYawRate * DeltaSeconds);
		TurnDirection = 0;
		bTurnRecovering = false;
	}
}

float UHuntAnimInstance::GetMoveYawOffset() const
{
	if (!MoveYawOffsetFB || !MoveYawOffsetLR || !bShouldMove)
	{
		return 0.f;
	}

	// ALS UpdateRotationValues: FYaw/BYaw = FB.X/Y, LYaw/RYaw = LR.X/Y. 이동 방향 구역 상태가 그 값을 YawOffset 커브로 내고 캐릭터가 시선 + YawOffset으로 돈다
	// 구역은 ALS CalculateQuadrant(경계 70/110 + 버퍼 5). ALS는 버퍼 조건이 항상 참이라 실제로는 앞 ±75, 옆 75~115로 고정이다
	// 구역이 바뀔 때 값이 튀는 것(예: 앞 75도 = 30 → 오른쪽 = 0)은 ALS도 같고, 위 FInterpTo가 부드럽게 한다(ALS는 상태 블렌드 + 회전 보간)
	const float Angle = VelocityDirection;
	const FVector FB = MoveYawOffsetFB->GetVectorValue(Angle);
	const FVector LR = MoveYawOffsetLR->GetVectorValue(Angle);
	const float AbsAngle = FMath::Abs(Angle);
	const float Offset = AbsAngle <= 75.f ? FB.X : AbsAngle > 115.f ? FB.Y : Angle > 0.f ? LR.Y : LR.X;
	// 방향을 빠르게 뒤집으면 속도가 0 근처를 지나며 속도 방향이 아무 값으로 튄다(입력이 있어 bShouldMove는 계속 true)
	// 느릴 때는 오프셋을 줄여 그 방향을 따라 몸이 이리저리 돌지 않게 한다(50 → 걷기 속도에서 0 → 1)
	const float SpeedWeight = FMath::GetMappedRangeValueClamped(FVector2D(50.f, WalkAnimSpeed), FVector2D(0.f, 1.f), GroundSpeed);
	// RelaxedAlpha는 그림자용으로 줄었을 수 있어(LocalShadowRelaxedScale) 원래 값을 쓴다. 몸 방향은 1인칭 하체에 보인다
	return Offset * Character->GetRelaxedBlend() * (1.f - SprintAlpha) * SpeedWeight;
}

void UHuntAnimInstance::AdvanceTurnTimers(float DeltaSeconds)
{
	if (TurnDirection != 0)
	{
		const float Lag = FMath::Clamp(FMath::GetRangePct(TurnStartAngle, MaxRootYawOffset, FMath::Abs(RootYawOffset)), 0.f, 1.f);
		TurnTime += DeltaSeconds * FMath::Lerp(1.f, MaxTurnPlayRate, Lag);
	}
	TurnCooldown = FMath::Max(0.f, TurnCooldown - DeltaSeconds);
}

void UHuntAnimInstance::ApplyTurnYawCurve()
{
	static const FName TurnYawWeightCurve(TEXT("TurnYawWeight"));
	static const FName RemainingTurnYawCurve(TEXT("RemainingTurnYaw"));

	// 회전 애니메이션이 돈 만큼 오프셋을 줄인다 (Lyra ProcessTurnYawCurve). 블렌드 중엔 커브에 가중치가 곱해져 있어 나눠서 복원
	const float TurnWeight = GetCurveValue(TurnYawWeightCurve);
	if (TurnWeight > KINDA_SMALL_NUMBER)
	{
		const float TurnYaw = GetCurveValue(RemainingTurnYawCurve) / TurnWeight;
		if (bTurnCurveActive)
		{
			RootYawOffset -= TurnYaw - PrevTurnYawCurve;
		}
		PrevTurnYawCurve = TurnYaw;
		bTurnCurveActive = true;
	}
	else if (bTurnCurveActive)
	{
		// 회전 구간이 끝났다 (TurnYawWeight가 0으로 떨어짐). 바로 Idle로 돌아가지 않고 TurnRecoveryTime 동안
		// 회전 애니메이션의 마무리(발 내려놓기)를 더 재생한다 (Lyra Turn Recovery)
		bTurnCurveActive = false;
		bTurnRecovering = true;
		TurnRecoveryLeft = TurnRecoveryTime;
	}
}

void UHuntAnimInstance::EndTurn()
{
	TurnDirection = 0;
	bTurnRecovering = false;
	TurnCooldown = TurnRetriggerDelay;
}

void UHuntAnimInstance::TryStartTurn(bool bCanTurn)
{
	if (bCanTurn && TurnDirection == 0 && TurnCooldown <= 0.f && FMath::Abs(RootYawOffset) > TurnStartAngle)
	{
		// TurnLeft의 RemainingTurnYaw는 -90 -> 0이라 위 식에서 +오프셋을 줄인다
		TurnDirection = RootYawOffset > 0.f ? 1 : 2;
		TurnTime = 0.f;
	}
}
