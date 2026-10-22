// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntFirstPersonAnimInstance.generated.h"

class AHuntCharacter;

/**
 * 1인칭 팔용. 팔은 카메라에 붙어 시선 회전을 그대로 따라가므로 에임 오프셋·제자리 회전·이동 방향은 필요 없다
 * 조준, 이동 여부, 왼손 IK 목표를 계산한다. 왼손 모양은 그립 포즈 애니메이션에 들어 있다. 블렌드는 ABP에서 구성
 */
UCLASS()
class HUNT_API UHuntFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 걷기/달리기 additive를 고르는 데 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	// 속도와 입력 가속이 모두 있을 때만 true (멈출 때 바로 Idle로)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	// 조준 블렌드(0~1). 지향사격 ↔ 조준 자세 블렌드 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AimAlpha = 0.f;

	// 총이 이 팔에 붙어 있고 1인칭 왼손 소켓이 있으면 true. 왼손 IK의 Alpha로 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bLeftHandIK = false;

	// 왼손 IK 목표: 총의 1인칭 왼손 소켓을 ik_hand_gun 뼈 기준으로 나타낸 값. ABP의 Two Bone IK(Bone Space, ik_hand_gun)에 넣는다
	// 손 모양·팔꿈치는 그립 포즈가 정하고, IK는 총마다 다른 잡는 위치로 손목만 옮긴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector LeftHandIKLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHuntCharacter> Character;
};
