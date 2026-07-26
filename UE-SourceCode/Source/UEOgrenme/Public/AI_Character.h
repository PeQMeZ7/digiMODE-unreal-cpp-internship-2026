// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AI_Character.generated.h"


UCLASS()
class UEOGRENME_API UAI_Character : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadOnly);
	float SpeedCpp;
	UPROPERTY(BlueprintReadOnly);
	bool bIsFallingCpp;
};
