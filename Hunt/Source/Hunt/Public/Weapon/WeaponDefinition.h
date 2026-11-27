// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Weapon/Attachment/AttachmentTypes.h"
#include "WeaponDefinition.generated.h"

class AHuntProjectile;
class UChassisAttachmentDefinition;
class UAnimInstance;
class USkeletalMesh;
class USoundBase;
class UTexture2D;

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

	// 그 슬롯의 필수 부품(없으면 nullptr). 필수 부품이 있는 슬롯은 비울 수 없다
	UAttachmentDefinition* GetRequiredPart(EHuntAttachmentSlot Slot) const;
	bool IsRequiredSlot(EHuntAttachmentSlot Slot) const { return GetRequiredPart(Slot) != nullptr; }

	// 총 바디가 주는 태그(예: Weapon.M4, Weapon.Feature.Rail.Top). 총의 태그 = 이것 + 장착된 부착물의 태그
	UPROPERTY(EditDefaultsOnly, Category = "Attachments")
	FGameplayTagContainer GrantedTags;

	// 필수 부품: 값을 넣으면 그 슬롯은 필수이고 이 부품을 달고 나온다. 선택 부착물보다 먼저 달리고, 바뀌면 선택 부착물을 다시 검사한다
	// 슬롯이 있는지는 소켓이 정한다(총 바디나 필수 부품 메시의 Socket_<슬롯>)
	UPROPERTY(EditDefaultsOnly, Category = "Attachments|Required Parts", meta = (DisplayName = "Chassis"))
	TObjectPtr<UChassisAttachmentDefinition> RequiredChassis;

	UPROPERTY(EditDefaultsOnly, Category = "Attachments|Required Parts", meta = (DisplayName = "Handguard"))
	TObjectPtr<UAttachmentDefinition> RequiredHandguard;

	UPROPERTY(EditDefaultsOnly, Category = "Attachments|Required Parts", meta = (DisplayName = "Magazine"))
	TObjectPtr<UAttachmentDefinition> RequiredMagazine;

	// 총이 생성될 때 추가로 달고 나오는 선택 부착물(조준경·총구·개머리판 등). 필수 부품은 위 Required Parts에 넣는다
	UPROPERTY(EditDefaultsOnly, Category = "Attachments")
	TArray<TObjectPtr<UAttachmentDefinition>> DefaultAttachments;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

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

	// 지향사격으로 멈춰 서 있을 때 탄이 퍼지는 원뿔의 반각(도). 완전 조준이면 0
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread", meta = (ClampMin = "0.0"))
	float HipSpread = 1.f;

	// 지향사격으로 최대 속도로 움직일 때의 반각(도). 속도에 따라 HipSpread와 섞는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread", meta = (ClampMin = "0.0"))
	float MovingSpread = 3.f;

	// 공중(점프·낙하)일 때의 반각(도). 땅에서의 탄퍼짐과 보간으로 섞는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Spread", meta = (ClampMin = "0.0"))
	float AirSpread = 6.f;

	// 크로스헤어 조각. 상하좌우는 탄퍼짐만큼 중앙에서 벌어진다. 비워 둔 조각은 그리지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom;

	// 총 계열 애님 레이어(Lyra 아이템 애님 레이어 방식). 장착할 때 전신(3인칭)·1인칭 팔 메시에 연결한다. 비어 있으면 메인 ABP 그대로
	// 같은 계열 총(권총 손잡이 = M4, 일반 개머리판 = BoltRifle)은 같은 레이어 ABP를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimLayers;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimLayers;
};
