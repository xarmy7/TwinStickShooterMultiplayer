// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/GameMode.h"
#include "TwinStickShooterGameMode.generated.h"


UCLASS(minimalapi)
class ATwinStickShooterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATwinStickShooterGameMode();

	UPROPERTY(Category = Gameplay, EditAnywhere, BlueprintReadWrite)
		FString PlayerName;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};



