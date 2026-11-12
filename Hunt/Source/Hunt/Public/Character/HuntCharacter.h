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

// 총 자세. Relaxed = 총 내림(기본), Ready = 허리 사격 자세, Aiming = 조준
enum class EHuntWeaponStance : uint8
{
	Relaxed,
	Ready,
	Aiming,
};

UCLASS()
class HUNT_API AHuntCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHuntCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	AHuntWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	AHuntWeapon* GetBodyWeapon() const { return BodyWeapon; }

	// 조준 블렌드(0 = 지향사격, 1 = 완전 조준). 시작·끝이 부드럽게 감속되는 값이라 카메라와 애님이 같이 쓴다
	float GetAimBlend() const { return FMath::SmoothStep(0.f, 1.f, AimAlpha); }

	// 걸음 상태(false = 걷기, true = 조깅·질주). 이동 애니메이션을 섞지 않고 이 값으로 바꿔 끼운다
	bool IsRunGait() const { return bRunGait; }

	// 키를 뗀 뒤 3인칭 애님이 ABP Stop 상태로 가는 속도
	float GetStopBrakeSpeed() const { return StopBrakeSpeed; }

	// 지금 쏘면 탄이 퍼지는 원뿔의 반각(도). 발사와 크로스헤어가 같이 쓴다
	float GetCurrentSpread() const;

	// 벽에 막혀 총을 내린 정도(0 = 평소, 1 = 다 내림). 애님은 내린 자세를 이만큼 섞고, 총은 이만큼 총구 방향으로 쏜다
	float GetWallBlockAlpha() const { return WallBlockAlpha; }

	// 총을 내린 정도(0~1) = 벽 앞 내림과 대기(Relaxed) 자세 중 큰 값. 내린 자세 애디티브, 크로스헤어, 에임 오프셋, 발사 방향이 쓴다
	float GetWeaponLowerAlpha() const { return FMath::Max(WallBlockAlpha, RelaxedAlpha); }

	// 대기(Relaxed) 자세 블렌드(0 = 준비, 1 = 대기). 시작·끝이 부드럽게 감속된다. 1인칭 대기 포즈가 쓴다
	float GetRelaxedBlend() const { return FMath::SmoothStep(0.f, 1.f, RelaxedAlpha); }

	// 발사 반동. Kick만큼 조준선을 RecoilSmoothTime 동안 올리고, 사격을 멈추면 RecoilRecovery 비율만큼 되돌린다
	void AddRecoil(const FRotator& Kick);

	// 장착한 총의 킥(화면에 보이는 총 반동)을 한 발 더한다. 연사하면 쌓이고 KickRecoverySpeed로 돌아온다
	void AddWeaponKick();


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

	// 머리 뼈에서 시선 방향으로 얼마나 앞에 둘지(cm). 아래를 볼 때 하체 메시의 허리 잘린 면이 보이면 늘린다
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraForwardOffset = 15.f;

	// 머리 위치를 따라가는 속도. 낮을수록 흔들림이 줄고 반응이 느려진다 (0 = 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 15.f;

	// 조준 시 시야각(가늠쇠 기준. 스코프 배율은 M3에서)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float AimFOV = 70.f;

	// 총 킥이 목표 위치로 튀어 나가는 속도. 높을수록 한 발이 날카롭게 튄다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Kick", meta = (ClampMin = "0.0"))
	float KickSnapSpeed = 30.f;

	// 무기 흔들림: 시선 회전 속도(도/초)에 곱해 총을 반대로 기울이는 비율
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Sway", meta = (ClampMin = "0.0"))
	float SwayAmount = 0.02f;

	// 흔들림 최대 각도(도)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Sway", meta = (ClampMin = "0.0"))
	float MaxSwayAngle = 3.f;

	// 흔들림이 목표를 따라가고 제자리로 돌아오는 속도. 낮을수록 총이 늦게 따라온다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Sway", meta = (ClampMin = "0.0"))
	float SwayInterpSpeed = 8.f;

	// 완전 조준 시 흔들림 비율 (가늠자가 화면 중앙에서 크게 벗어나지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Sway", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimSwayScale = 0.3f;

	// 사격을 멈춘 뒤 반동이 원래 조준점으로 돌아오는 속도. 높을수록 빨리 돌아온다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Recoil", meta = (ClampMin = "0.0"))
	float RecoilRecoverySpeed = 6.f;

	// 점프·착지 때 탄퍼짐이 공중 값으로(또는 땅 값으로) 바뀌는 속도. 크로스헤어가 튀지 않게 보간한다
	UPROPERTY(EditDefaultsOnly, Category = "Spread", meta = (ClampMin = "0.0"))
	float AirSpreadInterpSpeed = 10.f;

	// 카메라 정면으로 벽을 찾는 거리(cm). 벽이 이보다 가까워지면 총을 내리기 시작한다
	UPROPERTY(EditDefaultsOnly, Category = "WallBlock", meta = (ClampMin = "0.0"))
	float WallBlockStartDistance = 75.f;

	// 벽이 이 거리(cm) 이하면 총을 다 내린다
	UPROPERTY(EditDefaultsOnly, Category = "WallBlock", meta = (ClampMin = "0.0"))
	float WallBlockFullDistance = 45.f;

	// 총을 내리고 올리는 속도
	UPROPERTY(EditDefaultsOnly, Category = "WallBlock", meta = (ClampMin = "0.0"))
	float WallBlockInterpSpeed = 10.f;

	// 벽 감지 시작점: 카메라에서 총이 놓이는 쪽(시선 기준 X 앞, Y 오른쪽, Z 위)으로 옮긴 위치(cm)
	// 실제 총 위치를 쓰면 총을 내릴 때마다 벽에서 멀어져 다시 올라가며 떨리므로 카메라 기준 고정값을 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "WallBlock")
	FVector WallBlockSweepOffset = FVector(0.f, 15.f, -10.f);

	// 벽 감지 구의 반경(cm). 시선이 벽과 나란해도 총열이 지나는 공간에 벽이 있으면 걸리도록
	UPROPERTY(EditDefaultsOnly, Category = "WallBlock", meta = (ClampMin = "0.0"))
	float WallBlockSweepRadius = 10.f;

	// 조준을 풀거나 쏜 뒤 준비 자세(총구 앞)를 유지하는 시간(초). 지나면 총을 내린다
	UPROPERTY(EditDefaultsOnly, Category = "Stance", meta = (ClampMin = "0.0"))
	float ReadyHoldTime = 2.f;

	// 내린 총을 드는(또는 내리는) 시간(초). 내린 채 쏘면 이만큼 든 뒤 발사한다
	UPROPERTY(EditDefaultsOnly, Category = "Stance", meta = (ClampMin = "0.01"))
	float RaiseTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;

	// 누르고 있는 동안 질주
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 165.f;

	// 조깅(달리기 토글). BS_Hunt_Loco_Fwd의 ALS 달리기 샘플 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 350.f;

	// 질주. BS_Hunt_Loco_Fwd의 ALS 질주 샘플 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 600.f;

	// 입력 방향이 정면에서 이 각도(도) 안일 때만 질주한다. 벗어나면 조깅/걷기 속도로 돌아간다
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float SprintMaxAngle = 50.f;

	// 최대 속도를 낮출 때(질주 → 조깅/걷기) 초당 줄이는 양(cm/s). 한 번에 낮추면 몸이 0.2초 만에 멈춰 이동 애니메이션 블렌드가 따라오지 못한다
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0"))
	float SpeedDownRate = 600.f;

	// 옆·뒤로 갈 때 최대 속도 비율(정면 = 1). 이동 애니메이션 실제 속도(옆 약 0.75, 뒤 약 0.6배)에 맞춰 발 미끄러짐을 줄인다
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float StrafeSpeedScale = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float BackwardSpeedScale = 0.6f;

	// 키를 떼고 이 속도(cm/s) 아래가 되면 3인칭 애님이 ABP Stop 상태로 간다(ALS ShouldMove 값 150). UHuntAnimInstance가 읽는다
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float StopBrakeSpeed = 150.f;

	// BeginPlay에서 이 총을 스폰해 손에 든다. 비워두면 맨손
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> DefaultWeapon;

	// ik_hand_gun 기준 총 위치(회전 없음). AR4 손잡이가 LPSP 손에 오도록 잰 값, 총 메시마다 다르다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector FirstPersonWeaponOffset = FVector(-2.03f, -8.35f, -0.1f);

	// 장착 중인 총(발사용). 1인칭 팔에 붙고 내 화면에만 보인다. 없으면 맨손
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Weapon")
	TObjectPtr<AHuntWeapon> EquippedWeapon;

	// 전신 손에 든 같은 총. 다른 사람에게 보이고 내 화면에는 그림자만 남는다
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Weapon")
	TObjectPtr<AHuntWeapon> BodyWeapon;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleRun();
	void StartSprint();
	void StopSprint();
	void StartFire();
	void StopFire();
	void StartAim();
	void StopAim();
	void UpdateWeaponVisibility();

	// 생성자에서 컴포넌트 구성
	void SetupBodyMesh();
	void CreateFirstPersonCamera();
	void CreateFirstPersonArms();
	void CreateFirstPersonLegs();
	void SetupMovement();

	// 기본 총: 1인칭 팔용(EquippedWeapon)과 전신용(BodyWeapon) 두 개를 스폰해 붙인다
	void EquipDefaultWeapon();
	AHuntWeapon* SpawnWeapon();
	void AttachFirstPersonWeapon();
	void AttachBodyWeapon();

	const UWeaponDefinition* GetWeaponDefinition() const;

	// 이동 속도·걸음 상태
	void UpdateMovementSpeed(float DeltaSeconds);
	float GetInputAngle(const FVector& InputDirection) const;
	float GetDirectionSpeedScale(float Angle) const;
	float GetBaseSpeed(float Angle) const;
	// 이 입력 각도(정면 기준 0~180)로 질주할 수 있는지
	bool CanSprint(float Angle) const;
	void ApproachMaxWalkSpeed(float TargetSpeed, float DeltaSeconds);

	// 카메라·조준
	void UpdateHeadCameraLocation(float DeltaSeconds);
	void UpdateAimAlpha(float DeltaSeconds);
	void UpdateCameraView(float AimBlend);

	// 반동 (조준선). 이번 프레임에 적용할 몫을 꺼내고 남은 양을 줄인다
	void ApplyRecoil(float DeltaSeconds);
	FRotator ConsumePendingRecoil(float DeltaSeconds);
	FRotator ConsumeRecoilRecovery(float DeltaSeconds);

	// 1인칭 팔 (흔들림·킥·조준 정렬)
	void UpdateWeaponSway(float DeltaSeconds);
	void UpdateWallBlock(float DeltaSeconds);
	// 총 자세 전환(준비 유지 시간, 내린 정도, 들고 나서 발사)
	void UpdateStance(float DeltaSeconds);
	void UpdateWeaponKick(float DeltaSeconds);
	void UpdateFirstPersonArms(float AimBlend);
	FTransform GetSightAlignedArmsTransform(float AimBlend) const;
	FVector ArmsWorldToCamera(const FTransform& ArmsToCamera, const FVector& WorldLocation) const;


	// 달리기 토글 상태. 실제 최대 속도는 Tick에서 이동 방향 비율을 곱해 정한다
	bool bRunning = false;

	// 질주 키를 누르고 있는지. 조준 중이거나 정면이 아니면 무시한다
	bool bWantsToSprint = false;

	// shortcut: 복제하지 않아 다른 플레이어에게는 걷기 상태로 보인다, 멀티 점검 때 달리기·질주 상태와 함께 복제
	bool bRunGait = false;

	// 조준 진행도(0~1, 선형). 총의 AimDownSightsTime 동안 변한다
	float AimAlpha = 0.f;

	// 공중 탄퍼짐 비율(0 = 땅, 1 = 공중). AirSpreadInterpSpeed로 따라간다
	float AirSpreadAlpha = 0.f;

	// shortcut: 복제하지 않아 다른 플레이어에게는 총을 내린 자세가 안 보인다, 멀티 점검 때 조준 상태와 함께 복제
	float WallBlockAlpha = 0.f;

	// 조준 버튼을 누르고 있으면 Aiming
	// shortcut: 총 자세를 복제하지 않는다(다른 플레이어에게는 대기 자세로 보이고 조준 자세가 안 보임), 멀티 점검 때 복제 추가
	EHuntWeaponStance Stance = EHuntWeaponStance::Relaxed;

	// 대기 자세로 총을 내린 정도(0~1, 선형). RaiseTime 동안 변한다
	float RelaxedAlpha = 1.f;

	// 준비 자세 남은 시간(초)
	float ReadyTimeLeft = 0.f;

	// 발사 버튼을 누르고 있는지 / 총을 다 들면 쏠지
	bool bFireHeld = false;
	bool bFireAfterRaise = false;

	// 머리를 보간으로 따라가는 카메라 위치(캡슐 기준)
	FVector HeadCameraLocation = FVector::ZeroVector;

	// BP에 정한 팔의 카메라 기준 위치. 조준 정렬은 여기서부터 옮긴다
	FTransform ArmsBaseTransform = FTransform::Identity;

	// 총 킥(카메라 기준). 목표는 매 발 더해지고 0으로 돌아가며, 현재 값이 목표를 빠르게 따라간다
	FVector KickTargetLocation = FVector::ZeroVector;
	FRotator KickTargetRotation = FRotator::ZeroRotator;
	FVector KickLocation = FVector::ZeroVector;
	FRotator KickRotation = FRotator::ZeroRotator;

	// 현재 무기 흔들림(카메라 기준 회전)과 회전 속도 계산용 직전 컨트롤 회전
	FRotator WeaponSway = FRotator::ZeroRotator;
	FRotator PrevControlRotation = FRotator::ZeroRotator;

	float DefaultFOV = 90.f;

	// 아직 조준선에 적용하지 않은 반동과, 사격을 멈추면 되돌릴 반동
	FRotator PendingRecoil = FRotator::ZeroRotator;
	FRotator RecoilToRecover = FRotator::ZeroRotator;
};
