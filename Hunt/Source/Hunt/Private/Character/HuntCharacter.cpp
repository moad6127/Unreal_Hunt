// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HuntCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/WeaponDefinition.h"

AHuntCharacter::AHuntCharacter()
{
	// 카메라가 머리를 따라가도록 매 프레임 갱신. 애니 포즈가 끝난 뒤의 머리 위치를 쓰기 위해 PostPhysics
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	SetupBodyMesh();
	CreateFirstPersonCamera();
	CreateFirstPersonArms();
	CreateFirstPersonLegs();
	SetupMovement();
}

void AHuntCharacter::SetupBodyMesh()
{
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	// 전신은 다른 사람만 본다. WorldSpaceRepresentation이 내 화면에서 숨기고 그림자만 남긴다(VSM에서도 동작, 5.8 1인칭 템플릿 방식)
	// 안 보여도 머리 뼈(카메라)와 하체가 쓰므로 포즈는 계속 갱신
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AHuntCharacter::CreateFirstPersonCamera()
{
	// 카메라는 캡슐에 붙이고 위치만 Tick에서 머리를 부드럽게 따라가게 한다 (배그식 트루 1인칭)
	// 회전은 컨트롤 회전을 그대로 써서 머리 뼈 회전 흔들림은 화면에 실리지 않는다
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
}

void AHuntCharacter::CreateFirstPersonArms()
{
	// 1인칭 팔: 카메라에 붙어 시선과 함께 움직인다. 애니메이션의 메시 공간 = 카메라 공간이고 +Y가 정면이라 Yaw -90만 준다
	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHuntCharacter::CreateFirstPersonLegs()
{
	// 하체: 전신과 같은 자리에서 전신 포즈를 그대로 쓴다
	FirstPersonLegs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonLegs"));
	FirstPersonLegs->SetupAttachment(GetMesh());
	FirstPersonLegs->SetOnlyOwnerSee(true);
	FirstPersonLegs->SetCastShadow(false);
	FirstPersonLegs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHuntCharacter::SetupMovement()
{
	// 가감속을 완만하게(엔진 기본 2048/2048/8): 방향을 뒤집을 때 속도·방향이 한순간에 바뀌지 않아 이동 애니메이션 전환이 부드럽다
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxAcceleration = 1200.f;
	Movement->BrakingDecelerationWalking = 500.f;
	Movement->GroundFriction = 4.f;
	// 키를 뗐을 때의 제동(엔진 기본 마찰 ×2)을 줄여 한 번에 서지 않고 조깅 약 0.4초, 걷기 약 0.25초에 걸쳐 멈춘다
	Movement->BrakingFrictionFactor = 0.5f;
}

void AHuntCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기본 상태는 걷기 (BP에서 WalkSpeed를 바꿔도 반영되도록 BeginPlay에서 설정)
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 메시 애님(병렬 평가)은 TG_PostPhysics까지 늦게 끝날 수 있어, 같은 그룹인 Tick이 이전 프레임 포즈의 머리·가늠자를 읽으면 화면이 튄다
	AddTickPrerequisiteComponent(GetMesh());

	// 하체 메시는 뼈가 전신보다 적어도(보정 뼈 없음) 이름으로 매칭된다
	FirstPersonLegs->SetLeaderPoseComponent(GetMesh());

	HeadCameraLocation = FirstPersonCamera->GetRelativeLocation();

	ArmsBaseTransform = FirstPersonArms->GetRelativeTransform();
	// 조준 정렬에서 팔 포즈(가늠자 위치)를 이번 프레임 것으로 읽기 위해
	AddTickPrerequisiteComponent(FirstPersonArms);
	DefaultFOV = FirstPersonCamera->FieldOfView;

	if (DefaultWeapon)
	{
		EquipDefaultWeapon();
	}
}

void AHuntCharacter::EquipDefaultWeapon()
{
	EquippedWeapon = SpawnWeapon();
	AttachFirstPersonWeapon();

	BodyWeapon = SpawnWeapon();
	AttachBodyWeapon();

	UpdateWeaponVisibility();
}

AHuntWeapon* AHuntCharacter::SpawnWeapon()
{
	// shortcut: 서버·클라이언트가 각자 로컬로 스폰한다(복제 안 함), M1-④ 멀티 점검 때 서버 스폰 + 복제로 교체
	AHuntWeapon* Weapon = GetWorld()->SpawnActorDeferred<AHuntWeapon>(AHuntWeapon::StaticClass(), FTransform::Identity, this, this);
	Weapon->Definition = DefaultWeapon; // OnConstruction(FinishSpawning)에서 메시가 적용되도록 먼저 설정
	Weapon->FinishSpawning(FTransform::Identity);
	return Weapon;
}

void AHuntCharacter::AttachFirstPersonWeapon()
{
	EquippedWeapon->GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	EquippedWeapon->AttachToComponent(FirstPersonArms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FirstPersonWeaponBone);
	EquippedWeapon->GetRootComponent()->SetRelativeLocationAndRotation(FirstPersonWeaponOffset, FRotator::ZeroRotator);
}

void AHuntCharacter::AttachBodyWeapon()
{
	// 전신 메시 설정을 따른다: 기본은 내 화면에서 숨기고 그림자만 남김, 3인칭 테스트 BP(None)에서는 보임
	BodyWeapon->GetMesh()->SetFirstPersonPrimitiveType(GetMesh()->FirstPersonPrimitiveType);
	BodyWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
}

void AHuntCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// BeginPlay 뒤에 빙의되면 그때 내 캐릭터인지 알 수 있다
	UpdateWeaponVisibility();
}

void AHuntCharacter::UpdateWeaponVisibility()
{
	if (!EquippedWeapon)
	{
		return;
	}

	// 1인칭 팔의 총은 내 캐릭터에서만 보인다. 다른 사람 캐릭터는 전신 손의 총(BodyWeapon)만 보인다
	// 팔을 숨기면(3인칭 테스트 BP) 붙어 있는 총 액터는 같이 숨겨지지 않으므로 따로 숨긴다
	EquippedWeapon->SetActorHiddenInGame(!IsLocallyControlled() || FirstPersonArms->bHiddenInGame);
}

const UWeaponDefinition* AHuntCharacter::GetWeaponDefinition() const
{
	return EquippedWeapon ? EquippedWeapon->Definition.Get() : nullptr;
}

void AHuntCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovementSpeed(DeltaSeconds);

	// 다른 플레이어의 캐릭터는 내 화면 카메라가 아니므로 갱신하지 않는다
	if (!IsLocallyControlled())
	{
		return;
	}

	UpdateHeadCameraLocation(DeltaSeconds);
	UpdateAimAlpha(DeltaSeconds);
	ApplyRecoil(DeltaSeconds);

	const float AimBlend = GetAimBlend();
	UpdateCameraView(AimBlend);
	UpdateWeaponSway(DeltaSeconds);
	UpdateWeaponKick(DeltaSeconds);
	UpdateFirstPersonArms(AimBlend);
}

