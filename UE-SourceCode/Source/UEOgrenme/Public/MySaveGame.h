// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MySaveGame.generated.h"

/**
 * 
 */
UCLASS()
class UEOGRENME_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditDefaultsOnly)
	float Health = 100.f;
	
	UPROPERTY(EditDefaultsOnly)
	int Score = 0;
};
