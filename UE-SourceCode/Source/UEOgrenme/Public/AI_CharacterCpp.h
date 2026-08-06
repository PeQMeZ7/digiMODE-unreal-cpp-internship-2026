// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "AI/Navigation/NavigationTypes.h"  // FNavLocation için
#include "AI_CharacterCpp.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrnekDelegate2, int32, Saniye);

class UNavigationSystemV1;   // forward declaration: "böyle bir sınıf var" demek yeter



UCLASS()
class UEOGRENME_API AAI_CharacterCpp : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAI_CharacterCpp();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void HareketBitti(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void YeniHareket();
	
	UPROPERTY()
	AAIController* AIC_Ref;
	
	UPROPERTY()
	UNavigationSystemV1* NavSys;
	
	FNavLocation NavLocation;
	
	FTimerHandle TimerHandle;   // üye olmalı, yerel değil — timer'ın ömrü boyunca yaşamalı
	
	
	
	FTimerHandle GecikmeTimer;   // timer'ın ömrü boyunca yaşaması için üye

	void GecikmeliTetikle();     // gecikmeden sonra çalışacak fonksiyon
	void SaniyeGecti();
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOrnekDelegate2 OrnekDelegate2;
	
	UPROPERTY(EditAnywhere, Category = "Zaman")
	int Zaman = 15;
	
	
};