void AHuntCharacter::UpdateMovementSpeed(float DeltaSeconds)
{
	// 최대 속도 = 걷기/달리기 속도 × 입력 방향 비율(정면 1 → 옆 StrafeSpeedScale → 뒤 BackwardSpeedScale)
	// 넘친 속도는 이동 컴포넌트가 감속으로 줄인다. 입력이 없을 때는 직전 값을 유지한다
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector InputDirection = Movement->GetCurrentAcceleration().GetSafeNormal2D();
	if (InputDirection.IsNearlyZero())
	{
		// 멈추는 중: 실제 속도가 걷기와 조깅의 중간 아래로 내려가면 걷기 걸음으로 감속한다
		bRunGait = bRunGait && GetVelocity().Size2D() > (WalkSpeed + RunSpeed) * 0.5f;
		return;
	}

	const float Angle = GetInputAngle(InputDirection);
	const float DirectionScale = GetDirectionSpeedScale(Angle);
	ApproachMaxWalkSpeed(GetBaseSpeed(Angle) * DirectionScale, DeltaSeconds);

	// 최대 속도가 걷기와 조깅의 중간을 넘으면 조깅 걸음. 감속 중에는 최대 속도가 천천히 내려오므로 중간쯤에서 걷기로 바뀐다
	bRunGait = Movement->MaxWalkSpeed > (WalkSpeed + RunSpeed) * 0.5f * DirectionScale;
}

