// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#include "HandguardAttachmentDefinition.generated.h"

/**
 * 핸드가드(몸체 부품). 레일이 있는 핸드가드만 하부 슬롯을 켠다. 하부 부착물은 이 핸드가드 메시의 Socket_<슬롯>에 붙는다
 */
UCLASS()
class HUNT_API UHandguardAttachmentDefinition : public UAttachmentDefinition
{
	GENERATED_BODY()

public:
	UHandguardAttachmentDefinition() { Slot = EHuntAttachmentSlot::Handguard; }

	// 슬롯이 켜져 있으면 그 슬롯을 반환, 꺼져 있으면 nullptr
	const FHuntWeaponSlot* FindSlot(EHuntAttachmentSlot InSlot) const
	{
		switch (InSlot)
		{
		case EHuntAttachmentSlot::Underbarrel: return bUnderbarrel ? &Underbarrel : nullptr;
		case EHuntAttachmentSlot::Tactical:    return bTactical ? &Tactical : nullptr;
		default:                               return nullptr;
		}
	}

	UPROPERTY(EditDefaultsOnly, Category = "Attachments", meta = (InlineEditConditionToggle))
	bool bUnderbarrel = false;

	UPROPERTY(EditDefaultsOnly, Category = "Attachments", meta = (EditCondition = "bUnderbarrel"))
	FHuntWeaponSlot Underbarrel;

	UPROPERTY(EditDefaultsOnly, Category = "Attachments", meta = (InlineEditConditionToggle))
	bool bTactical = false;

	UPROPERTY(EditDefaultsOnly, Category = "Attachments", meta = (EditCondition = "bTactical"))
	FHuntWeaponSlot Tactical;
};
