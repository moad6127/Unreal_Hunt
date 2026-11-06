// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	const FName FootBones[2] = {TEXT("foot_l"), TEXT("foot_r")};
	const FName IKFootBones[2] = {TEXT("ik_foot_l"), TEXT("ik_foot_r")};
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
	if (IdleAnim && IdleAnim->GetSkeleton())
	{
		const FReferenceSkeleton& RefSkeleton = IdleAnim->GetSkeleton()->GetReferenceSkeleton();
		for (int32 Side = 0; Side < 2; ++Side)
		{
			FTransform ComponentTransform = FTransform::Identity;
			for (int32 Bone = RefSkeleton.FindBoneIndex(IKFootBones[Side]); Bone != INDEX_NONE; Bone = RefSkeleton.GetParentIndex(Bone))
			{
				FTransform Local;
				IdleAnim->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(0.0), false);
				ComponentTransform = ComponentTransform * Local;
			}
			Feet[Side].IdleLocation = ComponentTransform.GetLocation();
		}
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

	UpdateVelocity();
	UpdateFoldedDirections();
	UpdateGaitBlend();
	UpdateMovingBackward();
	UpdateWeapon();
	UpdateAimPitch();
	UpdateTurnInPlace(DeltaSeconds);
	UpdateFootLock(DeltaSeconds);
}

void UHuntAnimInstance::UpdateFootLock(float DeltaSeconds)
{
	// 제자리 회전 중에는 회전 애니메이션이 발을 옮기므로 고정하지 않는다
	const bool bStanding = !bShouldMove && !bIsFalling && TurnDirection == 0;
	// 이동하다 멈췄을 때만 고정한다. 시작 직후(첫 프레임은 기본 포즈)나 제자리 회전 뒤에는 애니메이션 그대로
	const bool bJustStopped = bStanding && bWasMovingForFootLock;
	bWasMovingForFootLock = bShouldMove;
	StandingTime = bStanding ? StandingTime + DeltaSeconds : 0.f;

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
			// 멈춘 순간: 지금 발 자리(지난 프레임 최종 포즈)의 바닥 높이에 고정한다
			// 디딘 발은 바로, 들려 있던 발은 내려놓으며 천천히 고정한다
			FVector FootLocal = MeshTransform.InverseTransformPosition(Mesh->GetBoneLocation(FootBones[Side]));
			const bool bGrounded = FootLocal.Z - Foot.IdleLocation.Z < FootStepLift;
			FootLocal.Z = Foot.IdleLocation.Z;
			Foot.LockedWorld = MeshTransform.TransformPosition(FootLocal);
			Foot.Alpha = bGrounded ? 1.f : 0.f;
			Foot.bLocked = true;
			Foot.bStepped = false;
			Target = Foot.LockedWorld;
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
	if (!bStanding || StandingTime < FootStepDelay || Feet[0].bStepping || Feet[1].bStepping)
	{
		return;
	}
	const bool bCrossed = Feet[0].bLocked && Feet[1].bLocked
		&& BodyX(Feet[0].LockedWorld) - BodyX(Feet[1].LockedWorld) < FootStepMinGap;
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

void UHuntAnimInstance::UpdateVelocity()
{
	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VelocityDirection = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw) : 0.f;
	// 이동 블렌드스페이스는 돌아간 몸 기준 방향으로 고른다(몸이 이동 방향 쪽으로 돌아가 있으면 옆걸음이 대각선 걸음이 된다)
	Direction = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(VelocityDirection - RootYawOffset) : 0.f;
	// 키를 떼도 몸이 미끄러지는 동안은 이동 애니메이션을 유지해 속도에 맞춰 발걸음이 줄어들며 멈춘다
	bShouldMove = GroundSpeed > 3.f;
	bIsFalling = Character->GetCharacterMovement()->IsFalling();
}

void UHuntAnimInstance::UpdateFoldedDirections()
{
	// 반대 반구의 방향은 앞뒤로 뒤집어 접는다(예: -120 → 앞쪽 -60). 두 값 모두 끊김 없이 이어진다
	const float AbsDirection = FMath::Abs(Direction);
	const float MirroredDirection = (Direction < 0.f ? -1.f : 1.f) * (180.f - AbsDirection);
	ForwardDirection = AbsDirection <= 90.f ? Direction : MirroredDirection;
	BackwardDirection = AbsDirection >= 90.f ? Direction : MirroredDirection;
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
	bRunGait = Character->IsRunGait() && GroundSpeed > WalkAnimSpeed;
	// 느릴 때는 재생 속도를 MinPlayRate까지만 내리고, 나머지는 Idle과 섞어(블렌드 속도) 보폭을 줄여 맞춘다
	WalkPlayRate = FMath::Max(GroundSpeed / WalkAnimSpeed, MinPlayRate);
	WalkBlendSpeed = FMath::Min(GroundSpeed / WalkPlayRate, WalkAnimSpeed);
	RunBlendSpeed = FMath::Max(GroundSpeed, JogAnimSpeed);
	LocomotionPlayRate = FMath::Max(GroundSpeed / FMath::Clamp(GroundSpeed, JogAnimSpeed, FMath::Max(JogAnimSpeed, GetRunAnimSpeed())), MinPlayRate);
}

void UHuntAnimInstance::UpdateMovingBackward()
{
	if (!bShouldMove)
	{
		return;
	}

	// 캐릭터 정면 기준 실제 이동 방향으로 판정한다. 히스테리시스는 뒤쪽(90~90+H)에만 둔다
	// 양쪽에 두면 정옆 이동 때 직전 상태에 따라 무릎 방향(LF/LB)이 달라진다
	const float AbsDirection = FMath::Abs(VelocityDirection);
	if (AbsDirection > 90.f + BackwardHysteresis)
	{
		bMovingBackward = true;
	}
	else if (AbsDirection < 91.f)
	{
		bMovingBackward = false;
	}
}

void UHuntAnimInstance::UpdateWeapon()
{
	const AHuntWeapon* Weapon = Character->GetBodyWeapon();
	bHasWeapon = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent();
	AimAlpha = Character->GetAimBlend();
	// 내린 자세 애디티브는 지향사격 상체 기준이라 질주 상체 자세 위에서는 뺀다
	// shortcut: 400~550은 ABP Move 상태의 질주 상체 블렌드(MapRangeClamped) 구간을 옮겨 적은 것, ABP를 바꾸면 여기도 맞춘다
	const float SprintUpperBodyWeight = FMath::GetMappedRangeValueClamped(FVector2D(400.f, 550.f), FVector2D(0.f, 1.f), GroundSpeed);
	WallBlockAlpha = Character->GetWallBlockAlpha() * (1.f - SprintUpperBodyWeight);
	RelaxedAlpha = Character->GetRelaxedBlend() * (1.f - SprintUpperBodyWeight);
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
	ApplyTurnYawCurve();

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
		// 이동 중: 몸을 시선 방향으로 되돌린다
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.f, DeltaSeconds, RootYawBlendOutSpeed);
		TurnDirection = 0;
	}
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
		// 회전 구간이 끝났다 (TurnYawWeight가 0으로 떨어짐)
		bTurnCurveActive = false;
		TurnDirection = 0;
		TurnCooldown = TurnRetriggerDelay;
	}
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
