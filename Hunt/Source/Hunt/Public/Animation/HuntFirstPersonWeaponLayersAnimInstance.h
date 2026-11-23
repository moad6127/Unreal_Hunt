#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon/Attachment/AttachmentTypes.h"
#include "HuntFirstPersonWeaponLayersAnimInstance.generated.h"

class UHuntFirstPersonAnimInstance;

/**
 * 1인칭 총 애님 레이어 ABP(ALI_HuntWeaponFP 구현)의 부모. 3인칭 UHuntWeaponLayersAnimInstance와 같은 방식으로
 * 총 DA(FirstPersonAnimLayers)가 정한 계열 레이어를 1인칭 팔 메시에 연결한다
 * 메인 1인칭 ABP에서 옮겨 온 노드가 같은 변수 이름으로 읽도록 메인 값을 복사하고, 그립 포즈를 고른다
 */
UCLASS()
class HUNT_API UHuntFirstPersonWeaponLayersAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 손잡이가 없을 때의 계열 그립 포즈. 자식 레이어 ABP 기본값에서 정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FHuntGripPoses DefaultGripPoses;

	// 지금 쓸 그립 포즈: 손잡이 부착물이 있으면 그 포즈, 없으면 DefaultGripPoses
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FHuntGripPoses GripPoses;

	// 메인 1인칭 ABP(UHuntFirstPersonAnimInstance) 값 복사. 뜻은 메인의 같은 이름 변수 설명을 본다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float GroundSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	bool bShouldMove = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float WalkSwayTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float SprintSwayTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main")
	float AimAlpha = 0.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UHuntFirstPersonAnimInstance> Main;
};
