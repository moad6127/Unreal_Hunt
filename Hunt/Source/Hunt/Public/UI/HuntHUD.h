// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HuntHUD.generated.h"

/**
 * 로컬 플레이어 화면 UI. 크로스헤어는 DrawHUD로 그리고, 그 밖의 UI 위젯도 여기서 만든다
 */
UCLASS()
class HUNT_API AHuntHUD : public AHUD
{
	GENERATED_BODY()
	
};
