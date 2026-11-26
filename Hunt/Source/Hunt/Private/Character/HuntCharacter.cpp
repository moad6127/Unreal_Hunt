// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HuntCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/Attachment/WeaponAttachmentComponent.h"
#include "Weapon/Attachment/OpticAttachmentDefinition.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Weapon/WeaponDefinition.h"

namespace
{
	// 카메라가 따라갈 뼈
	const FName CameraHeadBone(TEXT("head"));
	// 조준 시 화면 중앙에 올 총의 가늠자 소켓. 조준하는 동안 팔 전체를 옮겨 이 소켓을 시선 축 위에 둔다
	const FName SightSocketName(TEXT("Socket_Sight"));
	// ScopeLensParameters MPC의 벡터 파라미터 이름
	const FName ScopeLensParameterName(TEXT("ScopeLens"));
	// ScopeReticleMaterial의 레티클 텍스처 파라미터 이름
	const FName ScopeReticleTextureName(TEXT("ReticleTexture"));
	// 다른 사람이 보는 전신 총은 ik_hand_gun 뼈에 붙인다(ABP가 hand_r을 복사해 두고, 손잡이가 다른 총은 오른손을 IK로 총에 맞춘다)
	// 손 소켓(HandGrip_R)의 손 기준 위치·회전을 그대로 옮겨 와 손 IK가 없는 총은 손에 붙일 때와 같다
	const FName BodyWeaponBone(TEXT("ik_hand_gun"));
	const FName WeaponSocketName(TEXT("HandGrip_R"));
	// 1인칭 팔에서 총을 붙일 뼈. LPSP 애니메이션은 총을 ik_hand_gun(SOCKET_Weapon)으로 움직이고 손은 그 총을 잡는다
	const FName FirstPersonWeaponBone(TEXT("ik_hand_gun"));
	// 1인칭 오른손 기준점: ik_hand_gun 기준 위치(회전 없음). 총의 Socket_Grip이 여기에 온다. 총과 무관하게 손에 대해 한 번 정하는 값
	const FVector FirstPersonWeaponOffset(-2.03f, -7.11f, 1.88f);
	// 내려다볼 때 카메라를 앞에 둘 기준 뼈
	const FName PelvisBone(TEXT("pelvis"));

	// 카메라 앞 Distance(cm) 거리의 1cm가 화면에서 몇 "화면 세로 절반"인지. 총은 1인칭 FOV로 그려지므로 그 FOV를 넣는다
	// 엔진 기본 MaintainYFOV: FOV는 카메라 AspectRatio에서의 가로 값이고 세로 시야가 고정이라, 세로 기준이면 PIE 창 비율과 무관하다
	float CmToScreenHalfHeight(float Distance, float FirstPersonFOV, float AspectRatio)
	{
		const float TanHalfVerticalFOV = FMath::Tan(FMath::DegreesToRadians(FirstPersonFOV * 0.5f)) / AspectRatio;
		return 1.f / (Distance * TanHalfVerticalFOV);
	}