float AHuntCharacter::GetInputAngle(const FVector& InputDirection) const
{
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), InputDirection), -1.f, 1.f)));
}

float AHuntCharacter::GetDirectionSpeedScale(float Angle) const
{
	return Angle <= 90.f
		? FMath::Lerp(1.f, StrafeSpeedScale, Angle / 90.f)
		: FMath::Lerp(StrafeSpeedScale, BackwardSpeedScale, (Angle - 90.f) / 90.f);
}

float AHuntCharacter::GetBaseSpeed(float Angle) const
{
	// 조준 중에는 걷기만(걷기 애니메이션 샘플 속도보다 느리면 제자리 걸음처럼 보인다), 질주는 정면 쪽으로 갈 때만
	if (bWantsToAim)
	{
		return WalkSpeed;
	}
	if (bWantsToSprint && Angle <= SprintMaxAngle)
	{
		return SprintSpeed;
	}
	return bRunning ? RunSpeed : WalkSpeed;
}

void AHuntCharacter::ApproachMaxWalkSpeed(float TargetSpeed, float DeltaSeconds)
{
	// 올릴 때는 가속(MaxAcceleration)이 이미 완만하게 하므로 바로, 내릴 때만 SpeedDownRate로 천천히
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = TargetSpeed >= Movement->MaxWalkSpeed
		? TargetSpeed
		: FMath::FInterpConstantTo(Movement->MaxWalkSpeed, TargetSpeed, DeltaSeconds, SpeedDownRate);
}

void AHuntCharacter::UpdateHeadCameraLocation(float DeltaSeconds)
{
	// 목표 = 머리 뼈 위치 + 시선 방향으로 얼굴 앞까지 (캡슐 기준 좌표로 변환)
	const FTransform& CapsuleTransform = GetCapsuleComponent()->GetComponentTransform();
	const FVector AimDirection = GetControlRotation().Vector();
	const FVector HeadWorld = GetMesh()->GetSocketLocation(CameraHeadBone) + AimDirection * CameraForwardOffset;
	const FVector Target = CapsuleTransform.InverseTransformPosition(HeadWorld);

	// 보간으로 걷기·달리기의 머리 흔들림을 줄인다 (0이면 즉시 따라감)
	HeadCameraLocation = CameraFollowSpeed > 0.f ? FMath::VInterpTo(HeadCameraLocation, Target, DeltaSeconds, CameraFollowSpeed) : Target;
}

void AHuntCharacter::UpdateAimAlpha(float DeltaSeconds)
{
	// 조준 진행도: 총의 조준 시간 동안 일정하게 0↔1. 공중에서는 조준이 풀리고, 버튼을 누르고 있으면 착지 후 다시 조준한다
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	const float AimTarget = (bWantsToAim && Definition && !GetCharacterMovement()->IsFalling()) ? 1.f : 0.f;
	const float AimSpeed = Definition ? 1.f / Definition->AimDownSightsTime : 10.f;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, AimTarget, DeltaSeconds, AimSpeed);
}

void AHuntCharacter::ApplyRecoil(float DeltaSeconds)
{
	if (!Controller)
	{
		return;
	}

	// 반동: 남은 반동을 RecoilSmoothTime에 걸쳐 조준선에 더하고, 다 적용됐고 연사 중이 아니면 되돌릴 몫을 천천히 뺀다
	const FRotator KickStep = ConsumePendingRecoil(DeltaSeconds);
	const FRotator RecoverStep = ConsumeRecoilRecovery(DeltaSeconds);
	if (!KickStep.IsZero() || !RecoverStep.IsZero())
	{
		Controller->SetControlRotation(Controller->GetControlRotation() + KickStep - RecoverStep);
	}
}

