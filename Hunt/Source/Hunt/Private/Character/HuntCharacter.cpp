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

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	// 카메라는 캡슐에 붙이고 위치만 Tick에서 머리를 부드럽게 따라가게 한다 (배그식 트루 1인칭)
	// 회전은 컨트롤 회전을 그대로 써서 머리 뼈 회전 흔들림은 화면에 실리지 않는다
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

}

void AHuntCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기본 상태는 걷기 (BP에서 WalkSpeed를 바꿔도 반영되도록 BeginPlay에서 설정)
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 메시 애님(병렬 평가)은 TG_PostPhysics까지 늦게 끝날 수 있어, 같은 그룹인 Tick이 이전 프레임 포즈의 머리·가늠자를 읽으면 화면이 튄다
	AddTickPrerequisiteComponent(GetMesh());

	HeadCameraLocation = FirstPersonCamera->GetRelativeLocation();
	DefaultFOV = FirstPersonCamera->FieldOfView;

	if (DefaultWeapon)
	{
		// shortcut: 서버·클라이언트가 각자 로컬로 스폰한다(복제 안 함), M1-④ 멀티 점검 때 서버 스폰 + 복제로 교체
		AHuntWeapon* Weapon = GetWorld()->SpawnActorDeferred<AHuntWeapon>(AHuntWeapon::StaticClass(), FTransform::Identity, this, this);
		Weapon->Definition = DefaultWeapon; // OnConstruction(FinishSpawning)에서 메시가 적용되도록 먼저 설정
		Weapon->FinishSpawning(FTransform::Identity);
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		EquippedWeapon = Weapon;
	}
}

void AHuntCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 다른 플레이어의 캐릭터는 내 화면 카메라가 아니므로 갱신하지 않는다
	if (!IsLocallyControlled())
	{
		return;
	}

	// 목표 = 머리 뼈 위치 + 시선 방향으로 얼굴 앞까지 (캡슐 기준 좌표로 변환)
	const FTransform& CapsuleTransform = GetCapsuleComponent()->GetComponentTransform();
	const FVector AimDirection = GetControlRotation().Vector();
	const FVector HeadWorld = GetMesh()->GetSocketLocation(CameraHeadBone) + AimDirection * CameraForwardOffset;
	const FVector Target = CapsuleTransform.InverseTransformPosition(HeadWorld);

	// 보간으로 걷기·달리기의 머리 흔들림을 줄인다 (0이면 즉시 따라감)
	HeadCameraLocation = CameraFollowSpeed > 0.f ? FMath::VInterpTo(HeadCameraLocation, Target, DeltaSeconds, CameraFollowSpeed) : Target;

	// 조준 진행도: 총의 조준 시간 동안 일정하게 0↔1
	const UWeaponDefinition* Definition = EquippedWeapon ? EquippedWeapon->Definition.Get() : nullptr;
	const float AimTarget = (bWantsToAim && Definition) ? 1.f : 0.f;
	const float AimSpeed = Definition ? 1.f / Definition->AimDownSightsTime : 10.f;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, AimTarget, DeltaSeconds, AimSpeed);
	const float AimBlend = GetAimBlend();

	FVector CameraLocation = HeadCameraLocation;
	if (AimBlend > 0.f && EquippedWeapon)
	{
		// 가늠자를 시선 방향 정확히 앞에 두면 가늠자가 화면 정중앙에 온다. 보간으로 살짝 늦게 따라가 움직일 때 총이 화면에서 흔들린다
		const FVector SightEye = EquippedWeapon->GetMesh()->GetSocketLocation(SightSocketName) - AimDirection * SightCameraDistance;
		const FVector AimTargetLocation = CapsuleTransform.InverseTransformPosition(SightEye);
		const bool bJustStartedAim = AimCameraLocation.IsZero();
		AimCameraLocation = (AimCameraFollowSpeed > 0.f && !bJustStartedAim) ? FMath::VInterpTo(AimCameraLocation, AimTargetLocation, DeltaSeconds, AimCameraFollowSpeed) : AimTargetLocation;
		CameraLocation = FMath::Lerp(HeadCameraLocation, AimCameraLocation, AimBlend);
	}
	else
	{
		AimCameraLocation = FVector::ZeroVector;
	}
	FirstPersonCamera->SetRelativeLocation(CameraLocation);
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(DefaultFOV, AimFOV, AimBlend));
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
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AHuntCharacter::Fire);
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
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = (Movement->MaxWalkSpeed == RunSpeed) ? WalkSpeed : RunSpeed;
}

void AHuntCharacter::Fire()
{
	if (EquippedWeapon)
	{
		// 카메라가 보는 방향(컨트롤 회전)으로 쏜다
		EquippedWeapon->Fire(GetControlRotation());
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