	// 머리 뼈에서 시선 방향으로 얼마나 앞에 둘지(cm). 아래를 볼 때 하체 메시의 허리 잘린 면이 보이면 늘린다
	constexpr float CameraForwardOffset = 15.f;
	// 내려다볼수록 카메라를 골반보다 수평으로 최소 이만큼(cm, 똑바로 아래, 각도에 비례) 앞에 둔다. 상체가 덜 숙여지는 자세(대기 = ALS 시선 애디티브)에서도
	// 카메라가 하체 메시 바로 위에 와서 허벅지 잘린 면이 보이지 않게 (PIE에서 맞춘 값)
	constexpr float MinLeadOverPelvis = 40.f;
	// 머리 위치를 따라가는 속도. 낮을수록 흔들림이 줄고 반응이 느려진다 (0 = 즉시)
	constexpr float CameraFollowSpeed = 15.f;
	// 총 킥이 목표 위치로 튀어 나가는 속도. 높을수록 한 발이 날카롭게 튄다
	constexpr float KickSnapSpeed = 30.f;
	// 무기 흔들림: 시선 회전 속도(도/초)에 곱해 총을 반대로 기울이는 비율
	constexpr float SwayAmount = 0.02f;
	// 흔들림 최대 각도(도)
	constexpr float MaxSwayAngle = 10.f;
	// 흔들림이 목표를 따라가고 제자리로 돌아오는 속도. 낮을수록 총이 늦게 따라온다
	constexpr float SwayInterpSpeed = 8.f;
	// 완전 조준 시 흔들림 비율 (가늠자가 화면 중앙에서 크게 벗어나지 않게)
	constexpr float AimSwayScale = 0.4f;
	// 사격을 멈춘 뒤 반동이 원래 조준점으로 돌아오는 속도. 높을수록 빨리 돌아온다
	constexpr float RecoilRecoverySpeed = 6.f;
	// 점프·착지 때 탄퍼짐이 공중 값으로(또는 땅 값으로) 바뀌는 속도. 크로스헤어가 튀지 않게 보간한다
	constexpr float AirSpreadInterpSpeed = 10.f;
	// 카메라 정면으로 벽을 찾는 거리(cm). 벽이 이보다 가까워지면 총을 내리기 시작한다
	constexpr float WallBlockStartDistance = 75.f;
	// 벽이 이 거리(cm) 이하면 총을 다 내린다
	constexpr float WallBlockFullDistance = 45.f;
	// 총을 내리고 올리는 속도
	constexpr float WallBlockInterpSpeed = 10.f;
	// 벽 감지 시작점: 카메라에서 총이 놓이는 쪽(시선 기준 X 앞, Y 오른쪽, Z 위)으로 옮긴 위치(cm)
	// 실제 총 위치를 쓰면 총을 내릴 때마다 벽에서 멀어져 다시 올라가며 떨리므로 카메라 기준 고정값을 쓴다
	const FVector WallBlockSweepOffset = FVector(0.f, 15.f, -10.f);
	// 벽 감지 구의 반경(cm). 시선이 벽과 나란해도 총열이 지나는 공간에 벽이 있으면 걸리도록
	constexpr float WallBlockSweepRadius = 10.f;
	// 카메라 벽 검사 구의 반경(cm). 근평면(5)보다 커야 벽이 화면에서 잘려 벽 너머가 보이지 않는다
	constexpr float CameraWallRadius = 8.f;
	// 카메라가 벽에서 이만큼(cm) 안쪽이면 벽 쪽 이동 입력을 막는다. 밀려난 뒤 바로 다시 들어가 앞뒤로 떨리지 않게
	constexpr float CameraWallMargin = 5.f;
	// 조준을 풀거나 쏜 뒤 준비 자세(총구 앞)를 유지하는 시간(초). 지나면 총을 내린다
	constexpr float ReadyHoldTime = 2.f;
	// 내린 총을 드는 시간(초). 내린 채 쏘면 이만큼 든 뒤 발사한다
	constexpr float RaiseTime = 0.2f;
	// 총을 대기 자세로 내리는 시간(초). 드는 것보다 천천히 내려야 자연스럽다(11-24 PIE)
	constexpr float LowerTime = 0.5f;
	// 입력 방향이 정면에서 이 각도(도) 안일 때만 질주한다. 벗어나면 조깅/걷기 속도로 돌아간다
	constexpr float SprintMaxAngle = 20.f;
	// 최대 속도를 낮출 때(질주 → 조깅/걷기) 초당 줄이는 양(cm/s). 한 번에 낮추면 몸이 0.2초 만에 멈춰 이동 애니메이션 블렌드가 따라오지 못한다
	constexpr float SpeedDownRate = 600.f;
	// 옆·뒤로 갈 때 최대 속도 비율(정면 = 1). 이동 애니메이션 실제 속도(옆 약 0.75, 뒤 약 0.6배)에 맞춰 발 미끄러짐을 줄인다
	constexpr float StrafeSpeedScale = 0.9f;
	constexpr float BackwardSpeedScale = 0.6f;
	// 키를 떼고 이 속도(cm/s) 아래가 되면 3인칭 애님이 ABP Stop 상태로 간다(ALS ShouldMove 값 150). UHuntAnimInstance가 읽는다
	constexpr float StopBrakeSpeed = 150.f;
}

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
	// 총(1인칭 프리미티브)을 세상과 다른 FOV로 그린다: 조준경 배율로 세상만 좁히고 총은 확대하지 않는다
	FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
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
	DefaultFirstPersonScale = FirstPersonCamera->FirstPersonScale;
	if (ScopeReticleMaterial)
	{
		ScopeReticleInstance = UMaterialInstanceDynamic::Create(ScopeReticleMaterial, this);
		FirstPersonCamera->PostProcessSettings.AddBlendable(ScopeReticleInstance, 1.f);
	}

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

	LensWeapon = SpawnWeapon();

	// 부착물 메시가 총의 1인칭 렌더 설정을 따르므로 붙인 뒤에 장착한다
	EquippedWeapon->GetAttachments()->EquipDefaultAttachments();
	BodyWeapon->GetAttachments()->EquipDefaultAttachments();
	// shortcut: 기본 부착물만 따라 단다, 개조 UI(M3)로 부착물을 바꾸게 되면 1인칭 총과 같이 바꾼다
	LensWeapon->GetAttachments()->EquipDefaultAttachments();
	SetupLensWeapon();
	ApplyScopeReticle();

	UpdateWeaponVisibility();
	LinkWeaponAnimLayers();
}

