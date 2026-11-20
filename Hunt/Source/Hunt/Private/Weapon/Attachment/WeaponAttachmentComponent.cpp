// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Attachment/WeaponAttachmentComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/WeaponDefinition.h"
#include "Weapon/Attachment/HandguardAttachmentDefinition.h"
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

	// 슬롯을 가진 부모: 총에 켜진 슬롯이면 총, 아니면 장착된 핸드가드(2층). 부착물은 부모 메시의 소켓에 붙는다
	USkeletalMeshComponent* WeaponMesh = Weapon->GetMesh();
	USceneComponent* ParentMesh = WeaponMesh;
	const UObject* Parent = WeaponDefinition;
	const FHuntWeaponSlot* WeaponSlot = WeaponDefinition->FindSlot(Attachment->Slot);
	const UHandguardAttachmentDefinition* Handguard = Cast<UHandguardAttachmentDefinition>(EquippedAttachments.FindRef(EHuntAttachmentSlot::Handguard));
	if (!WeaponSlot && Handguard)
	{
		WeaponSlot = Handguard->FindSlot(Attachment->Slot);
		ParentMesh = AttachmentMeshes.FindRef(EHuntAttachmentSlot::Handguard);
		Parent = Handguard;
	}
	if (!WeaponSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s에는 %s 슬롯이 없어 장착하지 않음"),
			*Attachment->GetName(), *Parent->GetName(), *UEnum::GetValueAsString(Attachment->Slot));
		return false;
	}
	if (!WeaponSlot->Attachments.Contains(Attachment) && WeaponSlot->DefaultAttachment != Attachment)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s의 허용 부착물 목록에 없어 장착하지 않음"), *Attachment->GetName(), *Parent->GetName());
		return false;
	}

	const FName SocketName(*FString::Printf(TEXT("Socket_%s"), *StaticEnum<EHuntAttachmentSlot>()->GetNameStringByValue(static_cast<int64>(Attachment->Slot))));
	// 총 슬롯이라도 장착된 핸드가드 메시에 같은 소켓이 있으면 그쪽에 붙인다(총열이 핸드가드에 있어 총구 위치가 핸드가드마다 다름)
	UStaticMeshComponent* HandguardMesh = AttachmentMeshes.FindRef(EHuntAttachmentSlot::Handguard);
	if (ParentMesh == WeaponMesh && Attachment->Slot != EHuntAttachmentSlot::Handguard && HandguardMesh && HandguardMesh->DoesSocketExist(SocketName))
	{
		ParentMesh = HandguardMesh;
	}
	if (!ParentMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s 메시에 %s 소켓이 없어 장착하지 않음"), *Attachment->GetName(), *Parent->GetName(), *SocketName.ToString());
		return false;
	}

	TObjectPtr<UStaticMeshComponent>& MeshComponent = AttachmentMeshes.FindOrAdd(Attachment->Slot);
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->RegisterComponent();
	}
	// 핸드가드가 바뀌면 하부 부착물의 부모 메시도 바뀌므로 장착할 때마다 다시 붙인다
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

	if (Attachment->Slot == EHuntAttachmentSlot::Handguard)
	{
		ReattachAfterHandguardChange();
	}
	return true;
}

void UWeaponAttachmentComponent::ReattachAfterHandguardChange()
{
	// 하부 부착물뿐 아니라 핸드가드 소켓을 쓰는 총 슬롯(총구)도 있으므로 핸드가드 외 전부 다시 단다
	TArray<TObjectPtr<UAttachmentDefinition>> Others;
	EquippedAttachments.GenerateValueArray(Others);
	for (UAttachmentDefinition* Child : Others)
	{
		const EHuntAttachmentSlot ChildSlot = Child->Slot;
		// 다시 장착해 새 핸드가드 소켓으로 옮긴다. 새 핸드가드에 맞지 않으면(거부 로그가 남음) 뗀다
		if (ChildSlot != EHuntAttachmentSlot::Handguard && !EquipAttachment(Child))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: 핸드가드가 바뀌어 떼어 냄"), *Child->GetName());
			EquippedAttachments.Remove(ChildSlot);
			if (TObjectPtr<UStaticMeshComponent> MeshComponent = AttachmentMeshes.FindAndRemoveChecked(ChildSlot))
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

	// 하부 슬롯이 핸드가드에 있으므로 핸드가드를 먼저 단다
	if (const FHuntWeaponSlot* HandguardSlot = WeaponDefinition->FindSlot(EHuntAttachmentSlot::Handguard); HandguardSlot && HandguardSlot->DefaultAttachment)
	{
		EquipAttachment(HandguardSlot->DefaultAttachment);
	}

	const UEnum* SlotEnum = StaticEnum<EHuntAttachmentSlot>();
	for (int32 Index = 0; Index < SlotEnum->NumEnums() - 1; ++Index) // 마지막은 자동 생성된 _MAX
	{
		const FHuntWeaponSlot* WeaponSlot = WeaponDefinition->FindSlot(static_cast<EHuntAttachmentSlot>(SlotEnum->GetValueByIndex(Index)));
		if (WeaponSlot && WeaponSlot->DefaultAttachment && WeaponSlot->DefaultAttachment->Slot != EHuntAttachmentSlot::Handguard)
		{
			EquipAttachment(WeaponSlot->DefaultAttachment);
		}
	}
}

FHuntAttachmentStats UWeaponAttachmentComponent::ComputeFinalStats() const
{
	// 핸드가드 자식도 같은 맵에 슬롯별로 들어 있어 한 번만 돈다
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
