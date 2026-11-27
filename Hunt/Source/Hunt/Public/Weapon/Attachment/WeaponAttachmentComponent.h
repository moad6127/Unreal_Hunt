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
	// 총의 태그가 부착물 조건(Required·Blocked)에 맞고 그 슬롯 소켓이 있으면 메시를 붙인다(같은 슬롯의 이전 부착물은 교체). 거부하면 false와 경고 로그
	bool EquipAttachment(UAttachmentDefinition* Attachment);

	// 총 Definition의 출고 상태를 단다: 필수 부품(Required Parts) 먼저, 그다음 DefaultAttachments
	void EquipDefaultAttachments();

	// 그 슬롯에 달린 부착물 메시에 이 소켓이 있으면 그 메시, 없으면 nullptr (총 소켓을 쓰면 된다)
	const UStaticMeshComponent* FindAttachmentSocketMesh(EHuntAttachmentSlot Slot, FName SocketName) const;

	// 이 소켓이 있는 메시: 그 슬롯 부착물 → 필수 부품(예: 핸드가드, 총몸) → 총 메시. 소켓이 어디에도 없으면 총 메시(DoesSocketExist로 확인)
	// 하위 부착물이 붙는 자리·발사 위치·왼손 위치가 모두 이 순서를 쓴다
	const USceneComponent* FindSocketComponent(EHuntAttachmentSlot Slot, FName SocketName) const;

	// 그 슬롯에 달린 부착물 (없으면 nullptr)
	const UAttachmentDefinition* GetEquippedAttachment(EHuntAttachmentSlot Slot) const { return EquippedAttachments.FindRef(Slot); }

	// 장착된 부착물 전체(핸드가드 자식 포함)의 스탯 배율을 곱한 값
	FHuntAttachmentStats ComputeFinalStats() const;

private:
	// 총의 태그로 RequiredTags·BlockedTags를 검사한다. 거부하면 OutReason에 이유
	bool CanEquipByTags(const UAttachmentDefinition* Attachment, FString& OutReason) const;

	// 필수 부품을 바꾼 뒤 하위 부착물을 다시 달고, 태그가 맞지 않으면 뗀다
	void RecheckOptionalAttachments();

	// 이 소켓이 있는 필수 부품 메시 (없으면 nullptr)
	UStaticMeshComponent* FindRequiredPartSocketMesh(FName SocketName) const;

	// 부착물이 숨겼던 총 바디 뼈를 다시 보이게 한다(떼거나 교체할 때)
	void ShowHiddenBone(const UAttachmentDefinition* Attachment) const;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Attachment")
	TMap<EHuntAttachmentSlot, TObjectPtr<UAttachmentDefinition>> EquippedAttachments;

	UPROPERTY(Transient)
	TMap<EHuntAttachmentSlot, TObjectPtr<UStaticMeshComponent>> AttachmentMeshes;
};