void AHuntCharacter::LinkWeaponAnimLayers()
{
	// shortcut: 장착은 시작할 때 한 번뿐이라 이전 레이어를 끊지 않는다, 총 교체가 생기면 UnlinkAnimClassLayers를 먼저 부른다
	if (DefaultWeapon->ThirdPersonAnimLayers)
	{
		GetMesh()->LinkAnimClassLayers(DefaultWeapon->ThirdPersonAnimLayers);
	}
	if (DefaultWeapon->FirstPersonAnimLayers)
	{
		FirstPersonArms->LinkAnimClassLayers(DefaultWeapon->FirstPersonAnimLayers);
	}
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
	// 총의 Socket_Grip이 손 기준점에 오게 옮긴다. 회전은 그대로 둬야 총 축이 시선과 평행하다
	EquippedWeapon->GetRootComponent()->SetRelativeLocationAndRotation(FirstPersonWeaponOffset - EquippedWeapon->GetRightHandGripOffset(), FRotator::ZeroRotator);
}

void AHuntCharacter::AttachBodyWeapon()
{
	// 전신 메시 설정을 따른다: 기본은 내 화면에서 숨기고 그림자만 남김, 3인칭 테스트 BP(None)에서는 보임
	BodyWeapon->GetMesh()->SetFirstPersonPrimitiveType(GetMesh()->FirstPersonPrimitiveType);
	BodyWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, BodyWeaponBone);
	SetBodyWeaponRelaxedGrip(0.f);
}

void AHuntCharacter::SetupLensWeapon()
{
	// 1인칭 총에 붙여 킥·흔들림·조준 정렬을 같이 따라간다(앞 렌즈 시점 보정은 UpdateLensWeaponView). 1인칭이 아닌 보통 물체라 세상 FOV로 그려져,
	// 렌즈로 본 것처럼 배율만큼 확대되고 고배율에서는 총열이 렌즈 원 밖으로 벗어난다
	LensWeapon->AttachToComponent(EquippedWeapon->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// 조준경 자체는 렌즈 안을 가리므로 그리지 않는다
	const UStaticMeshComponent* Optic = LensWeapon->GetAttachments()->FindAttachmentSocketMesh(EHuntAttachmentSlot::Optic, SightSocketName);
	TInlineComponentArray<UPrimitiveComponent*> Primitives(LensWeapon);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		// MF_ScopeLensMask: Custom Primitive Data 0 = 1이면 1인칭 총이 잘리는 렌즈 원 안만 남긴다
		Primitive->SetCustomPrimitiveDataFloat(0, 1.f);
		Primitive->SetOnlyOwnerSee(true);
		Primitive->SetCastShadow(false);
		Primitive->SetAffectDistanceFieldLighting(false);
		Primitive->SetVisibleInRayTracing(false);
		Primitive->SetVisibility(Primitive != Optic);
	}
}

