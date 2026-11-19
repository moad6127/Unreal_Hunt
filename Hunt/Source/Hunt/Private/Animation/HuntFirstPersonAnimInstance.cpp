// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/HuntFirstPersonAnimInstance.h"
#include "Character/HuntCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/Attachment/GripAttachmentDefinition.h"

namespace
{
	// 1인칭 팔 흔들림이 그림자(전신) 걸음보다 이만큼(주기 비율) 앞선다. PIE에서 그림자보다 팔이 살짝 먼저 움직이게 맞춘 값(0.5 = 좌우 반대)
	constexpr float SwayPhaseOffset = 0.2f;
}

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
	// 공중에서는 발을 딛지 않으므로 걷기·달리기 흔들림을 멈춘다
	bShouldMove = GroundSpeed > 3.f && !Character->GetCharacterMovement()->IsFalling();
	AimAlpha = Character->GetAimBlend();
	// 1인칭은 대기 자세를 전용 포즈(RelaxedAlpha)로 하고, 내린 자세 애디티브는 벽 앞에서만 쓴다(대기에 쓰면 팔이 화면 밖으로 나간다)
	WallBlockAlpha = Character->GetWallBlockAlpha();
	// 질주 중에는 준비 자세 위에서 달린다(3인칭과 같게). 질주 비중 = ABP 질주 블렌드와 같은 GroundSpeed 300~600
	RelaxedAlpha = Character->GetRelaxedBlend() * (1.f - FMath::GetMappedRangeValueClamped(FVector2f(300.f, 600.f), FVector2f(0.f, 1.f), GroundSpeed));

	UpdateSwayTimes();
	UpdateGripPoses();
	UpdateLeftHandIK();
}

void UHuntFirstPersonAnimInstance::UpdateSwayTimes()
{
	// 전신 걸음 위치(0~1, Foot_L에서 시작) → 1인칭 걷기(Walk_F 4초 = 6주기)·질주(Run_02 13.33초 = 20주기) 시간. 두 애니메이션 모두 한 주기 0.667초
	// 0.033/0.3초 = 각 애니메이션에서 총이 처음 메시 +X 끝에 닿는 시각(measure: walk_sway.txt)이라 SwayPhaseOffset 하나로 둘 다 맞춘다
	const UAnimInstance* BodyAnim = Character->GetMesh()->GetAnimInstance();
	const FMarkerSyncAnimPosition StepPos = BodyAnim ? BodyAnim->GetSyncGroupPosition(TEXT("Locomotion")) : FMarkerSyncAnimPosition();
	if (StepPos.IsValid())
	{
		const float StepPhase = (StepPos.PreviousMarkerName == TEXT("Foot_R") ? 0.5f : 0.f) + StepPos.PositionBetweenMarkers * 0.5f;
		if (StepPhase < LastStepPhase - 0.5f)
		{
			StepCycle = (StepCycle + 1) % 60; // 60 = 6과 20의 최소공배수
		}
		LastStepPhase = StepPhase;
		constexpr float SwayCycle = 2.f / 3.f;
		const float Cycles = StepCycle + StepPhase + SwayPhaseOffset + 60.f; // +60: 오프셋이 음수여도 양수로
		WalkSwayTime = FMath::Fmod(Cycles * SwayCycle + 0.033f, 4.f);
		SprintSwayTime = FMath::Fmod(Cycles * SwayCycle + 0.3f, 13.333f);
	}
}

void UHuntFirstPersonAnimInstance::UpdateGripPoses()
{
	const AHuntWeapon* Weapon = Character->GetEquippedWeapon();
	const UGripAttachmentDefinition* Grip = Weapon ? Weapon->GetGrip() : nullptr;
	GripPoses = Grip ? Grip->FirstPersonGripPoses : DefaultGripPoses;
}

void UHuntFirstPersonAnimInstance::UpdateLeftHandIK()
{
	const AHuntWeapon* Weapon = Character->GetEquippedWeapon();
	FVector GripLocation;
	bLeftHandIK = Weapon && Weapon->GetRootComponent()->GetAttachParent() == GetSkelMeshComponent() && Weapon->GetFirstPersonLeftHandGripLocation(GripLocation);
	if (bLeftHandIK)
	{
		// 총이 ik_hand_gun에 붙어 있으므로 이 상대값은 프레임 지연 없이 정확하다
		LeftHandIKLocation = GetSkelMeshComponent()->GetSocketTransform(TEXT("ik_hand_gun")).InverseTransformPosition(GripLocation);
	}
}
