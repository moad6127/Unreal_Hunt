// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	EHuntAttachmentSlot Slot = EHuntAttachmentSlot::Optic;

	// 총 메시의 Socket_<Slot>에 붙는다
	UPROPERTY(EditDefaultsOnly, Category = "Attachment")
	TObjectPtr<UStaticMesh> Mesh;

	// 소켓에서 총 앞(+)·뒤(-)로 옮겨 붙일 거리(cm). 레일에 다는 부착물의 길이 차이를 맞춘다(예: 긴 조준경 +, 레드도트 -)
	UPROPERTY(EditDefaultsOnly, Category = "Attachment", meta = (Units = "cm"))
	float MountOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FHuntAttachmentStats Stats;
};