void AHuntCharacter::SetBodyWeaponRelaxedGrip(float RelaxedHandAlpha)
{
	// HandGrip_R 소켓이 오른손 기준점이다
	const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(WeaponSocketName);
	const FTransform HandSocketTransform = HandSocket ? HandSocket->GetSocketLocalTransform() : FTransform::Identity;
	FTransform Grip = FTransform(-BodyWeapon->GetRightHandGripOffset()) * HandSocketTransform;
	FTransform RelaxedHand;
	if (RelaxedHandAlpha > 0.f && BodyWeapon->GetRightHandRelaxedTransform(RelaxedHand))
	{
		// 대기 손 소켓 = 총 공간의 손이므로 그 역이 손 공간의 총이다
		Grip.BlendWith(RelaxedHand.Inverse(), RelaxedHandAlpha);
	}
	BodyWeapon->GetRootComponent()->SetRelativeTransform(Grip);
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
	LensWeapon->SetActorHiddenInGame(EquippedWeapon->IsHidden());
}

float AHuntCharacter::GetStopBrakeSpeed() const
{
	return StopBrakeSpeed;
}

const UWeaponDefinition* AHuntCharacter::GetWeaponDefinition() const
{
	return EquippedWeapon ? EquippedWeapon->Definition.Get() : nullptr;
}

bool AHuntCharacter::CanSprint(float Angle) const
{
	return Stance != EHuntWeaponStance::Aiming && bWantsToSprint && Angle <= SprintMaxAngle && WallBlockAlpha < 0.1f;
}

float AHuntCharacter::GetCurrentSpread() const
{
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	if (!Definition)
	{
		return 0.f;
	}
	// 걸음 상태마다 MaxWalkSpeed가 바뀌므로 질주 속도를 최대 속도로 삼는다
	const float MoveAlpha = FMath::Clamp(GetVelocity().Size2D() / SprintSpeed, 0.f, 1.f);
	const float GroundSpread = FMath::Lerp(Definition->HipSpread, Definition->MovingSpread, MoveAlpha);
	const float SpreadScale = EquippedWeapon->GetAttachments()->ComputeFinalStats().Spread;
	return FMath::Lerp(GroundSpread, Definition->AirSpread, AirSpreadAlpha) * SpreadScale * (1.f - GetAimBlend());
}

void AHuntCharacter::UpdateWallBlock(float DeltaSeconds)
{
	float Target = 0.f;
	if (EquippedWeapon && bLowerWeaponNearWall)
	{
		const FRotator ViewRotation = GetControlRotation();
		const FVector Start = FirstPersonCamera->GetComponentLocation() + ViewRotation.RotateVector(WallBlockSweepOffset);
		const FVector End = Start + ViewRotation.Vector() * WallBlockStartDistance;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWallBlock), false, this);
		Params.AddIgnoredActor(EquippedWeapon);
		Params.AddIgnoredActor(BodyWeapon);
		FHitResult Hit;
		// 시작부터 벽 안이면 Distance가 0이라 총을 다 내린다
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(WallBlockSweepRadius), Params))
		{
			Target = FMath::GetMappedRangeValueClamped(FVector2D(WallBlockFullDistance, WallBlockStartDistance), FVector2D(1.f, 0.f), Hit.Distance);
		}
	}
	WallBlockAlpha = FMath::FInterpTo(WallBlockAlpha, Target, DeltaSeconds, WallBlockInterpSpeed);
}

