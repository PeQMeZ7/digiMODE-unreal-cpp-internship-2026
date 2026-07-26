// Fill out your copyright notice in the Description page of Project Settings.


#include "AI_Character.h"

#include "GameFramework/PawnMovementComponent.h"


void UAI_Character::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	GEngine->AddOnScreenDebugMessage(101,10.f,FColor::Cyan,TEXT("UAI_Character::NativeUpdateAnimation"));

	if (TryGetPawnOwner())
	{
		SpeedCpp = TryGetPawnOwner()->GetVelocity().Size();	
		bIsFallingCpp = TryGetPawnOwner()->GetMovementComponent()->IsFalling();
	}
	
}
