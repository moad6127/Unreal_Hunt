#include "Animation/HuntFirstPersonWeaponLayersAnimInstance.h"
#include "Animation/HuntFirstPersonAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/Attachment/GripAttachmentDefinition.h"

void UHuntFirstPersonWeaponLayersAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 연결된 레이어는 메인보다 먼저 게임 스레드 업데이트를 받으므로 메인 값 복사는 스레드 세이프 업데이트에서 한다
	Main = Cast<UHuntFirstPersonAnimInstance>(GetOwningComponent()->GetAnimInstance());
	const AHuntCharacter* Character = Cast<AHuntCharacter>(GetOwningActor());
	const AHuntWeapon* Weapon = Character ? Character->GetEquippedWeapon() : nullptr;
	const UGripAttachmentDefinition* Grip = Weapon ? Weapon->GetGrip() : nullptr;
	GripPoses = Grip ? Grip->FirstPersonGripPoses : DefaultGripPoses;
}

void UHuntFirstPersonWeaponLayersAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!Main)
	{
		return;
	}
	GroundSpeed = Main->GroundSpeed;
	bShouldMove = Main->bShouldMove;
	WalkSwayTime = Main->WalkSwayTime;
	SprintSwayTime = Main->SprintSwayTime;
	AimAlpha = Main->AimAlpha;
}
