// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Attachment/AttachmentDefinition.h"
#include "OpticAttachmentDefinition.generated.h"

class UTexture2D;

/**
 * 조준경(Optic) 부착물. 달고 조준하면 시야가 이 배율만큼 좁아진다
 */
UCLASS()
class HUNT_API UOpticAttachmentDefinition : public UAttachmentDefinition
{
	GENERATED_BODY()

public:
	UOpticAttachmentDefinition() { Slot = EHuntAttachmentSlot::Optic; }

	// 맨눈 대비 실제 배율. 시야(100m에서 보이는 폭) = FieldOfViewAt100mAt1x / 배율
	// shortcut: 고정 배율, 가변 배율이 필요해지면 MaxMagnification과 휠 입력을 더하고 이 값을 최소 배율로 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Optic", meta = (ClampMin = "1.0"))
	float Magnification = 4.f;

	// 1배일 때 100m에서 렌즈 원에 보이는 폭(m) = 스펙 시야 × 그 배율. 조준 중 접안렌즈 원이 정확히 (이 값 / 배율) 폭을 보이도록 세상 FOV를 정한다
	// (렌즈 반지름 = 시야 절반 × 10 mil이라 레티클 mil 눈금이 실제와 같다). 기본 26.5 = S&B PM II 5배 시야 5.3m × 5
	UPROPERTY(EditDefaultsOnly, Category = "Optic", meta = (ClampMin = "0.1", Units = "m"))
	float FieldOfViewAt100mAt1x = 26.5f;

	// 접안렌즈 유리 반지름(cm). 조준 중 Socket_Sight(접안렌즈 중심)에서 이 반지름 원 안의 조준경 메시를 잘라 너머 세상이 보이게 한다
	UPROPERTY(EditDefaultsOnly, Category = "Optic", meta = (ClampMin = "0.1", Units = "cm"))
	float EyepieceRadius = 1.8f;

	// 레티클 모양(흑백, 흰색 = 선). 텍스처 전체가 접안렌즈 원에 맞춰 그려진다. 비우면 M_ScopeReticle 기본값(듀플렉스)
	// shortcut: 크기 고정(SFP). 가변 배율을 넣을 때 FFP 조준경은 배율에 비례해 키운다
	UPROPERTY(EditDefaultsOnly, Category = "Optic")
	TObjectPtr<UTexture2D> Reticle;
};
