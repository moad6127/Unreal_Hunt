# Unreal_Hunt


Unreal Engine 5.8과 C++로 만들고 있는 1인칭 사냥 게임 프로젝트입니다. (현재 개발 중)

theHunter: Call of the Wild 같은 사냥 게임에 총기 부착물 개조(조준경, 손잡이, 총구, 개머리판 등) 기능을 더해, 부착물을 바꾸면 총의 반동과 조준 속도, 배율이 눈에 띄게 달라지는 것을 목표로 하고 있습니다.

이 프로젝트는 Claude Code와 언리얼 엔진 MCP를 연결해 개발하고 있습니다. 무엇을 만들지와 완료 판단은 제가 하고, 구현은 AI에 맡기는 방식으로 진행하며, 언리얼 엔진 MCP의 기본 도구로 애니메이션 블루프린트를 다룰 수 없는 부분은 MCP 도구를 플러그인으로 직접 만들어 사용하고 있습니다.




<details><summary> 구분</summary>
<p>

 * [AI 개발 환경](#AI-개발-환경)

 * [HuntAnimTools](#HuntAnimTools)

 * [Character](#Character)

 * [Weapon](#Weapon)

 * [Attachment](#Attachment)

 * [진행 상황](#진행-상황)

</p>
</details>
<br/> <br>

-----------------------------------------------------
## AI 개발 환경

| 항목 | 내용 |
|---|---|
| 엔진 | Unreal Engine 5.8.4 (Blank 템플릿, C++) |
| IDE | Visual Studio 2022 (MSVC 14.44, 언리얼 엔진 공식 문서대로 설정) |
| AI | Claude Code + 언리얼 엔진 MCP 플러그인 (`ModelContextProtocol`, `AllToolsets`, `Terminal`) |

- [CLAUDE.md](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/CLAUDE.md)
- [.mcp.json](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/.mcp.json)

```json
{
	"mcpServers":
	{
		"unreal-mcp":
		{
			"type": "http",
			"url": "http://127.0.0.1:8000/mcp"
		}
	}
}
```

- **MCP 연결**: 에디터의 플러그인 창에서 `ModelContextProtocol`, `AllToolsets`, `Terminal`을 켜면 에디터가 실행될 때 MCP 서버가 열립니다. 이 주소를 `.mcp.json`에 등록해 Claude Code가 에셋과 블루프린트를 직접 보고 고칠 수 있게 했습니다.

- **CLAUDE.md**: 빌드 명령, 마일스톤별 완료 기준, 설계 원칙, 작업 규칙을 적어 두고 AI가 항상 같은 기준으로 작업하도록 했습니다.

- **라이브 코딩 끄기**: 라이브 코딩이 켜져 있으면 명령줄 빌드와 충돌해서 꺼 두고 사용합니다.

***CLAUDE.md 작업 규칙 (일부)***

> 현재 마일스톤의 완료 기준에 필요한 코드만 작성한다. 다음 단계를 위한 추상화는 미리 만들지 않는다.

> 요청한 부분만 수정하고, 관련 없는 코드는 정리하지 않는다.

> 불확실한 UE 5.8 API나 에셋 구조는 추측하지 말고 확인하거나 질문한다.

> 도구가 부족해도 바로 추가하지 않는다. 어떤 작업을 할 수 있게 되는 도구인지 먼저 설명하고, 허락을 받은 뒤에 추가한다.

***작업 진행 방식***

1. 원하는 동작을 말로 설명합니다.
2. AI가 원인과 수정안, 완료 기준을 계획 문서로 정리하면 확인한 뒤 진행시킵니다.
3. 에디터와 PIE에서 직접 실행해 완료 기준을 확인합니다. 발 위치처럼 눈으로 판단하기 어려운 부분은 측정 스크립트로 수치를 확인합니다.
4. 확인이 끝나면 계획 문서의 상태를 갱신하고 커밋합니다.

<br>

## HuntAnimTools

언리얼 엔진 MCP의 기본 도구로는 애니메이션 블루프린트의 스테이트 머신이나 전환, 애님 노드 삭제 같은 기능을 쓸 수 없어서, 필요한 기능을 MCP 도구 플러그인으로 직접 만들었습니다.

- [HuntAnimGraphToolset h](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Plugins/HuntAnimTools/Source/HuntAnimTools/Private/HuntAnimGraphToolset.h)
- [HuntAnimGraphToolset C++](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Plugins/HuntAnimTools/Source/HuntAnimTools/Private/HuntAnimGraphToolset.cpp)

```C++
UCLASS(BlueprintType)
class UHuntAnimGraphToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static TArray<FString> ListGraphs(UBlueprint* Blueprint);

	UFUNCTION(meta = (AICallable), Category = "HuntAnimGraph")
	static FString DescribeGraph(UEdGraph* Graph);

	...
};
```

```C++
UEdGraphNode* UHuntAnimGraphToolset::AddTransition(UEdGraphNode* From, UEdGraphNode* To)
{
	...
	UEdGraphPin* Output = FindDirectionPin(From, EGPD_Output);
	UEdGraphPin* Input = FindDirectionPin(To, EGPD_Input);

	...
	// 스키마가 사람이 화살표를 그릴 때와 같은 경로로 전환 노드를 만든다
	if (!From->GetGraph()->GetSchema()->TryCreateConnection(Output, Input))
	{
	...
}
```

- **AI 호출 도구**: `UToolsetDefinition`을 상속하고 함수에 `AICallable`을 붙여, MCP 서버에서 AI가 직접 호출할 수 있는 도구로 등록했습니다.

- **사람과 같은 편집 경로**: 전환을 만들 때 스키마의 `TryCreateConnection`을 사용해, 에디터에서 사람이 화살표를 그릴 때와 같은 방식으로 노드가 만들어지도록 했습니다.

| 도구 | 기능 |
|---|---|
| `ListGraphs`, `DescribeGraph` | 스테이트 머신, 전환 규칙까지 포함한 그래프 구조 읽기 |
| `AddState`, `AddConduit`, `AddTransition`, `SetEntryState` | 스테이트 머신에 스테이트와 전환 추가 |
| `RenameNode`, `RemoveNodes`, `CopyNodes`, `MoveNodes` | 애님 노드 편집, 그래프 간 복사와 이동 |
| `AddBlendPin`, `CompileWithMessages` | 블렌드 핀 추가, 컴파일 결과 확인 |

<br>

## Character


멀티플레이를 대비해 배틀그라운드처럼 메시 3개(전신, 1인칭 팔, 하체)로 1인칭 캐릭터를 구성했습니다.

- [HuntCharacter h](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Public/Character/HuntCharacter.h)
- [HuntCharacter C++](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Private/Character/HuntCharacter.cpp)

```C++
	// 전신은 다른 사람만 본다. WorldSpaceRepresentation이 내 화면에서 숨기고 그림자만 남긴다
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	...

void AHuntCharacter::CreateFirstPersonArms()
{
	// 1인칭 팔: 카메라에 붙어 시선과 함께 움직인다
	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	...
}

	...
	// 하체 메시는 전신 포즈를 그대로 따라간다
	FirstPersonLegs->SetLeaderPoseComponent(GetMesh());
```

- **전신 메시**: 다른 플레이어에게만 보이고, 내 화면에는 `WorldSpaceRepresentation`으로 그림자만 남깁니다.

- **1인칭 팔**: 카메라에 붙어 시선과 함께 움직이고, 내 총은 1인칭 팔에 붙습니다.

- **하체**: `SetLeaderPoseComponent`로 전신 포즈를 따라가, 아래를 내려다보면 다리와 그림자가 보입니다.

- **카메라**: 위치는 전신의 머리 뼈를 보간으로 따라가고, 회전은 컨트롤 회전을 사용해 머리 흔들림이 화면에 실리지 않게 했습니다.

<br>

## Weapon


- [HuntWeapon h](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Public/Weapon/HuntWeapon.h)
- [HuntWeapon C++](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Private/Weapon/HuntWeapon.cpp)

### 조준과 발사

```C++
FVector AHuntWeapon::GetAimDirection(const APawn* OwnerPawn, const FVector& MuzzleLocation) const
{
	...
	const FVector ViewDirection = ViewRotation.Vector();
	const FVector AimPoint = GetAimPoint(OwnerPawn, ViewLocation, ViewDirection);

	// 벽에 붙어 총구가 조준점보다 앞에 나가 있으면 뒤로 쏘지 않도록 시선 방향을 쓴다
	const FVector Direction = (AimPoint - MuzzleLocation).GetSafeNormal();
	const FVector AimDirection = (Direction | ViewDirection) > 0.f ? Direction : ViewDirection;

	// 벽에 막혀 총을 내린 만큼 크로스헤어 대신 총구가 향한 방향으로 쏜다
	const AHuntCharacter* Character = Cast<AHuntCharacter>(OwnerPawn);
	const float WallBlockAlpha = Character ? Character->GetWallBlockAlpha() : 0.f;
	return FMath::Lerp(AimDirection, Mesh->GetRightVector(), WallBlockAlpha).GetSafeNormal();
}
```

- **발사 방향**: 탄은 총구에서 나가고, 방향은 카메라 중앙 트레이스로 찾은 조준점을 향하도록 했습니다.

- **총구 위치**: 소음기 같은 총구 부착물이 있으면 부착물 끝의 `Socket_Muzzle`에서, 없으면 핸드가드 총열 끝이나 총 메시의 총구에서 탄이 나갑니다.

### 벽 앞에서 총 내리기

```C++
void AHuntCharacter::UpdateWallBlock(float DeltaSeconds)
{
	float Target = 0.f;
	if (EquippedWeapon && bLowerWeaponNearWall)
	{
		...
		// 시작부터 벽 안이면 Distance가 0이라 총을 다 내린다
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(WallBlockSweepRadius), Params))
		{
			Target = FMath::GetMappedRangeValueClamped(FVector2D(WallBlockFullDistance, WallBlockStartDistance), FVector2D(1.f, 0.f), Hit.Distance);
		}
	}
	WallBlockAlpha = FMath::FInterpTo(WallBlockAlpha, Target, DeltaSeconds, WallBlockInterpSpeed);
}
```

- **구 스윕 검사**: 카메라 앞을 구 스윕으로 검사해 벽과의 거리에 따라 `WallBlockAlpha`(0~1)를 만들고, 애니메이션 블루프린트가 총을 내린 자세를 그만큼 섞습니다.

- **조준 제한**: 총을 내린 동안에는 조준할 수 없고, 탄은 크로스헤어 대신 총구 방향으로 나갑니다.

### 조준경이 잘리지 않게 하기

```C++
void AHuntCharacter::UpdateCameraView(float AimBlend)
{
	FirstPersonCamera->SetRelativeLocation(HeadCameraLocation);
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(DefaultFOV, AimFOV, AimBlend));
	// 0.4면 근평면이 총 기준 12.5cm라 조준경 접안렌즈가 잘려, 조준 중에는 1로 둔다
	FirstPersonCamera->SetFirstPersonScale(FMath::Lerp(DefaultFirstPersonScale, 1.f, AimBlend));
}
```

- **First Person Scale 보간**: 조준경을 달고 쏘면 반동 때문에 렌즈가 잘리는 문제가 있었습니다. 원인은 1인칭 화면 배율 0.4에서 근평면이 실제 조준경 눈 거리보다 멀어지는 것이어서, 조준 중에만 배율을 1까지 보간해 해결했습니다.

<br>

## Attachment


총 본체와 부착물을 `UPrimaryDataAsset`으로 만들어 코드에 수치를 넣지 않고, 장착 상태는 `WeaponAttachmentComponent` 한 곳에서 관리합니다.

- [WeaponAttachmentComponent h](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Public/Weapon/Attachment/WeaponAttachmentComponent.h)
- [WeaponAttachmentComponent C++](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Private/Weapon/Attachment/WeaponAttachmentComponent.cpp)
- [HandguardAttachmentDefinition h](https://github.com/moad6127/Unreal_Hunt/blob/master/Hunt/Source/Hunt/Public/Weapon/Attachment/HandguardAttachmentDefinition.h)

| 슬롯 | 소켓 | 바뀌는 느낌 |
|---|---|---|
| Handguard | `Socket_Handguard` | ADS 속도, 반동, 하부 슬롯을 엶 |
| Optic | `Socket_Optic` | 배율, ADS 속도, 영점 거리 |
| Muzzle | `Socket_Muzzle` | 소음, 반동 |
| Underbarrel | `Socket_Underbarrel` | 반동, 흔들림, ADS 속도 |
| Magazine | `Socket_Magazine` | 장탄수, 무게 |
| Stock | `Socket_Stock` | 반동, ADS 속도 |

```C++
bool UWeaponAttachmentComponent::EquipAttachment(UAttachmentDefinition* Attachment)
{
	...
	// 슬롯을 가진 부모: 총에 켜진 슬롯이면 총, 아니면 장착된 핸드가드(2층)
	const FHuntWeaponSlot* WeaponSlot = WeaponDefinition->FindSlot(Attachment->Slot);
	const UHandguardAttachmentDefinition* Handguard = Cast<UHandguardAttachmentDefinition>(EquippedAttachments.FindRef(EHuntAttachmentSlot::Handguard));
	if (!WeaponSlot && Handguard)
	{
		WeaponSlot = Handguard->FindSlot(Attachment->Slot);
		ParentMesh = AttachmentMeshes.FindRef(EHuntAttachmentSlot::Handguard);
		Parent = Handguard;
	}
	if (!WeaponSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s에는 %s 슬롯이 없어 장착하지 않음"), ...);
		return false;
	}
	if (!WeaponSlot->Attachments.Contains(Attachment) && WeaponSlot->DefaultAttachment != Attachment)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s의 허용 부착물 목록에 없어 장착하지 않음"), ...);
		return false;
	}
	...
	MeshComponent->AttachToComponent(ParentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	...
	EquippedAttachments.Add(Attachment->Slot, Attachment);

	if (Attachment->Slot == EHuntAttachmentSlot::Handguard)
	{
		ReattachAfterHandguardChange();
	}
	return true;
}
```

- **2층 슬롯 구조**: 슬롯은 게임 전체에 하나의 목록으로 두고 총마다 쓸 수 있는 슬롯만 켭니다. 핸드가드에 따라 하부 슬롯이 열리는 2층 구조까지만 두어, 레일이 없는 클래식 핸드가드에는 앞손잡이를 달 수 없게 했습니다.

- **호환 거부**: 슬롯이 없거나 허용 목록에 없는 부착물은 장착하지 않고, 로그에 이유를 남깁니다.

- **핸드가드 교체**: 핸드가드가 바뀌면 `ReattachAfterHandguardChange`로 나머지 부착물을 새 핸드가드 소켓에 다시 달고, 맞지 않는 부착물은 떼어 냅니다.

- **기본 부품 숨기기**: 총 메시에 뼈로 들어 있는 기본 부품(탄창 등)은 부착물을 달면 `HideBoneByName`으로 숨깁니다.

- **손잡이 그립**: 손잡이를 달면 1인칭과 3인칭 왼손이 손잡이 메시의 소켓을 기준으로 손잡이를 쥐도록 했습니다.

<br>

## 진행 상황

각 단계는 완료 기준을 에디터와 PIE에서 확인한 뒤에 다음 단계로 넘어갑니다.

| 단계 | 내용 | 완료 기준 | 상태 |
|---|---|---|---|
| M1 | 총기 기본 + 캐릭터 | 하체와 그림자가 보이고, 총 두 종의 반동이 확실히 다르고, PIE 2인에서 서로의 몸과 총이 보임 | 완료 |
| M2 | 부착물 시스템 | 에디터에서 부착물을 바꾸면 메시가 붙고 스탯이 바뀌며, 호환되지 않는 부착물은 거부됨 | 진행 중 |
| M3 | 개조 UI, 작업대 | 마우스만으로 개조를 끝낼 수 있음 | 예정 |
| M4 | 사냥 루프 (사슴, 배회 → 경계 → 도주 AI) | 사슴을 발견해 쏘고 잡을 수 있음 | 예정 |
| M5 | 진행 루프 (보상, 부착물 해금, 세이브) | 사냥 → 개조 → 재사냥이 한 바퀴 돎 | 예정 |

***M2 세부 진행***

- [x] 부착물 장착, 메시 부착, 호환되지 않는 부착물 거부
- [x] 총을 M4 모듈러 킷으로 교체
- [x] 부착물 소켓 우선 사용 (조준경 정렬, 총구 끝에서 발사)
- [x] 손잡이 왼손 IK (1인칭, 3인칭)
- [x] 핸드가드 몸체 + 2층 슬롯
- [ ] 최종 스탯 계산 `ComputeFinalStats`
- [ ] 부착물 에셋 6개
- [ ] 스코프 배율, 영점
