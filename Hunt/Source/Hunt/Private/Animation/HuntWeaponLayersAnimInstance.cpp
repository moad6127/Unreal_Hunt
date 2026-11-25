#include "Animation/HuntWeaponLayersAnimInstance.h"
#include "Animation/HuntAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/Attachment/GripAttachmentDefinition.h"

void UHuntWeaponLayersAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 연결된 레이어는 메인보다 먼저 게임 스레드 업데이트를 받으므로 메인 값 복사는 스레드 세이프 업데이트에서 한다
	Main = Cast<UHuntAnimInstance>(GetOwningComponent()->GetAnimInstance());
	UpdatePoses();
}

void UHuntWeaponLayersAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!Main)
	{
		return;
	}
	AimYaw = Main->AimYaw;
	AimPitch = Main->AimPitch;
	AimOffsetAlpha = Main->AimOffsetAlpha;
	LandRecoveryTime = Main->LandRecoveryTime;
	TurnTime = Main->TurnTime;
	LeftHandGripPose = Main->LeftHandGripPose;
	LeftHandGripAlpha = Main->LeftHandGripAlpha;
	// 대기 오른손을 쓰는 만큼 방아쇠 손가락 포즈를 뺀다(대기 애니메이션 손가락이 총 목을 감싼다)
	RightHandGripAlpha = RightHandGripPose ? 1.f - Main->RightHandRelaxedAlpha : 0.f;
}

void UHuntWeaponLayersAnimInstance::UpdatePoses()
{
	const AHuntCharacter* Character = Cast<AHuntCharacter>(GetOwningActor());
	const AHuntWeapon* Weapon = Character ? Character->GetBodyWeapon() : nullptr;
	const UGripAttachmentDefinition* Grip = Weapon ? Weapon->GetGrip() : nullptr;
	const bool bGripRelaxed = Grip && Grip->ThirdPersonRelaxedPose;
	RelaxedPose = bGripRelaxed ? Grip->ThirdPersonRelaxedPose : DefaultRelaxedPose;
	RelaxedRunArms = bGripRelaxed && Grip->ThirdPersonRelaxedRunArms ? Grip->ThirdPersonRelaxedRunArms : DefaultRelaxedRunArms;
	RightHandGripPose = DefaultRightHandPose;
}