void AHuntCharacter::UpdateStance(float DeltaSeconds)
{
	// 준비 자세: 쏘는 동안은 유지, 손을 떼고 ReadyHoldTime이 지나면 총을 내린다
	if (Stance == EHuntWeaponStance::Ready)
	{
		ReadyTimeLeft = bFireHeld ? ReadyHoldTime : ReadyTimeLeft - DeltaSeconds;
		if (ReadyTimeLeft <= 0.f)
		{
			Stance = EHuntWeaponStance::Relaxed;
			bFireAfterRaise = false;
		}
	}
	const bool bLowering = Stance == EHuntWeaponStance::Relaxed;
	RelaxedAlpha = FMath::FInterpConstantTo(RelaxedAlpha, bLowering ? 1.f : 0.f, DeltaSeconds, 1.f / (bLowering ? LowerTime : RaiseTime));
}

void AHuntCharacter::UpdateAirSpread(float DeltaSeconds)
{
	AirSpreadAlpha = FMath::FInterpTo(AirSpreadAlpha, GetCharacterMovement()->IsFalling() ? 1.f : 0.f, DeltaSeconds, AirSpreadInterpSpeed);
}

void AHuntCharacter::FireAfterRaise()
{
	// 내린 채 누른 발사: 다 들고 나서 쏜다. 그사이 버튼을 뗐으면(짧게 클릭) 한 발만
	if (bFireAfterRaise && RelaxedAlpha <= 0.f && EquippedWeapon)
	{
		bFireAfterRaise = false;
		EquippedWeapon->StartFire();
		if (!bFireHeld)
		{
			EquippedWeapon->StopFire();
		}
	}
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
	UpdateCameraWall();
	UpdateAimAlpha(DeltaSeconds);
	UpdateAirSpread(DeltaSeconds);
	UpdateWallBlock(DeltaSeconds);
	UpdateStance(DeltaSeconds);
	FireAfterRaise();
	ApplyRecoil(DeltaSeconds);

	const float AimBlend = GetAimBlend();
	UpdateCameraView(AimBlend);
	UpdateWeaponSway(DeltaSeconds);
	UpdateWeaponKick(DeltaSeconds);
	UpdateFirstPersonArms(AimBlend);
	UpdateScopeLensMask(AimBlend);
	UpdateLensWeaponView();
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
	// 벽에 막혀 총을 내리면 질주하지 않는다(질주 상체 자세 위에 내린 자세가 겹치면 팔이 꺾인다)
	if (Stance == EHuntWeaponStance::Aiming)
	{
		return WalkSpeed;
	}
	if (CanSprint(Angle))
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
	FVector HeadWorld = GetMesh()->GetSocketLocation(CameraHeadBone) + AimDirection * CameraForwardOffset;

	// 내려다볼수록 카메라를 골반보다 앞에 둔다(MinLeadOverPelvis)
	const FVector YawForward = FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();
	const float DownAlpha = FMath::Clamp(-FRotator::NormalizeAxis(GetControlRotation().Pitch) / 90.f, 0.f, 1.f);
	const float Lead = FVector::DotProduct(HeadWorld - GetMesh()->GetSocketLocation(PelvisBone), YawForward);
	HeadWorld += YawForward * FMath::Max(0.f, MinLeadOverPelvis * DownAlpha - Lead);

	const FVector Target = CapsuleTransform.InverseTransformPosition(HeadWorld);

	// 보간으로 걷기·달리기의 머리 흔들림을 줄인다 (0이면 즉시 따라감)
	HeadCameraLocation = CameraFollowSpeed > 0.f ? FMath::VInterpTo(HeadCameraLocation, Target, DeltaSeconds, CameraFollowSpeed) : Target;
}

