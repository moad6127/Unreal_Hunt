// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/AttachmentDefinition.h"
#include "GripAttachmentDefinition.generated.h"

class UAnimSequence;

// 1인칭 왼손 그립 포즈 3개. 손 모양·팔꿈치를 정하고, 손목 위치는 IK가 Socket_LeftHand_FP에 맞춘다
USTRUCT(BlueprintType)
struct FHuntGripPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Aim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Relaxed;
};

/**
 * 손잡이(Underbarrel) 부착물. 달면 1인칭 왼손이 이 그립 포즈로 바뀐다
 */
UCLASS()
class HUNT_API UGripAttachmentDefinition : public UAttachmentDefinition
{
	GENERATED_BODY()

public:
	UGripAttachmentDefinition() { Slot = EHuntAttachmentSlot::Underbarrel; }

	UPROPERTY(EditDefaultsOnly, Category = "Grip")
	FHuntGripPoses FirstPersonGripPoses;

	// 손잡이를 쥔 손 모양. 3인칭은 상태 애니메이션 위에 손가락 뼈만 이 포즈로 덮고, 손목 위치·방향은 손잡이 Socket_LeftHand가 정한다
	UPROPERTY(EditDefaultsOnly, Category = "Grip")
	TObjectPtr<UAnimSequence> HandPose;

	// 3인칭 대기 자세(서기·걷기 포즈)와 앞 달리기 팔. 대기 자세에서 기본 포즈로는 손잡이를 쥘 수 없을 때만 둔다(총 기울기·왼팔이 손잡이에 맞춰져 있음)
	// 비어 있으면 기본 대기 포즈를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Grip")
	TObjectPtr<UAnimSequence> ThirdPersonRelaxedPose;

	UPROPERTY(EditDefaultsOnly, Category = "Grip")
	TObjectPtr<UAnimSequence> ThirdPersonRelaxedRunArms;
};
