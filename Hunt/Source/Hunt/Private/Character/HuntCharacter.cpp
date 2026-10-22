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

	// 전신은 다른 사람만 본다. WorldSpaceRepresentation이 내 화면에서 숨기고 그림자만 남긴다(VSM에서도 동작, 5.8 1인칭 템플릿 방식)
	// 안 보여도 머리 뼈(카메라)와 하체가 쓰므로 포즈는 계속 갱신
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 1인칭 팔: 카메라에 붙어 시선과 함께 움직인다. 애니메이션의 메시 공간 = 카메라 공간이고 +Y가 정면이라 Yaw -90만 준다
	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 하체: 전신과 같은 자리에서 전신 포즈를 그대로 쓴다
	FirstPersonLegs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonLegs"));
	FirstPersonLegs->SetupAttachment(GetMesh());
	FirstPersonLegs->SetOnlyOwnerSee(true);
	FirstPersonLegs->SetCastShadow(false);
	FirstPersonLegs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		// shortcut: 서버·클라이언트가 각자 로컬로 스폰한다(복제 안 함), M1-④ 멀티 점검 때 서버 스폰 + 복제로 교체
		AHuntWeapon* Weapon = GetWorld()->SpawnActorDeferred<AHuntWeapon>(AHuntWeapon::StaticClass(), FTransform::Identity, this, this);
		Weapon->Definition = DefaultWeapon; // OnConstruction(FinishSpawning)에서 메시가 적용되도록 먼저 설정
		Weapon->FinishSpawning(FTransform::Identity);
		EquippedWeapon = Weapon;
		AttachWeaponToHands();
	}
}

void AHuntCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// BeginPlay 뒤에 빙의되면 그때 내 캐릭터인지 알 수 있다
	AttachWeaponToHands();
}

void AHuntCharacter::AttachWeaponToHands()
{
	if (!EquippedWeapon)
	{
		return;
	}

	// 내 캐릭터는 1인칭 팔에, 다른 사람 캐릭터는 전신 손에 든다
	// shortcut: 내 화면에서는 내 그림자에 총이 안 생긴다(FirstPerson은 그림자 없음), 필요하면 총을 두 정(팔용·전신용)으로 나눈다
	const bool bFirstPerson = IsLocallyControlled();
	EquippedWeapon->GetMesh()->SetFirstPersonPrimitiveType(bFirstPerson ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None);
	if (bFirstPerson)
	{
		EquippedWeapon->AttachToComponent(FirstPersonArms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FirstPersonWeaponBone);
		EquippedWeapon->GetRootComponent()->SetRelativeLocationAndRotation(FirstPersonWeaponOffset, FRotator::ZeroRotator);
	}
	else
	{
		EquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
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
	FirstPersonCamera->SetRelativeLocation(HeadCameraLocation);
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(DefaultFOV, AimFOV, AimBlend));

	// 조준 정렬: 팔 자세는 애니메이션 그대로 두고 팔 전체를 옮겨 가늠자를 시선 축(카메라 X축) 위에 둔다
	// 가늠자의 팔 기준 위치는 팔 위치와 무관해서 옮겨도 되먹임이 없다
	FTransform ArmsToCamera = ArmsBaseTransform;
	const USkeletalMeshComponent* WeaponMesh = EquippedWeapon ? EquippedWeapon->GetMesh() : nullptr;
	if (WeaponMesh && WeaponMesh->DoesSocketExist(SightSocketName) && WeaponMesh->GetAttachParent() == FirstPersonArms)
	{
		const FVector SightInArms = FirstPersonArms->GetComponentTransform().InverseTransformPosition(WeaponMesh->GetSocketLocation(SightSocketName));
		const FVector SightInCamera = ArmsToCamera.TransformPosition(SightInArms);
		ArmsToCamera.AddToTranslation(-FVector(0.f, SightInCamera.Y, SightInCamera.Z) * AimBlend);
	}
	FirstPersonArms->SetRelativeTransform(ArmsToCamera);
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
