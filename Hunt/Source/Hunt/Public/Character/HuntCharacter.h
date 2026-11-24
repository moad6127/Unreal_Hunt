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
	float GetStopBrakeSpeed() const;

	// 지금 쏘면 탄이 퍼지는 원뿔의 반각(도). 발사와 크로스헤어가 같이 쓴다
	float GetCurrentSpread() const;

	// 벽에 막혀 총을 내린 정도(0 = 평소, 1 = 다 내림). 애님은 내린 자세를 이만큼 섞고, 총은 이만큼 총구 방향으로 쏜다
	float GetWallBlockAlpha() const { return WallBlockAlpha; }

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

	// 조준 시 시야각(가늠쇠 기준). 배율 있는 조준경을 달면 조준경 배율로 정한다(GetAimFOV)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float AimFOV = 70.f;

	// 조준경 렌즈 마스크 MPC. 벡터 ScopeLens = (접안렌즈 중심 X 오른쪽, Y 위, 반지름, 조준 정도). 위치·반지름은 화면 중앙 원점, 화면 세로 절반 = 1
	// 조준경 머티리얼이 이 원 안을 잘라내고, 카메라 포스트프로세스가 레티클을 그린다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	TObjectPtr<class UMaterialParameterCollection> ScopeLensParameters;

	// 레티클 포스트프로세스 머티리얼(텍스처 파라미터 ReticleTexture). 시작할 때 카메라에 인스턴스로 붙이고 조준경의 Reticle을 넣는다
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	TObjectPtr<class UMaterialInterface> ScopeReticleMaterial;

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

	// BeginPlay에서 이 총을 스폰해 손에 든다. 비워두면 맨손
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> DefaultWeapon;

	// 테스트용: false면 벽 앞에서도 총을 내리지 않는다. PIE 중 인스턴스에서도 바꿀 수 있다
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bLowerWeaponNearWall = true;

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
	// 총 DA의 계열 애님 레이어를 전신·1인칭 팔 애님에 연결한다
	void LinkWeaponAnimLayers();

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
	// 카메라가 벽에 들어가면 벽 앞에 세우고 캐릭터를 뒤로 걷게 한다
	void UpdateCameraWall();
	void UpdateAimAlpha(float DeltaSeconds);
	void UpdateCameraView(float AimBlend);
	// 장착된 조준경 (없거나 배율 없는 부착물이면 nullptr)
	const class UOpticAttachmentDefinition* GetEquippedOptic() const;
	// 조준이 끝났을 때의 시야각. 조준경이 있으면 그 배율, 없으면 AimFOV
	float GetAimFOV() const;
	// 조준이 끝났을 때 총을 그리는 시야각. 조준경이 있으면 그 조준경 값, 없으면 AimFOV(세상과 같음)
	float GetAimFirstPersonFOV() const;

	// 반동 (조준선). 이번 프레임에 적용할 몫을 꺼내고 남은 양을 줄인다
	void ApplyRecoil(float DeltaSeconds);
	FRotator ConsumePendingRecoil(float DeltaSeconds);
	FRotator ConsumeRecoilRecovery(float DeltaSeconds);

	// 1인칭 팔 (흔들림·킥·조준 정렬)
	void UpdateWeaponSway(float DeltaSeconds);
	void UpdateWallBlock(float DeltaSeconds);
	// 총 자세 전환(준비 유지 시간, 내린 정도)
	void UpdateStance(float DeltaSeconds);
	// 내린 채 누른 발사를 총을 다 든 뒤에 쏜다
	void FireAfterRaise();
	// 공중 탄퍼짐 비율
	void UpdateAirSpread(float DeltaSeconds);
	void UpdateWeaponKick(float DeltaSeconds);
	void UpdateFirstPersonArms(float AimBlend);
	// 접안렌즈를 1인칭 FOV로 화면에 투영해 ScopeLensParameters에 넣는다. 팔·총 위치가 정해진 뒤에 호출
	void UpdateScopeLensMask(float AimBlend);
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

	// 카메라가 닿은 벽의 수평 법선(벽 바깥쪽). 닿지 않으면 0. 이동 입력에서 벽 쪽 성분을 막는 데 쓴다
	FVector CameraWallNormal = FVector::ZeroVector;

	// 카메라가 벽에 들어가 뒤로 밀어내는 중인지. 벽에서 CameraWallMargin만큼 떨어지면 끝난다
	bool bCameraWallPush = false;

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

	// 카메라 포스트프로세스에 붙인 레티클 머티리얼 인스턴스와 지금 넣어 둔 레티클
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> ScopeReticleInstance;
	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> CurrentReticle;

	// 총 킥(카메라 기준). 목표는 매 발 더해지고 0으로 돌아가며, 현재 값이 목표를 빠르게 따라간다
	FVector KickTargetLocation = FVector::ZeroVector;
	FRotator KickTargetRotation = FRotator::ZeroRotator;
	FVector KickLocation = FVector::ZeroVector;
	FRotator KickRotation = FRotator::ZeroRotator;

	// 현재 무기 흔들림(카메라 기준 회전)과 회전 속도 계산용 직전 컨트롤 회전
	FRotator WeaponSway = FRotator::ZeroRotator;
	FRotator PrevControlRotation = FRotator::ZeroRotator;

	float DefaultFOV = 90.f;
	// BP 카메라의 First Person Scale(벽 뚫림 방지용). 조준하면 1로 보간한다
	float DefaultFirstPersonScale = 1.f;

	// 아직 조준선에 적용하지 않은 반동과, 사격을 멈추면 되돌릴 반동
	FRotator PendingRecoil = FRotator::ZeroRotator;
	FRotator RecoilToRecover = FRotator::ZeroRotator;
};