void AHuntCharacter::UpdateCameraWall()
{
	// 캡슐 중심(카메라 높이)에서 카메라까지 구 스윕. 내려다볼 때 캡슐 밖으로 나간 카메라가 벽에 들어가면
	// 이번 프레임은 카메라를 벽 앞에 세우고 캐릭터를 뒤로 걷게 해서(이동 애니메이션 재생) 몸째 벽에서 뗀다
	const FTransform& CapsuleTransform = GetCapsuleComponent()->GetComponentTransform();
	const FVector Camera = CapsuleTransform.TransformPosition(HeadCameraLocation);
	const FVector Start(CapsuleTransform.GetLocation().X, CapsuleTransform.GetLocation().Y, Camera.Z);
	const FVector Direction = (Camera - Start).GetSafeNormal();
	const float CameraDistance = FVector::Dist(Start, Camera);

	CameraWallNormal = FVector::ZeroVector;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntCameraWall), false, this);
	Params.AddIgnoredActor(EquippedWeapon);
	Params.AddIgnoredActor(BodyWeapon);
	FHitResult Hit;
	// 시작점(캡슐 중심)부터 겹쳐 있으면 법선을 믿을 수 없다
	if (Direction.IsNearlyZero()
		|| !GetWorld()->SweepSingleByChannel(Hit, Start, Camera + Direction * CameraWallMargin, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(CameraWallRadius), Params)
		|| Hit.bStartPenetrating)
	{
		bCameraWallPush = false;
		return;
	}

	CameraWallNormal = Hit.ImpactNormal.GetSafeNormal2D();
	const bool bPenetrating = Hit.Distance < CameraDistance;
	if (bPenetrating)
	{
		HeadCameraLocation = CapsuleTransform.InverseTransformPosition(Start + Direction * Hit.Distance);
	}
	// 한 번 밀기 시작하면 벽에서 CameraWallMargin만큼 떨어질 때까지 계속 민다. 벽에서 나오자마자 멈추면 입력이 프레임마다 켜졌다 꺼져 다리가 떨린다
	bCameraWallPush = bPenetrating || bCameraWallPush;
	if (bCameraWallPush)
	{
		AddMovementInput(CameraWallNormal);
	}
}

void AHuntCharacter::UpdateAimAlpha(float DeltaSeconds)
{
	// 조준 진행도: 총의 조준 시간 동안 일정하게 0↔1. 공중이거나 벽에 막혀 총을 내리면 조준이 풀리고, 버튼을 누르고 있으면 다시 조준한다
	const UWeaponDefinition* Definition = GetWeaponDefinition();
	const bool bCanAim = Definition && !GetCharacterMovement()->IsFalling() && WallBlockAlpha < 0.1f;
	const float AimTarget = (Stance == EHuntWeaponStance::Aiming && bCanAim) ? 1.f : 0.f;
	// 부착물 조준 시간 배율은 조준할 때만. 풀 때는 총 기본값이라 부착물과 상관없이 같다
	const float AimTimeScale = AimTarget > 0.f ? EquippedWeapon->GetAttachments()->ComputeFinalStats().AimDownSightsTime : 1.f;
	const float AimSpeed = Definition ? 1.f / (Definition->AimDownSightsTime * AimTimeScale) : 10.f;
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
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(DefaultFOV, GetAimFOV(), AimBlend));
	FirstPersonCamera->SetFirstPersonFieldOfView(FMath::Lerp(DefaultFOV, GetAimFirstPersonFOV(), AimBlend));
	// Scale은 카메라 중심으로 눌러 그려 화면 모양은 같고 깊이만 바뀐다. 0.4면 근평면이 총 기준 12.5cm라 조준경 접안렌즈가 잘려,
	// 조준 중에는 1로 둔다(벽 앞에서는 WallBlock이 조준을 막아 벽 뚫림이 없다)
	FirstPersonCamera->SetFirstPersonScale(FMath::Lerp(DefaultFirstPersonScale, 1.f, AimBlend));
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

