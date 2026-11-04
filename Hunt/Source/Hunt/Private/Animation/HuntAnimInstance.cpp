// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Animation/AnimSequence.h"

UAnimSequence* FHuntDirectionalAnims::Pick(float Direction) const
{
	// 각 방향의 기준 각도. 비어 있으면 다음으로 가까운 방향을 쓴다
	const TPair<float, UAnimSequence*> Slots[] = {
		{0.f, F}, {-45.f, FL}, {45.f, FR}, {-90.f, L}, {90.f, R}, {-135.f, BL}, {135.f, BR}, {180.f, B}};
	UAnimSequence* Best = nullptr;
	float BestDelta = TNumericLimits<float>::Max();
	for (const TPair<float, UAnimSequence*>& Slot : Slots)
	{
		const float Delta = FMath::Abs(FRotator::NormalizeAxis(Direction - Slot.Key));
		if (Slot.Value && Delta < BestDelta)
		{
			Best = Slot.Value;
			BestDelta = Delta;
		}
	}
	return Best;
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

	UpdateVelocity();
	UpdateStartStop(DeltaSeconds);
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

float UHuntAnimInstance::FindTimeAtDistance(const UAnimSequence* Anim, float Distance)
{
	static const FName CurveName(TEXT("Distance"));
	float Low = 0.f;
	float High = Anim->GetPlayLength();
	for (int32 Step = 0; Step < 16; ++Step)
	{
		const float Mid = (Low + High) * 0.5f;
		if (Anim->EvaluateCurveData(CurveName, FAnimExtractContext(Mid)) < Distance)
		{
			Low = Mid;
		}
		else
		{
			High = Mid;
		}
	}
	return High;
}

float UHuntAnimInstance::FindStopTimeAtSpeed(const UAnimSequence* Anim, float Speed)
{
	// 속도는 Distance 커브(루트 이동 거리)의 변화량으로 구한다. movedata_speed 커브는 방향마다 부호·유무가 달라 믿지 않는다
	// 멈추는 지점에서 거꾸로 훑어, 애니메이션이 마지막으로 Speed 이상이던 시각부터 재생한다(걸음마다 속도가 출렁여도 접근 걸음을 건너뛴다)
	// 애니메이션이 그만큼 빠른 적이 없으면(질주 600 > GASP 500) 멈추는 지점 1.5초 안에서 가장 빠른 시각
	static const FName CurveName(TEXT("Distance"));
	constexpr float Step = 1.f / 30.f;
	const float StopPoint = FindTimeAtDistance(Anim, -1.f);
	float FastestTime = StopPoint;
	float FastestSpeed = 0.f;
	for (float Time = StopPoint - Step; Time >= FMath::Max(StopPoint - 1.5f, 0.f); Time -= Step)
	{
		const float AnimSpeed = (Anim->EvaluateCurveData(CurveName, FAnimExtractContext(Time + Step))
			- Anim->EvaluateCurveData(CurveName, FAnimExtractContext(Time))) / Step;
		if (AnimSpeed >= Speed)
		{
			return Time;
		}
		if (AnimSpeed > FastestSpeed)
		{
			FastestSpeed = AnimSpeed;
			FastestTime = Time;
		}
	}
	return FastestTime;
}

float UHuntAnimInstance::EstimateBrakingTime(float Speed) const
{
	// 이동 컴포넌트 제동: dv/dt = -(Friction × v + Decel). v=0까지 걸리는 시간 = ln(1 + Friction × v / Decel) / Friction
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const float Friction = (Movement->bUseSeparateBrakingFriction ? Movement->BrakingFriction : Movement->GroundFriction) * Movement->BrakingFrictionFactor;
	const float Decel = FMath::Max(Movement->BrakingDecelerationWalking, 1.f);
	return Friction > KINDA_SMALL_NUMBER ? FMath::Loge(1.f + Friction * Speed / Decel) / Friction : Speed / Decel;
}

void UHuntAnimInstance::UpdateStartStop(float DeltaSeconds)
{
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FVector Acceleration = Movement->GetCurrentAcceleration();
	bHasAcceleration = !FVector2D(Acceleration).IsNearlyZero();

	// 키를 뗀 순간: 이동 방향과 속도로 정지 애니메이션을 고르고, 지금 속도와 같은 지점부터 재생한다
	// 거리 맞춤은 쓰지 않는다: 캐릭터는 십수 cm 만에 서는데 GASP 정지는 2~4m에 걸쳐 감속해 끝 자세로 바로 건너뛴다
	// shortcut: 450은 조깅(350)과 질주(600) 사이 기준, 걸음 속도를 바꾸면 맞춘다
	if (!bHasAcceleration && bWasAccelerating)
	{
		StopAnim = (GroundSpeed > 450.f ? SprintStops : WalkStops).Pick(Direction);
		if (StopAnim)
		{
			StopTime = FindStopTimeAtSpeed(StopAnim, GroundSpeed);
			StopPointTime = FMath::Max(FindTimeAtDistance(StopAnim, -1.f), StopTime);
			StopEndTime = FMath::Min(StopPointTime + StopSettleTime, StopAnim->GetPlayLength());
			// 캐릭터가 멈추는 시각에 애니메이션도 멈추는 지점에 닿게 한다(너무 느리거나 빠르지 않게 1~3배)
			StopRate = FMath::Clamp((StopPointTime - StopTime) / FMath::Max(EstimateBrakingTime(GroundSpeed), 0.05f), 1.f, 3.f);
		}
	}
	bWasAccelerating = bHasAcceleration;

	// 정지: 멈추는 지점까지는 StopRate, 마무리는 1배로 재생하고 끝 시간에 닿으면 끝
	if (!bHasAcceleration && StopAnim)
	{
		const float Rate = StopTime < StopPointTime ? StopRate : 1.f;
		StopTime = FMath::Min(StopTime + DeltaSeconds * Rate, StopAnim->GetPlayLength());
	}
	bStopFinished = !bHasAcceleration && (!StopAnim || StopTime >= StopEndTime);
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
	// 내린 자세 애디티브는 지향사격 상체 기준이라 질주 상체 자세 위에서는 뺀다
	// shortcut: 400~550은 ABP Move 상태의 질주 상체 블렌드(MapRangeClamped) 구간을 옮겨 적은 것, ABP를 바꾸면 여기도 맞춘다
	const float SprintUpperBodyWeight = FMath::GetMappedRangeValueClamped(FVector2D(400.f, 550.f), FVector2D(0.f, 1.f), GroundSpeed);
	WallBlockAlpha = Character->GetWallBlockAlpha() * (1.f - SprintUpperBodyWeight);
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
