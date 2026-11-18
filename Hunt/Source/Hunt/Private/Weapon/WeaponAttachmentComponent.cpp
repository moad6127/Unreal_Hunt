// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponAttachmentComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

bool UWeaponAttachmentComponent::EquipAttachment(UAttachmentDefinition* Attachment)
{
	const AHuntWeapon* Weapon = GetOwner<AHuntWeapon>();
	const UWeaponDefinition* WeaponDefinition = Weapon ? Weapon->Definition.Get() : nullptr;
	if (!Attachment || !WeaponDefinition)
	{
		return false;
	}

	const FHuntWeaponSlot* WeaponSlot = WeaponDefinition->FindSlot(Attachment->Slot);
	if (!WeaponSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s에는 %s 슬롯이 없어 장착하지 않음"),
			*Attachment->GetName(), *WeaponDefinition->GetName(), *UEnum::GetValueAsString(Attachment->Slot));
		return false;
	}
	if (!WeaponSlot->Attachments.Contains(Attachment) && WeaponSlot->DefaultAttachment != Attachment)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s의 허용 부착물 목록에 없어 장착하지 않음"), *Attachment->GetName(), *WeaponDefinition->GetName());
		return false;
	}

	USkeletalMeshComponent* WeaponMesh = Weapon->GetMesh();
	const FName SocketName(*FString::Printf(TEXT("Socket_%s"), *StaticEnum<EHuntAttachmentSlot>()->GetNameStringByValue(static_cast<int64>(Attachment->Slot))));
	if (!WeaponMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s 메시에 %s 소켓이 없어 장착하지 않음"), *Attachment->GetName(), *WeaponDefinition->GetName(), *SocketName.ToString());
		return false;
	}

	TObjectPtr<UStaticMeshComponent>& MeshComponent = AttachmentMeshes.FindOrAdd(Attachment->Slot);
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetupAttachment(WeaponMesh, SocketName);
		MeshComponent->RegisterComponent();
	}
	// 1인칭 팔 총이면 1인칭 렌더, 전신 총이면 내 화면에서 그림자만 남게 총 메시를 따른다
	MeshComponent->SetFirstPersonPrimitiveType(WeaponMesh->FirstPersonPrimitiveType);
	MeshComponent->SetStaticMesh(Attachment->Mesh);
	// 레일 슬롯 소켓은 +Y가 총 앞이다(M4·AR4 모두). AR4 총구 소켓은 축이 달라 총구 부착물은 0으로 둔다
	MeshComponent->SetRelativeLocation(FVector(0.f, Attachment->MountOffset, 0.f));

	// 기본 부품(예: 탄창 뼈)을 숨긴다
	if (!WeaponSlot->HiddenBone.IsNone())
	{
		WeaponMesh->HideBoneByName(WeaponSlot->HiddenBone, PBO_None);
	}

	EquippedAttachments.Add(Attachment->Slot, Attachment);
	return true;
}

void UWeaponAttachmentComponent::EquipDefaultAttachments()
{
	const AHuntWeapon* Weapon = GetOwner<AHuntWeapon>();
	const UWeaponDefinition* WeaponDefinition = Weapon ? Weapon->Definition.Get() : nullptr;
	if (!WeaponDefinition)
	{
		return;
	}

	const UEnum* SlotEnum = StaticEnum<EHuntAttachmentSlot>();
	for (int32 Index = 0; Index < SlotEnum->NumEnums() - 1; ++Index) // 마지막은 자동 생성된 _MAX
	{
		const FHuntWeaponSlot* WeaponSlot = WeaponDefinition->FindSlot(static_cast<EHuntAttachmentSlot>(SlotEnum->GetValueByIndex(Index)));
		if (WeaponSlot && WeaponSlot->DefaultAttachment)
		{
			EquipAttachment(WeaponSlot->DefaultAttachment);
		}
	}
}

const UStaticMeshComponent* UWeaponAttachmentComponent::FindAttachmentSocketMesh(EHuntAttachmentSlot Slot, FName SocketName) const
{
	const TObjectPtr<UStaticMeshComponent>* MeshComponent = AttachmentMeshes.Find(Slot);
	return MeshComponent && *MeshComponent && (*MeshComponent)->DoesSocketExist(SocketName) ? MeshComponent->Get() : nullptr;
}
