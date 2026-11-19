// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#include "WeaponAttachmentComponent.generated.h"

class UStaticMeshComponent;

/**
 * 총(AHuntWeapon)의 장착 상태. 장착은 EquipAttachment 하나로만 바꾼다
 */
UCLASS()
class HUNT_API UWeaponAttachmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 총이 그 슬롯을 쓰고 부착물이 이 총과 호환되면 소켓에 메시를 붙인다(같은 슬롯의 이전 부착물은 교체). 거부하면 false와 경고 로그
	bool EquipAttachment(UAttachmentDefinition* Attachment);

	// 총 Definition의 켜진 슬롯마다 기본 부착물(DefaultAttachment)을 단다
	void EquipDefaultAttachments();

	// 그 슬롯에 달린 부착물 메시에 이 소켓이 있으면 그 메시, 없으면 nullptr (총 소켓을 쓰면 된다)
	const UStaticMeshComponent* FindAttachmentSocketMesh(EHuntAttachmentSlot Slot, FName SocketName) const;

	// 그 슬롯에 달린 부착물 (없으면 nullptr)
	const UAttachmentDefinition* GetEquippedAttachment(EHuntAttachmentSlot Slot) const { return EquippedAttachments.FindRef(Slot); }

private:
	// 핸드가드를 바꾼 뒤 다른 부착물을 다시 달아 새 핸드가드 소켓으로 옮기고, 맞지 않으면(하부 부착물) 뗀다
	void ReattachAfterHandguardChange();

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Attachment")
	TMap<EHuntAttachmentSlot, TObjectPtr<UAttachmentDefinition>> EquippedAttachments;

	UPROPERTY(Transient)
	TMap<EHuntAttachmentSlot, TObjectPtr<UStaticMeshComponent>> AttachmentMeshes;
};
