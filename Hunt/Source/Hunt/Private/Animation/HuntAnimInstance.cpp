// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Curves/CurveVector.h"

namespace
{
	const FName FootBones[2] = {TEXT("foot_l"), TEXT("foot_r")};
	const FName IKFootBones[2] = {TEXT("ik_foot_l"), TEXT("ik_foot_r")};

	// shortcut: 이동 판정 진단용 화면 표시(콘솔 hunt.LocoDebug 1), 하체 방향 문제가 정리되면 지운다
	TAutoConsoleVariable<int32> CVarLocoDebug(TEXT("hunt.LocoDebug"), 0, TEXT("1 = 3인칭 이동 판정 값을 화면에 표시"));
}

void UHuntAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AHuntCharacter>(TryGetPawnOwner());
	if (Character)
	{
		PrevActorYaw = Character->GetActorRotation().Yaw;
	}

	// Idle 자세의 발 자리 = ik_foot 뼈의 컴포넌트 공간 위치(애니메이션마다 발 위치로 구워 둠). 뼈를 부모까지 거슬러 합친다
	const auto ReadIdleFeet = [](const UAnimSequence* Anim, FVector (&OutLocations)[2])
	{
		if (!Anim || !Anim->GetSkeleton())
		{
			return false;
		}
		const FReferenceSkeleton& RefSkeleton = Anim->GetSkeleton()->GetReferenceSkeleton();
		for (int32 Side = 0; Side < 2; ++Side)
		{
			FTransform ComponentTransform = FTransform::Identity;
			for (int32 Bone = RefSkeleton.FindBoneIndex(IKFootBones[Side]); Bone != INDEX_NONE; Bone = RefSkeleton.GetParentIndex(Bone))
			{
				FTransform Local;
				Anim->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(0.0), false);
				ComponentTransform = ComponentTransform * Local;
			}
			OutLocations[Side] = ComponentTransform.GetLocation();
		}
		return true;
	};
	ReadIdleFeet(IdleAnim, AimIdleFeet);
	if (!ReadIdleFeet(RelaxedIdleAnim, RelaxedIdleFeet))
	{
		RelaxedIdleFeet[0] = AimIdleFeet[0];
		RelaxedIdleFeet[1] = AimIdleFeet[1];
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
	if (CVarLocoDebug.GetValueOnGameThread() > 0 && GEngine && Character->IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(int32(GetUniqueID()), 0.f, FColor::Yellow, FString::Printf(
			TEXT("VelDir %.0f  Dir %.0f  Backward %d  Target %d  Fwd %.0f  Bwd %.0f  FeetSideGap %.0f  Speed %.0f | Turn %d  TurnTime %.2f  RootYaw %.0f  TurnW %.2f  Recover %d"),
			VelocityDirection, Direction, bMovingBackward, bHipsBackwardTarget, ForwardDirection, BackwardDirection,
			GetFeetSideGap(), GroundSpeed, TurnDirection, TurnTime, RootYawOffset, GetCurveValue(TEXT("TurnYawWeight")), bTurnRecovering));
	}
	UpdateWeapon();
	UpdateAimPitch();
	UpdateTurnInPlace(DeltaSeconds);
	UpdateFootLock(DeltaSeconds);
}

