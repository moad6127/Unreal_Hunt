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
};
