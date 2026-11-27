// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponDefinition.h"
#include "Weapon/Attachment/ChassisAttachmentDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UWeaponDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const UEnum* SlotEnum = StaticEnum<EHuntAttachmentSlot>();
	// 필수 부품 칸에는 그 슬롯의 부품만 넣는다
	const TPair<EHuntAttachmentSlot, const UAttachmentDefinition*> RequiredParts[] = {
		{ EHuntAttachmentSlot::Chassis, RequiredChassis },
		{ EHuntAttachmentSlot::Handguard, RequiredHandguard },
		{ EHuntAttachmentSlot::Magazine, RequiredMagazine } };
	for (const TPair<EHuntAttachmentSlot, const UAttachmentDefinition*>& Part : RequiredParts)
	{
		if (Part.Value && Part.Value->Slot != Part.Key)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("필수 부품 %s 칸에 %s 슬롯 부착물(%s)이 들어 있습니다"),
				*SlotEnum->GetNameStringByValue(static_cast<int64>(Part.Key)), *SlotEnum->GetNameStringByValue(static_cast<int64>(Part.Value->Slot)), *Part.Value->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}
	// 기본 부착물에는 선택 부착물만 둔다(필수 슬롯 부품이 두 곳에 있으면 어느 것이 달릴지 헷갈린다)
	for (const UAttachmentDefinition* Attachment : DefaultAttachments)
	{
		if (Attachment && IsRequiredSlot(Attachment->Slot))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("%s는 필수 슬롯(%s) 부품이라 DefaultAttachments가 아니라 Required Parts에 넣습니다"),
				*Attachment->GetName(), *SlotEnum->GetNameStringByValue(static_cast<int64>(Attachment->Slot)))));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

UAttachmentDefinition* UWeaponDefinition::GetRequiredPart(EHuntAttachmentSlot Slot) const
{
	switch (Slot)
	{
	case EHuntAttachmentSlot::Chassis:   return RequiredChassis;
	case EHuntAttachmentSlot::Handguard: return RequiredHandguard;
	case EHuntAttachmentSlot::Magazine:  return RequiredMagazine;
	default:                             return nullptr;
	}
}
