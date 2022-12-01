// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieFPSGameMode.h"
#include "ZombieFPSCharacter.h"
#include "UObject/ConstructorHelpers.h"

AZombieFPSGameMode::AZombieFPSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
