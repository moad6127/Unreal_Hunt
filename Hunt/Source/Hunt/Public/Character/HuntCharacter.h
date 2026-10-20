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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// 카메라가 따라갈 뼈
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName CameraHeadBone = TEXT("head");

	// 머리 뼈에서 시선 방향으로 얼마나 앞에 둘지(cm). 몸이나 머리가 화면에 걸리면 늘린다
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraForwardOffset = 15.f;

	// 머리 위치를 따라가는 속도. 낮을수록 흔들림이 줄고 반응이 느려진다 (0 = 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 15.f;

	// 조준 시 카메라가 맞춰질 총의 가늠자 소켓
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	FName SightSocketName = TEXT("Socket_Sight");

	// 조준 시 가늠자에서 눈까지 거리(cm). 너무 작으면 니어 클립(10cm)에 총이 잘린다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (ClampMin = "0.0"))
	float SightCameraDistance = 15.f;

	// 조준 중 카메라가 가늠자 위치를 따라가는 속도. 낮을수록 걷기·회전 때 총이 화면에서 더 흔들린다 (0 = 가늠자에 고정)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (ClampMin = "0.0"))
	float AimCameraFollowSpeed = 30.f;

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

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("HandGrip_R");

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

	// shortcut: 조준 상태를 복제하지 않아 다른 플레이어에게는 조준 자세가 안 보인다, 멀티 점검 때 복제 추가
	bool bWantsToAim = false;

	// 조준 진행도(0~1, 선형). 총의 AimDownSightsTime 동안 변한다
	float AimAlpha = 0.f;

	// 머리를 보간으로 따라가는 지향사격 카메라 위치(캡슐 기준). 조준 위치와 블렌드한다
	FVector HeadCameraLocation = FVector::ZeroVector;

	// 가늠자 뒤를 보간으로 따라가는 조준 카메라 위치(캡슐 기준). 0이면 조준 시작 전(카메라가 캡슐 중심에 올 일은 없음)
	FVector AimCameraLocation = FVector::ZeroVector;

	float DefaultFOV = 90.f;
};
