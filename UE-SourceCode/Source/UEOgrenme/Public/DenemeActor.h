// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyInterface.h"
#include "GameFramework/Actor.h"
#include "DenemeActor.generated.h"

UCLASS()
class UEOGRENME_API ADenemeActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADenemeActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Test")
	float Health = 100.f;

	//! inline ile farkı: inline bir öneri, derleyici reddedebilir.
	FORCEINLINE float GetHealth() const { return Health; }
	
	
};
