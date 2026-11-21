// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Attachment/WeaponAttachmentComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/WeaponDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	// 부착물이 총에 주는 태그: GrantedTags + 종류 태그 Attachment.<Slot>
	FGameplayTagContainer GetAttachmentTags(const UAttachmentDefinition* Attachment)
	{
		FGameplayTagContainer Tags = Attachment->GrantedTags;
		Tags.AddTag(GetAttachmentSlotTag(Attachment->Slot));
		return Tags;
	}
}

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
	FString Reason;
	if (!CanEquipByTags(Attachment, Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s 장착하지 않음"), *Attachment->GetName(), *Reason);
		return false;
	}

	// 하위 부착물은 필수 부품 메시(예: 핸드가드)에 같은 소켓이 있으면 거기에, 없으면 총 메시에 붙는다
	const FName SocketName(*FString::Printf(TEXT("Socket_%s"), *StaticEnum<EHuntAttachmentSlot>()->GetNameStringByValue(static_cast<int64>(Attachment->Slot))));
	USkeletalMeshComponent* WeaponMesh = Weapon->GetMesh();
	USceneComponent* ParentMesh = WeaponMesh;
	if (!WeaponSlot->bRequired)
	{
		if (UStaticMeshComponent* PartMesh = FindRequiredPartSocketMesh(SocketName))
		{
			ParentMesh = PartMesh;
		}
	}
	if (!ParentMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s 소켓이 없어 장착하지 않음"), *Attachment->GetName(), *SocketName.ToString());
		return false;
	}

	TObjectPtr<UStaticMeshComponent>& MeshComponent = AttachmentMeshes.FindOrAdd(Attachment->Slot);
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->RegisterComponent();
	}
	// 필수 부품이 바뀌면 하위 부착물의 부모 메시도 바뀌므로 장착할 때마다 다시 붙인다
	MeshComponent->AttachToComponent(ParentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
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

	if (WeaponSlot->bRequired)
	{
		RecheckOptionalAttachments();
	}
	return true;
}

bool UWeaponAttachmentComponent::CanEquipByTags(const UAttachmentDefinition* Attachment, FString& OutReason) const
{
	const UWeaponDefinition* WeaponDefinition = GetOwner<AHuntWeapon>()->Definition;

	// 총의 태그 = 총 바디 + 장착된 부착물. 같은 슬롯에서 교체될 부착물은 뺀다.
	// RequiredTags는 바디·필수 부품 태그로만 판단한다(하위 부착물이 준 태그로 붙으면 장착 순서에 따라 결과가 달라진다)
	FGameplayTagContainer WeaponTags = WeaponDefinition->GrantedTags;
	FGameplayTagContainer BaseTags = WeaponDefinition->GrantedTags;
	for (const TPair<EHuntAttachmentSlot, TObjectPtr<UAttachmentDefinition>>& Pair : EquippedAttachments)
	{
		if (Pair.Key != Attachment->Slot)
		{
			const FGameplayTagContainer PartTags = GetAttachmentTags(Pair.Value);
			WeaponTags.AppendTags(PartTags);
			const FHuntWeaponSlot* PartSlot = WeaponDefinition->FindSlot(Pair.Key);
			if (PartSlot && PartSlot->bRequired)
			{
				BaseTags.AppendTags(PartTags);
			}
		}
	}

	for (const FGameplayTag& Tag : Attachment->RequiredTags)
	{
		if (!BaseTags.HasTag(Tag))
		{
			OutReason = FString::Printf(TEXT("총 바디·필수 부품에 %s 태그가 없어"), *Tag.ToString());
			return false;
		}
	}
	if (WeaponTags.HasAny(Attachment->BlockedTags))
	{
		OutReason = FString::Printf(TEXT("총의 태그가 BlockedTags(%s)에 걸려"), *Attachment->BlockedTags.ToStringSimple());
		return false;
	}
	// 이미 달린 부착물이 이 부착물의 태그를 막는지(양방향)
	const FGameplayTagContainer NewTags = GetAttachmentTags(Attachment);
	for (const TPair<EHuntAttachmentSlot, TObjectPtr<UAttachmentDefinition>>& Pair : EquippedAttachments)
	{
		if (Pair.Key != Attachment->Slot && NewTags.HasAny(Pair.Value->BlockedTags))
		{
			OutReason = FString::Printf(TEXT("%s의 BlockedTags에 걸려"), *Pair.Value->GetName());
			return false;
		}
	}
	return true;
}

