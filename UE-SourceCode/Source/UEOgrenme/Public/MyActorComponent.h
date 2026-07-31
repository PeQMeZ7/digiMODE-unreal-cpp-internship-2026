// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyInterface.h"
#include "Components/ActorComponent.h"
#include "MyActorComponent.generated.h"

//! Dynamic multicast delegate tanımı.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOrnekDelegate); //80. Satır CPP de BeginPlay() 68. Satır İçinde


// YANLIŞ: int Saniye  (tek argüman gibi, virgül yok, int yasak)
// DOĞRU:
//TODO DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrnekDelegate2, int32, Saniye);
//                                                                 tip ↑    ↑ isim

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UEOGRENME_API UMyActorComponent : public UActorComponent, public IMyInterface
{
	GENERATED_BODY()

private:


public:
	// Sets default values for this component's properties
	UMyActorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Test")
	void SayiYaz(int32 Sayi);

	UFUNCTION(BlueprintCallable, Category = "Test")
	void YaziYaz(FString deneme);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	int health = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	int sayi = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	int counter = 0;

	UFUNCTION(BlueprintCallable, Category = "Test")
	void Counter(int counterArtisi);

	//Hem BP de hem C++'ta yazılabiliyor
	UFUNCTION(BlueprintNativeEvent)
	void MyNativeEvent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceDistance = 1000.f;

	UFUNCTION(BlueprintCallable, Category = "Trace")
	void TraceLine();

	virtual void Interact() override;

	void TetiklenecekFonksiyon();

	FTimerHandle TH_TimerHandle;

	
	
	//TODO
	//! TSubclassOf<AActor> = "AActor'den türeyen bir SINIF" tutar — nesnenin kendisini değil, kalıbını.
	// AActor* olsaydı sahnedeki mevcut bir nesneyi işaret ederdi; bu ise "hangi tipten üreteceğim" bilgisidir.
	// Editörde açılır liste olarak çıkar ve sadece AActor türevlerini gösterir — yanlış tip seçemezsin.
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<AActor> SpawnClass;


	FOrnekDelegate OrnekDelegate;


	UFUNCTION() // AddDynamic için zorunlu
	void OlayGeldi();
};
