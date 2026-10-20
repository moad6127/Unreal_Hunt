// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"

void UHuntAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AHuntCharacter>(TryGetPawnOwner());
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

	// 걸음 흔들림: 좌우 한 번 왕복할 때 상하 두 번(8자). 속도에 비례하고 멈추면 서서히 잦아든다
	const float BobTarget = bIsFalling ? 0.f : FMath::Clamp(GroundSpeed / BobReferenceSpeed, 0.f, 1.5f);
	BobWeight = FMath::FInterpTo(BobWeight, BobTarget, DeltaSeconds, 6.f);
	BobPhase = FMath::Fmod(BobPhase + DeltaSeconds * UE_TWO_PI * AimBobFrequency * BobWeight, UE_TWO_PI);
	AimHandBobOffset = FVector(FMath::Sin(BobPhase) * AimBobAmplitude.X, 0.f, FMath::Sin(2.f * BobPhase) * AimBobAmplitude.Z) * BobWeight;

	AimPitch = FRotator::NormalizeAxis(AimRotation.Pitch);
}
