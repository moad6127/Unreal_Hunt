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
	if (Weapon)
	{
		LeftHandIKTransform = Weapon->GetMesh()->GetSocketTransform(LeftHandSocketName)
			.GetRelativeTransform(Character->GetMesh()->GetSocketTransform(TEXT("hand_r")));
	}
	// GetBaseAimRotation은 다른 플레이어 캐릭터에서도 복제된 피치를 돌려준다
	AimPitch = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch);
}