FRotator AHuntCharacter::ConsumePendingRecoil(float DeltaSeconds)
{
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	const float SmoothTime = Definition ? Definition->RecoilSmoothTime : 0.f;
	const FRotator KickStep = SmoothTime > 0.f ? PendingRecoil * FMath::Min(1.f, DeltaSeconds / SmoothTime) : PendingRecoil;
	PendingRecoil -= KickStep;
	return KickStep;
}

FRotator AHuntCharacter::ConsumeRecoilRecovery(float DeltaSeconds)
{
	if (!PendingRecoil.IsNearlyZero(0.01f) || (EquippedWeapon && EquippedWeapon->IsAutoFiring()))
	{
		return FRotator::ZeroRotator;
	}

	const FRotator RecoverStep = RecoilToRecover * FMath::Min(1.f, DeltaSeconds * RecoilRecoverySpeed);
	RecoilToRecover -= RecoverStep;
	return RecoverStep;
}

void AHuntCharacter::UpdateCameraView(float AimBlend)
{
	FirstPersonCamera->SetRelativeLocation(HeadCameraLocation);
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(DefaultFOV, AimFOV, AimBlend));
}

void AHuntCharacter::UpdateWeaponSway(float DeltaSeconds)
{
	// 무기 흔들림: 시선을 돌린 반대 방향으로 총을 기울였다가 보간으로 제자리로 돌린다
	const FRotator ControlRotation = GetControlRotation();
	if (DeltaSeconds > 0.f)
	{
		const FRotator LookSpeed = (ControlRotation - PrevControlRotation).GetNormalized() * (1.f / DeltaSeconds);
		const float TargetPitch = FMath::Clamp(-LookSpeed.Pitch * SwayAmount, -MaxSwayAngle, MaxSwayAngle);
		const float TargetYaw = FMath::Clamp(-LookSpeed.Yaw * SwayAmount, -MaxSwayAngle, MaxSwayAngle);
		// 좌우로 돌릴 때 살짝 기울여 무게감을 준다
		WeaponSway = FMath::RInterpTo(WeaponSway, FRotator(TargetPitch, TargetYaw, TargetYaw * 0.5f), DeltaSeconds, SwayInterpSpeed);
	}
	PrevControlRotation = ControlRotation;
}

void AHuntCharacter::UpdateWeaponKick(float DeltaSeconds)
{
	// 목표는 KickRecoverySpeed로 0으로 돌아가고, 현재 값은 KickSnapSpeed로 목표를 빠르게 따라간다
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	const float KickRecoverySpeed = Definition ? Definition->KickRecoverySpeed : 0.f;
	KickTargetLocation = FMath::VInterpTo(KickTargetLocation, FVector::ZeroVector, DeltaSeconds, KickRecoverySpeed);
	KickTargetRotation = FMath::RInterpTo(KickTargetRotation, FRotator::ZeroRotator, DeltaSeconds, KickRecoverySpeed);
	KickLocation = FMath::VInterpTo(KickLocation, KickTargetLocation, DeltaSeconds, KickSnapSpeed);
	KickRotation = FMath::RInterpTo(KickRotation, KickTargetRotation, DeltaSeconds, KickSnapSpeed);
}

