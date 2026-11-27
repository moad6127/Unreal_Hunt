// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#include "ChassisAttachmentDefinition.generated.h"

class UAnimInstance;

/**
 * 총몸(필수 부품). 오른손 손잡이가 여기 있어서 손 자세(총 계열 애님 레이어)와 오른손 소켓(Socket_Grip, Socket_RightHand…)을 정한다
 */
UCLASS()
class HUNT_API UChassisAttachmentDefinition : public UAttachmentDefinition
{
	GENERATED_BODY()

public:
	UChassisAttachmentDefinition() { Slot = EHuntAttachmentSlot::Chassis; }

	// 비어 있으면 총 DA의 레이어를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimLayers;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimLayers;

	// 손잡이가 따로인 총몸(메시에 Socket_PistolGrip이 있음)의 기본 손잡이. 총몸을 달 때 손잡이가 비어 있으면 단다.
	// 손잡이가 일체형인 총몸(나무 몸통)은 비운다
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	TObjectPtr<UAttachmentDefinition> DefaultPistolGrip;
};