void UHuntAnimInstance::UpdateFootLock(float DeltaSeconds)
{
	// 서 있는 자세: 대기가 아니면(준비·조준) Lyra 사격 자세(왼발 앞), 대기면 ALS 기본자세(두 발 나란히)
	// ABP Idle 하체는 전환 애니메이션(LegTransitionAnim)을 ShootingLegsTime으로 평가한다. 발 옮기기도 애니메이션에 들어 있다
	ShootingLegsAlpha = FMath::FInterpConstantTo(ShootingLegsAlpha, Character->GetRelaxedBlend() < 0.5f ? 1.f : 0.f, DeltaSeconds, 1.f / LegStanceBlendTime);
	ShootingLegsTime = LegTransitionAnim ? ShootingLegsAlpha * LegTransitionAnim->GetPlayLength() : 0.f;
	const bool bAimLegs = ShootingLegsAlpha >= 0.5f;
	const bool bLegTransition = ShootingLegsAlpha > 0.f && ShootingLegsAlpha < 1.f;

	// 제자리 회전 중이나 하체 전환 중에는 애니메이션이 발을 옮기므로 고정하지 않는다
	const bool bStanding = !bShouldMove && !bIsFalling && TurnDirection == 0 && !bLegTransition;
	// 이동하다 멈췄을 때만 고정한다. 시작 직후(첫 프레임은 기본 포즈)나 하체 전환 뒤에는 애니메이션 그대로
	const bool bJustStopped = bStanding && bWasMovingForFootLock;
	bWasMovingForFootLock = bShouldMove;
	StandingTime = bStanding ? StandingTime + DeltaSeconds : 0.f;

	// 제자리 회전 뒤 발 정리는 회전 애니메이션의 마무리 구간(TurnRecoveryTime)이 한다. 회전 애니메이션은 대기면 ALS(두 발 나란히),
	// 준비·조준이면 Lyra(사격 자세)로 서 있는 자세와 같은 자세로 끝나서 코드로 발을 옮기지 않는다
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Feet[Side].IdleLocation = bAimLegs ? AimIdleFeet[Side] : RelaxedIdleFeet[Side];
	}

	const USkeletalMeshComponent* Mesh = GetOwningComponent();
	const FTransform& MeshTransform = Mesh->GetComponentTransform();
	// Idle 자리도 ABP의 Rotate Root Bone(RootYawOffset)만큼 돌아가 있다
	const FQuat RootYaw(FRotator(0.f, RootYawOffset, 0.f));
	// 몸 기준 옆 위치(X, + = 왼쪽). 루트 회전을 되돌린 컴포넌트 공간
	const auto BodyX = [&MeshTransform, ToBody = RootYaw.Inverse()](const FVector& World)
	{
		return ToBody.RotateVector(MeshTransform.InverseTransformPosition(World)).X;
	};
	FVector IdleWorld[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FFootLock& Foot = Feet[Side];
		IdleWorld[Side] = MeshTransform.TransformPosition(RootYaw.RotateVector(Foot.IdleLocation));
		FVector Target = Foot.LockedWorld;
		if (!bStanding || (!Foot.bLocked && !bJustStopped))
		{
			// 움직이기 시작하면 고정을 짧게 풀어 발이 순간 이동하지 않게 한다
			Foot.bLocked = false;
			Foot.bStepping = false;
			Foot.Alpha = FMath::FInterpConstantTo(Foot.Alpha, 0.f, DeltaSeconds, 1.f / FootUnlockTime);
		}
		else if (!Foot.bLocked)
		{
			// 멈춘 순간 (ALS (N) Stop States 방식): 디딘 발은 지금 자리(지난 프레임 최종 포즈)의 바닥 높이에 고정한다
			// 들려 있던 발(걸음 중, 교차해 지나가는 중일 수 있음)은 그 자리에 고정하지 않고 바로 서 있는 자세의 자리로 내딛는다
			// (ALS는 들린 발을 걷기 주기에서 그 발이 디딘 프레임 자세로 바꿔 심는다. 교차한 채 멈춰 서지 않게)
			const FVector FootWorld = Mesh->GetBoneLocation(FootBones[Side]);
			FVector FootLocal = MeshTransform.InverseTransformPosition(FootWorld);
			const bool bGrounded = FootLocal.Z - Foot.IdleLocation.Z < FootStepLift;
			FootLocal.Z = Foot.IdleLocation.Z;
			Foot.LockedWorld = MeshTransform.TransformPosition(FootLocal);
			Foot.Alpha = 1.f;
			Foot.bLocked = true;
			Foot.bStepped = false;
			if (!bGrounded)
			{
				Foot.bStepping = true;
				Foot.bStepped = true;
				Foot.StepTime = 0.f;
				Foot.StepStartWorld = FootWorld;
			}
			// 들린 발은 첫 프레임에도 지금 자리(들린 높이)에서 옮기기를 시작한다. 바닥 높이(LockedWorld)를 넣으면 한 프레임 바닥에 찍혔다가 튄다(11-08)
			Target = Foot.bStepping ? Foot.StepStartWorld : Foot.LockedWorld;
		}
		else
		{
			Foot.Alpha = FMath::FInterpConstantTo(Foot.Alpha, 1.f, DeltaSeconds, 1.f / 0.15f);
			// 한 걸음 중: 고정 자리 → Idle 자리로 발을 들어 옮긴다. 끝나면 Idle 자리에 다시 고정
			if (Foot.bStepping)
			{
				Foot.StepTime += DeltaSeconds;
				const float Progress = FMath::SmoothStep(0.f, 1.f, Foot.StepTime / FootStepDuration);
				Target = FMath::Lerp(Foot.StepStartWorld, IdleWorld[Side], Progress)
					+ MeshTransform.GetUnitAxis(EAxis::Z) * FootStepLift * FMath::Sin(PI * Progress);
				// 반대쪽 발을 넘어가지 않게 옆으로 최소 간격을 둔다. 꼬인 채 시작했으면 발을 들면서 점점 벌린다
				const FFootLock& Other = Feet[1 - Side];
				if (Other.bLocked)
				{
					const FVector Local = MeshTransform.InverseTransformPosition(Target);
					const float OtherX = BodyX(Other.LockedWorld);
					const float X = BodyX(Target);
					const float SafeX = Side == 0 ? FMath::Max(X, OtherX + FootStepMinGap) : FMath::Min(X, OtherX - FootStepMinGap);
					const FVector Side2D = RootYaw.RotateVector(FVector::XAxisVector);
					Target = MeshTransform.TransformPosition(Local + Side2D * (SafeX - X) * Progress);
				}
				if (Foot.StepTime >= FootStepDuration)
				{
					// 간격 때문에 밀린 자리도 그대로 고정한다(Idle 자리로 튀지 않게)
					Foot.bStepping = false;
					Foot.LockedWorld = Target;
				}
			}
		}
		(Side == 0 ? FootLockAlphaL : FootLockAlphaR) = Foot.Alpha;
		(Side == 0 ? FootLockLocationL : FootLockLocationR) = MeshTransform.InverseTransformPosition(Target);
	}

	// 서고 나서 고정된 발이 Idle 자리에서 많이 벗어나 있으면 더 벗어난 발부터 한 걸음씩 옮긴다(멈출 때마다 발마다 한 번)
	// 두 발이 꼬여 있으면(왼발이 오른발보다 오른쪽) 거리와 상관없이 옮긴다
	// 두 발이 교차한 채 디뎌 있으면 기다리지 않고 바로 옮긴다
	const bool bCrossed = Feet[0].bLocked && Feet[1].bLocked
		&& BodyX(Feet[0].LockedWorld) - BodyX(Feet[1].LockedWorld) < FootStepMinGap;
	if (!bStanding || (StandingTime < FootStepDelay && !bCrossed) || Feet[0].bStepping || Feet[1].bStepping)
	{
		return;
	}
	float Offsets[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Offsets[Side] = Feet[Side].bLocked && !Feet[Side].bStepped ? FVector::Dist2D(Feet[Side].LockedWorld, IdleWorld[Side]) : 0.f;
	}
	const int32 Side = Offsets[0] >= Offsets[1] ? 0 : 1;
	if (Offsets[Side] > FootStepThreshold || (bCrossed && Offsets[Side] > KINDA_SMALL_NUMBER))
	{
		FFootLock& Foot = Feet[Side];
		Foot.bStepping = true;
		Foot.bStepped = true;
		Foot.StepTime = 0.f;
		Foot.StepStartWorld = Foot.LockedWorld;
	}
}

