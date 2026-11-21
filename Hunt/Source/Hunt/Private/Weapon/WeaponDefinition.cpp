// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponDefinition.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UWeaponDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	// 필수 슬롯은 출고 목록에 그 슬롯 부착물이 있어야 한다(필수 부품 없는 총이 나오지 않게)
	const UEnum* SlotEnum = StaticEnum<EHuntAttachmentSlot>();
	for (int32 Index = 0; Index < SlotEnum->NumEnums() - 1; ++Index) // 마지막은 자동 생성된 _MAX
	{
		const EHuntAttachmentSlot Slot = static_cast<EHuntAttachmentSlot>(SlotEnum->GetValueByIndex(Index));
		const FHuntWeaponSlot* WeaponSlot = FindSlot(Slot);
		const bool bHasDefault = DefaultAttachments.ContainsByPredicate([Slot](const UAttachmentDefinition* A) { return A && A->Slot == Slot; });
		if (WeaponSlot && WeaponSlot->bRequired && !bHasDefault)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("필수 슬롯 %s의 부착물이 DefaultAttachments에 없습니다"), *SlotEnum->GetNameStringByIndex(Index))));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

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
