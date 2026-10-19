# CLAUDE.md


## Rules
- 항상 한국어로 답변한다.
- 코드를 작성하거나 수정할 때는 karpathy-guidelines 스킬을 먼저 불러와서 따른다.



This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`Hunt` is an Unreal Engine **5.8** C++ project (created from the Blank template, hence the `TP_Blank` → `Hunt` game-name redirects in `Config/DefaultEngine.ini`). It is at an early stage: a single runtime module with stub `AHuntCharacter` and `AHuntGameMode` classes. Engine is installed at `C:\Program Files\Epic Games\UE_5.8` (a 5.7 install also exists. Don't use it).

## Build commands (PowerShell)

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.8\Engine"

# Build the editor target (what you need after changing C++)
& "$UE\Build\BatchFiles\Build.bat" HuntEditor Win64 Development -Project="$PWD\Hunt.uproject" -WaitMutex

# Build the standalone game target
& "$UE\Build\BatchFiles\Build.bat" Hunt Win64 Development -Project="$PWD\Hunt.uproject" -WaitMutex

# Regenerate Visual Studio project files (after adding/removing source files or modules)
& "$UE\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="$PWD\Hunt.uproject" -game -engine
```

- Building `HuntEditor` fails while the editor has the project open with Live Coding enabled. Either close the editor or compile via Live Coding (Ctrl+Alt+F11 in the editor). Header changes (new `UPROPERTY`/`UFUNCTION`, new classes) generally need a full editor restart and rebuild.
- There are no automated tests or lint configuration in the project yet.

## Code layout & conventions

- Module: `Source/Hunt` (`Hunt.Build.cs`, deps: Core, CoreUObject, Engine, InputCore, EnhancedInput). Targets are `Source/Hunt.Target.cs` (Game) and `Source/HuntEditor.Target.cs` (Editor), both `BuildSettingsVersion.V7` / `EngineIncludeOrderVersion.Unreal5_8`.
- Classes are grouped by feature subfolder, mirrored between `Public/` and `Private/` (e.g. `Public/Character/HuntCharacter.h` ↔ `Private/Character/HuntCharacter.cpp`). Includes use the subfolder path: `#include "Character/HuntCharacter.h"`. Exported classes use `HUNT_API`.
- Input uses **Enhanced Input** (`DefaultInput.ini` sets `EnhancedPlayerInput` / `EnhancedInputComponent`). Bind actions through `UEnhancedInputComponent` and Input Mapping Contexts rather than legacy axis/action mappings.
- Content: `Content/Asset/Characters/Mannequins` holds the Manny/Quinn meshes and pistol/rifle/unarmed animation sets. `Content/Map/DefaultMap.umap` is the project map. `GameDefaultMap` currently still points to the engine's `/Engine/Maps/Templates/OpenWorld`.

## Editor MCP integration

The project enables the `ModelContextProtocol`, `AllToolsets` and `Terminal` plugins. When the editor is running they expose an MCP server at `http://127.0.0.1:8000/mcp`, registered in `.mcp.json` as `unreal-mcp`. Use those tools to inspect or edit assets, levels and Blueprints. If the server is unreachable (ECONNREFUSED), the editor is not running. `.uasset`/`.umap` files are binary and cannot be edited as text.




# Hunt

theHunter: Call of the Wild 스타일 사냥 게임에 **총기 부착물(스코프, 손잡이, 총구, 개머리판) 개조** 기능을 넣는 프로젝트입니다.
기존 서바이벌 게임과는 **별개의 새 프로젝트**입니다.

## 목표와 범위

- **1차 목표:** 싱글플레이 수직 슬라이스로 "총기 개조가 재밌다"를 증명한다. 멀티플레이는 M5 이후에 결정한다.
- **핵심 재미 3가지**
  1. 부착물을 바꾸면 총의 느낌(반동, 조준 속도, 배율, 소음)이 눈에 띄게 달라진다.
  2. 개조 화면(작업대)을 보는 맛이 있다.
  3. 개조한 총으로 사냥하는 게 만족스럽다.
- **M5 전에는 만들지 않는다:** 멀티플레이 본격 구현, 넓은 오픈월드, 날씨·바람 시뮬레이션, 여러 동물 종, 복잡한 인벤토리, 캐릭터 커스터마이징

## 프로젝트 설정

| 항목 | 내용 |
|---|---|
| 엔진 | Unreal Engine 5.8.x |
| 생성 방식 | Games → Blank → C++ (Starter Content 없음) |
| 프로젝트(모듈) 이름 | `Hunt` (`HUNT_API`, `HuntCharacter` 등). 게임 제목은 나중에 따로 정함 |
| 시점·캐릭터 | 1인칭 + **전신 메시 하나로 구성한 풀바디 캐릭터** (멀티 대비) |
| 캐릭터 에셋 | 기본 Manny (`UE5_Mannequin_Skeleton`). 5.8 Third Person 임시 프로젝트에서 에셋만 Migrate, 템플릿 코드는 가져오지 않음 |
| 입력 | Enhanced Input (`Hunt.Build.cs`에 `EnhancedInput` 모듈 필요) |

- MCP 플러그인은 5.8에서 **실험(Experimental)** 기능이다. 핵심 구조가 MCP에 의존하면 안 된다.
- Visual Studio는 `Hunt.sln`으로 연다. 5.8에는 `.slnx`로 열면 C++ 클래스 이동이 새 VS 창을 띄우는 버그가 있다.

## 핵심 시스템 설계: 본체 + 슬롯 + 부착물

| 요소 | 역할 | 구현 방식 |
|---|---|---|
| `WeaponDefinition` | 총 본체의 기본 스탯, 사용 가능한 슬롯 목록 | `UPrimaryDataAsset` |
| `AttachmentDefinition` | 슬롯 종류, 메시, 스탯 보정치, 호환 총기 | `UPrimaryDataAsset` |
| `WeaponAttachmentComponent` | 장착 상태 관리, 소켓에 메시 부착, 최종 스탯 계산 | C++ `UActorComponent` |
| 최종 스탯 | 기본값 + 부착물 보정치 합산 | 한 함수로 계산 (`ComputeFinalStats`) |

### 설계 원칙

- **스탯:** 반동, 탄퍼짐, 조준 속도(ADS), 스코프 배율, 소음. 필요한게 있다면 사용자에게 필요를 설명한후 허락을 하면 추가한다.
- **스코프 배율:** FOV 축소 + 렌즈 머티리얼 마스킹. SceneCapture(PIP)는 성능 비용 때문에 쓰지 않는다.
- **소켓 이름 규칙:** `Socket_Scope`, `Socket_Grip`, `Socket_Muzzle`, `Socket_Stock`, ADS 정렬용 `Socket_Sight`, 왼손 IK 목표 `Socket_LeftHand`(총 메시마다 추가). `Socket_Sight`는 기능이 없어도 M1부터 이름을 약속해 둔다.
- **풀바디 캐릭터:** 배그식 트루 1인칭. 카메라 위치는 매 Tick 머리 뼈(+시선 방향 앞 오프셋)를 보간으로 따라가고 회전은 컨트롤 회전을 쓴다(머리 흔들림 완화, 몸통은 항상 카메라 뒤·아래). 1인칭 전용 팔 메시는 쓰지 않는다. 머리는 숨기지 않는다(그림자에서 머리가 사라짐). 총은 에임 오프셋으로 시선 피치를 따라온다. 가장 큰 난관은 ADS 정렬(Aim Offset + 손 위치 IK)이며 M3 스코프 배율 작업에서 핵심이 된다.

## 마일스톤

각 단계는 **완료 기준을 만족해야 다음 단계로** 넘어간다.

| 단계 | 내용 | 완료 기준 |
|---|---|---|
| **M1** 총기 기본 + 캐릭터 | ① 풀바디 1인칭 캐릭터 ② 총 장착·발사 ③ 총 2종 차별화 ④ PIE 2인 점검 | 아래를 내려다보면 몸이 보이고 카메라가 몸과 겹치지 않음. 손 소켓에 총이 붙고 발사하면 표적에 맞음. 두 총의 반동·발사 느낌이 확실히 다름. PIE 2인 실행 시 서로의 몸과 총이 보임 |
| **M2** 부착물 시스템 (핵심) | 슬롯 4종, 부착물 6개 | 에디터에서 부착물을 바꾸면 메시가 붙고 스탯이 바뀌며, 호환 안 되는 부착물은 거부됨 |
| **M3** 개조 UI·작업대 | 총기 회전 보기, 슬롯 선택, 스탯 비교 | 마우스만으로 개조를 끝낼 수 있음 |
| **M4** 사냥 루프 | 동물 1종(사슴), 단순 AI(배회 → 경계 → 도주), 부위별 데미지 | 사슴을 발견해 쏘고 잡을 수 있음 |
| **M5** 진행 루프 | 사냥 보상 → 돈/재료 → 부착물 구매·해금, 세이브/로드 | 사냥 → 개조 → 재사냥이 한 바퀴 돎 |

M5 이후에 직접 재미를 판단하고, 멀티플레이와 콘텐츠 확장 여부를 결정한다.



## 작업 방식 (Claude Code용)

- **단순하게:** 현재 마일스톤의 완료 기준에 필요한 코드만 작성한다. 다음 단계를 위한 추상화는 미리 만들지 않는다.
- **최소 변경:** 요청한 부분만 수정하고, 관련 없는 코드는 정리하지 않는다.
- **가정은 먼저 밝힌다:** 불확실한 UE 5.8 API나 에셋 구조는 추측하지 말고 확인하거나 질문한다.
- **검증 가능하게:** 각 작업은 "무엇이 되면 끝인지"를 마일스톤 완료 기준에 맞춰 정하고, 끝나면 그 기준으로 확인한다.
- 데이터(스탯, 부착물 정보)는 코드에 하드코딩하지 않고 `UPrimaryDataAsset`으로 둔다.
- C++로 작성하고, Blueprint는 UI와 수치 튜닝에만 쓴다.
- UE 네이밍 규칙을 따른다 (`A`/`U`/`F`/`E` 접두사, PascalCase).
- 만약 새로운 C++파일을 생성할경우 비주얼 스튜디오를 새로고침하도록 한다.