void UHuntAnimInstance::UpdateVelocity(float DeltaSeconds)
{
	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VelocityDirection = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw) : 0.f;
	// 이동 블렌드스페이스는 돌아간 몸 기준 방향으로 고른다(몸이 이동 방향 쪽으로 돌아가 있으면 옆걸음이 대각선 걸음이 된다)
	Direction = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(VelocityDirection - RootYawOffset) : 0.f;
	// ALS ShouldMove 방식: 입력이 있거나 멈춤 속도(AHuntCharacter::StopBrakeSpeed)보다 빠르면 이동 중. 그 아래면 멈춤 처리를 하고 캐릭터도 빨리 제동한다
	// 11-08: 150(걷기 샘플 속도)에서 끊으면 Jog 보폭이 벌어진 채 몸이 서고 발만 끌려와 40으로 낮춤
	// (예전에 0까지 유지했을 때는 감속 중 교차 스텝이 느리게 늘어져 다리가 꼬여 보였다. 다시 그러면 StopBrakeSpeed를 올린다)
	// 다른 플레이어 캐릭터(시뮬레이티드 프록시)는 입력(가속)을 모르므로 입력이 있는 것으로 보고 속도로만 판정한다
	const bool bHasMovementInput = Character->GetLocalRole() == ROLE_SimulatedProxy
		|| !Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
	bShouldMove = GroundSpeed > 3.f && (bHasMovementInput || GroundSpeed > Character->GetStopBrakeSpeed());
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
	// ALS는 걷기에서 절반, 달리기 이상에서 전부 기울인다(Weight_Gait 커브)
	LeanAlpha = FMath::GetMappedRangeValueClamped(FVector2D(WalkAnimSpeed, JogAnimSpeed), FVector2D(0.5f, 1.f), GroundSpeed);
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
	const float AbsDirection = FMath::Abs(Direction);
	return AbsDirection <= 90.f
		? FMath::Lerp(ForwardRunAnimSpeed, SideRunAnimSpeed, AbsDirection / 90.f)
		: FMath::Lerp(SideRunAnimSpeed, BackwardRunAnimSpeed, (AbsDirection - 90.f) / 90.f);
}