void AHuntCharacter::UpdateFirstPersonArms(float AimBlend)
{
	FTransform ArmsToCamera = GetSightAlignedArmsTransform(AimBlend);

	// 총(ik_hand_gun)을 중심으로 돌려 총이 화면을 가로지르지 않고 제자리에서 기울게 한다. 조준 정렬 뒤에 적용
	const FVector Pivot = ArmsWorldToCamera(ArmsToCamera, FirstPersonArms->GetSocketLocation(FirstPersonWeaponBone));
	const FRotator Sway = WeaponSway * FMath::Lerp(1.f, AimSwayScale, AimBlend);

	// 총 킥: 조준 정렬 뒤에 더해야 정렬이 킥을 지우지 않는다
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	const float KickScale = Definition ? FMath::Lerp(1.f, Definition->AimKickScale, AimBlend) : 1.f;

	ArmsToCamera = ArmsToCamera * FTransform(-Pivot) * FTransform(Sway + KickRotation * KickScale) * FTransform(Pivot);
	ArmsToCamera.AddToTranslation(KickLocation * KickScale);
	FirstPersonArms->SetRelativeTransform(ArmsToCamera);
}

FTransform AHuntCharacter::GetSightAlignedArmsTransform(float AimBlend) const
{
	// 조준 정렬: 팔 자세는 애니메이션 그대로 두고 팔 전체를 옮겨 가늠자를 시선 축(카메라 X축) 위에 둔다
	// 가늠자의 팔 기준 위치는 팔 위치와 무관해서 옮겨도 되먹임이 없다
	FTransform ArmsToCamera = ArmsBaseTransform;
	const USkeletalMeshComponent* WeaponMesh = EquippedWeapon ? EquippedWeapon->GetMesh() : nullptr;
	if (WeaponMesh && WeaponMesh->DoesSocketExist(SightSocketName) && WeaponMesh->GetAttachParent() == FirstPersonArms)
	{
		const FVector SightInCamera = ArmsWorldToCamera(ArmsToCamera, WeaponMesh->GetSocketLocation(SightSocketName));
		ArmsToCamera.AddToTranslation(-FVector(0.f, SightInCamera.Y, SightInCamera.Z) * AimBlend);
	}
	return ArmsToCamera;
}

FVector AHuntCharacter::ArmsWorldToCamera(const FTransform& ArmsToCamera, const FVector& WorldLocation) const
{
	// 월드 위치를 현재 팔 기준으로 바꾼 뒤, 이번 프레임에 둘 팔 위치(ArmsToCamera)로 카메라 기준 위치를 구한다
	return ArmsToCamera.TransformPosition(FirstPersonArms->GetComponentTransform().InverseTransformPosition(WorldLocation));
}


void AHuntCharacter::AddRecoil(const FRotator& Kick)
{
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	PendingRecoil += Kick;
	RecoilToRecover += Kick * (Definition ? Definition->RecoilRecovery : 0.f);
}

void AHuntCharacter::AddWeaponKick()
{
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	if (!Definition)
	{
		return;
	}

	// 카메라 기준: -X = 뒤(얼굴 쪽), Pitch + = 총구 위
	KickTargetLocation.X -= Definition->KickBack;
	KickTargetRotation += FRotator(Definition->KickPitch, FMath::FRandRange(-Definition->KickYaw, Definition->KickYaw), 0.f);
}

void AHuntCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHuntCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHuntCharacter::Look);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &AHuntCharacter::ToggleRun);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AHuntCharacter::StartSprint);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHuntCharacter::StopSprint);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AHuntCharacter::StartFire);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AHuntCharacter::StopFire);
	EnhancedInput->BindAction(AimAction, ETriggerEvent::Started, this, &AHuntCharacter::StartAim);
	EnhancedInput->BindAction(AimAction, ETriggerEvent::Completed, this, &AHuntCharacter::StopAim);
}

void AHuntCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Input.X);
}

void AHuntCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AHuntCharacter::ToggleRun()
{
	bRunning = !bRunning;
}

void AHuntCharacter::StartSprint()
{
	bWantsToSprint = true;
}

void AHuntCharacter::StopSprint()
{
	bWantsToSprint = false;
}

void AHuntCharacter::StartFire()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StartFire();
	}
}

void AHuntCharacter::StopFire()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void AHuntCharacter::StartAim()
{
	bWantsToAim = true;
}

void AHuntCharacter::StopAim()
{
	bWantsToAim = false;
}
