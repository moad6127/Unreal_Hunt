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
