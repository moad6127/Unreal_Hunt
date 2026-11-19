// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponDefinition.h"

const FHuntWeaponSlot* UWeaponDefinition::FindSlot(EHuntAttachmentSlot Slot) const
{
	switch (Slot)
	{
	case EHuntAttachmentSlot::Optic:       return bOptic ? &Optic : nullptr;
	case EHuntAttachmentSlot::Muzzle:      return bMuzzle ? &Muzzle : nullptr;
	case EHuntAttachmentSlot::Underbarrel: return bUnderbarrel ? &Underbarrel : nullptr;
	case EHuntAttachmentSlot::Magazine:    return bMagazine ? &Magazine : nullptr;
	case EHuntAttachmentSlot::Stock:       return bStock ? &Stock : nullptr;
	case EHuntAttachmentSlot::Tactical:    return bTactical ? &Tactical : nullptr;
	case EHuntAttachmentSlot::Handguard:   return bHandguard ? &Handguard : nullptr;
	}
	return nullptr;
}
