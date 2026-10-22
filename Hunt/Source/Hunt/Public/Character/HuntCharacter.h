// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HuntCharacter.generated.h"

class AHuntWeapon;
class UCameraComponent;
class UInputAction;
class UWeaponDefinition;
struct FInputActionValue;

UCLASS()
class HUNT_API AHuntCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHuntCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	AHuntWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	// 조준 블렌드(0 = 지향사격, 1 = 완전 조준). 시작·끝이 부드럽게 감속되는 값이라 카메라와 애님이 같이 쓴다
	float GetAimBlend() const { return FMath::SmoothStep(0.f, 1.f, AimAlpha); }


protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// 내 화면에만 보이는 1인칭 팔(카메라에 붙음). 메시·애님 BP는 BP에서 정한다
	// 1인칭 애니메이션(LPSP 원본 리타깃)은 메시 공간이 곧 카메라 공간이라(머리 = 원점, +Y = 정면) 카메라에 그대로 붙인다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArms;

	// 내 화면에만 보이는 하체. 전신 메시 포즈를 그대로 따라간다(Leader Pose)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FirstPersonLegs;

	// 카메라가 따라갈 뼈
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName CameraHeadBone = TEXT("head");

	// 머리 뼈에서 시선 방향으로 얼마나 앞에 둘지(cm). 아래를 볼 때 하체 메시의 허리 잘린 면이 보이면 늘린다
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraForwardOffset = 15.f;

	// 머리 위치를 따라가는 속도. 낮을수록 흔들림이 줄고 반응이 느려진다 (0 = 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 15.f;

	// 조준 시 화면 중앙에 올 총의 가늠자 소켓. 조준하는 동안 팔 전체를 옮겨 이 소켓을 시선 축 위에 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	FName SightSocketName = TEXT("Socket_Sight");

	// 조준 시 시야각(가늠쇠 기준. 스코프 배율은 M3에서)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float AimFOV = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 600.f;

	// BeginPlay에서 이 총을 스폰해 손에 든다. 비워두면 맨손
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> DefaultWeapon;

	// 다른 사람이 보는 전신 손 소켓
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("HandGrip_R");

	// 1인칭 팔에서 총을 붙일 뼈. LPSP 애니메이션은 총을 ik_hand_gun(SOCKET_Weapon)으로 움직이고 손은 그 총을 잡는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName FirstPersonWeaponBone = TEXT("ik_hand_gun");

	// ik_hand_gun 기준 총 위치(회전 없음). AR4 손잡이가 LPSP 손에 오도록 잰 값, 총 메시마다 다르다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector FirstPersonWeaponOffset = FVector(-2.03f, -8.35f, -0.1f);

	// 장착 중인 총. 없으면 맨손
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Weapon")
	TObjectPtr<AHuntWeapon> EquippedWeapon;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleRun();
	void Fire();
	void StartAim();
	void StopAim();
	void AttachWeaponToHands();

	// shortcut: 조준 상태를 복제하지 않아 다른 플레이어에게는 조준 자세가 안 보인다, 멀티 점검 때 복제 추가
	bool bWantsToAim = false;

	// 조준 진행도(0~1, 선형). 총의 AimDownSightsTime 동안 변한다
	float AimAlpha = 0.f;

	// 머리를 보간으로 따라가는 카메라 위치(캡슐 기준)
	FVector HeadCameraLocation = FVector::ZeroVector;

	// BP에 정한 팔의 카메라 기준 위치. 조준 정렬은 여기서부터 옮긴다
	FTransform ArmsBaseTransform = FTransform::Identity;

	float DefaultFOV = 90.f;
};
