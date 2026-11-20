// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// 부착물 시스템에서 두 곳 이상이 같이 쓰는 enum·구조체. 한 클래스만 쓰는 타입은 그 클래스 헤더에 둔다

#include "CoreMinimal.h"
#include "AttachmentTypes.generated.h"

class UAnimSequence;
class UAttachmentDefinition;

// 부착 슬롯. 게임 전체에 하나의 목록이고, 총마다 쓸 수 있는 슬롯만 켠다. 총 메시 소켓 이름은 Socket_<슬롯 이름>
UENUM(BlueprintType)
enum class EHuntAttachmentSlot : uint8
{
	Optic,
	Muzzle,
	Underbarrel,
	Magazine,
	Stock,
	Tactical,
	// 몸체 부품. 핸드가드가 Underbarrel·Tactical 슬롯을 연다(UHandguardAttachmentDefinition)
	Handguard
};

// 부착 슬롯 하나의 설정. 총(UWeaponDefinition)과 핸드가드(UHandguardAttachmentDefinition)가 같이 쓴다
USTRUCT(BlueprintType)
struct FHuntWeaponSlot
{
	GENERATED_BODY()

	// 부착물을 달면 숨길 기본 부품 뼈(예: 탄창 b_gun_mag). 없으면 비워 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName HiddenBone;

	// 총이 생성될 때 이 슬롯에 다는 부착물(Attachments 목록에 없어도 된다). 비우면 빈 슬롯
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAttachmentDefinition> DefaultAttachment;

	// 이 슬롯에 달 수 있는 부착물. 목록에 없는 부착물은 붙지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TObjectPtr<UAttachmentDefinition>> Attachments;
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
