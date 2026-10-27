// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDefinition.generated.h"

class AHuntProjectile;
class UAnimSequenceBase;
class USkeletalMesh;
class USoundBase;

UENUM(BlueprintType)
enum class EHuntFireMode : uint8
{
	Single,
	Auto
};

/**
 * 총 본체 데이터. 총 한 종류당 에셋 하나 (DA_AR4 등)
 */
UCLASS()
class HUNT_API UWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> Mesh;

	// 탄속·낙차는 발사체 BP에서 조정한다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AHuntProjectile> ProjectileClass;

	// 시작 발사 모드. 연발은 방아쇠를 누르는 동안 FireInterval마다 쏜다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	EHuntFireMode FireMode = EHuntFireMode::Single;

	// 발사 간격(초)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float FireInterval = 0.5f;

	// 조준(ADS)에 걸리는 시간(초). 짧을수록 조준이 빠르다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0.01"))
	float AimDownSightsTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<USoundBase> FireSound;

	// 한 발에 총(1인칭 팔)이 뒤로 밀리는 거리(cm). 화면에 보이는 반동이고 탄착과는 무관하다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Kick", meta = (ClampMin = "0.0"))
	float KickBack = 3.f;

	// 한 발에 총구가 들리는 각도(도)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Kick", meta = (ClampMin = "0.0"))
	float KickPitch = 2.f;

	// 한 발에 총이 좌우로 틀어질 수 있는 최대 각도(도). 매 발 이 범위 안에서 무작위
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Kick", meta = (ClampMin = "0.0"))
	float KickYaw = 0.8f;

	// 킥이 제자리로 돌아오는 속도. 낮을수록 연사 중 킥이 더 쌓인다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Kick", meta = (ClampMin = "0.0"))
	float KickRecoverySpeed = 12.f;

	// 완전 조준일 때 킥 비율 (지향사격 = 1)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Kick", meta = (ClampMin = "0.0"))
	float AimKickScale = 0.5f;

	// 쏠 때 1인칭 팔에 재생할 애디티브 애니메이션 (지향사격 / 조준)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimSequenceBase> FireAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimSequenceBase> AimFireAnimation;

	// 지향사격일 때 발사 애니메이션 세기 배율(총이 튀는 크기). 완전 조준이면 1
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", meta = (ClampMin = "0.0"))
	float HipFireAnimationScale = 1.5f;

	// 한 발에 조준선이 위로 튀는 각도(도)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0"))
	float VerticalRecoil = 0.6f;

	// 한 발에 좌우로 튈 수 있는 최대 각도(도). 매 발 이 범위 안에서 무작위
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0"))
	float HorizontalRecoil = 0.3f;

	// 방아쇠를 당긴 첫 발의 반동 배율 (단발은 매 발이 첫 발)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0"))
	float FirstShotRecoilMultiplier = 1.5f;

	// 사격을 멈춘 뒤 원래 조준점으로 돌아오는 비율 (0 = 안 돌아옴, 1 = 전부)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoilRecovery = 0.5f;

	// 한 발의 반동이 조준선에 적용되는 시간(초). 0이면 즉시 튄다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0"))
	float RecoilSmoothTime = 0.08f;

	// 지향사격(허리 사격)일 때 조준선 반동 배율. 완전 조준이면 1
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil", meta = (ClampMin = "0.0"))
	float HipFireRecoilMultiplier = 1.5f;
};
