// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HuntHUD.generated.h"

class UTexture2D;

/**
 * 로컬 플레이어 화면 UI. 크로스헤어는 DrawHUD로 그리고, 그 밖의 UI 위젯도 여기서 만든다
 */
UCLASS()
class HUNT_API AHuntHUD : public AHUD
{
	GENERATED_BODY()

public:
	// 장착한 총의 크로스헤어. 상하좌우 조각은 실제 탄퍼짐 각도만큼 벌리고, 조준할수록 흐려진다
	virtual void DrawHUD() override;

private:
	// Texture 중심을 Center에 맞춰 그린다
	void DrawCrosshairPiece(UTexture2D* Texture, const FVector2D& Center, float Opacity);
};
