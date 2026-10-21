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

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	Velocity = Character->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	Direction = GroundSpeed > KINDA_SMALL_NUMBER ? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw) : 0.f;
	bShouldMove = GroundSpeed > 3.f && !Movement->GetCurrentAcceleration().IsNearlyZero();
	bIsFalling = Movement->IsFalling();
	const AHuntWeapon* Weapon = Character->GetEquippedWeapon();
	bHasWeapon = Weapon != nullptr;
	// GetBaseAimRotation은 다른 플레이어 캐릭터에서도 복제된 피치를 돌려준다
	const FRotator AimRotation = Character->GetBaseAimRotation();
	const float PrevAimAlpha = AimAlpha;
	AimAlpha = Character->GetAimBlend();
	if (Weapon)
	{
		const FTransform HandTransform = Character->GetMesh()->GetSocketTransform(TEXT("hand_r"));
		const USkeletalMeshComponent* WeaponMesh = Weapon->GetMesh();
		const FVector HipGrip = WeaponMesh->GetSocketLocation(LeftHandSocketName);
		const FVector ADSGrip = WeaponMesh->DoesSocketExist(LeftHandADSSocketName) ? WeaponMesh->GetSocketLocation(LeftHandADSSocketName) : HipGrip;
		LeftHandIKLocation = HandTransform.InverseTransformPosition(FMath::Lerp(HipGrip, ADSGrip, AimAlpha));

		// 총은 손에 고정돼 있어 손 기준 가늠자 회전은 일정하다. 가늠자(X=앞, Z=위)가 시선과 같아지도록 손 회전을 역산
		const FQuat SightInHand = HandTransform.GetRotation().Inverse() * Weapon->GetMesh()->GetSocketQuaternion(SightSocketName);
		const FQuat TargetHandQuat = AimRotation.Quaternion() * SightInHand.Inverse();

		// 조준 중에는 보간으로 늦게 따라가 마우스를 돌릴 때 총이 흔들리게 하고, 조준 전에는 바로 맞춰 시작할 때 튀지 않게 한다
		FQuat HandQuat = TargetHandQuat;
		if (PrevAimAlpha > 0.f && AimSwayRotationSpeed > 0.f)
		{
			HandQuat = FMath::QInterpTo(SmoothedHandQuat, TargetHandQuat, DeltaSeconds, AimSwayRotationSpeed);

			// 뒤처진 각도를 AimSwayMaxAngle로 제한 (보간만 쓰면 빠르게 돌릴수록 수십 도까지 벌어진다)
			FQuat Lag = TargetHandQuat.Inverse() * HandQuat;
			if (Lag.W < 0.f)
			{
				Lag = Lag * -1.f;
			}
			const float MaxLag = FMath::DegreesToRadians(AimSwayMaxAngle);
			if (Lag.GetAngle() > MaxLag)
			{
				HandQuat = TargetHandQuat * FQuat(Lag.GetRotationAxis(), MaxLag);
			}
		}
		SmoothedHandQuat = HandQuat;

		// 손만 돌리면 왼손이 따라오지 못해 손목이 꺾이므로, 같은 회전을 가슴(spine_05)에 줘서 양팔·총을 한 덩어리로 돌린다
		// 지금 손 회전에는 이전 프레임 보정이 들어 있으니 빼서 애니메이션만의 손 회전을 구한다 (월드 기준 D * 원래 회전으로 적용됨)
		const FQuat AnimHandQuat = AppliedSpineCorrection.Inverse() * HandTransform.GetRotation();
		const FQuat Correction = (HandQuat * AnimHandQuat.Inverse()).GetNormalized();
		AimSpineCorrection = Correction.Rotator();
		// ABP 노드가 Alpha(=AimAlpha)만큼 적용하는 양. 다음 프레임에서 빼기 위해 기억한다
		AppliedSpineCorrection = FQuat::Slerp(FQuat::Identity, Correction, AimAlpha).GetNormalized();
	}
	else
	{
		AppliedSpineCorrection = FQuat::Identity;
		AimSpineCorrection = FRotator::ZeroRotator;
	}

	AimPitch = FRotator::NormalizeAxis(AimRotation.Pitch);

	UpdateTurnInPlace(DeltaSeconds);
}

void UHuntAnimInstance::UpdateTurnInPlace(float DeltaSeconds)
{
	static const FName TurnYawWeightCurve(TEXT("TurnYawWeight"));
	static const FName RemainingTurnYawCurve(TEXT("RemainingTurnYaw"));

	const float ActorYaw = Character->GetActorRotation().Yaw;
	const float YawDelta = FRotator::NormalizeAxis(ActorYaw - PrevActorYaw);
	PrevActorYaw = ActorYaw;

	const bool bCanTurn = !bShouldMove && !bIsFalling;
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

	if (TurnDirection != 0)
	{
		const float Lag = FMath::Clamp(FMath::GetRangePct(TurnStartAngle, MaxRootYawOffset, FMath::Abs(RootYawOffset)), 0.f, 1.f);
		TurnTime += DeltaSeconds * FMath::Lerp(1.f, MaxTurnPlayRate, Lag);
	}
	TurnCooldown = FMath::Max(0.f, TurnCooldown - DeltaSeconds);

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

	RootYawOffset = FMath::Clamp(RootYawOffset, -MaxRootYawOffset, MaxRootYawOffset);

	if (bCanTurn && TurnDirection == 0 && TurnCooldown <= 0.f && FMath::Abs(RootYawOffset) > TurnStartAngle)
	{
		// TurnLeft의 RemainingTurnYaw는 -90 -> 0이라 위 식에서 +오프셋을 줄인다
		TurnDirection = RootYawOffset > 0.f ? 1 : 2;
		TurnTime = 0.f;
	}

	AimYaw = -RootYawOffset;
}