void AHuntCharacter::UpdateScopeLensMask(float AimBlend)
{
	if (!ScopeLensParameters)
	{
		return;
	}

	// 조준경이 없으면 조준 정도 0: 머티리얼이 아무것도 자르지 않는다
	FLinearColor Lens(0.f, 0.f, 0.f, 0.f);
	if (const UStaticMeshComponent* OpticMesh = GetEquippedOpticMesh())
	{
		// 카메라 기준(X 앞, Y 오른쪽, Z 위). 1인칭 Scale은 카메라 쪽으로 누르기만 해서 화면 위치는 같다
		const FVector Eyepiece = FirstPersonCamera->GetComponentTransform().InverseTransformPosition(OpticMesh->GetSocketLocation(SightSocketName));
		if (Eyepiece.X > KINDA_SMALL_NUMBER)
		{
			// 화면 중앙 원점, 화면 세로 절반 = 1인 단위. 가로 비율은 머티리얼이 ViewSize로 맞춘다
			const float CmToHalfHeight = CmToScreenHalfHeight(Eyepiece.X, FirstPersonCamera->FirstPersonFieldOfView, FirstPersonCamera->AspectRatio);
			Lens = FLinearColor(Eyepiece.Y * CmToHalfHeight, Eyepiece.Z * CmToHalfHeight, GetEquippedOptic()->EyepieceRadius * CmToHalfHeight, AimBlend);
		}
	}
	UKismetMaterialLibrary::SetVectorParameterValue(this, ScopeLensParameters, ScopeLensParameterName, Lens);
}

void AHuntCharacter::ApplyScopeReticle()
{
	const UOpticAttachmentDefinition* Optic = GetEquippedOptic();
	if (ScopeReticleInstance && Optic && Optic->Reticle)
	{
		ScopeReticleInstance->SetTextureParameterValue(ScopeReticleTextureName, Optic->Reticle);
	}
}

void AHuntCharacter::UpdateLensWeaponView()
{
	if (!LensWeapon)
	{
		return;
	}

	// 실제 조준경은 앞 렌즈로 들어온 상만 보인다. 앞 렌즈 깊이만큼 복제 총을 시선 축 뒤로 당기면
	// 앞 렌즈 뒤의 레일·리시버·가늠쇠 받침은 카메라 뒤(근평면 밖)로 가고, 앞 렌즈보다 앞의 총열·총구만 렌즈 아래에 남는다
	FVector Offset = FVector::ZeroVector;
	const UStaticMeshComponent* OpticMesh = GetEquippedOpticMesh();
	if (OpticMesh && OpticMesh->GetStaticMesh())
	{
		// 조준경 메시는 +Y가 앞이라 바운드 +Y 끝이 앞 렌즈
		const FVector Objective = OpticMesh->GetComponentTransform().TransformPosition(FVector(0.f, OpticMesh->GetStaticMesh()->GetBoundingBox().Max.Y, 0.f));
		const FVector Forward = FirstPersonCamera->GetForwardVector();
		Offset = -Forward * FVector::DotProduct(Objective - FirstPersonCamera->GetComponentLocation(), Forward);
	}
	LensWeapon->SetActorLocation(EquippedWeapon->GetActorLocation() + Offset);
}

const UOpticAttachmentDefinition* AHuntCharacter::GetEquippedOptic() const
{
	return EquippedWeapon
		? Cast<UOpticAttachmentDefinition>(EquippedWeapon->GetAttachments()->GetEquippedAttachment(EHuntAttachmentSlot::Optic))
		: nullptr;
}

const UStaticMeshComponent* AHuntCharacter::GetEquippedOpticMesh() const
{
	return GetEquippedOptic() ? EquippedWeapon->GetAttachments()->FindAttachmentSocketMesh(EHuntAttachmentSlot::Optic, SightSocketName) : nullptr;
}

float AHuntCharacter::GetAimFOV() const
{
	const UOpticAttachmentDefinition* Optic = GetEquippedOptic();
	const UStaticMeshComponent* OpticMesh = GetEquippedOpticMesh();
	if (!OpticMesh)
	{
		return AimFOV;
	}
	// 조준을 다 했을 때 접안렌즈 원의 화면 반지름(화면 세로 절반 = 1). 조준 정렬은 Y·Z만 옮기므로 눈 거리(X)는 기본 팔 위치로 구하고,
	// 킥은 넣지 않는다: 실제 조준경도 눈이 흔들려 렌즈가 커지고 작아질 뿐 렌즈 속 상의 배율은 그대로다
	const float EyepieceDistance = ArmsWorldToCamera(ArmsBaseTransform, OpticMesh->GetSocketLocation(SightSocketName)).X;
	if (EyepieceDistance <= KINDA_SMALL_NUMBER)
	{
		return AimFOV;
	}
	const float LensRadius = Optic->EyepieceRadius * CmToScreenHalfHeight(EyepieceDistance, GetAimFirstPersonFOV(), FirstPersonCamera->AspectRatio);
	// 렌즈 원 가장자리가 실제 시야의 반각을 보이게: tan(세로 반각) × 렌즈 반지름 = 시야 절반 / 100m. FOV는 MaintainYFOV라 가로 값으로 돌려준다
	const float FieldOfViewAt100m = Optic->FieldOfViewAt100mAt1x / Optic->Magnification;
	const float TanHalfVertical = (FieldOfViewAt100m * 0.5f / 100.f) / LensRadius;
	return FMath::RadiansToDegrees(2.f * FMath::Atan(TanHalfVertical * FirstPersonCamera->AspectRatio));
}

