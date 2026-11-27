// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Weapon/Attachment/AttachmentTypes.h"
#include "AttachmentDefinition.generated.h"

class UStaticMesh;

/**
 * 부착물 데이터. 부착물 한 종류당 에셋 하나
 */
UCLASS()
class HUNT_API UAttachmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 종류 태그 Attachment.<Slot>을 자동으로 총에 준다
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	EHuntAttachmentSlot Slot = EHuntAttachmentSlot::Optic;

	// 장착하면 총에 더해지는 태그(예: 레일 핸드가드 Weapon.Feature.Rail.Bottom, 소음기 Weapon.Feature.Suppressed)
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer GrantedTags;

	// 총에 모두 있어야 달린다. 총 바디와 필수 부품이 주는 태그만 요구한다(하위 부착물끼리 요구하지 않는다)
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer RequiredTags;

	// 총에 하나라도 있으면 달리지 않는다. 이미 달린 부착물의 BlockedTags에 걸려도 달리지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer BlockedTags;

	// 장착된 부착물 메시 중 Socket_<Slot>이 있는 것에, 없으면 총 메시에 붙는다
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	TObjectPtr<UStaticMesh> Mesh;

	// 달면 숨길 총 바디의 기본 부품 뼈(예: 조준경 b_gun_sight, 탄창 b_gun_mag, 핸드가드 b_gun_handguard). 떼면 다시 보인다
	// 뼈 이름은 모든 총이 같은 규칙을 쓴다. 그 뼈가 없는 총에서는 아무 일도 없다
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	FName HiddenBone;

	// 소켓에서 총 앞(+)·뒤(-)로 옮겨 붙일 거리(cm). 레일에 다는 부착물의 길이 차이를 맞춘다(예: 긴 조준경 +, 레드도트 -)
	UPROPERTY(EditDefaultsOnly, Category = "Attachment", meta = (Units = "cm"))
	float MountOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FHuntAttachmentStats Stats;
};
