// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 부착물 시스템에서 두 곳 이상이 같이 쓰는 enum·구조체. 한 클래스만 쓰는 타입은 그 클래스 헤더에 둔다

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "AttachmentTypes.generated.h"

class UAnimSequence;
class UAttachmentDefinition;

// 부착 슬롯. 게임 전체에 하나의 목록이고, 총마다 쓸 수 있는 슬롯만 켠다. 소켓 이름은 Socket_<슬롯 이름>,
// 종류 태그는 GetAttachmentSlotTag(슬롯을 추가하면 태그도 추가)
UENUM(BlueprintType)
enum class EHuntAttachmentSlot : uint8
{
	Optic,
	Muzzle,
	Underbarrel,
	Magazine,
	Stock,
	Tactical,
	Handguard
};

// 부착물 종류 태그. 장착된 부착물이 총에 자동으로 준다(코드가 쓰는 태그라 네이티브, 데이터 전용 태그는 Config/DefaultGameplayTags.ini)
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Optic);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Muzzle);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Underbarrel);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Magazine);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Stock);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Tactical);
HUNT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attachment_Handguard);

// 슬롯의 종류 태그
HUNT_API FGameplayTag GetAttachmentSlotTag(EHuntAttachmentSlot Slot);

// 총(UWeaponDefinition)의 부착 슬롯 하나. 무엇을 달 수 있는지는 슬롯이 아니라 태그가 정한다(UAttachmentDefinition::RequiredTags)
USTRUCT(BlueprintType)
struct FHuntWeaponSlot
{
	GENERATED_BODY()

	// 필수 부품(핸드가드·개머리판·탄창 등). 하위 부착물보다 먼저 달리고, 바뀌면 하위 부착물을 다시 검사한다. M3부터 비울 수 없음
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bRequired = false;

	// 부착물을 달면 숨길 기본 부품 뼈(예: 탄창 b_gun_mag). 없으면 비워 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName HiddenBone;
};

// 부착물 스탯 배율. 총(UWeaponDefinition) 값에 곱하고, 장착된 부착물끼리도 곱한다. 1 = 변화 없음
USTRUCT(BlueprintType)
struct FHuntAttachmentStats
{
	GENERATED_BODY()

	// VerticalRecoil, KickBack, KickPitch에 곱한다
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float VerticalRecoil = 1.f;

	// HorizontalRecoil, KickYaw에 곱한다
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float HorizontalRecoil = 1.f;

	// HipSpread, MovingSpread, AirSpread에 곱한다
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Spread = 1.f;

	// AimDownSightsTime에 곱한다(작을수록 조준이 빠름)
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.1"))
	float AimDownSightsTime = 1.f;
};

// 1인칭 왼손 그립 포즈 3개. 손 모양·팔꿈치를 정하고, 손목 위치는 IK가 Socket_LeftHand_FP에 맞춘다
USTRUCT(BlueprintType)
struct FHuntGripPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Aim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> Relaxed;
};
