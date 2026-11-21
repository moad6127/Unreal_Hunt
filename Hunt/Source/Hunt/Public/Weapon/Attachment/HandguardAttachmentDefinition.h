// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#include "HandguardAttachmentDefinition.generated.h"

/**
 * 핸드가드(필수 부품). 하부 장비를 열지는 GrantedTags(레일 태그)가 정하고, 하부 장비는 이 메시의 Socket_<슬롯>에 붙는다.
 * 핸드가드 전용 필드는 아직 없다(기존 DA가 이 클래스라 남겨 둔다)
 */
UCLASS()
class HUNT_API UHandguardAttachmentDefinition : public UAttachmentDefinition
{
	GENERATED_BODY()

public:
	UHandguardAttachmentDefinition() { Slot = EHuntAttachmentSlot::Handguard; }
};