void UHuntAnimInstance::UpdateGaitBlend()
{
	// 조깅 중이어도 걷기 샘플보다 느리면(벽에 막혀 미끄러질 때, 멈추는 중) 걷기로 바꾼다
	// 조깅 블렌드스페이스는 조깅 샘플 속도로 고정되어 느리면 큰 보폭을 슬로모션으로 밟는다
	// 키를 떼고 멈추는 중에는 바로 걷기용 블렌드스페이스로 넘기고 속도를 그대로 넣어, 감속하는 동안 Jog → 걷기 → Idle 보폭으로
	// 이어서 줄인다(ALS 스트라이드 블렌드). 조깅용은 조깅 샘플 속도 아래로 안 내려가 큰 보폭을 끝까지 밟다가 갑자기 섰다(11-08)
	// 평소(입력 있음)에는 걷기와 조깅을 섞지 않는다: 중간 자세에서 다리가 모여 보이기 때문. 멈추는 동안은 잠깐이라 섞는다
	const bool bStopping = Character->GetLocalRole() != ROLE_SimulatedProxy
		&& Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
	bRunGait = Character->IsRunGait() && GroundSpeed > WalkAnimSpeed && !bStopping;
	// 느릴 때는 재생 속도를 MinPlayRate까지만 내리고, 나머지는 Idle과 섞어(블렌드 속도) 보폭을 줄여 맞춘다
	WalkPlayRate = FMath::Max(GroundSpeed / WalkAnimSpeed, MinPlayRate);
	WalkBlendSpeed = FMath::Min(GroundSpeed / WalkPlayRate, WalkAnimSpeed);
	if (bStopping && GroundSpeed > WalkAnimSpeed)
	{
		// 걷기 샘플보다 빠르면 블렌드스페이스가 걷기~조깅 샘플을 속도대로 섞으므로 재생 속도는 그대로
		WalkPlayRate = 1.f;
		WalkBlendSpeed = GroundSpeed;
	}
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

	// 골반 방향은 두 발이 옆으로 벌어져 있을 때(왼발이 오른발보다 HipsSwitchFeetApart 이상 왼쪽, 즉 다리가 교차하지 않을 때)만 바꾼다
	// ALS의 Feet_Crossing == 0과 같은 조건. 커브는 Hunt ABP에서 Move 상태 애디티브(Alpha 0.4→0)를 거쳐 줄어들어 쓸 수 없어 발 위치로 잰다
	// 앞뒤 간격은 보지 않는다: 옆걸음에서 다리를 교차해 내딛는 순간에도 두 발은 앞뒤로 멀리 떨어져 있다
	if (bHipsBackwardTarget == bMovingBackward)
	{
		HipsSwitchWaitTime = 0.f;
		return;
	}
	HipsSwitchWaitTime += DeltaSeconds;
	if (GetFeetSideGap() >= HipsSwitchFeetApart || HipsSwitchWaitTime >= MaxHipsSwitchDelay)
	{
		bMovingBackward = bHipsBackwardTarget;
		HipsSwitchWaitTime = 0.f;
		TimeSinceHipsSwitch = 0.f;
	}
}

void UHuntAnimInstance::UpdateWeapon()
{
	const AHuntWeapon* Weapon = Character->GetBodyWeapon();
	bHasWeapon = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent();
	AimAlpha = Character->GetAimBlend();
	// 내린 자세 애디티브는 지향사격 상체 기준이라 질주 상체 자세 위에서는 뺀다
	SprintAlpha = FMath::GetMappedRangeValueClamped(SprintBlendSpeed, FVector2D(0.f, 1.f), GroundSpeed);
	WallBlockAlpha = Character->GetWallBlockAlpha() * (1.f - SprintAlpha);
	RelaxedAlpha = Character->GetRelaxedBlend() * (1.f - SprintAlpha);
	AimOffsetAlpha = 1.f - RelaxedAlpha;
	// Layering 부위 가중치(ABP_Hunt_ThirdPerson_Layered)
	const float ReadySpine = FMath::GetMappedRangeValueClamped(ReadySpineFadeSpeed, FVector2D(ReadySpineAdd, 0.f), GroundSpeed);
	const float RelaxedSpine = FMath::GetMappedRangeValueClamped(FVector2D(WalkAnimSpeed, JogAnimSpeed), FVector2D(RelaxedSpineAdd, RelaxedJogSpineAdd), GroundSpeed);
	SpineAddAlpha = FMath::Lerp(ReadySpine, RelaxedSpine, RelaxedAlpha);
	ArmLAddAlpha = RelaxedArmLAdd * RelaxedAlpha;
	ArmRAddAlpha = RelaxedArmRAdd * RelaxedAlpha;
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
	return Offset * RelaxedAlpha * SpeedWeight;
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