float AHuntCharacter::GetAimFirstPersonFOV() const
{
	const UOpticAttachmentDefinition* Optic = GetEquippedOptic();
	return Optic ? Optic->AimFirstPersonFOV : AimFOV;
}

FTransform AHuntCharacter::GetSightAlignedArmsTransform(float AimBlend) const
{
	// 조준 정렬: 팔 자세는 애니메이션 그대로 두고 팔 전체를 옮겨 가늠자를 시선 축(카메라 X축) 위에 둔다
	// 가늠자의 팔 기준 위치는 팔 위치와 무관해서 옮겨도 되먹임이 없다
	FTransform ArmsToCamera = ArmsBaseTransform;
	const USkeletalMeshComponent* WeaponMesh = EquippedWeapon ? EquippedWeapon->GetMesh() : nullptr;
	// 조준경에 Socket_Sight(접안렌즈 광축)가 있으면 조준경, 없으면 총의 가늠자
	const UStaticMeshComponent* Optic = WeaponMesh ? EquippedWeapon->GetAttachments()->FindAttachmentSocketMesh(EHuntAttachmentSlot::Optic, SightSocketName) : nullptr;
	const USceneComponent* SightComponent = Optic ? static_cast<const USceneComponent*>(Optic) : WeaponMesh;
	if (WeaponMesh && SightComponent->DoesSocketExist(SightSocketName) && WeaponMesh->GetAttachParent() == FirstPersonArms)
	{
		const FVector SightInCamera = ArmsWorldToCamera(ArmsToCamera, SightComponent->GetSocketLocation(SightSocketName));
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
	const FHuntAttachmentStats Stats = EquippedWeapon->GetAttachments()->ComputeFinalStats();
	const float KickYaw = Definition->KickYaw * Stats.HorizontalRecoil;
	KickTargetLocation.X -= Definition->KickBack * Stats.VerticalRecoil;
	KickTargetRotation += FRotator(Definition->KickPitch * Stats.VerticalRecoil, FMath::FRandRange(-KickYaw, KickYaw), 0.f);
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

	FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) * Input.Y + FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * Input.X;
	// 카메라가 벽에 닿아 있으면 벽으로 들어가는 성분만 뺀다(벽을 따라 옆으로는 간다)
	Direction -= CameraWallNormal * FMath::Min(0.f, FVector::DotProduct(Direction, CameraWallNormal));
	AddMovementInput(Direction);
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
	bFireHeld = true;
	if (!EquippedWeapon)
	{
		return;
	}
	if (Stance == EHuntWeaponStance::Relaxed)
	{
		Stance = EHuntWeaponStance::Ready;
		ReadyTimeLeft = ReadyHoldTime;
	}
	// 총이 아직 내려가 있으면 다 든 뒤 FireAfterRaise에서 쏜다
	if (RelaxedAlpha > 0.f)
	{
		bFireAfterRaise = true;
		return;
	}
	EquippedWeapon->StartFire();
}

void AHuntCharacter::StopFire()
{
	bFireHeld = false;
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void AHuntCharacter::StartAim()
{
	Stance = EHuntWeaponStance::Aiming;
}

void AHuntCharacter::StopAim()
{
	Stance = EHuntWeaponStance::Ready;
	ReadyTimeLeft = ReadyHoldTime;
}
