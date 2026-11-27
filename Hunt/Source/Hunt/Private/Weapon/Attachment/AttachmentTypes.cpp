// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Attachment/AttachmentTypes.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Optic, "Attachment.Optic", "조준경");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Muzzle, "Attachment.Muzzle", "총구 장치");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Underbarrel, "Attachment.Underbarrel", "하부 장비");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Magazine, "Attachment.Magazine", "탄창");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Stock, "Attachment.Stock", "개머리판");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Tactical, "Attachment.Tactical", "전술 장비");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Handguard, "Attachment.Handguard", "핸드가드");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_Chassis, "Attachment.Chassis", "총몸");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attachment_PistolGrip, "Attachment.PistolGrip", "권총 손잡이");

FGameplayTag GetAttachmentSlotTag(EHuntAttachmentSlot Slot)
{
	// default 없이 두어 슬롯을 추가하고 태그를 빠뜨리면 컴파일러 경고가 난다
	switch (Slot)
	{
	case EHuntAttachmentSlot::Optic:       return TAG_Attachment_Optic;
	case EHuntAttachmentSlot::Muzzle:      return TAG_Attachment_Muzzle;
	case EHuntAttachmentSlot::Underbarrel: return TAG_Attachment_Underbarrel;
	case EHuntAttachmentSlot::Magazine:    return TAG_Attachment_Magazine;
	case EHuntAttachmentSlot::Stock:       return TAG_Attachment_Stock;
	case EHuntAttachmentSlot::Tactical:    return TAG_Attachment_Tactical;
	case EHuntAttachmentSlot::Handguard:   return TAG_Attachment_Handguard;
	case EHuntAttachmentSlot::Chassis:     return TAG_Attachment_Chassis;
	case EHuntAttachmentSlot::PistolGrip:  return TAG_Attachment_PistolGrip;
	}
	return FGameplayTag();
}
