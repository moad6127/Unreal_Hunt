// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"

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

	UpdateVelocity();
	UpdateFoldedDirections();
	UpdateGaitBlend();
	UpdateMovingBackward();
	UpdateWeapon();
	UpdateAimPitch();
	UpdateTurnInPlace(DeltaSeconds);
}

void UHuntAnimInstance::UpdateVelocity()
{
	const FVector Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	Direction = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw) : 0.f;
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
	bRunGait = Character->IsRunGait();
	WalkBlendSpeed = FMath::Min(GroundSpeed, WalkAnimSpeed);
	WalkPlayRate = FMath::Max(1.f, GroundSpeed / WalkAnimSpeed);
	RunBlendSpeed = FMath::Max(GroundSpeed, JogAnimSpeed);
	LocomotionPlayRate = GroundSpeed / FMath::Clamp(GroundSpeed, JogAnimSpeed, FMath::Max(JogAnimSpeed, GetRunAnimSpeed()));
}

void UHuntAnimInstance::UpdateMovingBackward()
{
	if (!bShouldMove)
	{
		return;
	}

	const float AbsDirection = FMath::Abs(Direction);
	if (AbsDirection > 90.f + BackwardHysteresis)
	{
		bMovingBackward = true;
	}
	else if (AbsDirection < 90.f - BackwardHysteresis)
	{
		bMovingBackward = false;
	}
}

void UHuntAnimInstance::UpdateWeapon()
{
	const AHuntWeapon* Weapon = Character->GetBodyWeapon();
	bHasWeapon = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent();
	AimAlpha = Character->GetAimBlend();
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
	AimPitch = FRotator::NormalizeAxis(AimRotation.Pitch);
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
