// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntFirstPersonAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"

void UHuntFirstPersonAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AHuntCharacter>(TryGetPawnOwner());
}

void UHuntFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 프리뷰에는 캐릭터가 없으므로 값을 덮어쓰지 않는다
	if (!Character)
	{
		return;
	}

	GroundSpeed = Character->GetVelocity().Size2D();
	bShouldMove = GroundSpeed > 3.f && !Character->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero();
	AimAlpha = Character->GetAimBlend();

	const AHuntWeapon* Weapon = Character->GetEquippedWeapon();
	FVector GripLocation;
	bLeftHandIK = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent() && Weapon->GetFirstPersonLeftHandGripLocation(GripLocation);
	if (bLeftHandIK)
	{
		// 총이 ik_hand_gun에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
		LeftHandIKLocation = GetSkelMeshComponent()->GetSocketTransform(TEXT("ik_hand_gun")).InverseTransformPosition(GripLocation);
	}
}
