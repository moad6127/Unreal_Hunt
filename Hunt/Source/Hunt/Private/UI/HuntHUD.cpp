// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HuntHUD.h"
#include "Character/HuntCharacter.h"
#include "Weapon/HuntWeapon.h"
#include "Weapon/WeaponDefinition.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"

void AHuntHUD::DrawHUD()
{
	Super::DrawHUD();

	const AHuntCharacter* Character = Cast<AHuntCharacter>(GetOwningPawn());
	const AHuntWeapon* Weapon = Character ? Character->GetEquippedWeapon() : nullptr;
	const UWeaponDefinition* Definition = Weapon ? Weapon->Definition.Get() : nullptr;
	if (!Definition || !PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}

	// 조준하거나 벽에 막혀 총을 내리면(탄이 크로스헤어로 가지 않음) 흐려진다
	const float Opacity = (1.f - Character->GetAimBlend()) * (1.f - Character->GetWallBlockAlpha());
	if (Opacity <= 0.f)
	{
		return;
	}

	// 탄퍼짐 각도를 화면 픽셀로. 가로 FOV 기준이라 해상도·FOV(스코프 배율)가 바뀌어도 실제 탄착 범위와 맞는다
	const float HalfFOVRad = FMath::DegreesToRadians(PlayerOwner->PlayerCameraManager->GetFOVAngle() * 0.5f);
	const float SpreadRad = FMath::DegreesToRadians(Character->GetCurrentSpread());
	const float Offset = FMath::Tan(SpreadRad) / FMath::Tan(HalfFOVRad) * Canvas->ClipX * 0.5f;

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	DrawCrosshairPiece(Definition->CrosshairCenter, Center, Opacity);
	DrawCrosshairPiece(Definition->CrosshairLeft, Center + FVector2D(-Offset, 0.f), Opacity);
	DrawCrosshairPiece(Definition->CrosshairRight, Center + FVector2D(Offset, 0.f), Opacity);
	DrawCrosshairPiece(Definition->CrosshairTop, Center + FVector2D(0.f, -Offset), Opacity);
	DrawCrosshairPiece(Definition->CrosshairBottom, Center + FVector2D(0.f, Offset), Opacity);
}

void AHuntHUD::DrawCrosshairPiece(UTexture2D* Texture, const FVector2D& Center, float Opacity)
{
	if (!Texture)
	{
		return;
	}
	const float Width = Texture->GetSizeX();
	const float Height = Texture->GetSizeY();
	DrawTexture(Texture, Center.X - Width * 0.5f, Center.Y - Height * 0.5f, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor(1.f, 1.f, 1.f, Opacity));
}