void UWeaponAttachmentComponent::RecheckOptionalAttachments()
{
	// 필수 부품이 바뀌면 하위 부착물을 다시 달아 새 부품의 소켓으로 옮기고, 태그가 맞지 않으면(예: 레일 없는 핸드가드의 손잡이) 뗀다.
	// 하위 부착물은 총 바디·필수 부품의 태그만 요구하므로 여기서 한 번만 돌면 된다
	const UWeaponDefinition* WeaponDefinition = GetOwner<AHuntWeapon>()->Definition;
	TArray<TObjectPtr<UAttachmentDefinition>> Equipped;
	EquippedAttachments.GenerateValueArray(Equipped);
	for (UAttachmentDefinition* Child : Equipped)
	{
		const FHuntWeaponSlot* ChildSlot = WeaponDefinition->FindSlot(Child->Slot);
		if (ChildSlot && !ChildSlot->bRequired && !EquipAttachment(Child))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: 필수 부품이 바뀌어 떼어 냄"), *Child->GetName());
			EquippedAttachments.Remove(Child->Slot);
			if (TObjectPtr<UStaticMeshComponent> MeshComponent = AttachmentMeshes.FindAndRemoveChecked(Child->Slot))
			{
				MeshComponent->DestroyComponent();
			}
		}
	}
}

void UWeaponAttachmentComponent::EquipDefaultAttachments()
{
	const AHuntWeapon* Weapon = GetOwner<AHuntWeapon>();
	const UWeaponDefinition* WeaponDefinition = Weapon ? Weapon->Definition.Get() : nullptr;
	if (!WeaponDefinition)
	{
		return;
	}

	// 하위 부착물이 필수 부품의 태그·소켓을 쓰므로 목록 순서와 상관없이 필수 부품을 먼저 단다
	for (const bool bRequiredPass : { true, false })
	{
		for (UAttachmentDefinition* Attachment : WeaponDefinition->DefaultAttachments)
		{
			const FHuntWeaponSlot* Slot = Attachment ? WeaponDefinition->FindSlot(Attachment->Slot) : nullptr;
			if (Attachment && (Slot && Slot->bRequired) == bRequiredPass)
			{
				EquipAttachment(Attachment);
			}
		}
	}
}

FHuntAttachmentStats UWeaponAttachmentComponent::ComputeFinalStats() const
{
	// 필수 부품·하위 부착물이 모두 같은 맵에 슬롯별로 들어 있어 한 번만 돈다
	FHuntAttachmentStats Final;
	for (const TPair<EHuntAttachmentSlot, TObjectPtr<UAttachmentDefinition>>& Pair : EquippedAttachments)
	{
		const FHuntAttachmentStats& Stats = Pair.Value->Stats;
		Final.VerticalRecoil *= Stats.VerticalRecoil;
		Final.HorizontalRecoil *= Stats.HorizontalRecoil;
		Final.Spread *= Stats.Spread;
		Final.AimDownSightsTime *= Stats.AimDownSightsTime;
	}
	return Final;
}

const UStaticMeshComponent* UWeaponAttachmentComponent::FindAttachmentSocketMesh(EHuntAttachmentSlot Slot, FName SocketName) const
{
	const TObjectPtr<UStaticMeshComponent>* MeshComponent = AttachmentMeshes.Find(Slot);
	return MeshComponent && *MeshComponent && (*MeshComponent)->DoesSocketExist(SocketName) ? MeshComponent->Get() : nullptr;
}

UStaticMeshComponent* UWeaponAttachmentComponent::FindRequiredPartSocketMesh(FName SocketName) const
{
	const UWeaponDefinition* WeaponDefinition = GetOwner<AHuntWeapon>()->Definition;
	for (const TPair<EHuntAttachmentSlot, TObjectPtr<UStaticMeshComponent>>& Pair : AttachmentMeshes)
	{
		const FHuntWeaponSlot* PartSlot = WeaponDefinition->FindSlot(Pair.Key);
		if (PartSlot && PartSlot->bRequired && Pair.Value->DoesSocketExist(SocketName))
		{
			return Pair.Value;
		}
	}
	return nullptr;
}

const USceneComponent* UWeaponAttachmentComponent::FindSocketComponent(EHuntAttachmentSlot Slot, FName SocketName) const
{
	if (const UStaticMeshComponent* SlotMesh = FindAttachmentSocketMesh(Slot, SocketName))
	{
		return SlotMesh;
	}
	if (const UStaticMeshComponent* PartMesh = FindRequiredPartSocketMesh(SocketName))
	{
		return PartMesh;
	}
	return GetOwner<AHuntWeapon>()->GetMesh();
}
