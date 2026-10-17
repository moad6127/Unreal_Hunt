// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/HuntPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

void AHuntPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}
