#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HuntWeaponLayersAnimInstance.generated.h"

class UAnimSequence;
class UHuntAnimInstance;

/**
 * 3인칭 총 애님 레이어 ABP(ALI_HuntWeapon3P 구현)의 부모. 총 계열(권총 손잡이, 일반 개머리판)마다 자식 레이어 ABP가 있고
 * 총 DA(ThirdPersonAnimLayers)가 그 클래스를 정하면 캐릭터가 장착할 때 전신 메시에 연결한다(Lyra 아이템 애님 레이어 방식)
 * 메인 ABP에서 옮겨 온 노드가 같은 변수 이름으로 읽도록 메인 값을 복사하고, 계열 기본 포즈와 손잡이 부착물 포즈 중 지금 쓸 것을 고른다
 */
UCLASS()
class HUNT_API UHuntWeaponLayersAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 계열 기본 대기 자세 포즈·앞 달리기 팔. 자식 레이어 ABP 기본값에서 정한다. 손잡이 DA에 3인칭 대기 포즈가 있으면 그것이 먼저다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> DefaultRelaxedPose;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> DefaultRelaxedRunArms;

	// 계열 오른손가락 포즈. 상태 애니메이션 위에 손가락 뼈만 덮는다. 비어 있으면 상태 애니메이션 손가락 그대로
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> DefaultRightHandPose;

	// 지금 쓸 대기 포즈·앞 달리기 팔·오른손가락 포즈
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> RelaxedPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> RelaxedRunArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequence> RightHandGripPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float RightHandGripAlpha = 0.f;

	// 메인 ABP(UHuntAnimInstance) 값 복사. 뜻은 메인의 같은 이름 변수 설명을 본다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float AimYaw = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float AimPitch = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float AimOffsetAlpha = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float LandRecoveryTime = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float TurnTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	TObjectPtr<UAnimSequence> LeftHandGripPose;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float LeftHandGripAlpha = 0.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UHuntAnimInstance> Main;

	void UpdatePoses();
};
