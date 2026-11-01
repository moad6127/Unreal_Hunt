// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/HuntGameMode.h"
#include "UI/HuntHUD.h"

AHuntGameMode::AHuntGameMode()
{
	HUDClass = AHuntHUD::StaticClass();
}
